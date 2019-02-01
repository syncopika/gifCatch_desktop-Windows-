// capture.hh
// these functions define the screen capture behavior 

#ifndef CAPTURE_H
#define CAPTURE_H

#include <iostream> // for debugging / errors
#include <string>   // for int to string conversion 
#include <sstream>  // for int to string conversion 
#include <vector>   // used throughout 
#include <windows.h>  // for screen capture 
#include <gdiplus.h>  // for screen capture 
#include <memory>     // for screen capture 

// convert an int to string 
std::string int_to_string(int i);

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
void BitmapToBMP(HBITMAP hbmpImage, int width, int height, std::string filename);
bool ScreenCapture(int x, int y, int width, int height, const char *filename);

// this function relies on all the above 
// the result is creating a temp folder and populating it with screenshots
void getSnapshots(int nImages, int delay, int x, int y, int width, int height, std::vector<uint8_t> (*filter)(const std::string));

// this function helps resize any bmps. it doesn't respect ratios though currently so it might produce not-so-good frames.
int resizeBMPs(int nImages, std::vector<std::string> images, int width, int height, std::string memeText);

// this function assembles the gif from bmp images in a specified directory 
void assembleGif(int nImages, int delay, std::vector<std::string> images, std::vector<uint8_t> (*filter)(const std::string), std::string memeText);


#endif // CAPTURE_H