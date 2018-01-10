#makefile for screen capture 

CXX = g++

LIB = -lgdi32 -luser32 -lgdiplus
INCLUDE = -IC:\MinGW\include\gdiplus

FLAGS = -Wall -c -std=c++14 $(INCLUDE)
LINK_FLAGS = -mwindows -static-libstdc++ -static-libgcc $(LIB)

OBJ = capture.o

EXE = capture.exe

all: $(EXE)

$(EXE): $(OBJ)
	$(CXX) $(OBJ) $(LINK_FLAGS) -o $@

capture.o: capture.cpp
	$(CXX) $(FLAGS) $< -o  $@
	
clean:
	rm -f *.o && rm -f $(EXE)