#include "DispatcherManager.h"
#include "src/Net/PacketParser/Packet.h"
#include "src/Common/Log.h"
#include "src/GameCore/SkillManager.h"
#include "src/GameCore/PetManager.h"

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
    DispatcherManager::RegisterPacketEvent(2407, &DispatcherManager::OnChangePetCmdReceived);
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

    Log::WriteBattleLog("\n[UseSkill]\n用户 " + std::to_string(userId));
    Log::WriteBattleLog("使用技能：" + SkillName);
    Log::WriteBattleLog("剩余血量：" + std::to_string(finalHp));
}

void DispatcherManager::OnChangePetCmdReceived(const PacketData &Data)
{
    // 辅助：从 Data.Body[offset … offset+3] 以大端序读取 uint32_t
    auto readUint32BE = [&](int offset) -> uint32_t
    {
        return (static_cast<uint32_t>(Data.Body[offset]) << 24) |
               (static_cast<uint32_t>(Data.Body[offset + 1]) << 16) |
               (static_cast<uint32_t>(Data.Body[offset + 2]) << 8) |
               (static_cast<uint32_t>(Data.Body[offset + 3]));
    };

    uint32_t userId = readUint32BE(0);
    uint32_t petId = readUint32BE(4);
    uint32_t hp = readUint32BE(32);
    uint32_t maxHp = readUint32BE(36);

    string PetName = PetManager::GetPetName(petId);

    Log::WriteBattleLog("\n[ChangePet]\n用户: " + std::to_string(userId));
    Log::WriteBattleLog("当前精灵：" + PetName);
    Log::WriteBattleLog("当前血量: " + std::to_string(hp));
    Log::WriteBattleLog("最大血量: " + std::to_string(maxHp));
}
