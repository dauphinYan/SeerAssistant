TARGET = SocketHook.dll
SRC = src/SocketHook/SocketHook.cpp \
      src/Common/Log.cpp \
      src/Net/PacketParser/Packet.cpp \
      src/Net/PacketParser/Cryptor.cpp \
      src/Dispatcher/DispatcherManager.cpp \
      src/Net/MD5/MD5.cpp \
      src/GameCore/SkillManager.cpp \
      src/GameCore/PetManager.cpp \
      src/Third/minHook/src/buffer.c \
      src/Third/minHook/src/hook.c \
      src/Third/minHook/src/trampoline.c \
      src/Third/minHook/src/hde/hde64.c \
      src/Third/tinyxml2/tinyxml2.cpp

CXXFLAGS = -I. \
            -I./src/Third/minHook/include \
            -I./src/Third/nlohmann \
            -I./src/Third/tinyxml2 -shared -Wall -O2 
            
LDFLAGS = -lws2_32

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	del /f /q $(TARGET)