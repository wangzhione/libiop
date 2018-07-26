#
# 前期编译的辅助参数支持
#
ROOT		?= libiop

DIOP		?= $(ROOT)/iop
DUTL		?= $(ROOT)/util

OUTS		?= Outs
DOBJ		?= $(OUTS)/obj

#
# 指定一些目录, 还有编译参数支持
#
INS			= -I$(DIOP)/include -I$(DUTL)/include

CC			= gcc
LIB			= -lpthread -lm
CFLAGS		= -g -O2 -Wall -Wno-unused-result

RHAD		= $(CC) $(CFLAGS) $(INS)
RUNO		= $(RHAD) -c -o $(DOBJ)/$@ $<

#
# CALL_RUNO - 命令序列集
# $(notdir dir/*.c)		-> *.c
# $(basename *.c)		-> *
#
define CALL_RUNO
$(notdir $(basename $(1))).o : $(1) | $$(OUTS)
	$$(RUNO)
endef

#
# 具体产生具体东西了
#
.PHONY : all clean

all : main.exe

#
# *.o 映射到 $(DOBJ)/*.o
#
main.exe : main.o tstr.o strerr.o socket.o iop_poll.o iop.o iop_server.o
	$(CC) $(CFLAGS) -o $(OUTS)/$@ $(DOBJ)/*.o $(LIB)

main.o : $(ROOT)/main.c | $(OUTS)
	$(RUNO)

#
# 产生所有的链接文件命令
# $(wildcard *.c)		-> 获取工作目录下面所有.c文件列表
# $(foreach v, s, t)	-> v 是 s中每个子项, 去构建 t 串
# $(eval s)				-> s 当Makefile的一部分解析和执行
# $(call s, t)			-> 执行命令集s 参数是 t
#
SRCC = $(wildcard $(DIOP)/*.c $(DUTL)/*.c)
$(foreach v, $(SRCC), $(eval $(call CALL_RUNO, $(v))))

#
# 辅助操作, 构建目录, 清除操作
#
$(OUTS) :
	mkdir -p $(DOBJ)

clean :
	-rm -rf *core*
	-rm -rf $(OUTS)
	-rm -rf logs $(ROOT)/logs
	-rm -rf Debug Release x64
	-rm -rf $(ROOT)/Debug $(ROOT)/Release $(ROOT)/x64
