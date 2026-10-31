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
#include "ssl_context_impl.hpp"
#include "ssl_shim.h"
#include <stratosphere.hpp>

namespace ams::ssl::sf::impl {
Result SslContextImpl::SetOption(const ams::ssl::sf::OptionType &option,
                                 u32 value) {
    AMS_UNUSED(option, value);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslContextImpl::GetOption(const ams::ssl::sf::OptionType &option,
                                 ams::sf::Out<u32> value) {
    AMS_UNUSED(option, value);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslContextImpl::CreateConnection(
    ams::sf::Out<ams::sf::SharedPointer<ams::ssl::sf::ISslConnection>> out) {
    Service out_tmp;
    R_TRY(sslContextCreateConnection_sfMitm(m_forward_service.get(),
                                            std::addressof(out_tmp)));

    const ams::sf::cmif::DomainObjectId target_object_id{
        serviceGetObjectId(std::addressof(out_tmp))};

    out.SetValue(
        ams::sf::CreateSharedObjectEmplaced<ISslConnection, SslConnectionImpl>(
            std::make_unique<::Service>(out_tmp), m_client_info),
        target_object_id);

    R_SUCCEED();
}

Result SslContextImpl::GetConnectionCount(ams::sf::Out<u32> count) {
    AMS_UNUSED(count);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslContextImpl::ImportServerPki(
    const ams::ssl::sf::CertificateFormat &certificateFormat,
    const ams::sf::InBuffer &certificate, ams::sf::Out<u64> certificate_id) {
    AMS_UNUSED(certificateFormat, certificate, certificate_id);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslContextImpl::ImportClientPki(const ams::sf::InBuffer &certificate,
                                       const ams::sf::InBuffer &ascii_password,
                                       ams::sf::Out<u64> certificate_id) {
    AMS_UNUSED(certificate, ascii_password, certificate_id);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslContextImpl::RemoveServerPki(u64 certificate_id) {
    AMS_UNUSED(certificate_id);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslContextImpl::RemoveClientPki(u64 certificate_id) {
    AMS_UNUSED(certificate_id);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslContextImpl::RegisterInternalPki(const ams::ssl::sf::InternalPki &pki,
                                           ams::sf::Out<u64> certificate_id) {
    AMS_UNUSED(pki, certificate_id);
    return sm::mitm::ResultShouldForwardToSession();
}

Result
SslContextImpl::AddPolicyOid(const ams::sf::InBuffer &cert_policy_checking) {
    AMS_UNUSED(cert_policy_checking);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslContextImpl::ImportCrl(const ams::sf::InBuffer &crl,
                                 ams::sf::Out<u64> crl_id) {
    AMS_UNUSED(crl, crl_id);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslContextImpl::RemoveCrl(u64 crl_id) {
    AMS_UNUSED(crl_id);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslContextImpl::ImportClientCertKeyPki(
    const ams::ssl::sf::CertificateFormat &certificateFormat,
    const ams::sf::InBuffer &cert, const ams::sf::InBuffer &key,
    ams::sf::Out<u64> certificate_id) {
    AMS_UNUSED(certificateFormat, cert, key, certificate_id);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslContextImpl::GeneratePrivateKeyAndCert(
    u32 val, const ams::sf::InBuffer &params, const ams::sf::OutBuffer &cert,
    const ams::sf::OutBuffer &key, ams::sf::Out<u32> out_cert_size,
    ams::sf::Out<u32> out_key_size) {
    AMS_UNUSED(val, params, cert, key, out_cert_size, out_key_size);
    return sm::mitm::ResultShouldForwardToSession();
}

} // namespace ams::ssl::sf::impl
