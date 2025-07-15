#include "src/Common/Log.h"
#include "PetManager.h"

#include <iostream>

std::unordered_map<uint32_t, std::string> PetManager::PetNameMap;
std::once_flag PetManager::initFlag;

std::string PetManager::GetPetName(const uint32_t &PetID)
{
    std::call_once(initFlag, []()
                   { ReadPetBookData(); });

    auto it = PetNameMap.find(PetID);
    if (it != PetNameMap.end())
    {
        return it->second;
    }

    return "Unknown";
}

bool PetManager::ReadPetBookData()
{
    tinyxml2::XMLDocument1 doc;
    const char *filePath = "Config/Pet/PetBookData.xml";

    tinyxml2::XMLError result = doc.LoadFile(filePath);
    if (result != tinyxml2::XML_SUCCESS)
    {
        Log::WriteLog("Read pet book data failed.", LogLevel::Error);
        return false;
    }

    tinyxml2::XMLElement *root = doc.RootElement();
    if (!root)
    {
        Log::WriteLog("Can not find pet book data root.", LogLevel::Error);
        return false;
    }

    for (tinyxml2::XMLElement *elem = root->FirstChildElement("Monster");
         elem != nullptr;
         elem = elem->NextSiblingElement("Monster"))
    {
        const char *idStr = elem->Attribute("ID");
        const char *name = elem->Attribute("DefName");

        if (idStr && name)
        {
            uint32_t id = std::stoul(idStr);
            PetNameMap[id] = name;
        }
    }

    return true;
}
