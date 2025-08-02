#include "Src/Common/Log.h"
#include "PetManager.h"
#include "Src/Net/PacketParser/Packet.h"

#include <iostream>

std::unordered_map<uint32_t, std::string> PetManager::PetNameMap;
std::once_flag PetManager::initFlag;

UOtherPeoplePetInfo PetManager::GetOtherPeoplePetInfo(const PacketData &Data, int &Offset)
{
    UOtherPeoplePetInfo pet;
    pet.catchTime = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    pet.useflag = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    pet.id = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    pet.level = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    pet.dv = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    pet.nature = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    pet.hp = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    pet.maxHp = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    pet.maxhp_adj = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    pet.atk = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    pet.atk_adj = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    pet.sp_atk = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    pet.spatk_adj = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    pet.def = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    pet.def_adj = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    pet.sp_def = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    pet.spdef_adj = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    pet.spd = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    pet.spd_adj = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);

    for (int i = 0; i < 6; ++i)
    {
        uint8_t byteVal = PacketProcessor::ReadByte(Data.Body, Offset);
        pet.evlistArray.push_back(byteVal);
    }

    uint32_t loc3 = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    for (int i = 0; i < 5; ++i)
    {
        uint32_t moveID = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
        uint32_t pp = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
        if (i < loc3)
        {
            pet.skillArray.push_back({moveID, pp});
        }
    }

    for (int m = 0; m < 6; ++m)
    {
        uint32_t spMove = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
        pet.activated_sp_movesArray.push_back(spMove);
    }

    for (int n = 0; n < 3; ++n)
    {
        uint32_t mintmark = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
        pet.mintmarksArray.push_back(mintmark);
    }

    pet.common_slot_activated = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    pet.skinId = PacketProcessor::ReadUnsignedInt(Data.Body, Offset);
    return pet;
}

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
