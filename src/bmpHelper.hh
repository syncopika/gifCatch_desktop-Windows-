#include <iostream> // for reading in data (includes ios)
#include <fstream>  // for reading in bmp image data
#include <array>    // for reading in bmp image data
#include <vector>
#include <string>
#include <cmath>	// for using sqrt 

// correct channel value if > 255 or < 0
int correctRGB(int channel);

// get bmp image data 
std::vector<int> getBMPHeightWidth(const std::string filename);

std::vector<uint8_t> getBMPImageData(const std::string filename);
std::vector<uint8_t> getBMPImageData(const std::string filename, const std::string filtername);

std::vector<uint8_t> getBMPImageDataInverted(const std::string filename);
std::vector<uint8_t> getBMPImageDataSaturated(const std::string filename);
std::vector<uint8_t> getBMPImageDataWeird(const std::string filename);
std::vector<uint8_t> getBMPImageDataGrayscale(const std::string filename);
std::vector<uint8_t> getBMPImageDataEdgeDetection(const std::string filename);

// filters 
void inversionFilter(std::vector<char>& imageData);
void saturationFilter(float saturationVal, std::vector<char>& imageData);
void weirdFilter(std::vector<char>& imageData);
void grayscaleFilter(std::vector<char>& imageData);
void edgeDetectionFilter(std::vector<char>& imageData, int width, int height);