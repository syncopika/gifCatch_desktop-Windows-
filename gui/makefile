#makefile for screen capture 

CXX = g++

LIB = -lgdi32 -luser32 -lgdiplus -lcomctl32

# include gdiplus headers 
INCLUDE = -IC:\MinGW\include\gdiplus

FLAGS = -Wall -g -c -std=c++14 $(INCLUDE)
LINK_FLAGS = -mwindows -static-libstdc++ -static-libgcc $(LIB)

OBJ = captureGUI.o capture.o resources.o

EXE = gifcatch.exe

all: $(EXE)

$(EXE): $(OBJ)
	$(CXX) $(OBJ) $(LINK_FLAGS) -o $@

# compile the resource file with windres 
# the compiled rc file will have a .o extension 
resources.o: resources.rc
	windres resources.rc -o resources.o 

capture.o: capture.cpp capture.hh gif.h
	$(CXX) $(FLAGS) $< -o $@

captureGUI.o: captureGUI.cpp capture.hh
	$(CXX) $(FLAGS) $< -o  $@
	
clean:
	rm -f *.o && rm -f $(EXE)