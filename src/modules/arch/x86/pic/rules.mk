M:=\
    $(DIR_SRC)/modules/arch/x86/pic/pic.ko \

O:=\
    $(DIR_SRC)/modules/arch/x86/pic/main.o \
    $(DIR_SRC)/modules/arch/x86/pic/pic.o \

L:=\
    $(DIR_LIB)/fudge/fudge.a \

include $(DIR_MK)/kmod.mk
