========================================================================
    简易的网络库：libiop 项目概述
========================================================================

学习步骤:
	1. 参照 main.c 中 echo 服务器写法
		1.1 libiop.exe -> 启动服务器
		1.2 libiop.exe client -> 启动客户端
		
	2. 观看源码, 源码真的好简单

window 配置步骤
	1. 配置包含目录 VC++目录
		1.1 包含目录
			$(ProjectDir)pthread/include
			$(ProjectDir)iop/include
			$(PtojectDir)helper/include
		1.2 库目录
			$(ProjectDir)pthread/lib
			
	2. C/C++ 预处理器 -> 预处理定义
		_DEBUG
		_CRT_SECURE_NO_WARNINGS
		_CRT_NONSTDC_NO_DEPRECATE
		WIN32_LEAN_AND_MEAN
		_WINSOCK_DEPRECATED_NO_WARNINGS
		
	3. C/C++ 高级 -> 编译为 -> 编译为 C 代码 (/TC)
	   C/C++ -> 代码生成 -> 运行库 -> 多线程/MT

	4. 链接器 -> 命令行 -> /ignore:4099 /LTCG

linux 配置步骤
	1. 参照 Makefile 文件
	
	2. 参照 DEF 替换量
		2.1 _D_HAVE_EPOLL  表示开启 linux  epoll io api
	
/////////////////////////////////////////////////////////////////////////////
