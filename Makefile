GCC=gcc
CPP=g++
AR=ar
CFLAGS := -O2 
WARNINGS:= 
INCLUDES := -I/usr/include/ncurses -I.
LIB := -L/usr/lib -lncurses

CPP_FILES = 
CPP_FILES += db.cpp
CPP_FILES += ui.cpp

C_FILES = 
C_FILES += sqlite3.c

COMPILER_OBJECTS := 
COMPILER_OBJECTS_CPP := $(CPP_FILES:.cpp=.o)
COMPILER_OBJECTS_C := $(C_FILES:.c=.o) 	

OUTPUT=todo.out

all: $(COMPILER_OBJECTS_CPP) $(COMPILER_OBJECTS_C)
	@g++ $(COMPILER_OBJECTS_CPP) $(COMPILER_OBJECTS_C) $(LIB) -o $(OUTPUT)

%.o:%.cpp
	@echo "compiling $^"
	@$(CPP) $(CFLAGS) $(WARNINGS) $(INCLUDES) -c $^  -o $@


%.o:%.c
	@echo "compiling $^"
	@$(GCC) $(CFLAGS) $(WARNINGS) $(INCLUDES) -c $^  -o $@

clean:
	rm -f $(COMPILER_OBJECTS_C) $(COMPILER_OBJECTS_CPP) $(OUTPUT)
