#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

class MD5
{
public:
    MD5() { init(); }
    void update(const uint8_t *data, size_t len);
    void finalize();
    std::string hexdigest() const;

private:
    void init();
    void transform(const uint8_t block[64]);

    uint32_t _state[4];
    uint64_t _count;
    uint8_t _buffer[64];
    uint8_t _digest[16];
    bool _finished;
    static constexpr uint8_t PADDING[64] = {0x80};
};