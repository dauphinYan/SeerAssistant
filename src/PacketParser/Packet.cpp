#include "Packet.h"
#include "Cryptor.h"
#include "src/Common/Log.h"
#include "src/MD5/MD5.h"
#include "CommandID.h"

#include <sstream>
#include <iomanip>

vector<uint8_t> PacketProcessor::s_RecvBuf;
size_t PacketProcessor::s_RecvBufIndex = 0;
size_t PacketProcessor::s_RecvNum = 0;
SOCKET PacketProcessor::s_CurrentSocket = INVALID_SOCKET;
bool PacketProcessor::s_HaveLogin = false;
size_t PacketProcessor::s_SN = 0;
int32_t PacketProcessor::s_UserID = 0;

void PacketData::LogCout(bool bIsSend) const
{
    std::ostringstream oss;

    oss << "[Length=" << Length
        << " Version=" << static_cast<int>(Version)
        << " CmdID=" << CmdID
        << " Cmd=" << Command::GetCommandName(CmdID)
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
    if (bIsSend)
        Log::WriteLog("[Hooked send] Parsed Data:" + oss.str(), LogLevel::Temp);
    else
        Log::WriteLog("[Hooked recv] Parsed Data:" + oss.str(), LogLevel::Temp);
}

void PacketProcessor::ProcessSendPacket(SOCKET Socket, const vector<char> &Data, int Length)
{
    if (!(Data[0] == 0x00 && Data[1] == 0x00))
        return;

    vector<uint8_t> SendBuf(Data.begin(), Data.end());

    PacketData SendPacketData = PacketData();

    if (!s_HaveLogin)
    {
        s_CurrentSocket = Socket;
    }

    vector<uint8_t> Plain = ShouldDecrypt(SendBuf) ? DecryptPacket(SendBuf) : SendBuf;

    SendPacketData = ParsePacket(Plain);
    SendPacketData.LogCout(true);

    if (s_HaveLogin)
    {
        ++SendPacketData.SN;
    }
}

void PacketProcessor::ProcessRecvPacket(SOCKET Socket, const vector<char> &Data, int Length)
{
    PacketData RecvPacketData = PacketData();

    s_RecvBuf.insert(s_RecvBuf.end(), Data.begin(), Data.begin() + Length);

    // 判断接收的数据是否属于同一类数据
    if (s_CurrentSocket != Socket)
    {
        // string bufferHex;
        // for (size_t i = 0; i < s_RecvBuf.size(); ++i)
        // {
        //     char buf[4];
        //     sprintf(buf, "%02X ", static_cast<unsigned char>(s_RecvBuf[i]));
        //     bufferHex += buf;
        // }
        // Log::WriteLog("s_RecvBufIndex = " + std::to_string(s_RecvBufIndex));
        // Log::WriteLog("s_RecvBuf = " + bufferHex);

        s_RecvBufIndex += Length;

        // 此时索引等于缓冲区长度，则说明刚好取完此包。
        if (s_RecvBufIndex == s_RecvBuf.size())
        {
            s_RecvBuf.clear();
            s_RecvBufIndex = 0;
        }

        return;
    }

    while (true)
    {
        size_t Remain = s_RecvBuf.size() - s_RecvBufIndex;

        // 不足包头长度，需等待。
        if (Remain < sizeof(uint32_t))
            break;

        // 读包。
        uint32_t PacketLength = 0;
        memcpy(&PacketLength, &s_RecvBuf[s_RecvBufIndex], sizeof(PacketLength));
        PacketLength = ntohl(PacketLength);

        // 包未齐，需等待。
        if (Remain < PacketLength)
            break;

        vector<uint8_t> Cipher(s_RecvBuf.begin() + s_RecvBufIndex, s_RecvBuf.begin() + s_RecvBufIndex + PacketLength);

        vector<uint8_t> Plain = ShouldDecrypt(Cipher) ? DecryptPacket(Cipher) : Cipher;

        RecvPacketData = ParsePacket(Plain);
        ++s_RecvNum;
        RecvPacketData.LogCout(false);

        // 如果是登录包
        if (RecvPacketData.CmdID == 1001)
        {
            Logining(RecvPacketData);
            s_CurrentSocket = Socket;
            s_SN = RecvPacketData.SN;
            s_UserID = RecvPacketData.UserID;
            s_HaveLogin = true;
        }

        s_RecvBufIndex += PacketLength;

        if (s_RecvBufIndex == s_RecvBuf.size())
        {
            s_RecvBuf.clear();
            s_RecvBufIndex = 0;
            break;
        }

        if (s_RecvBufIndex > 0)
        {
            s_RecvBuf.erase(s_RecvBuf.begin(), s_RecvBuf.begin() + s_RecvBufIndex);
            s_RecvBufIndex = 0;
            s_CurrentSocket = Socket;
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

bool PacketProcessor::ShouldDecrypt(const std::vector<uint8_t> &Cipher)
{
    if (Cipher.size() < 8)
    {
        return true;
    }

    uint32_t temp = (static_cast<uint32_t>(Cipher[4]) << 24) |
                    (static_cast<uint8_t>(Cipher[5]) << 16) |
                    (static_cast<uint8_t>(Cipher[6]) << 8) |
                    static_cast<uint8_t>(Cipher[7]);

    if (temp == 0x31000000 || temp == 0x00000000)
    {
        return false;
    }

    return true;
}

std::vector<uint8_t> GetLenthHex(int32_t v)
{
    std::vector<uint8_t> b(4);
    b[0] = static_cast<uint8_t>((v >> 24) & 0xFF);
    b[1] = static_cast<uint8_t>((v >> 16) & 0xFF);
    b[2] = static_cast<uint8_t>((v >> 8) & 0xFF);
    b[3] = static_cast<uint8_t>(v & 0xFF);
    return b;
}

vector<uint8_t> PacketProcessor::DecryptPacket(const vector<uint8_t> &Cipher)
{
    // 从前4字节取出密文总长度。
    int32_t NetCipherLen = 0;
    memcpy(&NetCipherLen, Cipher.data(), sizeof(NetCipherLen));
    int32_t CipherLen = ntohl(NetCipherLen); // 大端转小端。

    // 因加密算法，明文长度 = 密文长度 - 1。
    vector<uint8_t> PlainLeHex = GetLenthHex(CipherLen - 1); // 小端转大端
    vector<uint8_t> CipherBody(Cipher.begin() + 4, Cipher.end());

    vector<uint8_t> Decrypted = Cryptor::Decrypt(CipherBody);

    vector<uint8_t> Plain;
    Plain.resize(4);
    memcpy(Plain.data(), &PlainLeHex, PlainLeHex.size());

    Plain.insert(Plain.end(), Decrypted.begin(), Decrypted.end());

    return Plain;
}

void PacketProcessor::Logining(PacketData &InPacketData)
{
    if (InPacketData.Body.size() < 4)
    {
        // 长度不足，直接返回
        return;
    }

    // 取后 4 字节（小端序）
    size_t n = InPacketData.Body.size();
    uint32_t tail4 = static_cast<uint32_t>(InPacketData.Body[n - 4]) | (static_cast<uint32_t>(InPacketData.Body[n - 3]) << 8) | (static_cast<uint32_t>(InPacketData.Body[n - 2]) << 16) | (static_cast<uint32_t>(InPacketData.Body[n - 1]) << 24);

    // 异或 userId 并转字符串
    uint32_t xorRes = tail4 ^ static_cast<uint32_t>(InPacketData.UserID);
    std::string plain = std::to_string(xorRes);

    // 计算 MD5
    MD5 md5;
    md5.update(reinterpret_cast<const uint8_t *>(plain.data()), plain.size());
    md5.finalize();
    std::string md5hex = md5.hexdigest();

    // 取前 10 字符作密钥
    std::string key = md5hex.substr(0, 10);

    // 调用外部算法初始化
    Cryptor::InitKey(key);
}