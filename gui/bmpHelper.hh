#include <iostream> // for reading in data (includes ios)
#include <fstream>  // for reading in bmp image data
#include <array>    // for reading in bmp image data
#include <vector>
#include <string>

// correct channel value if > 255 or < 0
int correctRGB(int channel);

// get bmp image data 
std::vector<uint8_t> getBMPImageData(const std::string filename);
std::vector<uint8_t> getBMPImageDataInverted(const std::string filename);
std::vector<uint8_t> getBMPImageDataSaturated(const std::string filename);

// filters 
void inversionFilter(std::vector<char>& imageData);
void saturationFilter(float saturationVal, std::vector<char>& imageData);