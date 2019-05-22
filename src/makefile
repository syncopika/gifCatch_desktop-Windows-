#makefile for screen capture 

CXX = g++

HEADER = headers/

LIB = -lgdi32 -luser32 -lgdiplus -lcomctl32

# include gdiplus headers 
# careful about the path!
# -IC:\MinGW\include\gdiplus
INCLUDE = -IC:\mingw-w64\mingw32\i686-w64-mingw32\include\gdiplus

FLAGS = -Wall -g -c -std=c++14 $(INCLUDE)
LINK_FLAGS = -mwindows -static-libstdc++ -static-libgcc $(LIB)

OBJ = captureGUI.o capture.o resources.o bmpHelper.o

EXE = gifcatch.exe

all: $(EXE)

$(EXE): $(OBJ)
	$(CXX) $(OBJ) $(LINK_FLAGS) -o $@

# compile the resource file with windres 
# the compiled rc file will have a .o extension 
resources.o: resources.rc $(HEADER)resources.h
	windres resources.rc -o resources.o 
	
bmpHelper.o: bmpHelper.cpp $(HEADER)bmpHelper.hh
	$(CXX) -Wall -g -c -std=c++14 $< -o $@

capture.o: capture.cpp $(HEADER)capture.hh $(HEADER)gif.h $(HEADER)bmpHelper.hh
	$(CXX) $(FLAGS) $< -o $@ 

captureGUI.o: captureGUI.cpp $(HEADER)captureGUI.hh $(HEADER)capture.hh $(HEADER)bmpHelper.hh $(HEADER)resources.h
	$(CXX) $(FLAGS) $< -o  $@
	
clean:
	rm -f *.o