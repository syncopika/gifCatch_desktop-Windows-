#makefile for screen capture 

CXX = g++

HEADER = headers/

LIB = -lgdi32 -luser32 -lgdiplus -lcomctl32

# include gdiplus headers 
# careful about the path!
# -IC:\mingw-w64\mingw32\i686-w64-mingw32\include\gdiplus
INCLUDE = -IC:\MinGW\include\gdiplus

FLAGS = -Wall -Os -g -c -std=c++14 $(INCLUDE)
LINK_FLAGS = -mwindows -static-libstdc++ -static-libgcc $(LIB)

OBJ = capture_gui.o capture.o resources.o bmp_helper.o voronoi.o

EXE = gifcatch

all: $(EXE)

$(EXE): $(OBJ)
	$(CXX) $(OBJ) $(LINK_FLAGS) -o $@

# compile the resource file with windres 
# the compiled rc file will have a .o extension 
resources.o: resources.rc $(HEADER)resources.h
	windres resources.rc -o resources.o 
	
voronoi.o: voronoi_helper.cpp $(HEADER)voronoi_helper.hh
	$(CXX) -Wall -g -c -std=c++14 $< -o $@
	
bmp_helper.o: bmp_helper.cpp $(HEADER)bmp_helper.hh $(HEADER)voronoi_helper.hh
	$(CXX) -Wall -g -c -std=c++14 $< -o $@

capture.o: capture.cpp $(HEADER)capture.hh $(HEADER)gif.h $(HEADER)bmp_helper.hh
	$(CXX) $(FLAGS) $< -o $@ 

capture_gui.o: capture_gui.cpp $(HEADER)capture_gui.hh $(HEADER)capture.hh $(HEADER)bmp_helper.hh $(HEADER)resources.h
	$(CXX) $(FLAGS) $< -o  $@

.PHONY: clean
clean:
	rm -f *.o