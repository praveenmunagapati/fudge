M:=\
    $(DIR_SRC)/modules/arch/x86/ps2/ps2.ko \

O:=\
    $(DIR_SRC)/modules/arch/x86/ps2/bus.o \

L:=\
    $(DIR_LIB)/fudge/fudge.a \

include $(DIR_MK)/mod.mk

M:=\
    $(DIR_SRC)/modules/arch/x86/ps2/ps2keyboard.ko.0 \

O:=\
    $(DIR_SRC)/modules/arch/x86/ps2/keyboard.o \

L:=\
    $(DIR_LIB)/fudge/fudge.a \

include $(DIR_MK)/mod.mk

M:=\
    $(DIR_SRC)/modules/arch/x86/ps2/ps2mouse.ko.0 \

O:=\
    $(DIR_SRC)/modules/arch/x86/ps2/mouse.o \

L:=\
    $(DIR_LIB)/fudge/fudge.a \

include $(DIR_MK)/mod.mk
