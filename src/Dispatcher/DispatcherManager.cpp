#include "DispatcherManager.h"
#include "src/Net/PacketParser/Packet.h"
#include "src/Common/Log.h"
#include "src/GameCore/SkillManager.h"
#include "src/GameCore/PetManager.h"

#include <iostream>

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
    int offset = 0;
    uint32_t userId = PacketProcessor::ReadUnsignedInt(Data.Body, offset);

    uint32_t skillId = PacketProcessor::ReadUnsignedInt(Data.Body, offset);

    uint32_t enemyLostHp =
        (static_cast<uint32_t>(Data.Body[24]) << 24) |
        (static_cast<uint32_t>(Data.Body[25]) << 16) |
        (static_cast<uint32_t>(Data.Body[26]) << 8) |
        (static_cast<uint32_t>(Data.Body[27]));

    uint32_t selfFinalHp =
        (static_cast<uint32_t>(Data.Body[36]) << 24) |
        (static_cast<uint32_t>(Data.Body[37]) << 16) |
        (static_cast<uint32_t>(Data.Body[38]) << 8) |
        (static_cast<uint32_t>(Data.Body[39]));

    uint32_t selfMaxHp =
        (static_cast<uint32_t>(Data.Body[40]) << 24) |
        (static_cast<uint32_t>(Data.Body[41]) << 16) |
        (static_cast<uint32_t>(Data.Body[42]) << 8) |
        (static_cast<uint32_t>(Data.Body[43]));

    uint32_t skillListCount =
        (static_cast<uint32_t>(Data.Body[52]) << 24) |
        (static_cast<uint32_t>(Data.Body[53]) << 16) |
        (static_cast<uint32_t>(Data.Body[54]) << 8) |
        (static_cast<uint32_t>(Data.Body[55]));

    int skillListLength = 8 * skillListCount;

    offset = 55 + 1 + skillListLength + 4;

    int StatusCount = static_cast<uint32_t>(Data.Body[offset]);

    offset += 1;

    offset += StatusCount;

    uint32_t SpecailArrCount = PacketProcessor::ReadUnsignedInt(Data.Body, offset);

    vector<uint32_t> Specails;
    for (int i = 0; i < SpecailArrCount; ++i)
    {
        uint32_t temp = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
        Specails.push_back(temp);
    }

    uint32_t SideEffectsCount = PacketProcessor::ReadUnsignedInt(Data.Body, offset);

    offset += 12 * SideEffectsCount;

    offset += 12;

    int ImmunizationCount = PacketProcessor::ReadUnsignedInt(Data.Body, offset);

    offset += 4 * ImmunizationCount;

    int ChangeHpsCount = PacketProcessor::ReadUnsignedInt(Data.Body, offset);

    struct UChangeHpInfo
    {
        uint32_t id;
        uint32_t hp;
        uint32_t maxhp;
        uint32_t lock;
        uint32_t chujueNumber;
        uint32_t chujueRound;
    };

    std::vector<UChangeHpInfo> changehps;

    for (int i = 0; i < ChangeHpsCount; ++i)
    {
        UChangeHpInfo ChangeHpInfo;
        ChangeHpInfo.id = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
        ChangeHpInfo.hp = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
        ChangeHpInfo.maxhp = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
        ChangeHpInfo.lock = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
        ChangeHpInfo.chujueNumber = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
        ChangeHpInfo.chujueRound = PacketProcessor::ReadUnsignedInt(Data.Body, offset);

        int MarkBuffCnt = static_cast<uint32_t>(Data.Body[offset++]);

        offset += 3 * MarkBuffCnt;

        changehps.push_back(ChangeHpInfo);
    }

    Log::WriteBattleLog("\n[UseSkill]\n用户 " + std::to_string(userId));
    Log::WriteBattleLog("使用技能：" + SkillManager::GetSkillNameByID(skillId));
    Log::WriteBattleLog("造成伤害：" + std::to_string(enemyLostHp));
    Log::WriteBattleLog("己方剩余血量：" + std::to_string(selfFinalHp));
    Log::WriteBattleLog("己方最大血量：" + std::to_string(selfMaxHp));

    for (const auto &info : changehps)
    {
        Log::WriteBattleLog(std::string("\n[ChangeHpInfo]\n") +
                                "ID: " + std::to_string(info.id) + "\n" +
                                "HP: " + std::to_string(info.hp) + "\n" +
                                "MaxHP: " + std::to_string(info.maxhp) + "\n" +
                                "Lock: " + std::to_string(info.lock) + "\n" +
                                "Chujue Number: " + std::to_string(info.chujueNumber) + "\n" +
                                "Chujue Round: " + std::to_string(info.chujueRound) + "\n",
                            false);
    }

    // if (SpecailArrCount > 37)
    // {
    //     uint32_t changeSelfValue = Specails[37];
    //     Log::WriteBattleLog("己方改变血量：" + std::to_string(changeSelfValue));
    // }

    // if (SpecailArrCount > 38)
    // {
    //     uint32_t changeEnemyValue = Specails[38];
    //     Log::WriteBattleLog("对方改变血量：" + std::to_string(changeEnemyValue));
    // }
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
