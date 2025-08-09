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

enum class ClientType;

class Injector
{
public:
    Injector();

    void StartInjector();

private:
    bool InjectDll(DWORD pid, const std::string &dllPath, ClientType clientType);

    void PipeServerLoop();

    bool ReadExact(HANDLE pipe, void *buffer, size_t size);

private:
    static const wchar_t *PIPE_NAME;

    static const std::string hookDllName;

    ClientType clientType;
};