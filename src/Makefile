${OBJ_DIR}/%.o:%.c
	$(CC) ${CFLAGS} -MMD -MF ${OBJ_DIR}/$*.d -c $< -o $@

CURRENT_SRC=$(wildcard *.c)
SRC=$(patsubst %.c,${OBJ_DIR}/%.o,$(notdir ${CURRENT_SRC}))

SIBDIR=\
	   app \
	   gui \
	   drivers \
	   hal \
	   wireless \


define make_loop
	@for mkdir in $(SIBDIR); do \
		make -C $$mkdir $1; \
   	done  ;
endef


all : $(SRC)
	$(call make_loop)

debug :
	@echo $(OBJ_DIR)
	$(call make_loop,debug)
