TARGET = SocketHook.dll
SRC = src/SocketHook/SocketHook.cpp \
      src/MinHook/src/buffer.c \
      src/MinHook/src/hook.c \
      src/MinHook/src/trampoline.c \
      src/MinHook/src/hde/hde64.c

# 加入 MinHook 头文件搜索路径
CXXFLAGS = -I./src/MinHook/include -shared -Wall -O2
LDFLAGS = -lws2_32

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	del /f /q $(TARGET)