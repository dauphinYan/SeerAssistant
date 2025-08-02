#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include "Src/Net/PacketParser/Packet.h"

class PetResistanceInfo
{
public:
    PetResistanceInfo(const PacketData &Data, int &Offset);

private:
    static uint32_t GetBitValue(uint32_t Value, int StartBit, int BitLength)
    {
        int shift = 32 - (StartBit + BitLength - 1);
        return (Value >> shift) & ((1u << BitLength) - 1);
    }

private:
    // 暴击相关
    uint16_t Cirt = 0;
    uint16_t CirtAdj = 0;

    // 稳定性相关
    uint16_t Regular = 0;
    uint16_t RegularAdj = 0;

    // 伤害减免相关
    uint16_t Precent = 0;
    uint16_t PrecentAdj = 0;

    // 控制抗性数组
    std::vector<uint8_t> CtlIdx;
    std::vector<uint8_t> Ctl;
    std::vector<uint8_t> CtlAdj;

    // 弱点抗性数组
    std::vector<uint8_t> WeakIdx;
    std::vector<uint8_t> Weak;
    std::vector<uint8_t> WeakAdj;

    // 其他抗性值
    uint32_t ResistAll = 0;
    uint32_t ResistState = 0;
    uint32_t RedGem = 0;
    uint32_t GreenGem = 0;
    uint32_t Reserve = 0;
};