#pragma once

#include <string>
#include <vector>
#include <cstdint>

class Cryptor
{
public:
    static void InitKey(const std::string &keyStr);
    static std::vector<uint8_t> Encrypt(const std::vector<uint8_t> &plain);
    static std::vector<uint8_t> Decrypt(const std::vector<uint8_t> &cipher);

private:
    static std::vector<uint8_t> key;
};