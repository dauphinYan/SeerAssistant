# SeerAssistant

基于C++的赛尔号巅峰辅助。

QT版下载：https://dauphinyan.lanzoub.com/b00b4xu1ef 密码：hggz

入门链接：[赛尔号通信协议逆向：封包分析](https://dolphindream.cn/article/blog/seer-protocol-analysis/)

### 功能

- 抓包并解析数据。

### 说明

- 适配程度：`Flash`、`Unity`双端均已适配。

- `SocketHook.dll`仅用作捕获网络通信中的数据包。

- `Start.cpp`为程序入口。

- `Log/System`目录下存放程序运行时日志，日志等级分为`Temp`与`Error`两种。

### 运行

！！！ 请注意：需要自行配置`CMake`与`MinGW`编译器

#### 使用CMake构建项目

1. 修改`Src/Start.cpp`中部分信息。

    ```cpp
    const ClientType Injector::clientType = ClientType::Unity;

    const std::string Injector::gamePath_Flash = R"(填写你的路径)";

    const std::string Injector::gamePath_Unity = R"(填写你的路径)";
    ```

2. 使用`CMake`进行项目构建。

3. 运行`SeerAssistant.exe`

### 特别声明

本项目采用的第三方库有：`MinHook`。

本项目仅用于**技术研究与学习交流**，请勿用于任何商业用途。

所有相关资源版权归**上海淘米网络科技有限公司**所有。

如因滥用本项目造成法律纠纷，**责任由使用者自行承担**。

### 联系作者

邮箱：584485321@qq.com
