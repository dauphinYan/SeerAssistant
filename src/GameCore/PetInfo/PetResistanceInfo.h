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
    static uint32_t GetBitValue(uint32_t value, int startBit, int bitLength)
    {
        int shift = 32 - (startBit + bitLength - 1);
        return (value >> shift) & ((1u << bitLength) - 1);
    }

private:
    // 暴击相关
    uint16_t cirt = 0;
    uint16_t cirtAdj = 0;

    // 稳定性相关
    uint16_t regular = 0;
    uint16_t regularAdj = 0;

    // 伤害减免相关
    uint16_t precent = 0;
    uint16_t precentAdj = 0;

    // 控制抗性数组
    std::vector<uint8_t> ctlIdx;
    std::vector<uint8_t> ctl;
    std::vector<uint8_t> ctlAdj;

    // 弱点抗性数组
    std::vector<uint8_t> weakIdx;
    std::vector<uint8_t> weak;
    std::vector<uint8_t> weakAdj;

    // 其他抗性值
    uint32_t resistAll = 0;
    uint32_t resistState = 0;
    uint32_t redGem = 0;
    uint32_t greenGem = 0;
    uint32_t reserve = 0;
};