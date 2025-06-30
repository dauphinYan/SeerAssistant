#pragma once

#include <vector>
#include <string>
#include <winsock2.h>
#include <cstdint>

using namespace std;

struct PacketData
{
    int32_t Length;  // 包长
    uint8_t Version; // 版本
    int32_t CmdID;   // 命令号
    int32_t UserID;  // 米米号
    int32_t Result;  // 序列号
    std::vector<uint8_t> Body; 

    void Display() const;
};

class PacketProcessor
{
public:
    static void ProcessRecvPacket(SOCKET Socket, const vector<char> &Data, int Length);

    static PacketData ParsePacket(const vector<uint8_t> &Packet);
    static vector<uint8_t> GroupPacket(const PacketData &Data);
};

class Encryptor
{
public:
    static void InitKey(const string &Key);
    static std::vector<uint8_t> Encrypt(const vector<uint8_t> &Plain);
    static std::vector<uint8_t> Decrypt(const vector<uint8_t> &Cipher);

private:
    static string g_Key;
};