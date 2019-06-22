// capture.cpp 

// needs gif.h to create the gif (https://github.com/ginsweater/gif-h/blob/master/gif.h) 
// include it here (and not the header file!) to prevent multiple definition errors
#include "headers/gif.h"         
#include "headers/capture.hh"    // function declarations

// probably should convert to non-namespace later 
using namespace Gdiplus;

// convert an integer to string 
std::string int_to_string(int i){
    std::stringstream ss;
    ss << i;
    std::string i_str = ss.str();
    return i_str;
}


int GetEncoderClsid(const WCHAR* format, CLSID* pClsid){
    UINT num = 0;          // number of image encoders
    UINT size = 0;         // size of the image encoder array in bytes

    ImageCodecInfo* pImageCodecInfo = NULL;

    GetImageEncodersSize(&num, &size);
    if(size == 0){
        return -1;  // Failure
    }

    pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
    if(pImageCodecInfo == NULL){
        return -1;  // Failure
    }

    GetImageEncoders(num, size, pImageCodecInfo);

    for(UINT j = 0; j < num; ++j){
        if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 ){
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;  // Success
        }    
    }

    free(pImageCodecInfo);
    return -1;  // Failure
}

void BitmapToBMP(HBITMAP hbmpImage, int width, int height, std::string filename){
    Bitmap *p_bmp = Bitmap::FromHBITMAP(hbmpImage, NULL);
    //Bitmap *p_bmp = new Bitmap(width, height, PixelFormat32bppARGB);
    
    CLSID pngClsid;
	// creating BMP images
    int result = GetEncoderClsid(L"image/bmp", &pngClsid);  
    if(result != -1){
        std::cout << "Encoder succeeded" << std::endl;
    }else{
        std::cout << "Encoder failed" << std::endl;
    }
    
    // convert filename to a wstring first
    std::wstring fname = std::wstring(filename.begin(), filename.end());
    
    // use .c_str to convert to wchar_t*
    p_bmp->Save(fname.c_str(), &pngClsid, NULL);
    delete p_bmp;
}

bool ScreenCapture(int x, int y, int width, int height, const char *filename){
    HDC hDc = CreateCompatibleDC(0);
    HBITMAP hBmp = CreateCompatibleBitmap(GetDC(0), width, height);
    SelectObject(hDc, hBmp);
    BitBlt(hDc, 0, 0, width, height, GetDC(0), x, y, SRCCOPY);
    BitmapToBMP(hBmp, width, height, filename);
    DeleteObject(hBmp);
    return true;
}

// notice this takes a function pointer!
void getSnapshots(int nImages, int delay, int x, int y, int width, int height, std::vector<uint8_t> (*filter)(const std::string, windowInfo*), windowInfo* gifParams){
    
	HWND mainWindow = gifParams->mainWindow;
	
	// Initialize GDI+.
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	
	// get curr time to use for naming the output 
	std::time_t timeNow = std::time(NULL);
	std::tm* ptm = std::localtime(&timeNow);
	
	char buff[32];
	std::strftime(buff, 32, "%d-%m-%Y_%H%M%S", ptm);
	std::string currTime = std::string(buff);
    
    // make a temp directory 
    std::string dirName = "temp_" + currTime;
    if(CreateDirectory(dirName.c_str(), NULL)){
        // do nothing
    }else if(ERROR_ALREADY_EXISTS == GetLastError()){
        // if it exists, empty out the directory
    }else{
        // directory couldn't be made
    }
    
    std::string name;
    for(int i = 0; i < nImages; i++){
        // put all images in temp folder 
        name = dirName + "/screen" + int_to_string(i) + ".bmp";
        ScreenCapture(x, y, width, height, name.c_str());
        Sleep(delay);
    }
    
    //Shutdown GDI+
    GdiplusShutdown(gdiplusToken);
    
    // make the gif here! using gif.h 
    GifWriter gifWriter;
    
    // initialize gifWriter
    // name the gif the current time - it'll be in the same directory as the executable 
	std::string gifName = currTime + ".gif";
    GifBegin(&gifWriter, gifName.c_str(), (uint32_t)width, (uint32_t)height, (uint32_t)delay/10);
    
    // pass in frames 
    std::string nextFrame; 
    for(int i = 0; i < nImages; i++){
        nextFrame = dirName + "/screen" + int_to_string(i) + ".bmp";
		
		// post message to indicate which frame is being processed 
		PostMessage(mainWindow, ID_PROCESS_FRAME, (WPARAM)i, 0);
		
		// get image data and apply a filter  
		GifWriteFrame(&gifWriter, (uint8_t *)((*filter)(nextFrame, gifParams).data()), (uint32_t)width, (uint32_t)height, (uint32_t)(delay/10));
    }
    GifEnd(&gifWriter);
}

// this function resizes bmp images. it's used to make sure all frames being fed to the gif generator 
// are the same dimension. this occurs when a user specifies a directory of bmps to generate a gif from.
// takes in a number indicating how many images to check for resize, and a width and height to resize to
// it returns an integer indicating if anything was resized (1 = something was resized);
// for now, create a new folder called temp_resized to store this new set of images (including the ones that weren't resized)
// last argument is memeText, which is a string that, if not empty (""), will be written near the bottom of each frame  
int resizeBMPs(int nImages, std::vector<std::string> images, int width, int height, std::string memeText){
	
	int resizeResult = 0;
	
	// initialize gdiplus 
	GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	
	// make a temp_resized directory 
    std::string dirName = "temp_resized";
    if(CreateDirectory(dirName.c_str(), NULL)){
        // do nothing
    }else if(ERROR_ALREADY_EXISTS == GetLastError()){
        // if it exists, empty out the directory
    }else{
        // directory couldn't be made
    }
	
	for(int i = 0; i < nImages; i++){
		
		std::string filename = images[i];
		std::wstring wstr = std::wstring(filename.begin(), filename.end());
		const wchar_t *widestr = wstr.c_str();
		
		Bitmap* bmp = new Bitmap(widestr, false);
		int h = bmp->GetHeight();
		int w = bmp->GetWidth();
		
		CLSID pngClsid;
		
		// if dimensions of current image match the initial image and no memetext, just skip this one 
		// but add it to the new temp directory
		if(h == height && w == width && memeText == ""){
			int result = GetEncoderClsid(L"image/bmp", &pngClsid);  
			if(result == -1){
				std::cout << "Encoder failed" << std::endl;
			}
			
			filename = "temp_resized/screen" + int_to_string(i) + ".bmp";
			std::wstring fname = std::wstring(filename.begin(), filename.end());
			bmp->Save(fname.c_str(), &pngClsid, NULL);
			delete bmp;
			continue;
		}
		
		// make a new empty bmp with the new dimensions
		Bitmap* newBMP = new Bitmap(width, height, bmp->GetPixelFormat());
		
		// resize the original bmp 
		Graphics graphics(newBMP); // the new bitmap is the new canvas to draw the resized image on 
		graphics.DrawImage(bmp, 0, 0, width, height);
		// delete bmp 
		delete bmp;
	
		// memefy if there's text in the specified box 
		if(memeText != ""){
			std::wstring mtext = std::wstring(memeText.begin(), memeText.end());
			const wchar_t* string = mtext.c_str(); //L"BLAH BLAH BLAH";
			int stringLen = mtext.size();
			
			// decide where to place the text, x-coordinate-wise 
			// assume each char in the string takes up 15 pixels?
			int xCoord = (w/2) - ((stringLen*15)/2);
			
			FontFamily impactFont(L"Impact");
			StringFormat strFormat;
			GraphicsPath gpath; 						// use this to hold the outline of the string we want to draw 
			gpath.AddString(string, 					// the string
							wcslen(string), 			// length of string
							&impactFont, 				// font family
							FontStyleRegular,  			// style of type face 
							32, 						// font size 
							Point(xCoord, (h/2 + h/3)),	// where to put the string 
							&strFormat 					// layout information for the string 
							);
			Pen pen(Color(0,0,0), 2); 					// color and width of pen 
			pen.SetLineJoin(LineJoinRound);				// prevent sharp pointers from occurring on some chars 
			graphics.SetSmoothingMode(SmoothingModeAntiAlias); // antialias the text so the outline doesn't look choppy
			graphics.DrawPath(&pen, &gpath);
			SolidBrush brush(Color(255,255,255,255));
			graphics.FillPath(&brush, &gpath);
		}
		
		// overwite old file with this new one
		int result = GetEncoderClsid(L"image/bmp", &pngClsid);  
		if(result != -1){
			//std::cout << "Encoder succeeded" << std::endl;
		}else{
			std::cout << "Encoder failed" << std::endl;
		}
		
		// convert filename to a wstring first
		//filename = filename.substr(0, filename.size() - 4);
		filename = "temp_resized/screen" + int_to_string(i) + ".bmp";
		std::wstring fname = std::wstring(filename.begin(), filename.end());
		
		newBMP->Save(fname.c_str(), &pngClsid, NULL);
		//std::cout << "status: " << newBMP->Save(fname.c_str(), &pngClsid, NULL) << std::endl;
	
		resizeResult = 1;
	
		delete newBMP;
	}
	
	// shutdown gdiplus 
	GdiplusShutdown(gdiplusToken);
	
	return resizeResult;
}

/***

	get bmp image data 
	also applies a filter to images if needed 

***/
// get a bmp image and extract the image data into a uint8_t array 
// which will be passed to gif functions from gif.h to create the gif 
std::vector<uint8_t> getBMPImageData(const std::string filename, windowInfo* gifParams){
	
	std::string filtername = (*(gifParams->filters))[gifParams->selectedFilter];
    
	// bmps have a 54 byte header 
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
		int paddedWidth = (int)width*3;
		
		while(paddedWidth%4 != 0){
			paddedWidth++;
		}
		
		// find out how much padding there is per row 
		int padding = paddedWidth - ((int)width*3);
		
		// figure out the size of the pixel data, which includes the padding 
		auto dataSize = (3*width*height) + (height*padding);
		img.resize(dataSize);
		bmp.read(img.data(), img.size());

		int RGBcounter = 0;
		int widthCount = 0;
		
		std::vector<uint8_t> image;
		
		// add in the alpha channel to the data 
		for(int i = 0; i < (int)dataSize; i++){
			
			image.push_back(img[i]);
			RGBcounter++;
		
			// after every third element, add a 255 (this is for the alpha channel)
			if(RGBcounter == 3){
				image.push_back(255);
				RGBcounter = 0;
			}
			
			widthCount++;
			
			// check if we've already gotten all the color channels for a row (if so, skip the padding!)
			if(widthCount == ((int)width*3)){
				widthCount = 0;
				i += padding;
			}			
		}

		// then swap the blue and red channels so we get RGBA
		// using std::size_t is dangerous here! if you use it instead of int, you get a segmentation fault :|
		for(int i = 0; i <= (int)image.size() - 4; i += 4){
			char temp = image[i];
			image[i] = image[i+2];
			image[i+2] = temp;
		}
		
		int widthSize = 4*(int)width; // total num channels per row 
		std::vector<uint8_t> image2;
		
		for(int j = (int)image.size() - 1; j >= 0; j -= widthSize){
			for(int k = widthSize - 1; k >= 0; k--){
				image2.push_back((uint8_t)image[j - k]);
			}
		}
		
		finalImageData = image2;
		
	}else if((int)depth == 32){

		// width*4 because each pixel is 4 bytes (32-bit bmp)
		// ((width*4 + 3) & (~3)) * height; -> this uses bit masking to get the width as a multiple of 4
		auto dataSize = ((width*4 + 3) & (~3)) * height;
		img.resize(dataSize);
		bmp.read(img.data(), img.size());
		
		// need to swap R and B (img[i] and img[i+2]) so that the sequence is RGBA, not BGRA
		// also, notice that each pixel is represented by 4 bytes, not 3, because
		// the bmp images are 32-bit
		for(int i = 0; i <= (int)(dataSize - 4); i += 4){
			char temp = img[i];
			img[i] = img[i+2];
			img[i+2] = temp;
		}
		
		// change char vector to uint8_t vector (why is this necessary, if at all?)
		// be careful! bmp image data is stored upside-down and flipped horizontally :<
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
	
	// use gifParams to get specific parameters for specific filters
	if(filtername == "inverted"){
		inversionFilter(imgDataAsChar);
    }else if(filtername == "saturated"){
		saturationFilter(gifParams->saturationValue, imgDataAsChar);
	}else if(filtername == "weird"){
		weirdFilter(imgDataAsChar);
	}else if(filtername == "grayscale"){
		grayscaleFilter(imgDataAsChar);
	}else if(filtername == "edge_detect"){
		edgeDetectionFilter(imgDataAsChar, (int)width, (int)height);
	}else if(filtername == "mosaic"){
		mosaicFilter(imgDataAsChar, (int)width, (int)height, gifParams->mosaicChunkSize);
	}else if(filtername == "outline"){
		outlineFilter(imgDataAsChar, (int)width, (int)height, gifParams->outlineColorDiffLimit);
	}else if(filtername == "voronoi"){
		voronoiFilter(imgDataAsChar, (int)width, (int)height, 30);
	}
	
	// go back to uint8_t from char 
	for(auto start = imgDataAsChar.begin(), end = imgDataAsChar.end(); start != end; start++){
		// get the index 
		auto index = std::distance(imgDataAsChar.begin(), start);
		finalImageData[index] = *start;
	}
	
	return finalImageData;
	
}


// this function assembles the gif from bmp images in a specified directory 
void assembleGif(int nImages, int delay, std::vector<std::string> images, std::vector<uint8_t> (*filter)(const std::string, windowInfo*), windowInfo* gifParams){
	
	std::string memeText = gifParams->memeText;
	HWND mainWindow = gifParams->mainWindow; // get the handle to the main window so we can post msgs to it 

    GifWriter gifWriter;
    
    // initialize gifWriter
    // the gif will be in the same directory as the executable 
	// set the gif to have the dimensions of the first image in the vector (ideally they should all have the same width and height)
	std::vector<int> initialD = getBMPHeightWidth(images[0]);
	
	// make a function for this step 
	std::time_t timeNow = std::time(NULL);
	std::tm* ptm = std::localtime(&timeNow);
	char buff[32];
	std::strftime(buff, 32, "%d-%m-%Y_%H%M%S", ptm);
	std::string currTime = std::string(buff);
	std::string gifName = currTime + ".gif";
	
	// bit depth is 8 by default and dither is false by default 
    GifBegin(&gifWriter, gifName.c_str(), (uint32_t)initialD[1], (uint32_t)initialD[0], (uint32_t)delay/10);
    
    // pass in frames 
	if(nImages > (int)images.size()){
		nImages = images.size();
	}
	
	// resize bmps if needed 
	resizeBMPs(nImages, images, initialD[1], initialD[0], memeText);
	
	// right now a temp directory of images is created, whether or not resizing occurred at all 
	// use that temp directory to generate the new gif from 
	images = std::vector<std::string>();
	for(int i = 0; i < nImages; i++){
		std::string fn = "temp_resized/screen" + int_to_string(i) + ".bmp";
		images.push_back(fn);
	}
	
	// make the gif 
	std::string nextFrame; 
    for(int i = 0; i < nImages; i++){
        nextFrame = images[i];
		
		PostMessage(mainWindow, ID_PROCESS_FRAME, (WPARAM)i, 0);
		
		// get image data and apply a filter  
		GifWriteFrame(&gifWriter, (uint8_t *)((*filter)(nextFrame, gifParams).data()), (uint32_t)initialD[1], (uint32_t)initialD[0], (uint32_t)(delay/10));
    }
	
    GifEnd(&gifWriter);
}


