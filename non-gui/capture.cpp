// https://github.com/ebonwheeler/Win32GrabScreen/blob/master/capture.cpp

/**

idea: take a bunch of screenshots in sequence (with interval as n milliseconds), 
	  create gif from screenshots

	have a GUI, user inputs 3 args
	- pre-delay, milliseconds (give the user some time to minimize the gui window if they want a gif that is full screen)
	- number of frames to collect 
	- interval between screenshots, in milliseconds (10 <= n <= 1000 ms) cap it at 1000

*/

#include <iostream>
#include <sstream>
#include <string>
#include <windows.h>
#include <gdiplus.h>
#include <memory>

using namespace Gdiplus;
using namespace std;

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
    UINT num = 0;          // number of image encoders
    UINT size = 0;         // size of the image encoder array in bytes

    ImageCodecInfo* pImageCodecInfo = NULL;

    GetImageEncodersSize(&num, &size);
    if(size == 0)
    {
        return -1;  // Failure
    }

    pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
    if(pImageCodecInfo == NULL)
    {
        return -1;  // Failure
    }

    GetImageEncoders(num, size, pImageCodecInfo);

    for(UINT j = 0; j < num; ++j)
    {
        if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
        {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;  // Success
        }    
    }

    free(pImageCodecInfo);
    return -1;  // Failure
}

void BitmapToJpg(HBITMAP hbmpImage, int width, int height, string filename)
{
    Bitmap *p_bmp = Bitmap::FromHBITMAP(hbmpImage, NULL);
    //Bitmap *p_bmp = new Bitmap(width, height, PixelFormat32bppARGB);
    
    CLSID pngClsid;
    int result = GetEncoderClsid(L"image/jpeg", &pngClsid);
    if(result != -1){
        std::cout << "Encoder succeeded" << std::endl;
	}else{
        std::cout << "Encoder failed" << std::endl;
	}
	
	// convert filename to a wstring first
	wstring fname = wstring(filename.begin(), filename.end());
	
	// use .c_str to convert to wchar_t*
    p_bmp->Save(fname.c_str(), &pngClsid, NULL);
    delete p_bmp;
}

bool ScreenCapture(int x, int y, int width, int height, const char *filename)
{
	HDC hDc = CreateCompatibleDC(0);
	HBITMAP hBmp = CreateCompatibleBitmap(GetDC(0), width, height);
	SelectObject(hDc, hBmp);
	BitBlt(hDc, 0, 0, width, height, GetDC(0), x, y, SRCCOPY);
    BitmapToJpg(hBmp, width, height, filename);
	DeleteObject(hBmp);
	return true;
}

string int_to_string(int i){
	stringstream ss;
	ss << i;
	string i_str = ss.str();
	return i_str;
}

int main() {
    // Initialize GDI+.
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    
	int x1 = 0;
	int y1 = 0;
	int x2 = GetSystemMetrics(SM_CXSCREEN);
	int y2 = GetSystemMetrics(SM_CYSCREEN);
	
	// make a temp directory 
	string dirName = "temp";
	if(CreateDirectory(dirName.c_str(), NULL)){
		// do nothing
	}else if(ERROR_ALREADY_EXISTS == GetLastError()){
		// if it exists, empty out the directory
	}else{
		// directory couldn't be made
	}
	
	// need to be able to move the images to the temp directory! (or not?) 
	string name;
	int numImages = 5;
	
	for(int i = 0; i < numImages; i++){
		
		// to_string is not working (not declared in scope error, despite having string header),
		// so this is a good workaround
		name = "screen" + int_to_string(i) + ".jpg";
		ScreenCapture(x1, y1, x2 - x1, y2 - y1, name.c_str());
		Sleep(500);
    }
	
    //Shutdown GDI+
    GdiplusShutdown(gdiplusToken);
	
	return 0;
}