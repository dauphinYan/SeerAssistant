#include "PetResistanceInfo.h"

PetResistanceInfo::PetResistanceInfo(const PacketData &Data, int &Offset)
{
    uint32_t value = 0;

    // 暴击
    value = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    Cirt = GetBitValue(value, 17, 16);
    CirtAdj = GetBitValue(value, 1, 16);

    // 稳定性
    value = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    Regular = GetBitValue(value, 17, 16);
    RegularAdj = GetBitValue(value, 1, 16);

    // 减伤百分比
    value = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    Precent = GetBitValue(value, 17, 16);
    PrecentAdj = GetBitValue(value, 1, 16);

    // 控制抗性：ctl_1 到 ctl_3
    CtlIdx.reserve(3);
    Ctl.reserve(3);
    CtlAdj.reserve(3);
    for (int i = 0; i < 3; ++i)
    {
        value = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
        CtlIdx.push_back(static_cast<uint8_t>(GetBitValue(value, 17, 8)));
        Ctl.push_back(static_cast<uint8_t>(GetBitValue(value, 9, 8)));
        CtlAdj.push_back(static_cast<uint8_t>(GetBitValue(value, 1, 8)));
    }

    // 虚弱抗性：weak_1 到 weak_3
    WeakIdx.reserve(3);
    Weak.reserve(3);
    WeakAdj.reserve(3);
    for (int i = 0; i < 3; ++i)
    {
        value = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
        WeakIdx.push_back(static_cast<uint8_t>(GetBitValue(value, 17, 8)));
        Weak.push_back(static_cast<uint8_t>(GetBitValue(value, 9, 8)));
        WeakAdj.push_back(static_cast<uint8_t>(GetBitValue(value, 1, 8)));
    }

    // 其余字段
    ResistAll = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    ResistState = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    RedGem = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    GreenGem = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    Reserve = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
}