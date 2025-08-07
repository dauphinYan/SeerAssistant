#include "FightPetInfo.h"
#include "Src/GameCore/PetInfo/PetResistanceInfo.h"

FightPetInfo::FightPetInfo(const PacketData &data, int &offset)
{
    userID = PacketProcessor::ReadUnsignedInt(data.body, offset);
    petId = PacketProcessor::ReadUnsignedInt(data.body, offset);
    petName = PacketProcessor::ReadUTFBytes(data.body, offset, 16);
    catchTime = PacketProcessor::ReadUnsignedInt(data.body, offset);
    hp = PacketProcessor::ReadUnsignedInt(data.body, offset);
    maxHp = PacketProcessor::ReadUnsignedInt(data.body, offset);
    Lv = PacketProcessor::ReadUnsignedInt(data.body, offset);
    catchType = PacketProcessor::ReadUnsignedInt(data.body, offset);

    if (hp > static_cast<int32_t>(maxHp))
    {
        maxHp = static_cast<uint32_t>(hp);
    }

    resistenceInfo = new PetResistanceInfo(data, offset);
    delete resistenceInfo;

    skinId = PacketProcessor::ReadUnsignedInt(data.body, offset);

    uint32_t ChangeCount = PacketProcessor::ReadUnsignedInt(data.body, offset);
    for (uint32_t i = 0; i < ChangeCount; ++i)
    {
        UChangeHpInfo info;
        info.id = PacketProcessor::ReadUnsignedInt(data.body, offset);
        info.hp = PacketProcessor::ReadUnsignedInt(data.body, offset);
        info.maxHp = PacketProcessor::ReadUnsignedInt(data.body, offset);
        info.lock = PacketProcessor::ReadUnsignedInt(data.body, offset);
        info.chujueNumber = PacketProcessor::ReadUnsignedInt(data.body, offset);
        info.chujueRound = PacketProcessor::ReadUnsignedInt(data.body, offset);
        changeHps.push_back(info);

        int MarkBuffCnt = static_cast<uint32_t>(data.body[offset++]);
        offset += 3 * MarkBuffCnt;
    }

    requireSwitchCthTime = static_cast<int32_t>(PacketProcessor::ReadUnsignedInt(data.body, offset));
    xinHp = PacketProcessor::ReadUnsignedInt(data.body, offset);
    xinMaxHp = PacketProcessor::ReadUnsignedInt(data.body, offset);
    if (xinHp > xinMaxHp)
    {
        xinMaxHp = xinHp;
    }

    isChangeFace = static_cast<int32_t>(PacketProcessor::ReadUnsignedInt(data.body, offset));
    secretLaw = static_cast<int32_t>(PacketProcessor::ReadUnsignedInt(data.body, offset));

    uint32_t runawayCount = PacketProcessor::ReadUnsignedInt(data.body, offset);
    for (uint32_t i = 0; i < runawayCount; ++i)
    {
        skillRunawayMarks.push_back(PacketProcessor::ReadUnsignedInt(data.body, offset));
    }

    holyAndEvilThoughts = static_cast<int32_t>(PacketProcessor::ReadUnsignedInt(data.body, offset));
    yearVip2022Shengjian = static_cast<int32_t>(PacketProcessor::ReadUnsignedInt(data.body, offset));
    yearVip2022Chujue = static_cast<int32_t>(PacketProcessor::ReadUnsignedInt(data.body, offset));

    offset += 3 * 2; // siteBuffInfo、bothSiteBuffInfo
    int markBuffCnt = PacketProcessor::ReadByte(data.body, offset);
    offset += 3 * markBuffCnt;

    uint32_t signCount = PacketProcessor::ReadUnsignedInt(data.body, offset);
    for (uint32_t i = 0; i < signCount; ++i)
    {
        PacketProcessor::ReadUnsignedInt(data.body, offset);
        PacketProcessor::ReadUnsignedInt(data.body, offset);
    }

    offset += 4 * 5; // lockedSkill

    changeFaceValue = 0;
}
