TARGET = SocketHook.dll
SRC = SocketHook/SocketHook.cpp \
      src/Third/minHook/src/buffer.c \
      src/Third/minHook/src/hook.c \
      src/Third/minHook/src/trampoline.c \
      src/Third/minHook/src/hde/hde64.c \
      src/Third/tinyxml2/tinyxml2.cpp

CXXFLAGS = -I. \
            -I./src/Third/minHook/include \
            -shared -Wall -O2 
            
LDFLAGS = -lws2_32

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	del /f /q $(TARGET)