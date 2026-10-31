"""Host checks for the SSL proxy's post-success capture boundary."""

import pathlib
import re
import subprocess
import tempfile
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCE = ROOT / "source"


class CaptureLengthTest(unittest.TestCase):
    def test_only_positive_in_bounds_original_counts_are_exported(self):
        # These counts model a successful Read and a partially accepted Write.
        code = r'''
            #include "ssl_capture_bounds.hpp"
            using ams::ssl::sf::impl::CaptureLength;
            static_assert(CaptureLength(0, 8) == 0);
            static_assert(CaptureLength(9, 8) == 0);
            static_assert(CaptureLength(3, 8) == 3);
            static_assert(CaptureLength(8, 8) == 8);
            int main() { return 0; }
        '''
        with tempfile.TemporaryDirectory() as temp:
            source = pathlib.Path(temp) / "test.cpp"
            binary = pathlib.Path(temp) / "test"
            source.write_text(code)
            build = subprocess.run(
                ["c++", "-std=c++23", "-I", str(SOURCE), str(source), "-o", str(binary)],
                capture_output=True,
                text=True,
            )
            self.assertEqual(build.returncode, 0, build.stderr)
            subprocess.run([str(binary)], check=True)

    def test_failed_io_cannot_read_count_or_enqueue(self):
        # The Switch shim cannot run on the host. Audit the IPC boundary's
        # ordering so failed calls never inspect an undefined output count.
        source = (SOURCE / "ssl_connection_impl.cpp").read_text()
        for method in ("Read", "Write"):
            with self.subTest(method=method):
                start = source.index(f"SslConnectionImpl::{method}(")
                next_method = re.search(r"\n(?:Result\s+)?SslConnectionImpl::", source[start + 1 :])
                self.assertIsNotNone(next_method)
                body = source[start : start + 1 + next_method.start()]
                shim = f"sslConnection{method}_sfMitm("
                count = f"{method.lower()}_count.GetValue()"
                self.assertEqual(body.count(shim), 1)
                self.assertLess(body.index(shim), body.index("R_SUCCEEDED(result)"))
                self.assertLess(body.index("R_SUCCEEDED(result)"), body.index(count))
                self.assertLess(body.index(count), body.index("m_stream_producer.Enqueue("))
                self.assertIn("return result;", body)

    def test_raw_forward_handlers_mark_their_arguments_unused(self):
        # Target builds use -Werror. Every raw-forward handler with arguments
        # must consume them locally only as an unused marker.
        for name in (
            "ssl_service_impl.cpp",
            "ssl_for_system_service_impl.cpp",
            "ssl_context_impl.cpp",
            "ssl_context_for_system_impl.cpp",
            "ssl_connection_impl.cpp",
        ):
            source = (SOURCE / name).read_text()
            for match in re.finditer(r"\bResult\s+\w+::(\w+)\(", source):
                method = match.group(1)
                close = source.index(")", match.end())
                arguments = source[match.end() : close]
                opening = source.index("{", close)
                depth = 1
                end = opening + 1
                while depth:
                    depth += (source[end] == "{") - (source[end] == "}")
                    end += 1
                body = source[opening + 1 : end - 1]
                if (
                    arguments.strip()
                    and "ResultShouldForwardToSession" in body
                    and "serviceIsDomain(" not in body
                ):
                    with self.subTest(source=name, method=method):
                        self.assertIn("AMS_UNUSED(", body)

    def test_non_domain_object_creation_forwards_before_creating_child(self):
        # Non-domain out objects get ordinary sessions in Stratosphere, where
        # a nested MITM dispatch would abort on forwarding.
        for name, method, shim in (
            ("ssl_service_impl.cpp", "CreateContext", "sslCreateContext_sfMitm("),
            ("ssl_for_system_service_impl.cpp", "CreateContext", "sslsCreateContext_sfMitm("),
            (
                "ssl_for_system_service_impl.cpp",
                "CreateContextForSystem",
                "sslsCreateContextForSystem_sfMitm(",
            ),
        ):
            with self.subTest(source=name, method=method):
                source = (SOURCE / name).read_text()
                start = source.index(f"::{method}(")
                guard = source.index("if (!serviceIsDomain(m_forward_service.get()))", start)
                forward = source.index("return sm::mitm::ResultShouldForwardToSession();", guard)
                child = source.index(shim, start)
                self.assertLess(guard, forward)
                self.assertLess(forward, child)


if __name__ == "__main__":
    unittest.main()
