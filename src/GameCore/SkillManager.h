#pragma once

#include <string>
#include <mutex>
#include "json.hpp"

using json = nlohmann::json;

class SkillManager
{
public:
    static std::string GetSkillNameByID(const uint32_t targetId);

private:
    static bool ReadPetSkillData();

    static json petSkills;

    static std::once_flag initFlag;
};
