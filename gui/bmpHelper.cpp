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
	
	for(int i = dataSize - 4; i >= 0; i -= 4){
        char temp = 255 - imageData[i];
        imageData[i] = 255 - imageData[i+2];
        imageData[i+2] = temp;
		imageData[i+1] = 255 - imageData[i+1];
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
	
    for(unsigned int i = dataSize - 4; i > 0; i-=4){
        char temp = imageData[i];
        imageData[i] = imageData[i+2];
        imageData[i+2] = temp;
		
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
	
	// reorder the rgb channels since it's currently bgr
	for(unsigned int i = dataSize - 4; i > 0; i-=4){
		char temp = imageData[i];
        imageData[i] = imageData[i+2];
        imageData[i+2] = temp;
	}
	
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

	regular bmp image (no filters)

***/
// get a bmp image and extract the image data into a uint8_t array 
// which will be passed to gif functions from gif.h to create the gif 
std::vector<uint8_t> getBMPImageData(const std::string filename){
    
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
	std::vector<uint8_t> image;
	
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
		
		for(int i = (int)dataSize - 1 ; i >= 0; i--){		
		
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

		int imageSize = (int)image.size();
		
		for(int i = imageSize - 4; i >= 0; i -= 4){
			char temp = image[i];
			image[i] = image[i+2];
			image[i+2] = temp;
		}
		
		int widthSize = 4*(int)width; // total num channels/values per row 
		std::vector<uint8_t> image2;
		
		// flip image horizontally to get the right orientation since it's flipped currently
		// mind the alpha channel! push them after the rgb channels, not before.
		for(int j = 0; j < imageSize; j += widthSize){
			for(int k = widthSize - 1; k >= 0; k-=4){
				// swap b and g 
				image2.push_back(image[j + k-3]); 
				image2.push_back(image[j + k-1]);
				image2.push_back(image[j + k-2]);
				image2.push_back(image[j + k]); // push back alpha channel last
			}
		
		}
		
		return image2;
		
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
		for(int i = dataSize - 4; i >= 0; i -= 4){
			char temp = img[i];
			img[i] = img[i+2];
			img[i+2] = temp;
		}
		
		// change char vector to uint8_t vector
		// be careful! bmp image data is stored upside-down :<
		// so traverse backwards, but also, for each row, invert the row also!
		int widthSize = 4 * (int)width;
		for(int j = (int)(dataSize - 1); j >= 0; j -= widthSize){
			for(int k = widthSize - 1; k >= 0; k--){
				image.push_back((uint8_t)img[j - k]);
			}
		}
		
		return image;
		
	}else{
		// return an empty vector 
		return image;
	}
}

/***

	generic function to get filtered image data from any filter 

***/
std::vector<uint8_t> getBMPImageDataFiltered(const std::string filename, const std::string filtername){
    
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
    //auto depth = *reinterpret_cast<uint16_t *>(&header[28]);

    // now get the image pixel data
    std::vector<char> img(dataOffset - HEADER_SIZE);
    bmp.read(img.data(), img.size());
    
    // width*4 because each pixel is 4 bytes (32-bit bmp)
    auto dataSize = ((width*4 + 3) & (~3)) * height;
    img.resize(dataSize);
    bmp.read(img.data(), img.size());
	
	/** do filter **/
    
    // need to swap R and B (img[i] and img[i+2]) so that the sequence is RGB, not BGR
	// need to keep in mind where the alpha channel is relative to RGB as well. this matters because depending 
	// on how you read the pixel data (i.e. starting from index 0 or the end), alpha might be the first value 
    // also, notice that each pixel is represented by 4 bytes, not 3, because the bmp images are 32-bit (RGBA)
	// this swapping is done in each filtering function (except grayscale, since RGB order does not matter in the end)
	if(filtername == "inverted"){
		inversionFilter(img);
    }else if(filtername == "saturated"){
		saturationFilter(2.1, img);
	}else if(filtername == "weird"){
		weirdFilter(img);
	}else if(filtername == "grayscale"){
		grayscaleFilter(img);
	}

    // change char vector to uint8_t vector
    // be careful! bmp image data is stored upside-down :<
    // so traverse backwards, but also, for each row, invert the row also!
    std::vector<uint8_t> image;
    int widthSize = 4 * (int)width;
    for(int j = (int)(dataSize - 1); j >= 0; j -= widthSize){
        for(int k = widthSize - 1; k >= 0; k--){
            image.push_back((uint8_t)img[j - k]);
        }
    }
    
    return image;
}


/***

	invert image color

***/
std::vector<uint8_t> getBMPImageDataInverted(const std::string filename){
	return getBMPImageDataFiltered(filename, "inverted");
}
   

/***
	saturate image 
***/
std::vector<uint8_t> getBMPImageDataSaturated(const std::string filename){
    return getBMPImageDataFiltered(filename, "saturated");
}

/***
	weird image filter 
***/
std::vector<uint8_t> getBMPImageDataWeird(const std::string filename){
    return getBMPImageDataFiltered(filename, "weird");
}

/***
	grayscale image filter 
***/
std::vector<uint8_t> getBMPImageDataGrayscale(const std::string filename){
    return getBMPImageDataFiltered(filename, "grayscale");
}

