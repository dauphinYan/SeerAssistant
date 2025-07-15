#pragma once

#include <mutex>
#include <string>
#include <unordered_map>

#include "tinyxml2.h"

using namespace tinyxml2;

class PetManager
{
public:
    static std::string GetPetName(const uint32_t &PetID);

private:
    static bool ReadPetBookData();

    static std::unordered_map<uint32_t, std::string> PetNameMap;

    static std::once_flag initFlag;
};