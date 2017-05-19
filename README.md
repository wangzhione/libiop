# libiop
特别适合学习网络io的一个简易开源库. 清洁和简单

### 项目简介
    这个项目是在 Visual Studio 2015/2017 or Ubuntu + GCC 环境下开发调试的.
    window 上使用的是 select io 构建
    linux 上使用的是 select io or epoll io 构建
    设计的总思路是: 事件注册 + 消息轮序, 微型的libevent套路
    
### 学习简介
    说真的, 这是个跨平台的C网络库, 特别精简及其适合入门的人去学习.
    
    window 上 只需要安装 Best New Visual Studio 编译.
    linux  上 只需要安装 Best New GCC 然后 make
    
    ./main.exe          -> 启动简易 echo 服务器
    ./main.exe client   -> 启动简易 客户端
    
    随后深入学习可以 从 main.c 看起, 搞一遍基本就明白基于注册的网络io套路.
    
### 项目构建步骤
    参照 libiop/ReadMe.txt
    
### 欢迎指正, 补充, 打脸
    
                            《血薇》
    
    把酒祝东风，且共从容。垂杨紫陌洛城东。总是当时携手处，游遍芳丛 。
    
    聚散苦匆匆，此恨无穷。今年花胜去年红。可惜明年花更好，知与谁同？
