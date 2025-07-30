#include "FightManager.h"
#include "Src/Net/PacketParser/Packet.h"
#include "Src/Common/Log.h"
#include "SkillManager.h"
#include "PetManager.h"

uint32_t PetFightManager::PlayerID_0 = 0;
uint32_t PetFightManager::PlayerID_1 = 0;
int PetFightManager::RoundNumber = 1;

bool PetFightManager::bIsChangePet = false;
PacketData PetFightManager::ChangePetData = PacketData();
uint32_t PetFightManager::ChangePetUser = 0;

void PetFightManager::OnNoteStartFight(const PacketData &Data)
{
    Log::WriteBattleLog("Battle begin!");
}

void PetFightManager::OnFightOver(const PacketData &Data)
{
    Log::WriteBattleLog("Battle end!");
    RoundNumber = 1;
}

void PetFightManager::OnNoteUseSkill(const PacketData &Data)
{
    Log::WriteBattleLog("———— 当前回合数：" + std::to_string(RoundNumber) + " ————");
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

        // Print log.

        if (bIsChangePet && userId == ChangePetUser)
        {
            ShowChangePetInfo(userId);
            bIsChangePet = false;
            ChangePetData = PacketData();
        }

        Log::WriteBattleLog("[UseSkill]\n用户 " + std::to_string(userId), false);
        Log::WriteBattleLog((userId == PlayerID_0 ? "己方" : "对方") + std::string("使用技能：【") + SkillManager::GetSkillNameByID(skillId) + "】造成伤害：" + std::to_string(lostHp) + "\n当前状态：" + std::to_string(remainHp) + "/" + std::to_string(maxHp));

        for (const auto &info : changehps)
        {
            Log::WriteBattleLog(std::string("[ChangeHpInfo]\n") +
                                    "ID: " + std::to_string(info.id) + "\n" +
                                    "HP: " + std::to_string(info.hp) + "\n" +
                                    "MaxHP: " + std::to_string(info.maxhp) + "\n" +
                                    "Lock: " + std::to_string(info.lock) + "\n" +
                                    "Chujue Number: " + std::to_string(info.chujueNumber) + "\n" +
                                    "Chujue Round: " + std::to_string(info.chujueRound),
                                false);
        }
    }

    ++RoundNumber;
}

void PetFightManager::OnGetUserPerInfoByID(const PacketData &Data)
{
    int offset = 0;

    uint32_t PetCount = PacketProcessor::ReadUnsignedInt(Data.Body, offset);

    uint32_t Rub = PacketProcessor::ReadUnsignedInt(Data.Body, offset);

    vector<EOtherPeoplePetInfo> Pets;
    for (int i = 0; i < PetCount; ++i)
    {
        EOtherPeoplePetInfo pet = PetManager::GetOtherPeoplePetInfo(Data, offset);
        Pets.push_back(pet);
    }

    string PetNames;
    for (auto Pet : Pets)
    {
        PetNames += (PetManager::GetPetName(Pet.id) + " ");
    }
    Log::WriteBattleLog("[Match Success]\n" +
                        std::string("对手ID：") + std::to_string(PlayerID_1) +
                        "\n对手当前精灵：" + PetNames);
}

void PetFightManager::OnChangePet(const PacketData &Data)
{
    bIsChangePet = true;
    ChangePetData = Data;
    int offset = 0;
    ChangePetUser = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
}

void PetFightManager::SetPlayerID_0(uint32_t InID)
{
    PlayerID_0 = InID;
}

void PetFightManager::SetPlayerID_1(uint32_t InID)
{
    PlayerID_1 = InID;
}

void PetFightManager::ShowChangePetInfo(const uint32_t curId)
{
    int offset = 0;

    uint32_t userId = PacketProcessor::ReadUnsignedInt(ChangePetData.Body, offset);
    uint32_t petId = PacketProcessor::ReadUnsignedInt(ChangePetData.Body, offset);
    uint32_t catchTime = PacketProcessor::ReadUnsignedInt(ChangePetData.Body, offset);
    offset += 16;
    uint32_t level = PacketProcessor::ReadUnsignedInt(ChangePetData.Body, offset);
    uint32_t hp = PacketProcessor::ReadUnsignedInt(ChangePetData.Body, offset);
    uint32_t maxHp = PacketProcessor::ReadUnsignedInt(ChangePetData.Body, offset);

    std::string PetName = PetManager::GetPetName(petId);
    Log::WriteBattleLog("[ChangePet]\n" +
                        std::string(userId == PlayerID_0 ? "己方切换精灵【" : "对方切换精灵【") + PetName + "】\n" +
                        "切换精灵状态: " + std::to_string(hp) + "/" + std::to_string(maxHp));
}
