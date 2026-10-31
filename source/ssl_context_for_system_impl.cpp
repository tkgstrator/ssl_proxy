#include "ssl_context_for_system_impl.hpp"
#include "ssl_shim.h"
#include <stratosphere.hpp>

namespace ams::ssl::sf::impl {
Result
SslContextForSystemImpl::SetOption(const ams::ssl::sf::OptionType &option,
                                   u32 value) {
    AMS_UNUSED(option, value);
    return sm::mitm::ResultShouldForwardToSession();
}

Result
SslContextForSystemImpl::GetOption(const ams::ssl::sf::OptionType &option,
                                   ams::sf::Out<u32> value) {
    AMS_UNUSED(option, value);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslContextForSystemImpl::CreateConnection(
    ams::sf::Out<ams::sf::SharedPointer<ams::ssl::sf::ISslConnection>> out) {
    Service out_tmp;
    R_TRY(sslContextForSystemCreateConnection_sfMitm(m_forward_service.get(),
                                                     std::addressof(out_tmp)));

    const ams::sf::cmif::DomainObjectId target_object_id{
        serviceGetObjectId(std::addressof(out_tmp))};

    out.SetValue(
        ams::sf::CreateSharedObjectEmplaced<ISslConnection, SslConnectionImpl>(
            std::make_unique<::Service>(out_tmp), m_client_info),
        target_object_id);

    R_SUCCEED();
}

Result SslContextForSystemImpl::GetConnectionCount(ams::sf::Out<u32> count) {
    AMS_UNUSED(count);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslContextForSystemImpl::ImportServerPki(
    const ams::ssl::sf::CertificateFormat &certificateFormat,
    const ams::sf::InBuffer &certificate, ams::sf::Out<u64> certificate_id) {
    AMS_UNUSED(certificateFormat, certificate, certificate_id);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslContextForSystemImpl::ImportClientPki(
    const ams::sf::InBuffer &certificate,
    const ams::sf::InBuffer &ascii_password, ams::sf::Out<u64> certificate_id) {
    AMS_UNUSED(certificate, ascii_password, certificate_id);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslContextForSystemImpl::RemoveServerPki(u64 certificate_id) {
    AMS_UNUSED(certificate_id);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslContextForSystemImpl::RemoveClientPki(u64 certificate_id) {
    AMS_UNUSED(certificate_id);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslContextForSystemImpl::RegisterInternalPki(
    const ams::ssl::sf::InternalPki &pki, ams::sf::Out<u64> certificate_id) {
    AMS_UNUSED(pki, certificate_id);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslContextForSystemImpl::AddPolicyOid(
    const ams::sf::InBuffer &cert_policy_checking) {
    AMS_UNUSED(cert_policy_checking);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslContextForSystemImpl::ImportCrl(const ams::sf::InBuffer &crl,
                                          ams::sf::Out<u64> crl_id) {
    AMS_UNUSED(crl, crl_id);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslContextForSystemImpl::RemoveCrl(u64 crl_id) {
    AMS_UNUSED(crl_id);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslContextForSystemImpl::ImportClientCertKeyPki(
    const ams::ssl::sf::CertificateFormat &certificateFormat,
    const ams::sf::InBuffer &cert, const ams::sf::InBuffer &key,
    ams::sf::Out<u64> certificate_id) {
    AMS_UNUSED(certificateFormat, cert, key, certificate_id);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslContextForSystemImpl::GeneratePrivateKeyAndCert(
    u32 val, const ams::sf::InBuffer &params, const ams::sf::OutBuffer &cert,
    const ams::sf::OutBuffer &key, ams::sf::Out<u32> out_cert_size,
    ams::sf::Out<u32> out_key_size) {
    AMS_UNUSED(val, params, cert, key, out_cert_size, out_key_size);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslContextForSystemImpl::CreateConnectionEx(
    ams::sf::Out<ams::sf::SharedPointer<ams::ssl::sf::ISslConnection>> out) {
    Service out_tmp;
    R_TRY(sslContextForSystemCreateConnectionEx_sfMitm(
        m_forward_service.get(), std::addressof(out_tmp)));

    const ams::sf::cmif::DomainObjectId target_object_id{
        serviceGetObjectId(std::addressof(out_tmp))};

    out.SetValue(
        ams::sf::CreateSharedObjectEmplaced<ISslConnection, SslConnectionImpl>(
            std::make_unique<::Service>(out_tmp), m_client_info),
        target_object_id);

    R_SUCCEED();
}

} // namespace ams::ssl::sf::impl
