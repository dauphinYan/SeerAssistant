#pragma once

#include <cstdint>

struct PacketData;

class PetFightManager
{
public:
    static void OnNoteStartFight(const PacketData &Data);

    static void OnFightOver(const PacketData &Data);

    static void OnNoteUseSkill(const PacketData &Data);

    static void OnGetUserPerInfoByID(const PacketData &Data);

    static void OnChangePet(const PacketData &Data);

    static void SetPlayerID_0(uint32_t InID);

    static void SetPlayerID_1(uint32_t InID);

private:
    static void ShowChangePetInfo(const uint32_t curId);

private:
    static uint32_t PlayerID_0;

    static uint32_t PlayerID_1;

    static int RoundNumber;

    static bool bIsChangePet;

    static uint32_t ChangePetUser;

    static PacketData ChangePetData;
};