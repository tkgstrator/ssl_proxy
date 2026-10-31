/*
 * Copyright (c) Mary Guillemard <mary@mary.zone>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "networkmitm_ssl_connection_impl.hpp"
#include "impl/stream/stream_config.hpp"
#include "shim/ssl_shim.h"
#include <stratosphere.hpp>

namespace ams::ssl::sf::impl {
Result
SslConnectionImpl::SetSocketDescriptor(u32 input_socket_fd,
                                       ams::sf::Out<u32> output_socket_fd) {
    AMS_UNUSED(input_socket_fd, output_socket_fd);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslConnectionImpl::SetHostName(const ams::sf::InBuffer &hostname) {
    AMS_UNUSED(hostname);
    return sm::mitm::ResultShouldForwardToSession();
}

Result
SslConnectionImpl::SetVerifyOption(const ams::ssl::sf::VerifyOption &option) {
    AMS_UNUSED(option);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslConnectionImpl::SetIoMode(const ams::ssl::sf::IoMode &mode) {
    AMS_UNUSED(mode);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslConnectionImpl::GetSocketDescriptor(ams::sf::Out<u32> socket_fd) {
    AMS_UNUSED(socket_fd);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslConnectionImpl::GetHostName(ams::sf::Out<u32> hostname_length,
                                      const ams::sf::OutBuffer &hostname) {
    AMS_UNUSED(hostname_length, hostname);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslConnectionImpl::GetVerifyOption(
    ams::sf::Out<ams::ssl::sf::VerifyOption> option) {
    AMS_UNUSED(option);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslConnectionImpl::GetIoMode(ams::sf::Out<ams::ssl::sf::IoMode> mode) {
    AMS_UNUSED(mode);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslConnectionImpl::DoHandshake() {
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslConnectionImpl::DoHandshakeGetServerCert(
    ams::sf::Out<u32> buffer_size, ams::sf::Out<u32> certificates_count,
    const ams::sf::OutBuffer &server_cert_buffer) {
    AMS_UNUSED(buffer_size, certificates_count, server_cert_buffer);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslConnectionImpl::Read(ams::sf::Out<u32> read_count,
                               const ams::sf::OutBuffer &buffer) {
    const Result result = sslConnectionRead_sfMitm(
        m_forward_service.get(), read_count.GetPointer(), buffer.GetPointer(),
        buffer.GetSize());
    if (R_SUCCEEDED(result) && stream::g_stream_config.active) {
        const std::size_t size =
            CaptureLength(read_count.GetValue(), buffer.GetSize());
        if (size != 0) {
            stream::Event event{
                stream::FrameType::Data,
                stream::Direction::ServerToClient,
                stream::g_stream_session_id,
                static_cast<u64>(m_client_info.program_id),
                m_stream_connection_id,
                0,
                static_cast<u64>(
                    os::GetSystemTick().ToTimeSpan().GetNanoSeconds())};
            m_stream_producer.Enqueue(stream::g_stream_queue, event,
                                      buffer.GetPointer(), size);
        }
    }
    return result;
}

Result SslConnectionImpl::Write(const ams::sf::InBuffer &buffer,
                                ams::sf::Out<u32> write_count) {
    const Result result =
        sslConnectionWrite_sfMitm(m_forward_service.get(), buffer.GetPointer(),
                                  buffer.GetSize(), write_count.GetPointer());
    if (R_SUCCEEDED(result) && stream::g_stream_config.active) {
        const std::size_t size =
            CaptureLength(write_count.GetValue(), buffer.GetSize());
        if (size != 0) {
            stream::Event event{
                stream::FrameType::Data,
                stream::Direction::ClientToServer,
                stream::g_stream_session_id,
                static_cast<u64>(m_client_info.program_id),
                m_stream_connection_id,
                0,
                static_cast<u64>(
                    os::GetSystemTick().ToTimeSpan().GetNanoSeconds())};
            m_stream_producer.Enqueue(stream::g_stream_queue, event,
                                      buffer.GetPointer(), size);
        }
    }
    return result;
}

Result SslConnectionImpl::Pending(ams::sf::Out<u32> pending_count) {
    AMS_UNUSED(pending_count);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslConnectionImpl::Peek(ams::sf::Out<u32> peek_count,
                               const ams::sf::OutBuffer &buffer) {
    AMS_UNUSED(peek_count, buffer);
    return sm::mitm::ResultShouldForwardToSession();
}

Result
SslConnectionImpl::Poll(const ams::ssl::sf::PollEvent &poll_event, u32 timeout,
                        ams::sf::Out<ams::ssl::sf::PollEvent> out_poll_event) {
    AMS_UNUSED(poll_event, timeout, out_poll_event);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslConnectionImpl::GetVerifyCertError() {
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslConnectionImpl::GetNeededServerCertBufferSize(
    ams::sf::Out<u32> needed_buffer_size) {
    AMS_UNUSED(needed_buffer_size);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslConnectionImpl::SetSessionCacheMode(
    const ams::ssl::sf::SessionCacheMode &mode) {
    AMS_UNUSED(mode);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslConnectionImpl::GetSessionCacheMode(
    ams::sf::Out<ams::ssl::sf::SessionCacheMode> mode) {
    AMS_UNUSED(mode);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslConnectionImpl::FlushSessionCache() {
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslConnectionImpl::SetRenegotiationMode(
    const ams::ssl::sf::RenegotiationMode &mode) {
    AMS_UNUSED(mode);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslConnectionImpl::GetRenegotiationMode(
    ams::sf::Out<ams::ssl::sf::RenegotiationMode> mode) {
    AMS_UNUSED(mode);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslConnectionImpl::SetOption(bool value,
                                    const ams::ssl::sf::OptionType &option) {
    AMS_UNUSED(value, option);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslConnectionImpl::GetOption(const ams::ssl::sf::OptionType &value,
                                    ams::sf::Out<bool> option) {
    AMS_UNUSED(value, option);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslConnectionImpl::GetVerifyCertErrors(ams::sf::Out<u32> unk0,
                                              ams::sf::Out<u32> unk1,
                                              const ams::sf::OutBuffer &unk2) {
    AMS_UNUSED(unk0, unk1, unk2);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslConnectionImpl::GetCipherInfo(u32 unk0,
                                        const ams::sf::OutBuffer &cipher_info) {
    AMS_UNUSED(unk0, cipher_info);
    return sm::mitm::ResultShouldForwardToSession();
}

Result
SslConnectionImpl::SetNextAlpnProto(const ams::sf::InBuffer &alpn_proto) {
    AMS_UNUSED(alpn_proto);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslConnectionImpl::GetNextAlpnProto(
    ams::sf::Out<ams::ssl::sf::AlpnProtoState> state,
    ams::sf::Out<u32> alpn_proto_out_size,
    const ams::sf::OutBuffer &alpn_proto) {
    AMS_UNUSED(state, alpn_proto_out_size, alpn_proto);
    return sm::mitm::ResultShouldForwardToSession();
}

Result
SslConnectionImpl::SetDtlsSocketDescriptor(u32 sock_fd,
                                           const ams::sf::InBuffer &sock_addr,
                                           ams::sf::Out<u32> out_sock_fd) {
    AMS_UNUSED(sock_fd, sock_addr, out_sock_fd);
    return sm::mitm::ResultShouldForwardToSession();
}

Result
SslConnectionImpl::GetDtlsHandshakeTimeout(const ams::sf::OutBuffer &timespan) {
    AMS_UNUSED(timespan);
    return sm::mitm::ResultShouldForwardToSession();
}

Result
SslConnectionImpl::SetPrivateOption(const ams::ssl::sf::OptionType &option,
                                    u32 value) {
    AMS_UNUSED(option, value);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslConnectionImpl::SetSrtpCiphers(const ams::sf::InBuffer &ciphers) {
    AMS_UNUSED(ciphers);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslConnectionImpl::GetSrtpCipher(ams::sf::Out<u16> cipher) {
    AMS_UNUSED(cipher);
    return sm::mitm::ResultShouldForwardToSession();
}

Result
SslConnectionImpl::ExportKeyingMaterial(const ams::sf::InBuffer &label,
                                        const ams::sf::InBuffer &context,
                                        const ams::sf::OutBuffer &material) {
    AMS_UNUSED(label, context, material);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslConnectionImpl::SetIoTimeout(u32 timeout) {
    AMS_UNUSED(timeout);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslConnectionImpl::GetIoTimeout(ams::sf::Out<u32> timeout) {
    AMS_UNUSED(timeout);
    return sm::mitm::ResultShouldForwardToSession();
}

Result
SslConnectionImpl::GetSessionTicket(const ams::sf::OutBuffer &session_ticket,
                                    ams::sf::Out<u32> out_session_ticket_size) {
    AMS_UNUSED(session_ticket, out_session_ticket_size);
    return sm::mitm::ResultShouldForwardToSession();
}

Result
SslConnectionImpl::SetSessionTicket(const ams::sf::InBuffer &session_ticket) {
    AMS_UNUSED(session_ticket);
    return sm::mitm::ResultShouldForwardToSession();
}

} // namespace ams::ssl::sf::impl
