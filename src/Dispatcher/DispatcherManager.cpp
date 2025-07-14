#include "DispatcherManager.h"
#include "src/Net/PacketParser/Packet.h"
#include "src/Common/Log.h"
#include "src/GameCore/SkillManager.h"

std::unordered_map<uint32_t, std::vector<DispatcherManager::PacketEventHandler>> DispatcherManager::PacketEventHandlers;

void DispatcherManager::RegisterPacketEvent(int32_t CmdID, PacketEventHandler Handler)
{
    PacketEventHandlers[CmdID].emplace_back(Handler);
}

void DispatcherManager::DispatchPacketEvent(int32_t CmdID, const PacketData &Data)
{
    auto it = PacketEventHandlers.find(CmdID);
    if (it != PacketEventHandlers.end())
    {
        for (const auto &Handler : it->second)
        {
            Handler(Data);
        }
    }
}

void DispatcherManager::InitDispatcher()
{
    DispatcherManager::RegisterPacketEvent(2505, &DispatcherManager::OnUseSkillCmdReceived);
}

void DispatcherManager::OnUseSkillCmdReceived(const PacketData &Data)
{
    // 解析用户ID（大端序，前4字节）
    uint32_t userId =
        (static_cast<uint32_t>(Data.Body[0]) << 24) |
        (static_cast<uint32_t>(Data.Body[1]) << 16) |
        (static_cast<uint32_t>(Data.Body[2]) << 8) |
        (static_cast<uint32_t>(Data.Body[3]));

    // 解析技能ID（大端序，接下来的4字节）
    uint32_t skillId =
        (static_cast<uint32_t>(Data.Body[4]) << 24) |
        (static_cast<uint32_t>(Data.Body[5]) << 16) |
        (static_cast<uint32_t>(Data.Body[6]) << 8) |
        (static_cast<uint32_t>(Data.Body[7]));

    uint32_t finalHp =
        (static_cast<uint32_t>(Data.Body[36]) << 24) |
        (static_cast<uint32_t>(Data.Body[37]) << 16) |
        (static_cast<uint32_t>(Data.Body[38]) << 8) |
        (static_cast<uint32_t>(Data.Body[39]));

    string SkillName = SkillManager::GetSkillNameByID(skillId);

    // 写入日志
    Log::WriteBattleLog("\n用户 " + std::to_string(userId));
    Log::WriteBattleLog("使用技能：" + SkillName);
    Log::WriteBattleLog("剩余血量：" + std::to_string(finalHp));
}