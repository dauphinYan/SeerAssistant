# SeerAssistant

基于C++的赛尔号巅峰辅助。

### 功能
  
- 记录双方历史操作。
  
- 记录对手精灵血量。

### 说明

- 适配程度：`Flash`、`Unity`双端均已适配。

- `SocketHook.dll`仅用作捕获网络通信中的数据包。

- `Start.cpp`为程序入口。

- `Log/System`目录下存放程序运行时日志，日志等级分为`Temp`与`Error`两种。
  
- `Log/User`目录下存放对战信息。

### 运行

#### 编译 `SocketHook.dll`

详见 [SeerHook](https://github.com/dauphinYan/SeerHook)。

#### 使用CMake构建项目

1. 修改`Src/Start.cpp`中`GamePath`为游戏启动路径。

2. 使用`CMake`进行项目构建。
   
3. 构建完成后，将`Config`文件夹放入`SeerAssistant.exe`所在的目录。
   
4. 将前面编译得到的`SocketHook.dll`放入`SeerAssistant.exe`所在的目录中。

### 声明

本项目采用的第三方库有：`nlohmann`、`tinyxml2`。

本项目仅用于**技术研究与学习交流**，请勿用于任何商业用途。

所有相关资源版权归**上海淘米网络科技有限公司**所有。

如因滥用本项目造成法律纠纷，**责任由使用者自行承担**。

