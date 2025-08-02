#include "FightManager.h"
#include "Src/Net/PacketParser/Packet.h"
#include "Src/Common/Log.h"
#include "SkillManager.h"
#include "PetManager.h"
#include "Src/GameCore/FightInfo/FightPetInfo.h"

uint32_t PetFightManager::PlayerID_0 = 0;
uint32_t PetFightManager::PlayerID_1 = 0;
int PetFightManager::RoundNumber = 1;

bool PetFightManager::bIsChangePet = false;
PacketData PetFightManager::ChangePetData = PacketData();
uint32_t PetFightManager::ChangePetUser = 0;

uint32_t PetFightManager::PetID_0 = 0;
uint32_t PetFightManager::PetID_1 = 0;

std::map<uint32_t, PetHealth> PetFightManager::PetsHealth;

std::map<uint32_t, uint32_t> PetFightManager::PetsIdByCatchTime;

void PetFightManager::OnNoteStartFight(const PacketData &Data)
{
    Log::WriteBattleLog("Battle begin!");
    PetsHealth.clear();

    int Offset = 0;
    Offset += 8;

    FightPetInfo fightPetInfo = FightPetInfo(Data, Offset);
    if (fightPetInfo.UserID == PlayerID_0)
    {
        SetPetID_0(fightPetInfo.PetID);

        FightPetInfo OtherFightPetInfo = FightPetInfo(Data, Offset);
        SetPetID_1(OtherFightPetInfo.PetID);

        PetHealth NewPetHealth;
        NewPetHealth.CurHp = OtherFightPetInfo.Hp;
        NewPetHealth.MaxHp = OtherFightPetInfo.MaxHP;
        PetsHealth[PetID_1] = NewPetHealth;
    }
    else
    {
        SetPetID_1(fightPetInfo.PetID);

        PetHealth NewPetHealth;
        NewPetHealth.CurHp = fightPetInfo.Hp;
        NewPetHealth.MaxHp = fightPetInfo.MaxHP;
        PetsHealth[PetID_1] = NewPetHealth;

        FightPetInfo OtherFightPetInfo = FightPetInfo(Data, Offset);
        SetPetID_0(OtherFightPetInfo.PetID);
    }
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

        std::vector<UChangeHpInfo> changehps;

        for (int i = 0; i < ChangeHpsCount; ++i)
        {
            UChangeHpInfo ChangeHpInfo;
            ChangeHpInfo.Id = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
            ChangeHpInfo.Hp = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
            ChangeHpInfo.MaxHp = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
            ChangeHpInfo.Lock = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
            ChangeHpInfo.ChujueNumber = PacketProcessor::ReadUnsignedInt(Data.Body, offset);
            ChangeHpInfo.ChujueRound = PacketProcessor::ReadUnsignedInt(Data.Body, offset);

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

        for (const auto &info : changehps)
        {
            Log::WriteBattleLog(std::string("[ChangeHpInfo]\n") +
                                    "ID: " + std::to_string(info.Id) + "\n" +
                                    "HP: " + std::to_string(info.Hp) + "\n" +
                                    "MaxHP: " + std::to_string(info.MaxHp) + "\n" +
                                    "Lock: " + std::to_string(info.Lock) + "\n" +
                                    "Chujue Number: " + std::to_string(info.ChujueNumber) + "\n" +
                                    "Chujue Round: " + std::to_string(info.ChujueRound),
                                false);

            PetHealth NewPetHealth;
            NewPetHealth.CurHp = info.Hp;
            NewPetHealth.MaxHp = info.MaxHp;
            PetsHealth[PetsIdByCatchTime[info.Id]] = NewPetHealth;
        }

        Log::WriteBattleLog("[UseSkill]\n用户 " + std::to_string(userId), false);
        if (userId == PlayerID_0)
        {
            Log::WriteBattleLog("己方" + std::string("【") + PetManager::GetPetName(PetID_0) + "】" + "使用技能：【" + SkillManager::GetSkillNameByID(skillId) + "】造成伤害：" + std::to_string(lostHp) + "\n当前状态：" + std::to_string(remainHp) + "/" + std::to_string(maxHp));
        }
        else
        {
            PetHealth NewPetHealth;
            NewPetHealth.CurHp = remainHp;
            NewPetHealth.MaxHp = maxHp;
            PetsHealth[PetID_1] = NewPetHealth;

            Log::WriteBattleLog("对方" + std::string("【") + PetManager::GetPetName(PetID_1) + "】" + "使用技能：【" + SkillManager::GetSkillNameByID(skillId) + "】造成伤害：" + std::to_string(lostHp) + "\n当前状态：" + std::to_string(remainHp) + "/" + std::to_string(maxHp));
            PrintOtherPetInfo();
        }
    }

    ++RoundNumber;
}

void PetFightManager::OnGetUserPerInfoByID(const PacketData &Data)
{
    int offset = 0;

    uint32_t PetCount = PacketProcessor::ReadUnsignedInt(Data.Body, offset);

    uint32_t Rub = PacketProcessor::ReadUnsignedInt(Data.Body, offset);

    vector<UOtherPeoplePetInfo> Pets;
    for (int i = 0; i < PetCount; ++i)
    {
        UOtherPeoplePetInfo pet = PetManager::GetOtherPeoplePetInfo(Data, offset);
        PetsIdByCatchTime[pet.catchTime] = pet.id;
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

void PetFightManager::SetPetID_0(uint32_t InID)
{
    PetID_0 = InID;
}

void PetFightManager::SetPetID_1(uint32_t InID)
{
    PetID_1 = InID;
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

    if (userId == PlayerID_0)
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
    int count = 0;
    std::string line = "对方精灵状况：\n";

    for (auto it : PetsHealth)
    {
        if (it.first == 0)
            continue;
        std::string petName = PetManager::GetPetName(it.first);
        std::string hpStr = std::to_string(it.second.CurHp) + "/" + std::to_string(it.second.MaxHp);

        std::string petInfo = petName + "：" + hpStr + "  ";

        if (count >= 4)
        {
            line += "\n";
            count = 0;
        }
        line += petInfo;
        count++;
    }

    Log::WriteBattleLog(line);
}
