#pragma once

#include <vector>
#include <string>
#include <winsock2.h>
#include <cstdint>
#include <unordered_set>

using namespace std;

enum class ClientType
{
    Flash,
    Unity
};

struct PacketData
{
    int32_t length;  // 包长 4字节
    uint8_t version; // 版本 1字节
    int32_t cmdId;   // 命令号 4字节
    int32_t userID;  // 米米号 4字节
    int32_t sn;      // 序列号 4字节
    vector<uint8_t> body;

    void LogCout(bool bIsSend) const;
};

class PacketProcessor
{
public:
    static void SetClientType(ClientType clientType);

    static void ProcessSendPacket(SOCKET socket, const vector<char> &data, int length);
    static void ProcessRecvPacket(SOCKET socket, const vector<char> &data, int length);

    static PacketData ParsePacket(const vector<uint8_t> &packet);
    static vector<uint8_t> GroupPacket(const PacketData &data);

    static bool ShouldDecrypt(const vector<uint8_t> &cipher);
    static vector<uint8_t> DecryptPacket(const vector<uint8_t> &cipher);

    static void Logining(PacketData &inPacketData);

public:
    static uint32_t ReadUnsignedInt(const vector<uint8_t> &data, int &index);

    static uint8_t ReadByte(const vector<uint8_t> &data, int &index);

    static std::string ReadUTFBytes(const std::vector<uint8_t> &data, int &index, size_t length);

private:
    static ClientType clientType;

private:
    static vector<uint8_t> s_RecvBuf;
    static size_t s_RecvBufIndex;
    static size_t s_RecvBufLen;
    static size_t s_RecvNum;
    static SOCKET s_CurrentSocket;
    static bool s_HaveLogin;
    static size_t s_Sn;
    static int32_t s_UserID;

public:
    static unordered_set<int32_t> filterCmd;
};