M:=\
    $(DIR_SRC)/modules/internals/internals.ko \

O:=\
    $(DIR_SRC)/modules/internals/main.o \

L:=\
    $(DIR_LIB)/fudge/fudge.a \

include $(DIR_MK)/kmod.mk
