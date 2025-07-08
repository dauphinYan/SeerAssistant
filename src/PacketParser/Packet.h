#pragma once

#include <vector>
#include <string>
#include <winsock2.h>
#include <cstdint>

using namespace std;

enum class EClientType;

struct PacketData
{
    int32_t Length;  // 包长 4字节
    uint8_t Version; // 版本 1字节
    int32_t CmdID;   // 命令号 4字节
    int32_t UserID;  // 米米号 4字节
    int32_t SN;      // 序列号 4字节
    std::vector<uint8_t> Body;

    void LogCout(bool bIsSend) const;
};

class PacketProcessor
{
public:
    static void SetClientType(EClientType InClientType);

    static void ProcessSendPacket(SOCKET Socket, const vector<char> &Data, int Length);
    static void ProcessRecvPacket(SOCKET Socket, const vector<char> &Data, int Length);

    static PacketData ParsePacket(const vector<uint8_t> &Packet);
    static vector<uint8_t> GroupPacket(const PacketData &Data);

    static bool ShouldDecrypt(const vector<uint8_t> &Cipher);
    static vector<uint8_t> DecryptPacket(const vector<uint8_t> &Cipher);

    static void Logining(PacketData &InPacketData);

private:
    static vector<uint8_t> s_RecvBuf;
    static size_t s_RecvBufIndex;
    static size_t s_RecvBufLen;
    static size_t s_RecvNum;
    static SOCKET s_CurrentSocket;
    static bool s_HaveLogin;
    static size_t s_SN;
    static int32_t s_UserID;
    static EClientType ClientType;
};