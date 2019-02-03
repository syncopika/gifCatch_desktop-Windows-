#ifndef BMP_HELPER_H
#define BMP_HELPER_H

#include <iostream> // for reading in data (includes ios)
#include <fstream>  // for reading in bmp image data
#include <array>    // for reading in bmp image data
#include <vector>
#include <string>
#include <cmath>	// for using sqrt 

// correct channel value if > 255 or < 0
int correctRGB(int channel);

// get height and width of bmp image 
std::vector<int> getBMPHeightWidth(const std::string filename);

// filters 
void inversionFilter(std::vector<char>& imageData);
void saturationFilter(float saturationVal, std::vector<char>& imageData);
void weirdFilter(std::vector<char>& imageData);
void grayscaleFilter(std::vector<char>& imageData);
void edgeDetectionFilter(std::vector<char>& imageData, int width, int height);
void mosaicFilter(std::vector<char>& imageData, int width, int height, int chunkSize);
void outlineFilter(std::vector<char>& imageData, int width, int height, int colorDiffLimit);

#endif 