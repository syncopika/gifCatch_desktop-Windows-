// capture.hh
// these functions define the screen capture behavior 

#ifndef CAPTURE_H
#define CAPTURE_H

#include <iostream> // for debugging / errors
#include <fstream>  // for reading in bmp image data
#include <array>    // for reading in bmp image data
#include <windows.h>  // for screen capture 
#include <gdiplus.h>  // for screen capture 
#include <memory>     // for screen capture 
#include <string>   // for int to string conversion 
#include <sstream>  // for int to string conversion 
#include <vector>   // used throughout
#include <map>
#include <ctime>
#include "bmp_helper.hh" // filter function declarations
#include "resources.h"   // contains definition for ID_PROCESS_FRAME


// struct to provide arguments needed for gif creation 
// notice how in c++ you can declare non-typedef'd structs without struct!
struct windowInfo {
	int numFrames;
	int timeDelay;
	int selectedFilter; // this is the index of the filter in the dropdown box 
	std::string directory;
	std::string captionText;
	HWND mainWindow; // main window so the worker thread can post messages to its queue 
	
	std::map<int, std::string>* filters;
	
	// parameters from the parameters page 
	COLORREF selectionWindowColor;
	float saturationValue;
	int mosaicChunkSize; 		 // for mosaic filter 
	int outlineColorDiffLimit;   // for outline filter 
	int voronoiNeighborConstant; // for Voronoi filter
	int blurFactor;              // for blur filter
	bool getCursor;
};

// convert an int to string 
std::string intToString(int i);

// check if a point is within a particular rect
bool ptIsInRange(POINT start, int width, int height, POINT pt);

// screen capturing code 
int getEncoderClsid(const WCHAR* format, CLSID* pClsid);
void bitmapToBMP(HBITMAP hbmpImage, int width, int height, std::string filename);
bool screenCapture(int x, int y, int width, int height, const char *filename, bool getCursor);

// this function relies on all the above 
// the result is creating a temp folder and populating it with screenshots
void getSnapshots(
	int nImages, 
	int delay, 
	int x, 
	int y, 
	int width, 
	int height, 
	std::vector<uint8_t> (*filter)(const std::string, windowInfo*), 
	windowInfo* gifParams
);

// this function assembles the gif from bmp images in a specified directory 
void assembleGif(
	int nImages, 
	int delay, 
	std::vector<std::string> images, 
	std::vector<uint8_t> (*filter)(const std::string, windowInfo*), 
	windowInfo* gifParams
);

// this function helps resize any bmps. it doesn't respect ratios though currently so it might produce not-so-good frames.
int resizeBMPs(int nImages, std::vector<std::string> images, int width, int height, std::string captionText);

// get bmp image data 
std::vector<uint8_t> getBMPImageData(const std::string filename, windowInfo* gifParams);



#endif // CAPTURE_H