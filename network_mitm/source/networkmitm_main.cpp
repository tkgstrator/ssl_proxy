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
#include "networkmitm_ssl_for_system_service_impl.hpp"
#include "networkmitm_ssl_service_impl.hpp"
#include "impl/stream/stream_config.hpp"
#include "impl/stream/stream_sender.hpp"
#include <stratosphere.hpp>

namespace ams {
namespace stream = ssl::mitm::stream;
namespace impl {
#define AMS_DEFINE_SYSTEM_THREAD(__AMS_THREAD_PRIORITY__, __AMS_MODULE__,      \
                                 __AMS_THREAD_NAME__)                          \
    constexpr inline const ::ams::impl::SystemThreadDefinition                 \
        SystemThreadDefinition_##__AMS_MODULE__##_##__AMS_THREAD_NAME__ = {    \
            __AMS_THREAD_PRIORITY__,                                           \
            "ams." #__AMS_MODULE__ "." #__AMS_THREAD_NAME__}

AMS_DEFINE_SYSTEM_THREAD(10, network_mitm, Main);
#undef AMS_DEFINE_SYSTEM_THREAD
} // namespace impl

namespace {
constexpr size_t MallocBufferSize = 1_MB;
alignas(os::MemoryPageSize) constinit u8 g_malloc_buffer[MallocBufferSize];
} // namespace

namespace init {
void InitializeSystemModule() {
    R_ABORT_UNLESS(sm::Initialize());
    R_ABORT_UNLESS((Result)::setsysInitialize());
}

void FinalizeSystemModule() {
    setsysExit();
    R_ABORT_UNLESS(sm::Finalize());
}

void Startup() {
    init::InitializeAllocator(g_malloc_buffer, sizeof(g_malloc_buffer));
}
} // namespace init

namespace ssl::sf::impl {
enum PortIndex {
    PortIndex_SslMitm,
    PortIndex_SslSystemMitm,
    PortIndex_Count,
};

constexpr sm::ServiceName MitmSslServiceName = sm::ServiceName::Encode("ssl");
constexpr sm::ServiceName MitmSslSystemServiceName =
    sm::ServiceName::Encode("ssl:s");

struct ServerOptions {
    static constexpr size_t PointerBufferSize = 0x10000;
    static constexpr size_t MaxDomains = 0x40;
    static constexpr size_t MaxDomainObjects = 0x4000;
    static constexpr bool CanDeferInvokeRequest = false;
    static constexpr bool CanManageMitmServers = true;
};

class ServerManager final
    : public ams::sf::hipc::ServerManager<PortIndex_Count, ServerOptions> {
  private:
    virtual Result OnNeedsToAccept(int port_index, Server *server) override;
};

ServerManager g_server_manager_for_user;
ServerManager g_server_manager_for_system;

Result ServerManager::OnNeedsToAccept(int port_index, Server *server) {
    std::shared_ptr<::Service> forward_service;
    sm::MitmProcessInfo client_info;
    server->AcknowledgeMitmSession(std::addressof(forward_service),
                                   std::addressof(client_info));

    switch (port_index) {
    case PortIndex_SslMitm:
        R_RETURN(this->AcceptMitmImpl(
            server,
            ams::sf::CreateSharedObjectEmplaced<ISslService, SslServiceImpl>(
                decltype(forward_service)(forward_service), client_info),
            forward_service));
    case PortIndex_SslSystemMitm:
        R_RETURN(this->AcceptMitmImpl(
            server,
            ams::sf::CreateSharedObjectEmplaced<ISslServiceForSystem,
                                                SslServiceForSystemImpl>(
                decltype(forward_service)(forward_service), client_info),
            forward_service));
        AMS_UNREACHABLE_DEFAULT_CASE();
    }
}

constexpr size_t TotalThreads = 5;
constexpr size_t NumExtraThreads = TotalThreads - 1;
constexpr size_t ThreadStackSize = 0x8000;
alignas(os::MemoryPageSize) u8
    g_extra_thread_stacks[NumExtraThreads][ThreadStackSize];
os::ThreadType g_extra_threads[NumExtraThreads];

void LoopServerThreadForUser(void *) {
    g_server_manager_for_user.LoopProcess();
}

void LoopServerThreadForSystem(void *) {
    g_server_manager_for_system.LoopProcess();
}

void ProcessForServerOnAllThreads() {
    const bool has_system_manager = hos::GetVersion() >= hos::Version_15_0_0;
    if constexpr (NumExtraThreads > 0) {
        const s32 priority =
            os::GetThreadCurrentPriority(os::GetCurrentThread());
        for (size_t i = 0; i < NumExtraThreads; i++) {
            if (has_system_manager) {
                R_ABORT_UNLESS(os::CreateThread(
                    g_extra_threads + i,
                    i % 2 ? LoopServerThreadForUser : LoopServerThreadForSystem,
                    nullptr, g_extra_thread_stacks[i], ThreadStackSize,
                    priority));
            } else {
                R_ABORT_UNLESS(os::CreateThread(
                    g_extra_threads + i, LoopServerThreadForUser, nullptr,
                    g_extra_thread_stacks[i], ThreadStackSize, priority));
            }
        }
        for (size_t i = 0; i < NumExtraThreads; i++)
            os::StartThread(g_extra_threads + i);
    }

    if (has_system_manager)
        LoopServerThreadForSystem(nullptr);
    else
        LoopServerThreadForUser(nullptr);

    if constexpr (NumExtraThreads > 0) {
        for (size_t i = 0; i < NumExtraThreads; i++)
            os::WaitThread(g_extra_threads + i);
    }
}
} // namespace ssl::sf::impl

void Main() {
    os::SetThreadNamePointer(os::GetCurrentThread(),
                             AMS_GET_SYSTEM_THREAD_NAME(network_mitm, Main));
    AMS_ASSERT(os::GetThreadPriority(os::GetCurrentThread()) ==
               AMS_GET_SYSTEM_THREAD_PRIORITY(network_mitm, Main));

    u8 enabled = 0;
    const bool setting_present =
        settings::fwdbg::GetSettingsItemValue(
            std::addressof(enabled), sizeof(enabled), "ssl_proxy",
            "enabled") == sizeof(enabled);
    char export_host[16]{};
    u32 export_port = 0;
    const size_t host_size = settings::fwdbg::GetSettingsItemValue(
        export_host, sizeof(export_host), "ssl_proxy", "export_host");
    const size_t port_size = settings::fwdbg::GetSettingsItemValue(
        std::addressof(export_port), sizeof(export_port), "ssl_proxy",
        "export_port");

    stream::g_stream_config = stream::ParseConfig(
        setting_present && enabled != 0, export_host,
        host_size <= sizeof(export_host) ? host_size : 0, export_port,
        port_size == sizeof(export_port));
    if (!stream::g_stream_config.active) {
        AMS_LOG("ssl_proxy inactive: configure enabled, export_host and export_port.\n");
        return;
    }

    stream::g_stream_session_id = static_cast<u64>(
        os::GetSystemTick().ToTimeSpan().GetNanoSeconds());
    if (!stream::StartSender(stream::g_stream_config.destination_ipv4,
                             stream::g_stream_config.destination_port,
                             stream::g_stream_session_id)) {
        AMS_LOG("ssl_proxy sender unavailable; SSL service remains unchanged.\n");
        return;
    }

    using namespace ams::ssl::sf::impl;
    R_ABORT_UNLESS(
        (g_server_manager_for_user.RegisterMitmServer<SslServiceImpl>(
            PortIndex_SslMitm, MitmSslServiceName)));
    if (hos::GetVersion() >= hos::Version_15_0_0) {
        R_ABORT_UNLESS(
            (g_server_manager_for_system.RegisterMitmServer<
                SslServiceForSystemImpl>(PortIndex_SslSystemMitm,
                                         MitmSslSystemServiceName)));
    }

    AMS_LOG("ssl_proxy accepting SSL requests.\n");
    ProcessForServerOnAllThreads();
}
} // namespace ams
