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
#include "ssl_service_impl.hpp"
#include "ssl_shim.h"
#include <stratosphere.hpp>

namespace ams::ssl::sf::impl {
Result SslServiceImpl::CreateContext(
    const ams::ssl::sf::SslVersion &version,
    const ams::sf::ClientProcessId &client_pid,
    ams::sf::Out<ams::sf::SharedPointer<ams::ssl::sf::ISslContext>> out) {
    if (!serviceIsDomain(m_forward_service.get())) {
        return sm::mitm::ResultShouldForwardToSession();
    }

    Service out_tmp;
    R_TRY(sslCreateContext_sfMitm(
        m_forward_service.get(), static_cast<u32>(version),
        static_cast<u64>(client_pid.GetValue()),
        static_cast<u64>(client_pid.GetValue()), std::addressof(out_tmp)));

    const ams::sf::cmif::DomainObjectId target_object_id{
        serviceGetObjectId(std::addressof(out_tmp))};
    out.SetValue(
        ams::sf::CreateSharedObjectEmplaced<ISslContext, SslContextImpl>(
            std::make_unique<::Service>(out_tmp), m_client_info),
        target_object_id);

    R_SUCCEED();
}

Result SslServiceImpl::GetCertificates(
    const ams::sf::InArray<ams::ssl::sf::CaCertificateId> &ids,
    ams::sf::Out<u32> certificates_count,
    const ams::sf::OutBuffer &certificates) {
    AMS_UNUSED(ids, certificates_count, certificates);
    return sm::mitm::ResultShouldForwardToSession();
}

Result SslServiceImpl::GetCertificateBufSize(
    const ams::sf::InArray<ams::ssl::sf::CaCertificateId> &ids,
    ams::sf::Out<u32> buffer_size) {
    AMS_UNUSED(ids, buffer_size);
    return sm::mitm::ResultShouldForwardToSession();
}

} // namespace ams::ssl::sf::impl
