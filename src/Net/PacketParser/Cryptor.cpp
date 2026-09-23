#include "Cryptor.h"
#include "Src/Common/Log.h"

std::vector<uint8_t> Cryptor::key;

// 将 vector A 的 [start, end) 部分切片出来。
std::vector<uint8_t> Slice(const std::vector<uint8_t> &A, size_t start, size_t end)
{
    if (start >= end || end > A.size())
        return {};
    return std::vector<uint8_t>(A.begin() + start, A.begin() + end);
}

// 合并两个 vector。
std::vector<uint8_t> Merge(const std::vector<uint8_t> &A, const std::vector<uint8_t> &B)
{
    std::vector<uint8_t> C;
    C.reserve(A.size() + B.size());
    C.insert(C.end(), A.begin(), A.end());
    C.insert(C.end(), B.begin(), B.end());
    return C;
}

void Cryptor::InitKey(const std::string &keyStr)
{
    key.assign(keyStr.begin(), keyStr.end());
    Log::WriteLog("Initialize key: " + std::string(key.begin(), key.end()));
}

std::vector<uint8_t> Cryptor::Encrypt(const std::vector<uint8_t> &plain)
{
    return std::vector<uint8_t>();
}

std::vector<uint8_t> Cryptor::Decrypt(const std::vector<uint8_t> &cipher)
{
    size_t len = cipher.size();
    if (len == 0 || key.empty())
        return {};

    // 计算旋转量
    int result = key[(len - 1) % key.size()] * 13 % len;

    // 环形右移 result 个字节
    std::vector<uint8_t> rotated = Merge(
        Slice(cipher, len - result, len),
        Slice(cipher, 0, len - result));

    // 右移/左移组合恢复原始字节序列（去掉最后一个字节）
    std::vector<uint8_t> plain(len - 1);
    for (size_t i = 0; i < len - 1; ++i)
    {
        plain[i] = static_cast<uint8_t>((rotated[i] >> 5) | (rotated[i + 1] << 3));
    }

    // 异或解密
    size_t j = 0;
    bool needBecomeZero = false;
    for (size_t i = 0; i < plain.size(); ++i)
    {
        if (j == 1 && needBecomeZero)
        {
            j = 0;
            needBecomeZero = false;
        }
        if (j == key.size())
        {
            j = 0;
            needBecomeZero = true;
        }
        plain[i] = static_cast<uint8_t>(plain[i] ^ key[j]);
        ++j;
    }

    return plain;
}
