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
	
    for(unsigned int i = 0; i <= dataSize - 4; i+=4){
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
		
		// this has some good info: https://www.codeguru.com/cpp/g-m/bitmap/article.php/c4909/Exploring-the-Internal-Structure-of-a-24Bit-Uncompressed-Bitmap-File.htm
		// about 24-bit bitmaps 
		
		// since 24-bit bmps round up to nearest width divisible by 4, 
		// there might be some extra padding at the end of each pixel row 
		int paddedWidth = (int)width * 3;
		
		while(paddedWidth % 4 != 0){
			paddedWidth++;
		}
		
		// find out how much padding there is 
		int padding = paddedWidth - ((int)width * 3);

		// figure out the size of the pixel data, which includes the padding 
		auto dataSize = (3 * width * height) + (height * padding);
		img.resize(dataSize);
		bmp.read(img.data(), img.size());
		
		// this proves that there is extra padding added 
		/*
		for(int i = ((int)(width*3) + padding - 20); i < (int)(width*3) + padding; i++){
			std::cout << (int)img[i] << std::endl;
		}*/

		int counter = 0;
		int widthCount = 0;
		for(int i = (int)dataSize - 1; i >= 0; i--){
			
			// INVESTIGATE THIS PART HERE 
			// why does adding 840 correct the image? (but not color)
			if(widthCount >= (3*(int)width) + 840 + padding){
				i -= padding;
				widthCount = 0;
			}else{		
				if(counter % 3 == 0 && i != 0 && i != (int)dataSize - 1){
					image.push_back((char)255);
					image.push_back(img[i]);
					counter = 1;
				}else{
					image.push_back(img[i]);
					counter++;
				}
				widthCount++;
			}
		}
		int imageSize = (int)image.size();
		
		for(int i = imageSize - 4; i >= 0; i -= 4){
			uint8_t temp = image[i];
			image[i] = image[i+2];
			image[i+2] = temp;
		}
		
		return image;
		
		/*
		int widthSize = 4 * (int)width;
		int totalSize = image.size();
		
		std::vector<uint8_t> finalImage;
		for(int j = totalSize - 1; j >= 0; j -= widthSize){
			for(int k = widthSize - 1; k >= 0; k--){
				finalImage.push_back((uint8_t)image[j - k]);
			}
		}
		
		return finalImage;
		*/
		
	}else if((int)depth == 32){

		// width*4 because each pixel is 4 bytes (32-bit bmp)
		auto dataSize = ((width*4 + 3) & (~3)) * height;
		img.resize(dataSize);
		bmp.read(img.data(), img.size());
		
		// need to swap R and B (img[i] and img[i+2]) so that the sequence is RGB, not BGR
		// also, notice that each pixel is represented by 4 bytes, not 3, because
		// the bmp images are 32-bit
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

	invert image color

***/
std::vector<uint8_t> getBMPImageDataInverted(const std::string filename){
    
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
	
	/** do inversion filter **/
    
    // need to swap R and B (img[i] and img[i+2]) so that the sequence is RGB, not BGR
    // also, notice that each pixel is represented by 4 bytes, not 3, because
    // the bmp images are 32-bit
    inversionFilter(img);
    
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

	saturate image color

***/
std::vector<uint8_t> getBMPImageDataSaturated(const std::string filename){
    
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
	std::cout << "the depth is: " << depth << std::endl;
	
    // now get the image pixel data
    std::vector<char> img(dataOffset - HEADER_SIZE);
    bmp.read(img.data(), img.size());
    
    // width*4 because each pixel is 4 bytes (32-bit bmp)
    auto dataSize = ((width*4 + 3) & (~3)) * height;
    img.resize(dataSize);
    bmp.read(img.data(), img.size());
	
	/** do saturation filter **/
    saturationFilter(2.1, img);
    
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
