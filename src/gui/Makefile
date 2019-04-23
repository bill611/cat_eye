${OBJ_DIR}/%.o:%.c
	$(CC) ${CFLAGS} -MMD -MF ${OBJ_DIR}/$*.d -c $< -o $@

CURRENT_SRC=$(wildcard *.c)
SRC=$(patsubst %.c,${OBJ_DIR}/%.o,$(notdir ${CURRENT_SRC}))
all: make_C $(SRC)

make_C:
	    @make -C my_controls

-include $(DEPS)

debug:
	@echo include:${INC_DIR}

