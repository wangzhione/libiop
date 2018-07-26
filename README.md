# libiop

特别适合学习 网络io 的一个简易入门库. 清洁而简单 ~

## 项目简介

    开发环境是 Best New Visual Studio or Ubuntu + GCC 环境下开发和调试.
    winds 上使用的是 select io 构建
    linux 上使用的是 select io or epoll io 构建
    设计的总思路是 -> 事件注册 + 消息轮序, 微型 libevent 方式
    
### 学习简介

    说真, 这个跨平台的 C网络库, 特别精简, 极其适合尝试去了解基于事件注册网络库设计套路
    
    winds  上 只需要安装 Best New Visual Studio 编译
    linux  上 只需要安装 Best New GCC 然后 make
    
    ./main.exe          -> 启动简易 echo 服务器
    ./main.exe client   -> 启动简易 客户端
    
    深入学习可以从 main.c 看起, 搞一遍基本就明白其中直白思路 ~
    
### 项目构建

    参照 libiop/ReadMe.txt
    
### 欢迎指正
    
                            《血薇》
    
    把酒祝东风，且共从容。垂杨紫陌洛城东。总是当时携手处，游遍芳丛 。
    
    聚散苦匆匆，此恨无穷。今年花胜去年红。可惜明年花更好，知与谁同？

***

[負け犬達のレクイエム](https://music.163.com/song?id=29751658&userid=16529894)

***

O_O