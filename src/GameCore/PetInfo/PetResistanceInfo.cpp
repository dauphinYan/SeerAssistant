#include "PetResistanceInfo.h"

PetResistanceInfo::PetResistanceInfo(const PacketData &data, int &offset)
{
    uint32_t value = 0;

    // 暴击
    value = PacketProcessor::ReadUnsignedInt(data.body, offset);
    cirt = GetBitValue(value, 17, 16);
    cirtAdj = GetBitValue(value, 1, 16);

    // 稳定性
    value = PacketProcessor::ReadUnsignedInt(data.body, offset);
    regular = GetBitValue(value, 17, 16);
    regularAdj = GetBitValue(value, 1, 16);

    // 减伤百分比
    value = PacketProcessor::ReadUnsignedInt(data.body, offset);
    precent = GetBitValue(value, 17, 16);
    precentAdj = GetBitValue(value, 1, 16);

    // 控制抗性：ctl_1 到 ctl_3
    ctlIdx.reserve(3);
    ctl.reserve(3);
    ctlAdj.reserve(3);
    for (int i = 0; i < 3; ++i)
    {
        value = PacketProcessor::ReadUnsignedInt(data.body, offset);
        ctlIdx.push_back(static_cast<uint8_t>(GetBitValue(value, 17, 8)));
        ctl.push_back(static_cast<uint8_t>(GetBitValue(value, 9, 8)));
        ctlAdj.push_back(static_cast<uint8_t>(GetBitValue(value, 1, 8)));
    }

    // 虚弱抗性：weak_1 到 weak_3
    weakIdx.reserve(3);
    weak.reserve(3);
    weakAdj.reserve(3);
    for (int i = 0; i < 3; ++i)
    {
        value = PacketProcessor::ReadUnsignedInt(data.body, offset);
        weakIdx.push_back(static_cast<uint8_t>(GetBitValue(value, 17, 8)));
        weak.push_back(static_cast<uint8_t>(GetBitValue(value, 9, 8)));
        weakAdj.push_back(static_cast<uint8_t>(GetBitValue(value, 1, 8)));
    }

    // 其余字段
    resistAll = PacketProcessor::ReadUnsignedInt(data.body, offset);
    resistState = PacketProcessor::ReadUnsignedInt(data.body, offset);
    redGem = PacketProcessor::ReadUnsignedInt(data.body, offset);
    greenGem = PacketProcessor::ReadUnsignedInt(data.body, offset);
    reserve = PacketProcessor::ReadUnsignedInt(data.body, offset);
}