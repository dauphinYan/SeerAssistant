#pragma once

#include <cstdint>
#include <map>

struct PacketData;

struct PetHealth
{
    int CurHp = 0;
    int maxHp = 0;
};

class PetFightManager
{
public:
    static void OnNoteStartFight(const PacketData &data);

    static void OnFightOver(const PacketData &data);

    static void OnNoteUseSkill(const PacketData &data);

    static void OnGetUserPerInfoByID(const PacketData &data);

    static void OnChangePet(const PacketData &data);

    static void SetPlayerID_0(uint32_t inId);

    static void SetPlayerID_1(uint32_t inId);

    static void SetPetID_0(uint32_t inId);

    static void SetPetID_1(uint32_t inId);

private:
    static void ShowChangePetInfo(const uint32_t curId);

    static void PrintOtherPetInfo();

private:
    static uint32_t playerID_0;

    static uint32_t playerID_1;

    static int roundNumber;

    static bool bIsChangePet;

    static uint32_t changePetUser;

    static PacketData changePetData;

    static uint32_t petId_0;

    static uint32_t petId_1;

private:
    static std::map<uint32_t, PetHealth> petsHealth;

    static std::map<uint32_t, uint32_t> petsIdByCatchTime;
};