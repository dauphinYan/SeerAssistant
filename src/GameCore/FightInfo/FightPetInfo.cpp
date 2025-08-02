#include "FightPetInfo.h"
#include "Src/GameCore/PetInfo/PetResistanceInfo.h"

FightPetInfo::FightPetInfo(const PacketData &Data, int &Offset)
{
    UserID = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    PetID = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    PetName = PacketProcessor::ReadUTFBytes(Data.Body, Offset, 16);
    CatchTime = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    Hp = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    MaxHP = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    Lv = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    CatchType = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);

    if (Hp > static_cast<int32_t>(MaxHP))
    {
        MaxHP = static_cast<uint32_t>(Hp);
    }

    ResistenceInfo = new PetResistanceInfo(Data, Offset);
    delete ResistenceInfo;

    SkinId = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);

    uint32_t ChangeCount = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    for (uint32_t i = 0; i < ChangeCount; ++i)
    {
        UChangeHpInfo info;
        info.Id = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
        info.Hp = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
        info.MaxHp = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
        info.Lock = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
        info.ChujueNumber = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
        info.ChujueRound = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
        ChangeHps.push_back(info);

        int MarkBuffCnt = static_cast<uint32_t>(Data.Body[Offset++]);
        Offset += 3 * MarkBuffCnt;
    }

    RequireSwitchCthTime = static_cast<int32_t>(PacketProcessor::ReadUnsignedInt(Data.Body, Offset));
    XinHp = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    XinMaxHp = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    if (XinHp > XinMaxHp)
    {
        XinMaxHp = XinHp;
    }

    IsChangeFace = static_cast<int32_t>(PacketProcessor::ReadUnsignedInt(Data.Body, Offset));
    SecretLaw = static_cast<int32_t>(PacketProcessor::ReadUnsignedInt(Data.Body, Offset));

    uint32_t runawayCount = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    for (uint32_t i = 0; i < runawayCount; ++i)
    {
        SkillRunawayMarks.push_back(PacketProcessor::ReadUnsignedInt(Data.Body, Offset));
    }

    HolyAndEvilThoughts = static_cast<int32_t>(PacketProcessor::ReadUnsignedInt(Data.Body, Offset));
    YearVip2022Shengjian = static_cast<int32_t>(PacketProcessor::ReadUnsignedInt(Data.Body, Offset));
    YearVip2022Chujue = static_cast<int32_t>(PacketProcessor::ReadUnsignedInt(Data.Body, Offset));

    Offset += 3 * 2; // siteBuffInfo、bothSiteBuffInfo
    int markBuffCnt = PacketProcessor::ReadByte(Data.Body, Offset);
    Offset += 3 * markBuffCnt;

    uint32_t signCount = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    for (uint32_t i = 0; i < signCount; ++i)
    {
        PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
        PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    }

    Offset += 4 * 5; // lockedSkill

    ChangeFaceValue = 0;
}
