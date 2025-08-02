#pragma once

#include <cstdint>
#include <map>

struct PacketData;

struct PetHealth
{
    int CurHp = 0;
    int MaxHp = 0;
};

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

    static void SetPetID_0(uint32_t InID);

    static void SetPetID_1(uint32_t InID);

private:
    static void ShowChangePetInfo(const uint32_t curId);

    static void PrintOtherPetInfo();

private:
    static uint32_t PlayerID_0;

    static uint32_t PlayerID_1;

    static int RoundNumber;

    static bool bIsChangePet;

    static uint32_t ChangePetUser;

    static PacketData ChangePetData;

    static uint32_t PetID_0;

    static uint32_t PetID_1;

private:
    static std::map<uint32_t, PetHealth> PetsHealth;

    static std::map<uint32_t, uint32_t> PetsIdByCatchTime;
};