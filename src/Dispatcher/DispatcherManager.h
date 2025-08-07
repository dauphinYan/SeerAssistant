#pragma once

#include <functional>
#include <vector>
#include <unordered_map>
#include <cstdint>

struct PacketData;

class DispatcherManager
{
public:
    using PacketEventHandler = std::function<void(const PacketData &)>;

    static void RegisterPacketEvent(int32_t cmdId, PacketEventHandler handler);

    static void DispatchPacketEvent(int32_t cmdId, const PacketData &data);

private:
    static std::unordered_map<uint32_t, std::vector<PacketEventHandler>> packetEventHandlers;

public:
    static void InitDispatcher();

private:
    static void OnNoteStartFightCmdReceived(const PacketData &data);

    static void OnFightOverCmdReceived(const PacketData &data);

    static void OnNoteUseSkillCmdReceived(const PacketData &data);

    static void OnChangePetCmdReceived(const PacketData &data);

    static void OnGetUserPerInfoByIDCmdReceived(const PacketData &data);

    static void OnGetSimUserInfoCmdReceived(const PacketData &data);

    static void On45139CmdReceived(const PacketData &data);

    static void On45141CmdReceived(const PacketData &data);

private:
    static bool bIsGetPlayer_1;
};