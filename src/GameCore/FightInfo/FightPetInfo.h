#pragma once

#include "Src/Net/PacketParser/Packet.h"

struct UChangeHpInfo
{
    uint32_t id;
    uint32_t hp;
    uint32_t maxHp;
    uint32_t lock;
    uint32_t chujueNumber;
    uint32_t chujueRound;
};

class PetResistanceInfo;

class FightPetInfo
{
public:
    FightPetInfo(const PacketData &data, int &offset);

public:
    uint32_t userID;
    uint32_t petId;
    std::string petName;
    uint32_t catchTime;
    int32_t hp;
    uint32_t maxHp;
    uint32_t Lv;
    PetResistanceInfo* resistenceInfo;
    uint32_t skinId;
    uint32_t catchType;
    std::vector<UChangeHpInfo> changeHps;
    int32_t requireSwitchCthTime;
    uint32_t xinHp;
    uint32_t xinMaxHp;
    int32_t isChangeFace;
    int32_t secretLaw;
    std::vector<uint32_t> skillRunawayMarks;
    int32_t holyAndEvilThoughts;
    int32_t yearVip2022Shengjian;
    int32_t yearVip2022Chujue;

    int32_t changeFaceValue = 0;
};