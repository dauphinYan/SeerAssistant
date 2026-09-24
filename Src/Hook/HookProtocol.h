#pragma once

#include <cstdint>

// Shared by SeerAssistant and SocketHook.dll. Keep the wire layout stable.
enum class ClientType
{
    Flash,
    Unity
};

struct PacketHeader
{
    uint32_t totalSize;
    uint32_t socket;
    uint32_t payloadSize;
    uint8_t direction; // 0 = recv, 1 = send
};

inline constexpr wchar_t HOOK_PIPE_NAME[] = L"\\\\.\\pipe\\SeerSocketHook";

static_assert(sizeof(PacketHeader) == 16, "SocketHook pipe protocol layout changed");
