${OBJ_DIR}/%.o:%.cpp 
	$(CXX) ${CFLAGS} -MMD -MF ${OBJ_DIR}/$*.d -c $< -o $@

CURRENT_SRC=$(wildcard *.cpp)
SRC=$(patsubst %.cpp,${OBJ_DIR}/%.o,$(notdir ${CURRENT_SRC}))
all: make_C $(SRC)

make_C:                 
	@make -C buffer
	@make -C camera
	@make -C process
	@make -C lcd

-include $(DEPS_CPP)

debug:
	@echo include:${INC_DIR}
	
