#
# Basic MAKEFILE
#
# For use with GNU Make, calls DJGPP

CC = GCC
LINK = GCC
ASM = NASM
INC = ..\inc

NAME = zvgTweak
OBJS = $(NAME).o \
       ..\shared\zvgPort.o \
       ..\shared\timer.o \
       ..\shared\zvgError.o \
       ..\shared\zvgBan.o \
       ..\shared\zvgFrame.o \
       ..\shared\zvgEnc.o

all: ..\bin\$(NAME).exe

# Clean up intermediate files

clean:
	-del ..\bin\$(NAME)
	-del *.o
	-del ..\shared\*.o

# Flags for linker
#
# /x = No map
# /m = Map with publics
# /v = Include debug symbols in link

..\bin\$(NAME).exe : $(OBJS)
	$(LINK) -o ..\bin\$(NAME) $(OBJS)

# Flags for C++ compiler
#
# -N = Check for stack overflow
# -v = Debugger on
# -w = Max warnings

%.o : %.c
	$(CC) -c -I$(INC) -o $@ $<

# Flags for assembler
#
# -f = Set the file format type
# -X = Set the error type (-Xvc or -Xgnu)

%.o : %.asm
	$(ASM) -f coff -I$(INC) $<
