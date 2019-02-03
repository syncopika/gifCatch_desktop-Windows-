#include "headers/bmpHelper.hh"

// helper function to normalise rgb colors in case > 255 or < 0 
int correctRGB(int channel){
	if(channel > 255){
		return 255;
	}
	if(channel < 0){
		return 0;
	}
	return channel;
}

// get the height and width of a BMP image 
std::vector<int> getBMPHeightWidth(const std::string filename){
	
    static constexpr size_t HEADER_SIZE = 54;
    
    std::ifstream bmp(filename, std::ios::binary);
    
    std::array<char, HEADER_SIZE> header;
    
    bmp.read(header.data(), header.size());
    
    auto width = *reinterpret_cast<uint32_t *>(&header[18]);
    auto height = *reinterpret_cast<uint32_t *>(&header[22]);
	
	// height and width will be placed in that order in the vector 
	std::vector<int> dimensions;
	dimensions.push_back((int)height);
	dimensions.push_back((int)width);
	
	return dimensions;
}

/***

	inversion filter
	
***/
void inversionFilter(std::vector<char>& imageData){
	
	unsigned int dataSize = imageData.size();
	
	for(unsigned int i = 0; i <= dataSize - 4; i += 4){
        imageData[i] = 255 - imageData[i];
        imageData[i+1] = 255 - imageData[i+1];
		imageData[i+2] = 255 - imageData[i+2];
    }
}

/***

	saturation filter
	
***/
void saturationFilter(float saturationVal, std::vector<char>& imageData){
	
	unsigned int dataSize = imageData.size();
	double saturationValue = saturationVal;
	double lumR = .3086; //constant for determining luminance of red
	double lumG = .6094; //constant for determining luminance of green
	double lumB = .0820; //constant for determining luminance of blue
	
	//one of these equations per r,g,b
	double r1 = ((1 - saturationValue) * lumR) + saturationValue;
	double g1 = ((1 - saturationValue) * lumG) + saturationValue;
	double b1 = ((1 - saturationValue) * lumB) + saturationValue;	
	
	//then one of these for each
	double r2 = (1 - saturationValue) * lumR;
	double g2 = (1 - saturationValue) * lumG;
	double b2 = (1 - saturationValue) * lumB;	
	
    for(unsigned int i = 0; i <= dataSize - 4; i += 4){
		
		unsigned char r = imageData[i];
		unsigned char g = imageData[i+1];
		unsigned char b = imageData[i+2];
		
		int newR = (r*r1 + g*g2 + b*b2);
		int newG =(r*r2 + g*g1 + b*b2);
		int newB = (r*r2 + g*g2 + b*b1);
		
		newR = correctRGB(newR);
		newG = correctRGB(newG);
		newB = correctRGB(newB);
		
		imageData[i] = (unsigned char)newR;
		imageData[i+1] = (unsigned char)newG;
		imageData[i+2] = (unsigned char)newB;
    }
}

/***

	weird filter 

***/
void weirdFilter(std::vector<char>& imageData){
	
	unsigned int dataSize = imageData.size();
	
	// do the filtering stuff
	for(unsigned int i = 0; i < dataSize - 1; i++){
		
		// a bit misleading, since we go through every channel, and not pixel for each 
		// iteration of the loop. so in other words, each channel gets treated as r 
		// and the following channel gets treated as g, but they are not necessarily 
		// the r and g channels 
        unsigned char r = imageData[i];
		unsigned char g = imageData[i+1];

		if(g > 100 && g < 200){
			imageData[i+1] = (unsigned char)0;
		}
		if(r < 100){
			imageData[i] = (unsigned char)imageData[i]*2;
		}
    }
}

/***
	
	grayscale filter 
	
***/
void grayscaleFilter(std::vector<char>& imageData){
	
	unsigned int dataSize = imageData.size();
	
	// for each pixel, set each channel's value to the average of the RGB channels 
	for(unsigned int i = dataSize - 4; i > 0; i-=4){
		
		unsigned char r = imageData[i]; 
		unsigned char g = imageData[i+1];
		unsigned char b = imageData[i+2];
		
		unsigned char avg = (r+g+b) / 3;
		
		imageData[i] = avg;
		imageData[i+1] = avg;
		imageData[i+2] = avg;	

	}
}

/***
	
	edge detection filter 
	
***/
void edgeDetectionFilter(std::vector<char>& imageData, int width, int height){
	
	//unsigned int dataSize = imageData.size();
	
	// need to create a copy of the source image
	// so that the calculations won't get messed up with overwritten values 
	// use this data for the calculations
	// it's best to grayscale the image first! that way each channel has the same value for each pixel 
	std::vector<char> sourceImageCopy(imageData);
	grayscaleFilter(imageData);
	
	// the kernels needed to form the Sobel filter 
	int xKernel[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
	int yKernel[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};

	// cycle through the pixels. since each pixel basically maps to 1 color value thanks 
	// to grayscaling, we can use 1 channel from each pixel to use in the formula 
	for(int i = 1; i < height - 1; i++){
		for(int j = 4; j < 4*width - 4; j+=4){
			
			int left = (4*i*width) + (j-4);
			int right = (4*i*width) + (j+4);
			int top = (4*(i-1)*width) + j;
			int bottom = (4*(i+1)*width) + j;
			int topLeft = (4*(i-1)*width) + (j-4);
			int topRight = (4*(i-1)*width) + (j+4);
			int bottomLeft = (4*(i+1)*width) + (j-4);
			int bottomRight = (4*(i+1)*width) + (j+4);
			int center = (4*width*i) + j;
			
			// use the xKernel to detect edges horizontally 
			int pX = (xKernel[0][0]*sourceImageCopy[topLeft]) + (xKernel[0][1]*sourceImageCopy[top]) + (xKernel[0][2]*sourceImageCopy[topRight]) + 
					(xKernel[1][0]*sourceImageCopy[left]) + (xKernel[1][1]*sourceImageCopy[center]) + (xKernel[1][2]*sourceImageCopy[right]) +
					(xKernel[2][0]*sourceImageCopy[bottomLeft]) + (xKernel[2][1]*sourceImageCopy[bottom]) + (xKernel[2][2]*sourceImageCopy[bottomRight]);
			
			// use the yKernel to detect edges vertically 
			int pY = (yKernel[0][0]*sourceImageCopy[topLeft]) + (yKernel[0][1]*sourceImageCopy[top]) + (yKernel[0][2]*sourceImageCopy[topRight]) + 
					(yKernel[1][0]*sourceImageCopy[left]) + (yKernel[1][1]*sourceImageCopy[center]) + (yKernel[1][2]*sourceImageCopy[right]) +
					(yKernel[2][0]*sourceImageCopy[bottomLeft]) + (yKernel[2][1]*sourceImageCopy[bottom]) + (yKernel[2][2]*sourceImageCopy[bottomRight]);
			
			// finally set the current pixel to the new value based on the formula 
			int newVal = (int)( std::ceil( std::sqrt((pX * pX) + (pY * pY)) ) );
			imageData[center] = newVal;
			imageData[center+1] = newVal;
			imageData[center+2] = newVal;
			imageData[center+3] = 255;
		}
	}
	
}

/***

	mosaic filter 

***/
void mosaicFilter(std::vector<char>& imageData, int width, int height, int chunkSize){
	
	// make a copy of the data 
	std::vector<char> sourceImageCopy(imageData);
	
	// change sampling size here. lower for higher detail preservation, higher for less detail (because larger chunks)
	int chunkWidth = chunkSize; //30;
	int chunkHeight = chunkSize; //30;
	
	// make sure chunkWidth can completely divide the image width * 4 
	while(width % chunkWidth != 0){
		chunkWidth--;
		chunkHeight--;
	}

	// when looking at each chunk of the image, for these 2 outer for loops, 
	// focus on looking at each chunk as if looking at a single pixel first
	for(int i = 0; i < width; i += chunkWidth){
		for(int j = 0; j < height; j += chunkHeight){
			
			// 4*i + 4*j*width = index of first pixel in chunk 
			// get the color of the first pixel in this chunk
			// multiply by 4 because 4 channels per pixel
			// multiply by width because all the image data is in a single array and a row is dependent on width
			int r = sourceImageCopy[4*i+4*j*width];
			int g = sourceImageCopy[4*i+4*j*width+1];
			int b = sourceImageCopy[4*i+4*j*width+2];
			
			// now for all the other pixels in this chunk, set them to this color 
			for(int k = i; k < i+chunkWidth; k++){
				for(int l = j; l < j+chunkHeight; l++){
					
					imageData[4*k+4*l*width] = r;
					imageData[4*k+4*l*width+1] = g;
					imageData[4*k+4*l*width+2] = b;
				}
			}
		}
	}

}

/***

	outline filter 

***/
void outlineFilter(std::vector<char>& imageData, int width, int height, int colorDiffLimit){
	
	// make a copy of the data 
	std::vector<char> sourceImageCopy(imageData);
	
	// for each pixel, check the above pixel (if it exists)
	// if the above pixel is 'significantly' different (i.e. more than +/- 5 of rgb),
	// color the above pixel black and the current pixel white. otherwise, both become white. 
	int limit = colorDiffLimit; //10;
	
	bool setSameColor = false;
	
	for(int i = 0; i < height; i++){
		for(int j = 0; j < width; j++){
			// the current pixel is i*width + j
			// the above pixel is (i-1)*width + j
			if(i > 0){
				
				int aboveIndexR = (i-1)*width*4 + j*4;
				int aboveIndexG = (i-1)*width*4 + j*4 + 1;
				int aboveIndexB = (i-1)*width*4 + j*4 + 2;
				
				int currIndexR = i*width*4 + j*4;
				int currIndexG = i*width*4 + j*4 + 1;
				int currIndexB = i*width*4 + j*4 + 2;
				
				unsigned char aboveR = sourceImageCopy[aboveIndexR];
				unsigned char aboveG = sourceImageCopy[aboveIndexG];
				unsigned char aboveB = sourceImageCopy[aboveIndexB];
			
				unsigned char currR = sourceImageCopy[currIndexR]; 
				unsigned char currG = sourceImageCopy[currIndexG];
				unsigned char currB = sourceImageCopy[currIndexB];
				
				if(aboveR - currR < limit && aboveR - currR > -limit){
					if(aboveG - currG < limit && aboveG - currG > -limit){
						if(aboveB - currB < limit && aboveB - currB > -limit){
							setSameColor = true;
						}else{
							setSameColor = false;
						}
					}else{
						setSameColor = false;
					}
				}else{
					setSameColor = false;
				}
				
				if(!setSameColor){
					imageData[aboveIndexR] = 0;
					imageData[aboveIndexG] = 0;
					imageData[aboveIndexB] = 0;
					
					imageData[currIndexR] = 255; 
					imageData[currIndexG] = 255; 
					imageData[currIndexB] = 255; 
				}else{
					imageData[aboveIndexR] = 255;
					imageData[aboveIndexG] = 255;
					imageData[aboveIndexB] = 255;
					
					imageData[currIndexR] = 255; 
					imageData[currIndexG] = 255; 
					imageData[currIndexB] = 255; 
				}
			}
		}
	}
}
