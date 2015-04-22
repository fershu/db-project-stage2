CXX	= g++
CFLAGS	= -std=c++11 -Wall -Wextra
SRC	= src\main.cpp
INCLUDE	= src\luajit
LIBDIR	= bin
LUA	= lua51
OBJ	= bin\main.o
EXE	= bin\main.exe

all:	$(EXE)

$(EXE):	$(OBJ)
	$(CXX) $(CFLAGS) -o $(EXE) -s $(OBJ) -L$(LIBDIR) -l$(LUA)

$(OBJ): $(SRC)
	$(CXX) $(CFLAGS) -c -o $(OBJ) -I$(INCLUDE) $(SRC)

run:	all
	$(EXE) input.txt

clean:
	-del $(EXE) $(OBJ)