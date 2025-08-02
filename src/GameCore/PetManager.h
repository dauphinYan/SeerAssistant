#pragma once

#include <mutex>
#include <string>
#include <vector>
#include <unordered_map>

#include "tinyxml2.h"

using namespace tinyxml2;

struct MoveSkill
{
    uint32_t moveID;
    uint32_t pp;
};

struct UOtherPeoplePetInfo
{
    uint32_t catchTime; // 捕获时间
    uint32_t useflag;   // 使用标志
    uint32_t id;        // 宠物ID
    uint32_t level;     // 等级
    uint32_t dv;        // 
    uint32_t nature;    // 性格
    uint32_t hp;        // 当前HP
    uint32_t maxHp;     // 最大HP
    uint32_t maxhp_adj; // 战队加成
    uint32_t atk;       // 攻击
    uint32_t atk_adj;   // 战队加成
    uint32_t sp_atk;    // 特攻
    uint32_t spatk_adj; // 战队加成
    uint32_t def;       // 防御力
    uint32_t def_adj;   // 防御力调整值
    uint32_t sp_def;    // 特防
    uint32_t spdef_adj; // 战队加成
    uint32_t spd;       // 速度
    uint32_t spd_adj;   // 战队加成

    std::vector<uint8_t> evlistArray;              // 努力值数组
    std::vector<MoveSkill> skillArray;             // 技能数组
    std::vector<uint32_t> activated_sp_movesArray; // 激活的特殊技能数组
    std::vector<uint32_t> mintmarksArray;          // Mint标记数组
    uint32_t common_slot_activated;                // 通用槽激活状态
    uint32_t skinId;                               // 皮肤ID
};

class PetManager
{
public:
    static UOtherPeoplePetInfo GetOtherPeoplePetInfo(const PacketData &Data, int &Offset);

public:
    static std::string GetPetName(const uint32_t &PetID);

private:
    static bool ReadPetBookData();

    static std::unordered_map<uint32_t, std::string> PetNameMap;

    static std::once_flag initFlag;
};

// http://seerh5.61.com/resource/assets/pet/head/1.png 精灵头像下载。