TARGET = SocketHook.dll
SRC = src/SocketHook/SocketHook.cpp \
      src/Common/Log.cpp \
      src/PacketParser/Packet.cpp \
      src/PacketParser/Cryptor.cpp \
      src/MD5/MD5.cpp \
      src/MinHook/src/buffer.c \
      src/MinHook/src/hook.c \
      src/MinHook/src/trampoline.c \
      src/MinHook/src/hde/hde64.c

CXXFLAGS = -I. -I./src/MinHook/include -shared -Wall -O2 
LDFLAGS = -lws2_32

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	del /f /q $(TARGET)