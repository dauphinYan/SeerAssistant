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

    static void RegisterPacketEvent(int32_t CmdID, PacketEventHandler Handler);

    static void DispatchPacketEvent(int32_t CmdID, const PacketData &Data);

private:
    static std::unordered_map<uint32_t, std::vector<PacketEventHandler>> PacketEventHandlers;

public:
    static void InitDispatcher();

private:
    static void OnUseSkillCmdReceived(const PacketData &Data);

    static void OnChangePetCmdReceived(const PacketData &Data);
};