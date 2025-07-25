#include "FightManager.h"
#include "src/Net/PacketParser/Packet.h"
#include "src/Common/Log.h"
#include "SkillManager.h"

void PetFightManager::ReadPetFightInfo(const PacketData &Data)
{
    int offset = 0;
    for (int t = 0; t < 2; ++t)
    {
        uint32_t userId = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
        uint32_t skillId = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
        PacketProcessor::ReadUnsignedInt(Data.Body, offset);
        PacketProcessor::ReadUnsignedInt(Data.Body, offset);
        uint32_t effectId = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
        uint32_t atkTimes = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
        uint32_t lostHp = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
        uint32_t realHurtHp = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
        int gainHp = static_cast<int>(PacketProcessor::ReadUnsignedInt(Data.Body, offset));
        int remainHp = static_cast<int>(PacketProcessor::ReadUnsignedInt(Data.Body, offset));
        uint32_t maxHp = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
        uint32_t state = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
        uint32_t petStatus = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
        uint32_t skillListCount = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
        offset += 8 * skillListCount;
        uint32_t isCrit = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
        int StatusCount = static_cast<uint32_t>(Data.Body[offset++]);

        offset += StatusCount;

        uint32_t SpecailArrCount = PacketProcessor::ReadUnsignedInt(Data.Body, offset);

        vector<uint32_t> Specails;
        for (int i = 0; i < SpecailArrCount; ++i)
        {
            uint32_t temp = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
            Specails.push_back(temp);
        }

        uint32_t SideEffectsCount = PacketProcessor::ReadUnsignedInt(Data.Body, offset);

        offset += 12 * SideEffectsCount;

        offset += 12;

        int ImmunizationCount = PacketProcessor::ReadUnsignedInt(Data.Body, offset);

        offset += 4 * ImmunizationCount;

        int ChangeHpsCount = PacketProcessor::ReadUnsignedInt(Data.Body, offset);

        struct UChangeHpInfo
        {
            uint32_t id;
            uint32_t hp;
            uint32_t maxhp;
            uint32_t lock;
            uint32_t chujueNumber;
            uint32_t chujueRound;
        };

        std::vector<UChangeHpInfo> changehps;

        for (int i = 0; i < ChangeHpsCount; ++i)
        {
            UChangeHpInfo ChangeHpInfo;
            ChangeHpInfo.id = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
            ChangeHpInfo.hp = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
            ChangeHpInfo.maxhp = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
            ChangeHpInfo.lock = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
            ChangeHpInfo.chujueNumber = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
            ChangeHpInfo.chujueRound = PacketProcessor::ReadUnsignedInt(Data.Body, offset);

            int MarkBuffCnt = static_cast<uint32_t>(Data.Body[offset++]);

            offset += 3 * MarkBuffCnt;

            changehps.push_back(ChangeHpInfo);
        }

        Log::WriteBattleLog("\n[UseSkill]\n用户 " + std::to_string(userId));
        Log::WriteBattleLog("使用技能：【" + SkillManager::GetSkillNameByID(skillId) + "】造成伤害：" + std::to_string(lostHp) + "\n当前状态：" + std::to_string(remainHp) + "/" + std::to_string(maxHp));

        for (const auto &info : changehps)
        {
            Log::WriteBattleLog(std::string("\n[ChangeHpInfo]\n") +
                                    "ID: " + std::to_string(info.id) + "\n" +
                                    "HP: " + std::to_string(info.hp) + "\n" +
                                    "MaxHP: " + std::to_string(info.maxhp) + "\n" +
                                    "Lock: " + std::to_string(info.lock) + "\n" +
                                    "Chujue Number: " + std::to_string(info.chujueNumber) + "\n" +
                                    "Chujue Round: " + std::to_string(info.chujueRound),
                                false);
        }

        uint32_t requireSwitchCthTime = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
        uint32_t maxHpSelf = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
        uint32_t maxHpOther = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
        int secretLaw = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
        int skillRunawayMarkCount = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
        offset += 4 * skillRunawayMarkCount;
        offset += 3 * 2; // siteBuffInfo、bothSiteBuffInfo

        int markBuffCnt = PacketProcessor::ReadByte(Data.Body, offset);
        offset += 3 * markBuffCnt;

        int32_t signInfoCount = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
        for (int i = 0; i < signInfoCount; ++i)
        {
            PacketProcessor::ReadUnsignedInt(Data.Body, offset);
            PacketProcessor::ReadUnsignedInt(Data.Body, offset);
        }

        offset += 4 * 5; // lockedSkill

        int skillResult = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
        offset += 4 * skillResult;

        uint32_t zhuijiId = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
        uint32_t zhuijiHurt = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
    }
}