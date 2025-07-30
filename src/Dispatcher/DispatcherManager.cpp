#include "DispatcherManager.h"
#include "Src/Net/PacketParser/Packet.h"
#include "Src/Common/Log.h"
#include "Src/GameCore/SkillManager.h"
#include "Src/GameCore/PetManager.h"
#include "Src/GameCore/FightManager.h"

#include <iostream>

std::unordered_map<uint32_t, std::vector<DispatcherManager::PacketEventHandler>> DispatcherManager::PacketEventHandlers;

bool DispatcherManager::bIsGetPlayer_1 = false;

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
    DispatcherManager::RegisterPacketEvent(2504, &DispatcherManager::OnNoteStartFightCmdReceived);
    DispatcherManager::RegisterPacketEvent(2506, &DispatcherManager::OnFightOverCmdReceived);
    DispatcherManager::RegisterPacketEvent(2505, &DispatcherManager::OnNoteUseSkillCmdReceived);
    DispatcherManager::RegisterPacketEvent(2407, &DispatcherManager::OnChangePetCmdReceived);
    DispatcherManager::RegisterPacketEvent(41635, &DispatcherManager::OnGetUserPerInfoByIDCmdReceived);
    DispatcherManager::RegisterPacketEvent(2051, &DispatcherManager::OnGetSimUserInfoCmdReceived);
    DispatcherManager::RegisterPacketEvent(45139, &DispatcherManager::On45139CmdReceived);
    DispatcherManager::RegisterPacketEvent(45141, &DispatcherManager::On45141CmdReceived);
}

void DispatcherManager::OnNoteStartFightCmdReceived(const PacketData &Data)
{
    PetFightManager::OnNoteStartFight(Data);
}

void DispatcherManager::OnFightOverCmdReceived(const PacketData &Data)
{
    PetFightManager::OnFightOver(Data);
    bIsGetPlayer_1 = false;
}

void DispatcherManager::OnNoteUseSkillCmdReceived(const PacketData &Data)
{
    PetFightManager::OnNoteUseSkill(Data);
}

void DispatcherManager::OnChangePetCmdReceived(const PacketData &Data)
{
    PetFightManager::OnChangePet(Data);
}

void DispatcherManager::OnGetUserPerInfoByIDCmdReceived(const PacketData &Data)
{
    PetFightManager::OnGetUserPerInfoByID(Data);
}

void DispatcherManager::OnGetSimUserInfoCmdReceived(const PacketData &Data)
{
    int offset = 4;
    PetFightManager::SetPlayerID_1(PacketProcessor::ReadUnsignedInt(Data.Body, offset));
}

void DispatcherManager::On45139CmdReceived(const PacketData &Data)
{
    int offset = 0;
    uint32_t PlayerID_1 = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
    if (PlayerID_1 != 0)
    {
        PetFightManager::SetPlayerID_1(PlayerID_1);
        bIsGetPlayer_1 = true;
    }
}

void DispatcherManager::On45141CmdReceived(const PacketData &Data)
{
    if (bIsGetPlayer_1)
        return;
    int offset = 4;
    PetFightManager::SetPlayerID_1(PacketProcessor::ReadUnsignedInt(Data.Body, offset));
}
