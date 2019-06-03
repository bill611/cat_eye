MAKEROOT = $(shell pwd)

ifeq ($(PLATFORM), RV1108)
	PREFIX =$(RV1108_CROOS_PATH)/bin/arm-buildroot-linux-gnueabihf-
	CFLAGS += -DRV1108 -DLINUX -DSUPPORT_ION -DENABLE_ASSERT
	LIB_DIR += $(MAKEROOT)/lib/arm
	LIB_DIR += ${RV1108_SDK_PATH}/out/system/lib
	LIB_DIR += ${RV1108_SDK_PATH}/out/sec_rootfs/lib
	INC_DIR += $(MAKEROOT)/include/sdk_include
	EX_LIB += -lion -lrkfb -lrkrga -lts -ladk -lcam_ia -lcam_engine_cifisp

SRC_CPP = \
		  $(wildcard ${SRC_DIR}/hal/hal_video/*.cpp) \
		  $(wildcard ${SRC_DIR}/hal/hal_video/buffer/*.cpp) \
		  $(wildcard ${SRC_DIR}/hal/hal_video/camera/*.cpp) \
		  $(wildcard ${SRC_DIR}/hal/hal_video/process/*.cpp) \
		  $(wildcard ${SRC_DIR}/hal/hal_video/lcd/*.cpp) \

OBJ_CPP = $(patsubst %.cpp,${OBJ_DIR}/%.o,$(notdir ${SRC_CPP}))
DEPS_CPP = $(patsubst %.cpp, ${OBJ_DIR}/%.d, $(notdir ${SRC_CPP}))
endif

ifeq ($(PLATFORM), PC)
	PREFIX =
	CFLAGS += -DX86
	LIB_DIR += $(MAKEROOT)/lib/x86
	LIB_DIR += /usr/local/lib
	LIB_DIR += /usr/lib/x86_64-linux-gnu
	INC_DIR += /usr/local/include
	EX_LIB +=
endif

# 主程序Makefile

# 在指定目录下生成的应用程序
EXE = cat_eye
BIN_TARGET = ${BIN_DIR}/${EXE}

SRC_DIR = $(MAKEROOT)/src
OBJ_DIR = $(MAKEROOT)/obj
BIN_DIR = $(MAKEROOT)/out

CC =$(PREFIX)gcc
CXX =$(PREFIX)g++

# INC_DIR 目录下为相应库的头文件
INC_DIR += \
		  $(MAKEROOT)/src\
		  $(MAKEROOT)/include\
		  $(MAKEROOT)/include/mqtt\
		  $(MAKEROOT)/src/app\
		  $(MAKEROOT)/src/gui\
		  $(MAKEROOT)/src/gui/my_controls\
		  $(MAKEROOT)/src/drivers\
		  $(MAKEROOT)/src/hal\
		  $(MAKEROOT)/src/hal/hal_video \
		  $(MAKEROOT)/src/hal/hal_video/buffer \
		  $(MAKEROOT)/src/hal/hal_video/camera \
		  $(MAKEROOT)/src/hal/hal_video/process \
		  $(MAKEROOT)/src/hal/hal_video/lcd \
		  $(MAKEROOT)/src/wireless\
		  # $(MAKEROOT)/src/gui/my_controls/my_scrollview\


SRC = \
		$(wildcard ${SRC_DIR}/*.c) \
		$(wildcard ${SRC_DIR}/app/*.c) \
		$(wildcard ${SRC_DIR}/gui/*.c) \
		$(wildcard ${SRC_DIR}/gui/my_controls/*.c) \
		$(wildcard ${SRC_DIR}/drivers/*.c) \
		$(wildcard ${SRC_DIR}/drivers/iniparser/*.c) \
		$(wildcard ${SRC_DIR}/hal/*.c) \
		$(wildcard ${SRC_DIR}/wireless/*.c) \




XLINKER = -Xlinker "-(" -lsqlite3 \
		  ${EX_LIB}\
		  -lminigui_ths -lpng12 -lpng -ljpeg -lfreetype \
		  -lqrencode \
		  -lpaho-mqtt3a -lssl -lcurl -lcjson -lcrypto \
		  -lz -lm -lpthread -ldl -lrt -Xlinker "-)"

CP_TARGET = $(MAKEROOT)/../nand/v2.0/nand1-2/

ifeq ($(DBG), 1)
	CFLAGS += -g -O0 -DWATCHDOG_DEBUG
	LINKFLAGS += -g -O0
	STRIP =
	# CP_TARGET := ${HOME}/arm_share/ankgw
else
	CFLAGS += -O2
	LINKFLAGS += -O2
	STRIP = $(PREFIX)strip $(BIN_TARGET) $@
endif


CFLAGS += -D_PLATFORM_IS_LINUX_ -D_GNU_SOURCE ${addprefix -I,${INC_DIR}}


# wildcard:扩展通配符，notdir;去除路径，patsubst;替换通配符

OBJ = $(patsubst %.c,${OBJ_DIR}/%.o,$(notdir ${SRC}))
DEPS = $(patsubst %.c, ${OBJ_DIR}/%.d, $(notdir ${SRC}))

# 链接路径
# -Xlinker编译时可重复查找依赖库，和库的次序无关



export CC CXX LIB_DIR CFLAGS CPPFLAGS OBJ_DIR INC_DIR DEPS DEPS_CPP
# $@：表示目标文件，$^：表示所有的依赖文件，$<：表示第一个依赖文件，$?：表示比目标还要新的依赖文件列表
all: make_C ${BIN_TARGET}
	@# cp -u ${BIN_TARGET} ${CP_TARGET}

make_C:
	@mkdir -p ${OBJ_DIR}
	@make -C src

# 在指定目录下，将所有的.c文件编译为相应的同名.o文件
${BIN_TARGET}:${OBJ} ${OBJ_CPP}
	@$(CXX) $(LINKFLAGS) -o $@ $(OBJ) $(OBJ_CPP) ${addprefix -L,${LIB_DIR}} ${XLINKER}
	@${STRIP}

debug:
	make -C src debug

.PHONY:clean
clean:
	@-rm -rf ${BIN_TARGET} obj* ${BIN_DIR}
