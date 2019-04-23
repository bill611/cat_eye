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


all :
	$(call make_loop)

debug :
	@echo $(OBJ_DIR)
	$(call make_loop,debug)
