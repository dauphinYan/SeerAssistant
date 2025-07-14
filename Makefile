TARGET = SocketHook.dll
SRC = src/SocketHook/SocketHook.cpp \
      src/Common/Log.cpp \
      src/Net/PacketParser/Packet.cpp \
      src/Net/PacketParser/Cryptor.cpp \
      src/Dispatcher/DispatcherManager.cpp \
      src/Net/MD5/MD5.cpp \
      src/GameCore/SkillManager.cpp \
      src/Third/MinHook/src/buffer.c \
      src/Third/MinHook/src/hook.c \
      src/Third/MinHook/src/trampoline.c \
      src/Third/MinHook/src/hde/hde64.c \

CXXFLAGS = -I. \
            -I./src/Third/MinHook/include \
            -I./src/Third/nlohmann -shared -Wall -O2 
LDFLAGS = -lws2_32

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	del /f /q $(TARGET)