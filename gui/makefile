#makefile for screen capture 

CXX = g++

LIB = -lgdi32 -luser32 -lgdiplus

# include gdiplus headers 
INCLUDE = -IC:\MinGW\include\gdiplus

FLAGS = -Wall -c -std=c++14 $(INCLUDE)
LINK_FLAGS = -mwindows -static-libstdc++ -static-libgcc $(LIB)

OBJ = capture.o

EXE = capture.exe

all: $(EXE)

$(EXE): $(OBJ)
	$(CXX) $(OBJ) $(LINK_FLAGS) -o $@

# don't need a resource file unless you have an icon or something else.
# compile the resource file with windres 
# the compiled rc file will have a .o extension (but convention uses res?)
#gui.o: captureGUI.rc
#	windres captureGUI.rc -o gui.o 
	
capture.o: captureGUI.cpp gif.h
	$(CXX) $(FLAGS) $< -o  $@
	
clean:
	rm -f *.o && rm -f $(EXE)