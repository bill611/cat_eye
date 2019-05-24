${OBJ_DIR}/%.o:%.c 
	$(CC) ${CFLAGS} -MMD -MF ${OBJ_DIR}/$*.d -c $< -o $@

CURRENT_SRC=$(wildcard *.c)
SRC=$(patsubst %.c,${OBJ_DIR}/%.o,$(notdir ${CURRENT_SRC}))

ifeq ($(PLATFORM), RV1108)
all: make_C $(SRC)

make_C:                 
	    @make -C hal_video
endif

ifeq ($(PLATFORM), PC)

all: $(SRC)

endif

-include $(DEPS)

debug:
	@echo include:${INC_DIR}
	
