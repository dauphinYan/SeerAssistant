#pragma once

#include "Src/Net/PacketParser/Packet.h"

struct UChangeHpInfo
{
    uint32_t Id;
    uint32_t Hp;
    uint32_t MaxHp;
    uint32_t Lock;
    uint32_t ChujueNumber;
    uint32_t ChujueRound;
};

class PetResistanceInfo;

class FightPetInfo
{
public:
    FightPetInfo(const PacketData &Data, int &Offset);

public:
    uint32_t UserID;
    uint32_t PetID;
    std::string PetName;
    uint32_t CatchTime;
    int32_t Hp;
    uint32_t MaxHP;
    uint32_t Lv;
    PetResistanceInfo* ResistenceInfo;
    uint32_t SkinId;
    uint32_t CatchType;
    std::vector<UChangeHpInfo> ChangeHps;
    int32_t RequireSwitchCthTime;
    uint32_t XinHp;
    uint32_t XinMaxHp;
    int32_t IsChangeFace;
    int32_t SecretLaw;
    std::vector<uint32_t> SkillRunawayMarks;
    int32_t HolyAndEvilThoughts;
    int32_t YearVip2022Shengjian;
    int32_t YearVip2022Chujue;

    int32_t ChangeFaceValue = 0;
};