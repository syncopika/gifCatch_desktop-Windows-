#ifndef BMP_HELPER_H
#define BMP_HELPER_H

#include <iostream> // for reading in data (includes ios)
#include <fstream>  // for reading in bmp image data
#include <array>    // for reading in bmp image data
#include <vector>
#include <string>
#include <cmath>	// for using sqrt 
#include <ctime>	// for time() 
#include <cstdlib>	// for rand() and srand()

#include "voronoiHelper.hh"

// correct channel value if > 255 or < 0
int correctRGB(int channel);

// get height and width of bmp image 
std::vector<int> getBMPHeightWidth(const std::string filename);

// get the pixel coordinates given an r channel index of a pixel in an array of image data 
// that is arranged like r,g,b,a,r,g,b,a,... 
std::vector<int> getPixelCoords(int index, int width, int height);


// filters 
void inversionFilter(std::vector<char>& imageData);
void saturationFilter(float saturationVal, std::vector<char>& imageData);
void weirdFilter(std::vector<char>& imageData);
void grayscaleFilter(std::vector<char>& imageData);
void edgeDetectionFilter(std::vector<char>& imageData, int width, int height);
void mosaicFilter(std::vector<char>& imageData, int width, int height, int chunkSize);
void outlineFilter(std::vector<char>& imageData, int width, int height, int colorDiffLimit);
void voronoiFilter(std::vector<char>& imageData, int width, int height, int neighborConstant);


#endif 