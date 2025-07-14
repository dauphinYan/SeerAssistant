#pragma once

#include <string>
#include <mutex>
#include "json.hpp"

using json = nlohmann::json;

class SkillManager
{
public:
    static std::string GetSkillNameByID(const uint32_t TargetID);

private:
    static bool ReadPetSkillData();

    static json PetSkills;

    static std::once_flag initFlag;
};
