#include "bmpHelper.hh"

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
void mosaicFilter(std::vector<char>& imageData, int width, int height){
	
	// make a copy of the data 
	std::vector<char> sourceImageCopy(imageData);
	
	// change sampling size here. lower for higher detail preservation, higher for less detail (because larger chunks)
	int chunkWidth = 30;
	int chunkHeight = 30;
	
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
void outlineFilter(std::vector<char>& imageData, int width, int height){
	
	// make a copy of the data 
	std::vector<char> sourceImageCopy(imageData);
	
	// for each pixel, check the above pixel (if it exists)
	// if the above pixel is 'significantly' different (i.e. more than +/- 5 of rgb),
	// color the above pixel black and the current pixel white. otherwise, both become white. 
	int limit = 10;
	
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

/***

	get bmp image data 
	also applies a filter to images if needed 

***/
// get a bmp image and extract the image data into a uint8_t array 
// which will be passed to gif functions from gif.h to create the gif 
std::vector<uint8_t> getBMPImageData(const std::string filename, const std::string filtername){
    
	// bmp's have a 54 byte header 
    static constexpr size_t HEADER_SIZE = 54;
    
    // read in bmp file as stream
    std::ifstream bmp(filename, std::ios::binary);
    
    // this represents the header of the bmp file 
    std::array<char, HEADER_SIZE> header;
    
    // read in 54 bytes of the file and put that data in the header array
    bmp.read(header.data(), header.size());
    
    //auto fileSize = *reinterpret_cast<uint32_t *>(&header[2]);
    auto dataOffset = *reinterpret_cast<uint32_t *>(&header[10]);
    auto width = *reinterpret_cast<uint32_t *>(&header[18]);
    auto height = *reinterpret_cast<uint32_t *>(&header[22]);
    auto depth = *reinterpret_cast<uint16_t *>(&header[28]);
	
	// now get the image pixel data
	// not sure this part is necessary since dataOffset comes out to be 54 
	// which is the header size, when I test it
	std::vector<char> img(dataOffset - HEADER_SIZE);
	bmp.read(img.data(), img.size());
	
	// use this vector to store all the pixel data, which will be returned
	std::vector<uint8_t> finalImageData;
	
	if((int)depth == 24){
		
		// since 24-bit bmps round up to nearest width divisible by 4, 
		// there might be some extra padding at the end of each pixel row 
		int paddedWidth = (int)width * 3;
		
		while(paddedWidth % 4 != 0){
			paddedWidth++;
		}
		
		// find out how much padding there is per row 
		int padding = paddedWidth - ((int)width * 3);
		
		// figure out the size of the pixel data, which includes the padding 
		auto dataSize = (3 * width * height) + (height * padding);
		img.resize(dataSize);
		bmp.read(img.data(), img.size());

		int RGBcounter = 0;
		int widthCount = 0;
		//int rowCount = 0;
		
		std::vector<uint8_t> image;
		for(int i = dataSize - 1 ; i >= 0; i--){		
		
			// after every third element, add a 255 (this is for the alpha channel)
			image.push_back(img[i]);
			RGBcounter++;
			
			if(RGBcounter == 3){
				image.push_back(255);
				RGBcounter = 0;
			}
			
			widthCount++;
			// check if we've already gotten all the color channels for a row (if so, skip the padding!)
			if(widthCount == ((int)width * 3)){
				widthCount = 0;
				
				// skip the padding
				i -= padding;
				//std::cout << "at row number: " << rowCount++ << std::endl;
			}
			
		}

		// using std::size_t is dangerous here! if you use it instead of int, you get a segmentation fault :|
		// I think casting is ok here 
		for(int i = (int)image.size() - 4; i >= 0; i -= 4){
			char temp = image[i];
			image[i] = image[i+2];
			image[i+2] = temp;
		}
		
		int widthSize = 4*(int)width; // total num channels/values per row 
		std::vector<uint8_t> image2;
		
		// flip image horizontally to get the right orientation since it's flipped currently
		// mind the alpha channel! push them after the rgb channels, not before.
		for(int j = 0; j < (int)image.size(); j += widthSize){
			for(int k = widthSize - 1; k >= 0; k-=4){
				// swap b and g 
				image2.push_back(image[j + k-3]); 
				image2.push_back(image[j + k-1]);
				image2.push_back(image[j + k-2]);
				image2.push_back(image[j + k]); // push back alpha channel last
			}
		
		}
		
		finalImageData = image2;
		
	}else if((int)depth == 32){

		// width*4 because each pixel is 4 bytes (32-bit bmp)
		// ((width*4 + 3) & (~3)) * height; -> this uses bit masking to get the width as a multiple of 4
		auto dataSize = ((width*4 + 3) & (~3)) * height;
		img.resize(dataSize);
		bmp.read(img.data(), img.size());
		
		// need to swap R and B (img[i] and img[i+2]) so that the sequence is RGB, not BGR
		// also, notice that each pixel is represented by 4 bytes, not 3, because
		// the bmp images are 32-bit
		// reading backwards gets you BGRA instead of ARGB if reading from index 0 
		// also, intersting note: if you change int to auto, you get a segmentation fault. :|
		for(int i = dataSize - 4; i >= 0; i -= 4){
			char temp = img[i];
			img[i] = img[i+2];
			img[i+2] = temp;
		}
		
		// change char vector to uint8_t vector
		// be careful! bmp image data is stored upside-down :<
		// so traverse backwards, but also, for each row, invert the row also!
		std::vector<uint8_t> image;
		int widthSize = 4 * (int)width;
		for(int j = (dataSize - 1); j >= 0; j -= widthSize){
			for(int k = widthSize - 1; k >= 0; k--){
				image.push_back((uint8_t)img[j - k]);
			}
		}
		
		finalImageData = image;
		
	}else{
		// return an empty vector 
		return finalImageData;
	}
	
	// apply filters as needed 
	if(filtername == "none"){
		// ready to move on to next step 
		return finalImageData;
	}
	
	// use this for passing to the filters (which need a char vector)
	// cast the uint8_t to chars first
	std::vector<char> imgDataAsChar;
	for(auto start = finalImageData.begin(), end = finalImageData.end(); start != end; start++){
		imgDataAsChar.push_back(static_cast<char>(*start));
	}
	
	if(filtername == "inverted"){
		inversionFilter(imgDataAsChar);
    }else if(filtername == "saturated"){
		saturationFilter(2.1, imgDataAsChar);
	}else if(filtername == "weird"){
		weirdFilter(imgDataAsChar);
	}else if(filtername == "grayscale"){
		grayscaleFilter(imgDataAsChar);
	}else if(filtername == "edgeDetection"){
		edgeDetectionFilter(imgDataAsChar, (int)width, (int)height);
	}else if(filtername == "mosaic"){
		mosaicFilter(imgDataAsChar, (int)width, (int)height);
	}else if(filtername == "outline"){
		outlineFilter(imgDataAsChar, (int)width, (int)height);
	}
	
	// go back to uint8_t from char 
	for(auto start = imgDataAsChar.begin(), end = imgDataAsChar.end(); start != end; start++){
		// get the index 
		auto index = std::distance(imgDataAsChar.begin(), start);
		finalImageData[index] = *start;
	}
	
	return finalImageData;
	
}

/***

	just get image data (no filters)

***/
std::vector<uint8_t> getBMPImageData(const std::string filename){
	return getBMPImageData(filename, "none");
}


/***

	invert image color

***/
std::vector<uint8_t> getBMPImageDataInverted(const std::string filename){
	return getBMPImageData(filename, "inverted");
}
   

/***
	saturate image 
***/
std::vector<uint8_t> getBMPImageDataSaturated(const std::string filename){
    return getBMPImageData(filename, "saturated");
}

/***
	weird image filter 
***/
std::vector<uint8_t> getBMPImageDataWeird(const std::string filename){
    return getBMPImageData(filename, "weird");
}

/***
	grayscale image filter 
***/
std::vector<uint8_t> getBMPImageDataGrayscale(const std::string filename){
    return getBMPImageData(filename, "grayscale");
}

/***
	edge detection with Sobel filter 
***/
std::vector<uint8_t> getBMPImageDataEdgeDetection(const std::string filename){
	return getBMPImageData(filename, "edgeDetection");
}

/***
	mosaic filter 
***/
std::vector<uint8_t> getBMPImageDataMosaic(const std::string filename){
	return getBMPImageData(filename, "mosaic");
}

/***
	outline filter 
***/
std::vector<uint8_t> getBMPImageDataOutline(const std::string filename){
	return getBMPImageData(filename, "outline");
}
