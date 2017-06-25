#
# 前期编译的辅助参数支持
#
SRC_PATH		?= ./libiop
IOP_DIR			?= iop
TAR_PATH		?= ./Output
OBJ_DIR			?= obj
HLP_DIR			?= helper

#
# 指定一些目录, 还有编译参数支持
#
DIR			= -I$(SRC_PATH)/$(IOP_DIR)/include -I$(SRC_PATH)/$(HLP_DIR)/include
DEF			= -D_HAVE_EPOLL

CC			= gcc
LIB			= -lpthread -lm
CFLAGS		= -g -O2 -Wall -Wno-unused-result -std=gnu11

RHAD		= $(CC) $(CFLAGS) $(DIR) $(DEF)
RUNO		= $(RHAD) -c -o $(TAR_PATH)/$(OBJ_DIR)/$@ $<
RUN			= $(RHAD) -o $(TAR_PATH)/$@ $(foreach v, $^, $(TAR_PATH)/$(OBJ_DIR)/$(v)) $(LIB)

#
# CALL_RUNO - 命令序列集
# $(notdir dir/*.c) 	-> *.c
# $(basename *.c) 		-> *
#

define CALL_RUNO
$(notdir $(basename $(1))).o : $(1) | $$(TAR_PATH)
	$$(RUNO)
endef

#
# 具体产生具体东西了
#
.PHONY:all clean cleanall

all:main.exe

#
# *.o 映射到 $(TAR_PATH)/$(BUILD_DIR)/*.o
#
main.exe:main.o tstr.o vlist.o iop_util.o iop_poll.o iop.o iop_server.o
	$(RUN)

main.o:$(SRC_PATH)/main.c | $(TAR_PATH)
	$(RUNO)

#
# 产生所有的链接文件命令
# $(wildcard *.c)		-> 获取工作目录下面所有.c文件列表
# $(foreach v, s, t)	-> v 是 s中每个子项, 去构建 t 串
# $(eval s)				-> s 当Makefile的一部分解析和执行
# $(call s, t)			-> 执行命令集s 参数是 t
#
SRC_CS = $(wildcard\
		 $(SRC_PATH)/$(IOP_DIR)/*.c\
		 $(SRC_PATH)/$(HLP_DIR)/*.c\
)
$(foreach v, $(SRC_CS), $(eval $(call CALL_RUNO, $(v))))

#
# 辅助操作, 构建目录, 清除操作
#
$(TAR_PATH):
	-mkdir -p $@/$(OBJ_DIR)

clean:
	-rm -rf $(TAR_PATH)/$(OBJ_DIR)/*

cleanall:
	-rm -rf $(TAR_PATH)
