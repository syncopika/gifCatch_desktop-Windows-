// capture.cpp 

#include "capture.hh"  // function declarations
#include "bmpHelper.hh" // function declarations
#include "gif.h"    // needs gif.h to create the gif (https://github.com/ginsweater/gif-h/blob/master/gif.h)
	
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
void getSnapshots(int nImages, int delay, int x, int y, int width, int height, std::vector<uint8_t> (*filter)(const std::string)){
    // Initialize GDI+.
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    
    // make a temp directory 
    std::string dirName = "temp";
    if(CreateDirectory(dirName.c_str(), NULL)){
        // do nothing
    }else if(ERROR_ALREADY_EXISTS == GetLastError()){
        // if it exists, empty out the directory
    }else{
        // directory couldn't be made
    }
    
    // need to be able to move the images to the temp directory! (or not?) 
    std::string name;
    
    for(int i = 0; i < nImages; i++){
        // put all images in temp folder 
        name = "temp/screen" + int_to_string(i) + ".bmp";
        ScreenCapture(x, y, width, height, name.c_str());
        Sleep(delay);
    }
    
    //Shutdown GDI+
    GdiplusShutdown(gdiplusToken);
    
    // make the gif here! using gif.h 
    //int width = x2 - x1;
    //int height = y2 - y1;
    GifWriter gifWriter;
    
    // initialize gifWriter
    // call the gif "test" - it'll be in the same directory as the executable 
    GifBegin(&gifWriter, "test.gif", (uint32_t)width, (uint32_t)height, (uint32_t)delay/10);
    
    // pass in frames 
    std::string nextFrame; 
    for(int i = 0; i < nImages; i++){
        nextFrame = "temp/screen" + int_to_string(i) + ".bmp";
		
		// get image data and apply a filter  
		GifWriteFrame(&gifWriter, (uint8_t *)((*filter)(nextFrame).data()), (uint32_t)width, (uint32_t)height, (uint32_t)(delay/10));

    }
    GifEnd(&gifWriter);
}

// this function resizes bmp images. it's used to make sure all frames being fed to the gif generator 
// are the same dimension. this occurs when a user specifies a directory of bmps to generate a gif from.
// takes in a number indicating how many images to check for resize, and a width and height to resize to
// it returns an integer indicating if anything was resized (1 = something was resized);
// for now, create a new folder called temp_resized to store this new set of images (including the ones that weren't resized)
int resizeBMPs(int nImages, std::vector<std::string> images, int width, int height){
	
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
		
		
		// if dimensions of current image match the initial image, just skip this one 
		// but add it to the new temp directory
		if(h == height && w == width){
			
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
		
		/* testing for writing text to bmp - i.e. memefying 
		// https://msdn.microsoft.com/en-us/library/windows/desktop/ms535993(v=vs.85).aspx
		// https://msdn.microsoft.com/en-us/library/windows/desktop/ms536170(v=vs.85).aspx
		// https://stackoverflow.com/questions/7299196/drawing-text-with-gdi
		// https://stackoverflow.com/questions/44265273/drawtext-with-outline-using-gdi-c
		// https://www.codeproject.com/Articles/42529/Outline-Text#singleoutline1  -> very helpful!
		
		// WCHAR string[] = L"F%^& YEAH";
		// Font impactFont(L"impact", 28);
		// SolidBrush brush(Color(255,0,0,0));
		// Status st = graphics.DrawString(string, 9, &impactFont, PointF(w/3, h/2 + (h/3)), &brush);
		// std::cout << "status: " << st << std::endl;
		
		wchar_t string[] = L"BLAH BLAH BLAH";
		FontFamily impactFont(L"Impact");
		StringFormat strFormat;
		GraphicsPath gpath; 						// use this to hold the outline of the string we want to draw 
		gpath.AddString(string, 					// the string
						wcslen(string), 			// length of string
						&impactFont, 				// font family
						FontStyleRegular,  			// style of type face 
						30, 						// font size 
						Point(w/3, (h/2 + h/3)),	// where to put the string 
						&strFormat 					// layout information for the string 
						);
		Pen pen(Color(0,0,0), 2); 					// color and width of pen 
		graphics.DrawPath(&pen, &gpath);
		SolidBrush brush(Color(255,255,255,255));
		graphics.FillPath(&brush, &gpath);
		
		// end testing
		
		*/
		
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

// this function assembles the gif from bmp images in a specified directory 
void assembleGif(int nImages, int delay, std::vector<std::string> images, std::vector<uint8_t> (*filter)(const std::string)){

    GifWriter gifWriter;
    
    // initialize gifWriter
    // call the gif "test" - it'll be in the same directory as the executable 
	// set the gif to have the dimensions of the first image in the vector (ideally they should all have the same width and height)
	std::vector<int> initialD = getBMPHeightWidth(images[0]);
	
    GifBegin(&gifWriter, "test.gif", (uint32_t)initialD[1], (uint32_t)initialD[0], (uint32_t)delay/10);
    
    // pass in frames 
	if(nImages > (int)images.size()){
		nImages = images.size();
	}
	
	// resize bmps if needed 
	int res = resizeBMPs(nImages, images, initialD[1], initialD[0]);
	
    std::string nextFrame; 
	
	if(res == 1){
		images = std::vector<std::string>();
		for(int i = 0; i < nImages; i++){
			std::string fn = "temp_resized/screen" + int_to_string(i) + ".bmp";
			images.push_back(fn);
		}
	}
	
    for(int i = 0; i < nImages; i++){
        nextFrame = images[i];
		
		//std::vector<int> dimensions = getBMPHeightWidth(images[i]);
		std::cout << nextFrame << std::endl;
		// get image data and apply a filter  
		GifWriteFrame(&gifWriter, (uint8_t *)((*filter)(nextFrame).data()), (uint32_t)initialD[1], (uint32_t)initialD[0], (uint32_t)(delay/10));

    }
	
    GifEnd(&gifWriter);
}


