#include <cstdint>
#include <string>
#include <winsock2.h>
#include <windows.h>

struct PacketHeader
{
    uint32_t totalSize;
    uint32_t socket;
    uint32_t payloadSize;
    uint8_t direction; // 0 = recv, 1 = send
};

enum class EClientType;

class Injector
{
public:
    Injector();

    void StartInjector();

private:
    bool InjectDll(DWORD Pid, const std::string &DllPath, EClientType ClientType);

    void PipeServerLoop();

private:
    static const wchar_t *PIPE_NAME;

    EClientType ClientType;
};