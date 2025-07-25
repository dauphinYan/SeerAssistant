#include "DispatcherManager.h"
#include "src/Net/PacketParser/Packet.h"
#include "src/Common/Log.h"
#include "src/GameCore/SkillManager.h"
#include "src/GameCore/PetManager.h"
#include "src/GameCore/FightManager.h"

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
    DispatcherManager::RegisterPacketEvent(41635, &DispatcherManager::OnGetUserPerInfoByIDCmdReceived);
}

void DispatcherManager::OnUseSkillCmdReceived(const PacketData &Data)
{
    PetFightManager::ReadPetFightInfo(Data);
}

void DispatcherManager::OnChangePetCmdReceived(const PacketData &Data)
{
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

void DispatcherManager::OnGetUserPerInfoByIDCmdReceived(const PacketData &Data)
{
    int offset = 0;

    uint32_t PetCount = PacketProcessor::ReadUnsignedInt(Data.Body, offset);

    uint32_t Rub = PacketProcessor::ReadUnsignedInt(Data.Body, offset);

    vector<EOtherPeoplePetInfo> Pets;
    for (int i = 0; i < PetCount; ++i)
    {
        EOtherPeoplePetInfo pet = PetManager::GetOtherPeoplePetInfo(Data, offset);
        Pets.push_back(pet);
    }

    Log::WriteBattleLog("\n[Match Success]\n对手当前精灵：");
    for (auto Pet : Pets)
    {
        Log::WriteBattleLog(PetManager::GetPetName(Pet.id));
    }
}
