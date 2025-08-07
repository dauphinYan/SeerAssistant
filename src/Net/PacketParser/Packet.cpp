#include "Packet.h"
#include "Cryptor.h"
#include "Src/Common/Log.h"
#include "Src/Net/MD5/MD5.h"
#include "CommandID.h"
#include "Src/Dispatcher/DispatcherManager.h"
#include "Src/GameCore/FightManager.h"

#include <sstream>
#include <iomanip>

ClientType PacketProcessor::clientType = ClientType::Flash;

vector<uint8_t> PacketProcessor::s_RecvBuf;
size_t PacketProcessor::s_RecvBufIndex = 0;
size_t PacketProcessor::s_RecvNum = 0;
SOCKET PacketProcessor::s_CurrentSocket = INVALID_SOCKET;
bool PacketProcessor::s_HaveLogin = false;
size_t PacketProcessor::s_Sn = 0;
int32_t PacketProcessor::s_UserID = 0;

/*
3405:
40002:
41080:
46046:
4047:
1002:
41228:
2441: LOAD_PERCENT
9908: 未知 in unity
*/

unordered_set<int32_t> PacketProcessor::filterCmd = {3405, 40002, 41080, 46046, 4047, 1002, 41228};

void PacketData::LogCout(bool bIsSend) const
{
    if (PacketProcessor::filterCmd.find(cmdId) != PacketProcessor::filterCmd.end())
    {
        return;
    }

    std::ostringstream oss;

    oss << "[Length=" << length
        << " Version=" << static_cast<int>(version)
        << " CmdID=" << cmdId
        << " Cmd=" << Command::GetCommandName(cmdId)
        << " UserID=" << userID
        << " SN=" << sn
        << " BodySize=" << body.size()
        << "] ";

    if (!body.empty())
    {
        oss << "Body=[";
        size_t toPrint = std::min<size_t>(body.size(), 16); // 最多打印前16个字节
        // if (CmdID == 42399)
        //     toPrint = body.size();
        // size_t toPrint = body.size();
        for (size_t i = 0; i < toPrint; ++i)
        {
            oss << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(body[i]) << " ";
        }
        if (body.size() > toPrint)
            oss << "...";
        oss << "]" << std::dec;
    }
    if (bIsSend)
    {
        Log::WriteLog("[Hooked send] Parsed Data:" + oss.str());

        // if (CmdID == 2051) // GET_SIM_USERINFO
        //     DispatcherManager::DispatchPacketEvent(CmdID, *this);
    }
    else
    {
        Log::WriteLog("[Hooked recv] Parsed Data:" + oss.str());
        if (cmdId == 2504)
            DispatcherManager::DispatchPacketEvent(cmdId, *this);
        if (cmdId == 2505)
            DispatcherManager::DispatchPacketEvent(cmdId, *this);
        if (cmdId == 2506)
            DispatcherManager::DispatchPacketEvent(cmdId, *this);
        if (cmdId == 2407)
            DispatcherManager::DispatchPacketEvent(cmdId, *this);
        if (cmdId == 41635) // GET_USERPERINFO_BY_ID
            DispatcherManager::DispatchPacketEvent(cmdId, *this);
        if (cmdId == 45139) // 用于获取对方ID
            DispatcherManager::DispatchPacketEvent(cmdId, *this);
        if (cmdId == 45141) // 用于获取对方ID，45139未获取则在45141当中。
            DispatcherManager::DispatchPacketEvent(cmdId, *this);
    }
}

void PacketProcessor::SetClientType(ClientType inClientType)
{
    clientType = inClientType;
}

void PacketProcessor::ProcessSendPacket(SOCKET socket, const vector<char> &data, int length)
{
    if (!(data[0] == 0x00 && data[1] == 0x00))
        return;

    vector<uint8_t> cipher(data.begin(), data.end());

    PacketData sendPacketData = PacketData();

    if (!s_HaveLogin)
    {
        s_CurrentSocket = socket;
    }

    vector<uint8_t> plain = ShouldDecrypt(cipher) ? DecryptPacket(cipher) : cipher;

    sendPacketData = ParsePacket(plain);
    sendPacketData.LogCout(true);

    if (s_HaveLogin)
    {
        ++sendPacketData.sn;
    }
}

void PacketProcessor::ProcessRecvPacket(SOCKET socket, const vector<char> &data, int length)
{
    PacketData recvPacketData = PacketData();

    s_RecvBuf.insert(s_RecvBuf.end(), data.begin(), data.begin() + length);

    // 是否是同一连接。
    if (s_CurrentSocket != socket)
    {
        s_RecvBufIndex += length;

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
        size_t remain = s_RecvBuf.size() - s_RecvBufIndex;

        // 不足包头长度，需等待。
        if (remain < sizeof(uint32_t))
            break;

        // 读包。
        uint32_t packetLength = 0;
        memcpy(&packetLength, &s_RecvBuf[s_RecvBufIndex], sizeof(packetLength));
        packetLength = ntohl(packetLength);

        // 包未齐，需等待。
        if (remain < packetLength)
            break;

        vector<uint8_t> cipher(s_RecvBuf.begin() + s_RecvBufIndex, s_RecvBuf.begin() + s_RecvBufIndex + packetLength);

        vector<uint8_t> plain = ShouldDecrypt(cipher) ? DecryptPacket(cipher) : cipher;

        recvPacketData = ParsePacket(plain);
        ++s_RecvNum;
        recvPacketData.LogCout(false);

        // 如果是登录包
        if (recvPacketData.cmdId == 1001)
        {
            Logining(recvPacketData);
            s_CurrentSocket = socket;
            s_Sn = recvPacketData.sn;
            s_UserID = recvPacketData.userID;
            s_HaveLogin = true;
            PetFightManager::SetPlayerID_0(s_UserID);
        }

        s_RecvBufIndex += packetLength;

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
            // s_CurrentSocket = socket;
        }
    }
}

PacketData PacketProcessor::ParsePacket(const std::vector<uint8_t> &packet)
{
    PacketData res;

    if (packet.size() < 17)
    {
        return res;
    }

    size_t offset = 0;

    // 1. 包长（4字节）
    int32_t netLen;
    memcpy(&netLen, packet.data() + offset, sizeof(netLen));
    res.length = ntohl(netLen);
    offset += 4;

    // 2. 版本号（1字节）
    res.version = packet[offset++];

    // 3. 命令号（4 字节）
    int32_t netCmd;
    memcpy(&netCmd, packet.data() + offset, sizeof(netCmd));
    res.cmdId = ntohl(netCmd);
    offset += 4;

    // 4. 用户 ID（4 字节）
    int32_t netUser;
    memcpy(&netUser, packet.data() + offset, sizeof(netUser));
    res.userID = ntohl(netUser);
    offset += 4;

    // 5. 结果/序列号（4 字节）
    int32_t netRes;
    memcpy(&netRes, packet.data() + offset, sizeof(netRes));
    res.sn = ntohl(netRes);
    offset += 4;

    // 6. 包体
    if (res.length > static_cast<int>(offset))
    {
        res.body.assign(
            packet.begin() + offset,
            packet.begin() + std::min<size_t>(packet.size(), res.length));
    }

    return res;
}

bool PacketProcessor::ShouldDecrypt(const std::vector<uint8_t> &cipher)
{
    if (clientType == ClientType::Unity)
        return false;

    if (cipher.size() < 8)
    {
        return true;
    }

    uint32_t temp = (static_cast<uint32_t>(cipher[4]) << 24) |
                    (static_cast<uint8_t>(cipher[5]) << 16) |
                    (static_cast<uint8_t>(cipher[6]) << 8) |
                    static_cast<uint8_t>(cipher[7]);

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

vector<uint8_t> PacketProcessor::DecryptPacket(const vector<uint8_t> &cipher)
{
    // 从前4字节取出密文总长度。
    int32_t netCipherLen = 0;
    memcpy(&netCipherLen, cipher.data(), sizeof(netCipherLen));
    int32_t cipherLen = ntohl(netCipherLen); // 大端转小端。

    // 因加密算法，明文长度 = 密文长度 - 1。
    vector<uint8_t> plainLeHex = GetLenthHex(cipherLen - 1); // 小端转大端
    vector<uint8_t> cipherBody(cipher.begin() + 4, cipher.end());

    vector<uint8_t> decrypted = Cryptor::Decrypt(cipherBody);

    vector<uint8_t> plain;
    plain.resize(4);
    memcpy(plain.data(), plainLeHex.data(), plainLeHex.size());

    plain.insert(plain.end(), decrypted.begin(), decrypted.end());

    return plain;
}

void PacketProcessor::Logining(PacketData &inPacketData)
{
    if (inPacketData.body.size() < 4)
    {
        return;
    }

    // 1. 取尾 4 字节并按“大端”组装
    size_t n = inPacketData.body.size();
    uint32_t tail4 = (static_cast<uint32_t>(inPacketData.body[n - 1])) | (static_cast<uint32_t>(inPacketData.body[n - 2]) << 8) | (static_cast<uint32_t>(inPacketData.body[n - 3]) << 16) | (static_cast<uint32_t>(inPacketData.body[n - 4]) << 24);

    // 2. 异或 userId
    uint32_t xorRes = tail4 ^ static_cast<uint32_t>(inPacketData.userID);

    // 3. 转为字符串
    std::string plain = std::to_string(xorRes);

    // 4. 计算 MD5
    MD5 md5;
    md5.update(reinterpret_cast<const uint8_t *>(plain.data()), plain.size());
    md5.finalize();
    std::string md5hex = md5.hexdigest();

    // 5. 取前 10 字符作密钥
    std::string key = md5hex.substr(0, 10);

    // 初始化加密算法
    Cryptor::InitKey(key);
}

uint32_t PacketProcessor::ReadUnsignedInt(const vector<uint8_t> &data, int &index)
{
    uint32_t temp = (static_cast<uint32_t>(data[index]) << 24) |
                    (static_cast<uint8_t>(data[index + 1]) << 16) |
                    (static_cast<uint8_t>(data[index + 2]) << 8) |
                    static_cast<uint8_t>(data[index + 3]);
    index += 4;
    return temp;
}

uint8_t PacketProcessor::ReadByte(const vector<uint8_t> &data, int &index)
{
    uint8_t temp = static_cast<uint8_t>(data[index++]);
    return temp;
}

std::string PacketProcessor::ReadUTFBytes(const std::vector<uint8_t> &data, int &index, size_t length)
{
    std::string result;
    result.reserve(length);

    for (size_t i = 0; i < length && index < data.size(); ++i)
    {
        result.push_back(static_cast<char>(data[index++]));
    }

    return result;
}
