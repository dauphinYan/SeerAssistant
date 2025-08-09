#include "FightManager.h"
#include "Src/Net/PacketParser/Packet.h"
#include "Src/Common/Log.h"
#include "SkillManager.h"
#include "PetManager.h"
#include "Src/GameCore/FightInfo/FightPetInfo.h"

uint32_t PetFightManager::playerID_0 = 0;
uint32_t PetFightManager::playerID_1 = 0;
int PetFightManager::roundNumber = 1;

bool PetFightManager::bIsChangePet = false;
PacketData PetFightManager::changePetData = PacketData();
uint32_t PetFightManager::changePetUser = 0;

uint32_t PetFightManager::petId_0 = 0;
uint32_t PetFightManager::petId_1 = 0;

std::map<uint32_t, PetHealth> PetFightManager::petsHealth;

std::map<uint32_t, uint32_t> PetFightManager::petsIdByCatchTime;

void PetFightManager::OnNoteStartFight(const PacketData &data)
{
    Log::WriteBattleLog("Battle begin!");
    petsHealth.clear();

    int offset = 0;
    offset += 8;

    FightPetInfo fightPetInfo = FightPetInfo(data, offset);
    if (fightPetInfo.userID == playerID_0)
    {
        SetPetID_0(fightPetInfo.petId);

        FightPetInfo otherFightPetInfo = FightPetInfo(data, offset);
        SetPetID_1(otherFightPetInfo.petId);

        PetHealth newPetHealth;
        newPetHealth.CurHp = otherFightPetInfo.hp;
        newPetHealth.maxHp = otherFightPetInfo.maxHp;
        petsHealth[petId_1] = newPetHealth;
    }
    else
    {
        SetPetID_1(fightPetInfo.petId);

        PetHealth newPetHealth;
        newPetHealth.CurHp = fightPetInfo.hp;
        newPetHealth.maxHp = fightPetInfo.maxHp;
        petsHealth[petId_1] = newPetHealth;

        FightPetInfo otherFightPetInfo = FightPetInfo(data, offset);
        SetPetID_0(otherFightPetInfo.petId);
    }
}

void PetFightManager::OnFightOver(const PacketData &data)
{
    Log::WriteBattleLog("Battle end!");
    roundNumber = 1;
}

void PetFightManager::OnNoteUseSkill(const PacketData &data)
{
    Log::WriteBattleLog("———— 当前回合数：" + std::to_string(roundNumber) + " ————");
    int offset = 0;
    for (int t = 0; t < 2; ++t)
    {
        uint32_t userId = PacketProcessor::ReadUnsignedInt(data.body, offset);
        uint32_t skillId = PacketProcessor::ReadUnsignedInt(data.body, offset);
        PacketProcessor::ReadUnsignedInt(data.body, offset);
        PacketProcessor::ReadUnsignedInt(data.body, offset);
        uint32_t effectId = PacketProcessor::ReadUnsignedInt(data.body, offset);
        uint32_t atkTimes = PacketProcessor::ReadUnsignedInt(data.body, offset);
        uint32_t lostHp = PacketProcessor::ReadUnsignedInt(data.body, offset);
        uint32_t realHurtHp = PacketProcessor::ReadUnsignedInt(data.body, offset);
        int gainHp = static_cast<int>(PacketProcessor::ReadUnsignedInt(data.body, offset));
        int remainHp = static_cast<int>(PacketProcessor::ReadUnsignedInt(data.body, offset));
        uint32_t maxHp = PacketProcessor::ReadUnsignedInt(data.body, offset);
        uint32_t state = PacketProcessor::ReadUnsignedInt(data.body, offset);
        uint32_t petStatus = PacketProcessor::ReadUnsignedInt(data.body, offset);
        uint32_t skillListCount = PacketProcessor::ReadUnsignedInt(data.body, offset);
        offset += 8 * skillListCount;
        uint32_t isCrit = PacketProcessor::ReadUnsignedInt(data.body, offset);
        int statusCount = static_cast<uint32_t>(data.body[offset++]);

        offset += statusCount;

        uint32_t specailArrCount = PacketProcessor::ReadUnsignedInt(data.body, offset);

        vector<uint32_t> specails;
        for (int i = 0; i < specailArrCount; ++i)
        {
            uint32_t temp = PacketProcessor::ReadUnsignedInt(data.body, offset);
            specails.push_back(temp);
        }

        uint32_t sideEffectsCount = PacketProcessor::ReadUnsignedInt(data.body, offset);

        offset += 12 * sideEffectsCount;

        offset += 12;

        int immunizationCount = PacketProcessor::ReadUnsignedInt(data.body, offset);

        offset += 4 * immunizationCount;

        int changeHpsCount = PacketProcessor::ReadUnsignedInt(data.body, offset);

        std::vector<UChangeHpInfo> changeHps;

        for (int i = 0; i < changeHpsCount; ++i)
        {
            UChangeHpInfo ChangeHpInfo;
            ChangeHpInfo.id = PacketProcessor::ReadUnsignedInt(data.body, offset);
            ChangeHpInfo.hp = PacketProcessor::ReadUnsignedInt(data.body, offset);
            ChangeHpInfo.maxHp = PacketProcessor::ReadUnsignedInt(data.body, offset);
            ChangeHpInfo.lock = PacketProcessor::ReadUnsignedInt(data.body, offset);
            ChangeHpInfo.chujueNumber = PacketProcessor::ReadUnsignedInt(data.body, offset);
            ChangeHpInfo.chujueRound = PacketProcessor::ReadUnsignedInt(data.body, offset);

            int markBuffCnt = static_cast<uint32_t>(data.body[offset++]);

            offset += 3 * markBuffCnt;

            changeHps.push_back(ChangeHpInfo);
        }

        uint32_t requireSwitchCthTime = PacketProcessor::ReadUnsignedInt(data.body, offset);
        uint32_t maxHpSelf = PacketProcessor::ReadUnsignedInt(data.body, offset);
        uint32_t maxHpOther = PacketProcessor::ReadUnsignedInt(data.body, offset);
        int secretLaw = PacketProcessor::ReadUnsignedInt(data.body, offset);
        int skillRunawayMarkCount = PacketProcessor::ReadUnsignedInt(data.body, offset);
        offset += 4 * skillRunawayMarkCount;
        offset += 3 * 2; // siteBuffInfo、bothSiteBuffInfo

        int markBuffCnt = PacketProcessor::ReadByte(data.body, offset);
        offset += 3 * markBuffCnt;

        int32_t signInfoCount = PacketProcessor::ReadUnsignedInt(data.body, offset);
        for (int i = 0; i < signInfoCount; ++i)
        {
            PacketProcessor::ReadUnsignedInt(data.body, offset);
            PacketProcessor::ReadUnsignedInt(data.body, offset);
        }

        offset += 4 * 5; // lockedSkill

        int skillResult = PacketProcessor::ReadUnsignedInt(data.body, offset);
        offset += 4 * skillResult;

        uint32_t zhuijiId = PacketProcessor::ReadUnsignedInt(data.body, offset);
        uint32_t zhuijiHurt = PacketProcessor::ReadUnsignedInt(data.body, offset);

        // Print log.

        if (bIsChangePet && userId == changePetUser)
        {
            ShowChangePetInfo(userId);
            bIsChangePet = false;
            changePetData = PacketData();
        }

        for (const auto &info : changeHps)
        {
            Log::WriteBattleLog(std::string("[ChangeHpInfo]\n") +
                                    "ID: " + std::to_string(info.id) + "\n" +
                                    "HP: " + std::to_string(info.hp) + "\n" +
                                    "MaxHP: " + std::to_string(info.maxHp) + "\n" +
                                    "Lock: " + std::to_string(info.lock) + "\n" +
                                    "Chujue Number: " + std::to_string(info.chujueNumber) + "\n" +
                                    "Chujue Round: " + std::to_string(info.chujueRound),
                                false);

            PetHealth newPetHealth;
            newPetHealth.CurHp = info.hp;
            newPetHealth.maxHp = info.maxHp;
            petsHealth[petsIdByCatchTime[info.id]] = newPetHealth;
        }

        Log::WriteBattleLog("[UseSkill]\n用户 " + std::to_string(userId), false);
        if (userId == playerID_0)
        {
            Log::WriteBattleLog("己方" + std::string("【") + PetManager::GetPetName(petId_0) + "】" + "使用技能：【" + SkillManager::GetSkillNameByID(skillId) + "】造成伤害：" + std::to_string(lostHp) + "\n当前状态：" + std::to_string(remainHp) + "/" + std::to_string(maxHp));
        }
        else
        {
            PetHealth newPetHealth;
            newPetHealth.CurHp = remainHp;
            newPetHealth.maxHp = maxHp;
            petsHealth[petId_1] = newPetHealth;

            Log::WriteBattleLog("对方" + std::string("【") + PetManager::GetPetName(petId_1) + "】" + "使用技能：【" + SkillManager::GetSkillNameByID(skillId) + "】造成伤害：" + std::to_string(lostHp) + "\n当前状态：" + std::to_string(remainHp) + "/" + std::to_string(maxHp));
        }
    }

    PrintOtherPetInfo();

    ++roundNumber;
}

void PetFightManager::OnGetUserPerInfoByID(const PacketData &data)
{
    int offset = 0;

    uint32_t petCount = PacketProcessor::ReadUnsignedInt(data.body, offset);

    uint32_t rub = PacketProcessor::ReadUnsignedInt(data.body, offset);

    vector<UOtherPeoplePetInfo> pets;
    for (int i = 0; i < petCount; ++i)
    {
        UOtherPeoplePetInfo pet = PetManager::GetOtherPeoplePetInfo(data, offset);
        petsIdByCatchTime[pet.catchTime] = pet.id;
        pets.push_back(pet);
    }

    string petNames;
    for (auto pet : pets)
    {
        petNames += (PetManager::GetPetName(pet.id) + " ");
    }
    Log::WriteBattleLog("[Match Success]\n" +
                        std::string("对手ID：") + std::to_string(playerID_1) +
                        "\n对手当前精灵：" + petNames);
}

void PetFightManager::OnChangePet(const PacketData &data)
{
    bIsChangePet = true;
    changePetData = data;
    int offset = 0;
    changePetUser = PacketProcessor::ReadUnsignedInt(data.body, offset);
}

void PetFightManager::SetPlayerID_0(uint32_t inId)
{
    playerID_0 = inId;
}

void PetFightManager::SetPlayerID_1(uint32_t inId)
{
    playerID_1 = inId;
}

void PetFightManager::SetPetID_0(uint32_t inId)
{
    petId_0 = inId;
}

void PetFightManager::SetPetID_1(uint32_t inId)
{
    petId_1 = inId;
}

void PetFightManager::ShowChangePetInfo(const uint32_t curId)
{
    int offset = 0;

    uint32_t userId = PacketProcessor::ReadUnsignedInt(changePetData.body, offset);
    uint32_t petId = PacketProcessor::ReadUnsignedInt(changePetData.body, offset);
    uint32_t catchTime = PacketProcessor::ReadUnsignedInt(changePetData.body, offset);
    offset += 16;
    uint32_t level = PacketProcessor::ReadUnsignedInt(changePetData.body, offset);
    uint32_t hp = PacketProcessor::ReadUnsignedInt(changePetData.body, offset);
    uint32_t maxHp = PacketProcessor::ReadUnsignedInt(changePetData.body, offset);

    if (userId == playerID_0)
    {
        Log::WriteBattleLog("[ChangePet]\n" +
                            std::string("己方切换精灵【") + PetManager::GetPetName(petId) + "】\n" +
                            "切换精灵状态: " + std::to_string(hp) + "/" + std::to_string(maxHp));
        SetPetID_0(petId);
    }
    else
    {
        Log::WriteBattleLog("[ChangePet]\n" +
                            std::string("对方切换精灵【") + PetManager::GetPetName(petId) + "】\n" +
                            "切换精灵状态: " + std::to_string(hp) + "/" + std::to_string(maxHp));
        SetPetID_1(petId);
    }
}

void PetFightManager::PrintOtherPetInfo()
{

}
