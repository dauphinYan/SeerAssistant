#include <string>
#include <winsock2.h>
#include <windows.h>
#include "Src/Hook/HookProtocol.h"

class Injector
{
public:
    Injector();

    void StartInjector();

private:
    bool InjectDll(DWORD pid, const std::string &dllPath, ClientType clientType);

    void PipeServerLoop();


private:
    static const std::string hookDllName;

    static const ClientType clientType;

    static const std::string gamePath_Flash;

    static const std::string gamePath_Unity;
};
