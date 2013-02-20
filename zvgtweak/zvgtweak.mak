#
# Basic MAKEFILE
#
# For use with GNU Make, calls DJGPP

CC = g++
LINK = g++
ASM = NASM
INC = ../inc
ZLIB = ../shared

NAME = zvgtweak
OBJS = $(NAME).o \
       $(ZLIB)/zvgport.o \
       $(ZLIB)/timer.o \
       $(ZLIB)/zvgerror.o \
       $(ZLIB)/zvgban.o \
       $(ZLIB)/zvgframe.o \
       $(ZLIB)/zvgenc.o \
       $(ZLIB)/wrappers.o

all: ../bin/$(NAME).exe

# Clean up intermediate files

clean:
	-rm ../bin/$(NAME)
	-rm *.o
	-rm $(ZLIB)/*.o

# Flags for linker
#

../bin/$(NAME).exe : $(OBJS)
	$(LINK) $(OBJS) -lncurses -lpthread -o ../bin/$(NAME)

# Flags for C++ compiler
#

%.o : %.c
	$(CC) -c -I$(INC) -o $@ $<

# Flags for assembler
#
# -f = Set the file format type
# -X = Set the error type (-Xvc or -Xgnu)

%.o : %.asm
	$(ASM) -f coff -I$(INC) $<
