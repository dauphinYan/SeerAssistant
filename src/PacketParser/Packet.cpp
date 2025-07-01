#include "Packet.h"
#include "Cryptor.h"
#include "src/Common/Log.h"

#include <sstream>
#include <iomanip>

vector<uint8_t> PacketProcessor::s_RecvBuf;
size_t PacketProcessor::s_RecvIndex = 0;
SOCKET PacketProcessor::s_CurrentSocket = INVALID_SOCKET;
bool PacketProcessor::s_HaveLogin = false;

void PacketData::LogCout() const
{
    std::ostringstream oss;

    oss << "[Length=" << Length
        << " Version=" << static_cast<int>(Version)
        << " CmdID=" << CmdID
        << " UserID=" << UserID
        << " SN=" << SN
        << " BodySize=" << Body.size()
        << "] ";

    if (!Body.empty())
    {
        oss << "Body=[";
        size_t toPrint = std::min<size_t>(Body.size(), 16); // 最多打印前16个字节
        for (size_t i = 0; i < toPrint; ++i)
        {
            oss << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(Body[i]) << " ";
        }
        if (Body.size() > toPrint)
            oss << "...";
        oss << "]" << std::dec; // 恢复十进制
    }

    // 将日志写入文件
    Log::WriteLog(oss.str(), LogLevel::Temp);
}

void PacketProcessor::ProcessRecvPacket(SOCKET Socket, const vector<char> &Data, int Length)
{
    if (s_CurrentSocket == INVALID_SOCKET)
    {
        s_CurrentSocket = Socket;
    }

    PacketData RecvPacketData = PacketData();

    s_RecvBuf.insert(s_RecvBuf.end(), Data.begin(), Data.begin() + Length);

    // if (s_CurrentSocket != Socket)
    // {
    //     // size_t Peek = min<size_t>(Length, 20);

    //     s_RecvBuf.clear();
    //     s_RecvIndex = 0;
    //     return;
    // }

    while (true)
    {
        size_t Remain = s_RecvBuf.size() - s_RecvIndex;

        // 不足包头长度，需等待。
        if (Remain < sizeof(uint32_t))
            break;

        // 读包。
        uint32_t PacketLength = 0;
        memcpy(&PacketLength, &s_RecvBuf[s_RecvIndex], sizeof(PacketLength));
        PacketLength = ntohl(PacketLength); // 将32位整数从网络字节序转换为主机字节序。

        // 包未齐，需等待。
        Log::WriteLog("PacketLength = " + std::to_string(PacketLength) + ", Remain = " + std::to_string(Remain));
        if (Remain < PacketLength)
            break;

        Log::WriteLog("1");
        vector<uint8_t> Cipher(s_RecvBuf.begin() + s_RecvIndex, s_RecvBuf.begin() + s_RecvIndex + PacketLength);

        Log::WriteLog("Decrypting packet...");
        vector<uint8_t> Plain = DecryptPacket(Cipher);

        RecvPacketData = ParsePacket(Plain);
        RecvPacketData.LogCout();

        s_RecvIndex += PacketLength;

        if (s_RecvIndex == s_RecvBuf.size())
        {
            s_RecvBuf.clear();
            s_RecvIndex = 0;
            break;
        }

        if (s_RecvIndex > 0)
        {
            s_RecvBuf.erase(s_RecvBuf.begin(), s_RecvBuf.begin() + s_RecvIndex);
            s_RecvIndex = 0;
        }
    }
}

PacketData PacketProcessor::ParsePacket(const std::vector<uint8_t> &Packet)
{
    PacketData res;

    if (Packet.size() < 17)
    {
        return res;
    }

    size_t Offset = 0;

    // 1. 包长（4字节）
    int32_t NetLen;
    memcpy(&NetLen, Packet.data() + Offset, sizeof(NetLen));
    res.Length = ntohl(NetLen);
    Offset += 4;

    // 2. 版本号（1字节）
    res.Version = Packet[Offset++];

    // 3. 命令号（4 字节）
    int32_t NetCmd;
    memcpy(&NetCmd, Packet.data() + Offset, sizeof(NetCmd));
    res.CmdID = ntohl(NetCmd);
    Offset += 4;

    // 4. 用户 ID（4 字节）
    int32_t NetUser;
    memcpy(&NetUser, Packet.data() + Offset, sizeof(NetUser));
    res.UserID = ntohl(NetUser);
    Offset += 4;

    // 5. 结果/序列号（4 字节）
    int32_t NetRes;
    memcpy(&NetRes, Packet.data() + Offset, sizeof(NetRes));
    res.SN = ntohl(NetRes);
    Offset += 4;

    // 6. 包体
    if (res.Length > static_cast<int>(Offset))
    {
        res.Body.assign(
            Packet.begin() + Offset,
            Packet.begin() + std::min<size_t>(Packet.size(), res.Length));
    }

    return res;
}

vector<uint8_t> PacketProcessor::DecryptPacket(const vector<uint8_t> &Cipher)
{
    // 从前4字节取出密文总长度。
    int32_t NetCipherLen = 0;
    memcpy(&NetCipherLen, Cipher.data(), sizeof(NetCipherLen));
    int32_t CipherLen = ntohl(NetCipherLen);

    // 因加密算法，明文长度 = 密文长度 - 1。
    vector<uint8_t> CipherBody(Cipher.begin() + 4, Cipher.end());

    vector<uint8_t> Decrypted = Cryptor::Decrypt(CipherBody);

    int32_t NetPlainLen = htonl(CipherLen);
    vector<uint8_t> Plain;
    Plain.resize(4);
    memcpy(Plain.data(), &NetPlainLen, sizeof(NetPlainLen));

    Plain.insert(Plain.end(), Decrypted.begin(), Decrypted.end());

    return Plain;
}
