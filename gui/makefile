#makefile for screen capture 

CXX = g++

LIB = -lgdi32 -luser32 -lgdiplus -lcomctl32

# include gdiplus headers 
INCLUDE = -IC:\MinGW\include\gdiplus

FLAGS = -Wall -g -c -std=c++14 $(INCLUDE)
LINK_FLAGS = -mwindows -static-libstdc++ -static-libgcc $(LIB)

OBJ = captureGUI.o capture.o resources.o bmpHelper.o

EXE = gifcatch.exe

all: $(EXE)

$(EXE): $(OBJ)
	$(CXX) $(OBJ) $(LINK_FLAGS) -o $@

# compile the resource file with windres 
# the compiled rc file will have a .o extension 
resources.o: resources.rc resources.h
	windres resources.rc -o resources.o 
	
bmpHelper.o: bmpHelper.cpp bmpHelper.hh 
	$(CXX) -Wall -g -c -std=c++14 $< -o $@

capture.o: capture.cpp capture.hh gif.h bmpHelper.hh
	$(CXX) $(FLAGS) $< -o $@ 

captureGUI.o: captureGUI.cpp capture.hh bmpHelper.hh resources.h
	$(CXX) $(FLAGS) $< -o  $@
	
clean:
	rm -f *.o && rm -f $(EXE)