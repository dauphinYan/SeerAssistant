#include "SkillManager.h"
#include "src/Common/Log.h"

#include <fstream>

json SkillManager::PetSkills;
std::once_flag SkillManager::initFlag;

std::string SkillManager::GetSkillNameByID(uint32_t TargetID)
{
    if (PetSkills.empty())
    {
        std::call_once(initFlag, []()
                       {
            if (!ReadPetSkillData())
                Log::WriteLog("读取技能数据失败", LogLevel::Error); });
    }

    for (const auto &skill : PetSkills)
    {
        auto itID = skill.find("ID");
        if (itID == skill.end() || !itID->is_string())
            continue;
        const std::string &idStr = itID->get_ref<const std::string &>();

        uint32_t id = 0;
        try
        {
            id = static_cast<uint32_t>(std::stoul(idStr));
        }
        catch (...)
        {
            continue;
        }

        if (id == TargetID)
        {
            auto itName = skill.find("Name");
            if (itName != skill.end() && itName->is_string())
                return itName->get<std::string>();
        }
    }

    return "Unknown";
}

bool SkillManager::ReadPetSkillData()
{
    std::ifstream inFile("Config/Pet/PetSkillData.json");
    if (!inFile.is_open())
    {
        Log::WriteLog("无法打开 PetSkillData.json，路径检查是否正确", LogLevel::Error);
        return false;
    }

    try
    {
        inFile >> PetSkills;
    }
    catch (const json::parse_error &e)
    {
        Log::WriteLog(
            std::string("解析 PetSkillData.json 时出错：") + e.what(),
            LogLevel::Error);
        PetSkills.clear();
        return false;
    }

    return true;
}
