#include "Src/Common/Log.h"
#include "PetManager.h"
#include "Src/Net/PacketParser/Packet.h"

#include <iostream>

std::unordered_map<uint32_t, std::string> PetManager::petNameMap;
std::once_flag PetManager::initFlag;

OtherPeoplePetInfo PetManager::GetOtherPeoplePetInfo(const PacketData &data, int &offset)
{
    OtherPeoplePetInfo pet;
    pet.catchTime = PacketProcessor::ReadUnsignedInt(data.body, offset);
    pet.useflag = PacketProcessor::ReadUnsignedInt(data.body, offset);
    pet.id = PacketProcessor::ReadUnsignedInt(data.body, offset);
    pet.level = PacketProcessor::ReadUnsignedInt(data.body, offset);
    pet.dv = PacketProcessor::ReadUnsignedInt(data.body, offset);
    pet.nature = PacketProcessor::ReadUnsignedInt(data.body, offset);
    pet.hp = PacketProcessor::ReadUnsignedInt(data.body, offset);
    pet.maxHp = PacketProcessor::ReadUnsignedInt(data.body, offset);
    pet.maxhp_adj = PacketProcessor::ReadUnsignedInt(data.body, offset);
    pet.atk = PacketProcessor::ReadUnsignedInt(data.body, offset);
    pet.atk_adj = PacketProcessor::ReadUnsignedInt(data.body, offset);
    pet.sp_atk = PacketProcessor::ReadUnsignedInt(data.body, offset);
    pet.spatk_adj = PacketProcessor::ReadUnsignedInt(data.body, offset);
    pet.def = PacketProcessor::ReadUnsignedInt(data.body, offset);
    pet.def_adj = PacketProcessor::ReadUnsignedInt(data.body, offset);
    pet.sp_def = PacketProcessor::ReadUnsignedInt(data.body, offset);
    pet.spdef_adj = PacketProcessor::ReadUnsignedInt(data.body, offset);
    pet.spd = PacketProcessor::ReadUnsignedInt(data.body, offset);
    pet.spd_adj = PacketProcessor::ReadUnsignedInt(data.body, offset);

    for (int i = 0; i < 6; ++i)
    {
        uint8_t byteVal = PacketProcessor::ReadByte(data.body, offset);
        pet.evlistArray.push_back(byteVal);
    }

    uint32_t loc3 = PacketProcessor::ReadUnsignedInt(data.body, offset);
    for (int i = 0; i < 5; ++i)
    {
        uint32_t moveId = PacketProcessor::ReadUnsignedInt(data.body, offset);
        uint32_t pp = PacketProcessor::ReadUnsignedInt(data.body, offset);
        if (i < loc3)
        {
            pet.skillArray.push_back({moveId, pp});
        }
    }

    for (int m = 0; m < 6; ++m)
    {
        uint32_t spMove = PacketProcessor::ReadUnsignedInt(data.body, offset);
        pet.activated_sp_movesArray.push_back(spMove);
    }

    for (int n = 0; n < 3; ++n)
    {
        uint32_t mintmark = PacketProcessor::ReadUnsignedInt(data.body, offset);
        pet.mintmarksArray.push_back(mintmark);
    }

    pet.common_slot_activated = PacketProcessor::ReadUnsignedInt(data.body, offset);
    pet.skinId = PacketProcessor::ReadUnsignedInt(data.body, offset);
    return pet;
}

std::string PetManager::GetPetName(const uint32_t &petId)
{
    std::call_once(initFlag, []()
                   { ReadPetBookData(); });

    auto it = petNameMap.find(petId);
    if (it != petNameMap.end())
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
            petNameMap[id] = name;
        }
    }

    return true;
}
