#pragma once

#include <string>
#include <vector>
#include <cstdint>

class Cryptor
{
public:
    static void InitKey(const std::string &KeyStr);
    static std::vector<uint8_t> Encrypt(const std::vector<uint8_t> &Plain);
    static std::vector<uint8_t> Decrypt(const std::vector<uint8_t> &Cipher);

private:
    static std::vector<uint8_t> Key;
};