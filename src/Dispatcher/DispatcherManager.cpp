#include "DispatcherManager.h"
#include "Src/Net/PacketParser/Packet.h"
#include "Src/Common/Log.h"
#include "Src/GameCore/SkillManager.h"
#include "Src/GameCore/PetManager.h"
#include "Src/GameCore/FightManager.h"

#include <iostream>

std::unordered_map<uint32_t, std::vector<DispatcherManager::PacketEventHandler>> DispatcherManager::packetEventHandlers;

bool DispatcherManager::bIsGetPlayer_1 = false;

void DispatcherManager::RegisterPacketEvent(int32_t cmdId, PacketEventHandler Handler)
{
    packetEventHandlers[cmdId].emplace_back(Handler);
}

void DispatcherManager::DispatchPacketEvent(int32_t cmdId, const PacketData &data)
{
    auto it = packetEventHandlers.find(cmdId);
    if (it != packetEventHandlers.end())
    {
        for (const auto &Handler : it->second)
        {
            Handler(data);
        }
    }
}

void DispatcherManager::InitDispatcher()
{
    DispatcherManager::RegisterPacketEvent(2504, &DispatcherManager::OnNoteStartFightCmdReceived);
    DispatcherManager::RegisterPacketEvent(2506, &DispatcherManager::OnFightOverCmdReceived);
    DispatcherManager::RegisterPacketEvent(2505, &DispatcherManager::OnNoteUseSkillCmdReceived);
    DispatcherManager::RegisterPacketEvent(2407, &DispatcherManager::OnChangePetCmdReceived);
    DispatcherManager::RegisterPacketEvent(41635, &DispatcherManager::OnGetUserPerInfoByIDCmdReceived);
    DispatcherManager::RegisterPacketEvent(2051, &DispatcherManager::OnGetSimUserInfoCmdReceived);
    DispatcherManager::RegisterPacketEvent(45139, &DispatcherManager::On45139CmdReceived);
    DispatcherManager::RegisterPacketEvent(45141, &DispatcherManager::On45141CmdReceived);
}

void DispatcherManager::OnNoteStartFightCmdReceived(const PacketData &data)
{
    PetFightManager::OnNoteStartFight(data);
}

void DispatcherManager::OnFightOverCmdReceived(const PacketData &data)
{
    PetFightManager::OnFightOver(data);
    bIsGetPlayer_1 = false;
}

void DispatcherManager::OnNoteUseSkillCmdReceived(const PacketData &data)
{
    PetFightManager::OnNoteUseSkill(data);
}

void DispatcherManager::OnChangePetCmdReceived(const PacketData &data)
{
    PetFightManager::OnChangePet(data);
}

void DispatcherManager::OnGetUserPerInfoByIDCmdReceived(const PacketData &data)
{
    PetFightManager::OnGetUserPerInfoByID(data);
}

void DispatcherManager::OnGetSimUserInfoCmdReceived(const PacketData &data)
{
    int offset = 4;
    PetFightManager::SetPlayerID_1(PacketProcessor::ReadUnsignedInt(data.body, offset));
}

void DispatcherManager::On45139CmdReceived(const PacketData &data)
{
    int offset = 0;
    uint32_t playerID_1 = PacketProcessor::ReadUnsignedInt(data.body, offset);
    if (playerID_1 != 0)
    {
        PetFightManager::SetPlayerID_1(playerID_1);
        bIsGetPlayer_1 = true;
    }
}

void DispatcherManager::On45141CmdReceived(const PacketData &data)
{
    if (bIsGetPlayer_1)
        return;
    int offset = 4;
    PetFightManager::SetPlayerID_1(PacketProcessor::ReadUnsignedInt(data.body, offset));
}
