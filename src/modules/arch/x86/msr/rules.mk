MOD_$(DIR):=$(DIR)/msr.ko
OBJ_$(DIR):=$(DIR)/main.o $(DIR)/msr.o

$(MOD_$(DIR)): $(OBJ_$(DIR))
	$(LD) -o $@ $(LDFLAGS) $^

MODULES:=$(MODULES) $(MOD_$(DIR))
CLEAN:=$(CLEAN) $(MOD_$(DIR)) $(OBJ_$(DIR))