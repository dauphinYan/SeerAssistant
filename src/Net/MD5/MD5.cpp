#include "MD5.h"

namespace {
    // Constants for MD5Transform routine.
    constexpr uint32_t S[64] = {
        7,12,17,22,  7,12,17,22,  7,12,17,22,  7,12,17,22,
        5, 9,14,20,  5, 9,14,20,  5, 9,14,20,  5, 9,14,20,
        4,11,16,23,  4,11,16,23,  4,11,16,23,  4,11,16,23,
        6,10,15,21,  6,10,15,21,  6,10,15,21,  6,10,15,21
    };
    constexpr uint32_t K[64] = {
        0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,
        0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501,
        0x698098d8,0x8b44f7af,0xffff5bb1,0x895cd7be,
        0x6b901122,0xfd987193,0xa679438e,0x49b40821,
        0xf61e2562,0xc040b340,0x265e5a51,0xe9b6c7aa,
        0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8,
        0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,
        0xa9e3e905,0xfcefa3f8,0x676f02d9,0x8d2a4c8a,
        0xfffa3942,0x8771f681,0x6d9d6122,0xfde5380c,
        0xa4beea44,0x4bdecfa9,0xf6bb4b60,0xbebfbc70,
        0x289b7ec6,0xeaa127fa,0xd4ef3085,0x04881d05,
        0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,
        0xf4292244,0x432aff97,0xab9423a7,0xfc93a039,
        0x655b59c3,0x8f0ccc92,0xffeff47d,0x85845dd1,
        0x6fa87e4f,0xfe2ce6e0,0xa3014314,0x4e0811a1,
        0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391
    };
    inline uint32_t F(uint32_t x, uint32_t y, uint32_t z) { return (x & y) | (~x & z); }
    inline uint32_t G(uint32_t x, uint32_t y, uint32_t z) { return (x & z) | (y & ~z); }
    inline uint32_t H(uint32_t x, uint32_t y, uint32_t z) { return x ^ y ^ z; }
    inline uint32_t I(uint32_t x, uint32_t y, uint32_t z) { return y ^ (x | ~z); }
    inline uint32_t rotate_left(uint32_t x, uint32_t n) { return (x << n) | (x >> (32 - n)); }
}

void MD5::init() {
    _count = 0;
    _finished = false;
    // 初始化常量
    _state[0] = 0x67452301;
    _state[1] = 0xefcdab89;
    _state[2] = 0x98badcfe;
    _state[3] = 0x10325476;
}

void MD5::update(const uint8_t* input, size_t length) {
    size_t index = (_count / 8) % 64;
    _count += static_cast<uint64_t>(length) * 8;
    size_t partLen = 64 - index;
    size_t i = 0;
    if (length >= partLen) {
        std::memcpy(&_buffer[index], input, partLen);
        transform(_buffer);
        for (i = partLen; i + 63 < length; i += 64)
            transform(&input[i]);
        index = 0;
    }
    std::memcpy(&_buffer[index], &input[i], length - i);
}

void MD5::finalize() {
    if (_finished) return;
    uint8_t bits[8];
    for (int i = 0; i < 8; ++i)
        bits[i] = static_cast<uint8_t>((_count >> (8 * i)) & 0xFF);
    // 填充
    size_t index = (_count / 8) % 64;
    size_t padLen = (index < 56) ? (56 - index) : (120 - index);
    update(PADDING, padLen);
    update(bits, 8);
    // 输出
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            _digest[i * 4 + j] = static_cast<uint8_t>(
                (_state[i] >> (8 * j)) & 0xFF);
    _finished = true;
}

void MD5::transform(const uint8_t block[64]) {
    uint32_t a = _state[0], b = _state[1], c = _state[2], d = _state[3], f, g, temp;
    uint32_t M[16];
    for (int i = 0; i < 16; ++i)
        M[i] =  static_cast<uint32_t>(block[i*4])
              | (static_cast<uint32_t>(block[i*4+1]) << 8)
              | (static_cast<uint32_t>(block[i*4+2]) << 16)
              | (static_cast<uint32_t>(block[i*4+3]) << 24);
    for (int i = 0; i < 64; ++i) {
        if (i < 16)      { f = F(b,c,d); g = i; }
        else if (i < 32) { f = G(b,c,d); g = (5*i + 1) % 16; }
        else if (i < 48) { f = H(b,c,d); g = (3*i + 5) % 16; }
        else             { f = I(b,c,d); g = (7*i) % 16; }
        temp = d;
        d = c;
        c = b;
        b = b + rotate_left(a + f + K[i] + M[g], S[i]);
        a = temp;
    }
    _state[0] += a;
    _state[1] += b;
    _state[2] += c;
    _state[3] += d;
}

std::string MD5::hexdigest() const {
    static const char hexChars[] = "0123456789abcdef";
    std::string str;
    str.reserve(32);
    for (int i = 0; i < 16; ++i) {
        uint8_t byte = _digest[i];
        str.push_back(hexChars[(byte >> 4) & 0xF]);
        str.push_back(hexChars[ byte       & 0xF]);
    }
    return str;
}
