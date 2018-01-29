// capture.cpp 

#include "capture.hh"  // function declarations
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


// get a bmp image and extract the image data into a uint8_t array 
// which will be passed to gif functions from gif.h to create the gif 
std::vector<uint8_t> getBMPImageData(const std::string filename){
    
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
    
    //std::cout << "file size: " << fileSize << std::endl;
    //std::cout << "dataOffset: " << dataOffset << std::endl;
    //std::cout << "width: " << width << std::endl;
    //std::cout << "height: " << height << std::endl;
    //std::cout << "depth: " << depth << "-bit" << std::endl;
    
    // now get the image pixel data
    std::vector<char> img(dataOffset - HEADER_SIZE);
    bmp.read(img.data(), img.size());
    
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
    std::vector<uint8_t> image;
    int widthSize = 4 * (int)width;
    for(int j = (int)(dataSize - 1); j >= 0; j -= widthSize){
        for(int k = widthSize - 1; k >= 0; k--){
            image.push_back((uint8_t)img[j - k]);
        }
    }
    
    return image;
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

void getSnapshots(int nImages, int delay, int x, int y, int width, int height){
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
        GifWriteFrame(&gifWriter, (uint8_t*)getBMPImageData(nextFrame).data(), (uint32_t)width, (uint32_t)height, (uint32_t)(delay/10));
    }
    GifEnd(&gifWriter);

}