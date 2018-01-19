/**
	idea: take a bunch of screenshots in sequence (with interval as n milliseconds), 
	create gif from screenshots

	have a GUI, user inputs 3 args
	- pre-delay, milliseconds (give the user some time to minimize the gui window if they want a gif that is full screen)
	- number of frames to collect 
	- interval between screenshots, in milliseconds (10 <= n <= 1000 ms) cap it at 1000

	maybe give fps option and length of gif, not choose delay and number of frames.

	check out resources.txt in the main directory outside this one for helpful links/resources!
*/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <string>
#include <vector>
#include <array>
#include <fstream>
#include <iterator>
#include <windows.h>
#include <gdiplus.h>
#include <memory>
#include "gif.h"
//#include "resource.h"

// give some identifiers for the GUI numbers 
#define ID_TITLE_LABEL 99

#define ID_NUMFRAMES_LABEL 100
#define ID_NUMFRAMES_TEXTBOX 101
#define ID_ADD_NUMFRAMES 102 

#define ID_DELAY_LABEL 103 
#define ID_DELAY_TEXTBOX 104
#define ID_ADD_DELAY 105

#define ID_SELECT_SCREENAREA_BUTTON 106 
#define ID_START_BUTTON 107

using namespace Gdiplus;
using namespace std;

/*****************

	global variables

*****************/

int x1 = 0; // top left x coord 
int y1 = 0; // top left y coord
int x2 = GetSystemMetrics(SM_CXSCREEN); // screen bottom right x coord 
int y2 = GetSystemMetrics(SM_CYSCREEN); // screen bottom right y coord 

// for selection window 
bool bDrag = false;
bool bDraw = false;

// point structs for keeping track of start and final coordinates
POINT ptCurr = {0, 0};
POINT ptNew = {0, 0};

// register 2 different windows 
const char g_szClassName[] = "mainGUI";
const char g_szClassName2[] = "selectionWindow";

// handler variables for the windows 
HWND hwnd;
HWND selectionWindow;



// convert an integer to string 
string int_to_string(int i){
	stringstream ss;
	ss << i;
	string i_str = ss.str();
	return i_str;
}


// get a bmp image and extract the image data into a uint8_t array 
// which will be passed to gif functions from gif.h to create the gif 
vector<uint8_t> getBMPImageData(const string filename){
	
	static constexpr size_t HEADER_SIZE = 54;
	
	ifstream bmp(filename, ios::binary);
	
	// this represents the header of the bmp file 
	array<char, HEADER_SIZE> header;
	
	// read in 54 bytes of the file and put that data in the header array
	bmp.read(header.data(), header.size());
	
	//auto fileSize = *reinterpret_cast<uint32_t *>(&header[2]);
	auto dataOffset = *reinterpret_cast<uint32_t *>(&header[10]);
	auto width = *reinterpret_cast<uint32_t *>(&header[18]);
	auto height = *reinterpret_cast<uint32_t *>(&header[22]);
	//auto depth = *reinterpret_cast<uint16_t *>(&header[28]);
	
	//cout << "file size: " << fileSize << endl;
	//cout << "dataOffset: " << dataOffset << endl;
	//cout << "width: " << width << endl;
	//cout << "height: " << height << endl;
	//cout << "depth: " << depth << "-bit" << endl;
	
	// now get the image pixel data
	vector<char> img(dataOffset - HEADER_SIZE);
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
	vector<uint8_t> image;
	int widthSize = 4 * (int)width;
	for(int j = (int)(dataSize - 1); j >= 0; j -= widthSize){
		for(int k = widthSize - 1; k >= 0; k--){
			image.push_back((uint8_t)img[j - k]);
		}
	}
	
	return image;
}


int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
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

void BitmapToBMP(HBITMAP hbmpImage, int width, int height, string filename)
{
    Bitmap *p_bmp = Bitmap::FromHBITMAP(hbmpImage, NULL);
    //Bitmap *p_bmp = new Bitmap(width, height, PixelFormat32bppARGB);
    
	// how about pngs? bmp?
    CLSID pngClsid;
    int result = GetEncoderClsid(L"image/bmp", &pngClsid);  
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
    BitmapToBMP(hBmp, width, height, filename);
	DeleteObject(hBmp);
	return true;
}

void getSnapshots(int nImages, int delay){
    // Initialize GDI+.
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	
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
	
	for(int i = 0; i < nImages; i++){
		// put all images in temp folder 
		// notice x1, y1, x2, and y2 are global variables 
		name = "temp/screen" + int_to_string(i) + ".bmp";
		ScreenCapture(x1, y1, x2 - x1, y2 - y1, name.c_str());
		Sleep(delay);
    }
	
    //Shutdown GDI+
    GdiplusShutdown(gdiplusToken);
	
	// make the gif here! using gif.h 
	int width = x2 - x1;
	int height = y2 - y1;
	GifWriter gifWriter;
	
	// initialize gifWriter
	// call the gif "test" - it'll be in the same directory as the executable 
	GifBegin(&gifWriter, "test.gif", (uint32_t)width, (uint32_t)height, (uint32_t)delay/10);
	
	// pass in frames 
	string nextFrame; 
	for(int i = 0; i < nImages; i++){
		nextFrame = "temp/screen" + int_to_string(i) + ".bmp";
		GifWriteFrame(&gifWriter, (uint8_t*)getBMPImageData(nextFrame).data(), (uint32_t)width, (uint32_t)height, (uint32_t)(delay/10));
	}
	GifEnd(&gifWriter);

}



/******************

DO Win32 GUI STUFF HERE 

*****************/
/* 

	the window procedure 
	this handles the functionality of the window it's attached to 
	
*/
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
	
	switch(msg){

		case WM_COMMAND:
			/* LOWORD takes the lower 16 bits of wParam => the element clicked on */
			switch(LOWORD(wParam)){

				case ID_SELECT_SCREENAREA_BUTTON:
				{
					//implement me!
					// make a new window to select the area 
					// hide this window until selection has been made 
					int x = GetSystemMetrics(SM_CXSCREEN);
					int y = GetSystemMetrics(SM_CYSCREEN);
					selectionWindow = CreateWindowEx(
										WS_EX_LAYERED,
										g_szClassName2,
										"selection",
										WS_TILEDWINDOW,
										0, 0, x, y,
										NULL, NULL, GetModuleHandle(NULL), NULL
									);

					// make window transparent
					SetLayeredWindowAttributes(
						selectionWindow,
						0, 
						(255 * 70) / 100, 
						LWA_ALPHA
					);
					
					// show window
					ShowWindow(selectionWindow, SW_MAXIMIZE);
					UpdateWindow(selectionWindow);
					
				}
				break;
				case ID_START_BUTTON:
				{	
					// implement me!
					// get the parameters 
					HWND frames = GetDlgItem(hwnd, ID_NUMFRAMES_TEXTBOX);
					HWND delay = GetDlgItem(hwnd, ID_DELAY_TEXTBOX);
					
					TCHAR numFrames[3];
					TCHAR timeDelay[5];
					
					GetWindowText(frames, numFrames, 3);
					GetWindowText(delay, timeDelay, 5);
					
					int nFrames = atoi(numFrames);
					int tDelay = atoi(timeDelay);
					
					/* for debugging */
					//cout << "num frames: " << int_to_string(nFrames) << endl;
					//cout << "delay: " << int_to_string(tDelay) << endl;
					
					// get window parameters 
					// by default screenshot whole screen
					getSnapshots(nFrames, tDelay);
				}
				break;
			}
		break;
		case WM_CLOSE:
		{
			DestroyWindow(hwnd);
		}
		break;
		case WM_DESTROY:
		{
			PostQuitMessage(0);
		}
		break;
		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

/*
	
	window procedure for the selection window 
	
*/
void reset(POINT *p1, POINT *p2, bool *drag, bool *draw){
	*drag = false;
	*draw = false;
	p1->x = 0;
	p1->y = 0;
	p2->x = 0;
	p2->y = 0;
}

LRESULT CALLBACK WndProcSelection(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){

	switch(msg){
		case WM_LBUTTONDOWN:
		{
			SetCapture(hwnd);
			bDrag = true; // need button down to initiate mousemove step 

			// log the coordinates 
			GetCursorPos(&ptCurr);
			// check out the difference between ScreenToClient vs ClientToScreen 
			ScreenToClient(hwnd, &ptCurr);
		}
		break;
		case WM_MOUSEMOVE:
		{
			if(bDrag){
			
				bDraw = true;
				
				// log the coordinates 
				GetCursorPos(&ptNew);
				
				// convert coordinates
				ScreenToClient(hwnd, &ptNew);
				bDrag = false;
			
			}else if(bDraw){
				
				HDC hdc = GetDC(hwnd);
				SelectObject(hdc,GetStockObject(DC_BRUSH));
				SetDCBrushColor(hdc, RGB(255,0,0)); // set color (red)
				
				SetROP2(hdc, R2_NOTXORPEN);
				
				// erase old rectangle 
				Rectangle(hdc, ptCurr.x, ptCurr.y, ptNew.x, ptNew.y);
				
				// collect current coordinates, set them as new 
				GetCursorPos(&ptNew);
				ScreenToClient(hwnd, &ptNew);
				
				// draw new rectangle 
				Rectangle(hdc, ptCurr.x, ptCurr.y, ptNew.x, ptNew.y);
				ReleaseDC(hwnd, hdc);
				
			}
		}
		break;
		case WM_LBUTTONUP:
		{
			if(bDraw){
				
				// get coordinates 
				//cout << "start x: " << ptCurr.x << ", start y: " << ptCurr.y << endl;
				//cout << "end x: " << ptNew.x << ", end y: " << ptNew.y << endl;
				
				// check if ok with user 
				int response = MessageBox(hwnd, "Is this selection okay?", "Confirm Selection", MB_YESNOCANCEL);
				if(response == IDCANCEL){
					reset(&ptCurr, &ptNew, &bDrag, &bDraw);
					ReleaseCapture();
					DestroyWindow(hwnd);
					return 0; 
				}else if(response == IDYES){
					// done, record new parameters 
					x1 = ptCurr.x;
					y1 = ptCurr.y;
					x2 = ptNew.x;
					y2 = ptNew.y;
					
					// do some correction to take into account the title bar height 
					if(ptCurr.y > 0){
						y1 = y1 + GetSystemMetrics(SM_CYCAPTION);
					}
					
					reset(&ptCurr, &ptNew, &bDrag, &bDraw);
					DestroyWindow(hwnd);
					ReleaseCapture();
					return 0;
				}else if(response == IDNO){
					// need to clear screen!!
					HDC hdc = GetDC(hwnd);
					SelectObject(hdc,GetStockObject(DC_BRUSH));
					SetDCBrushColor(hdc, RGB(255,0,0)); 
					SetROP2(hdc, R2_NOTXORPEN);
					// erase old rectangle 
					Rectangle(hdc, ptCurr.x, ptCurr.y, ptNew.x, ptNew.y);
					ReleaseDC(hwnd, hdc);
					// reset stuff
					reset(&ptCurr, &ptNew, &bDrag, &bDraw);
				}else{
					// failure to show message box	
					cout << "error with message box!!" << endl;
					ReleaseCapture();
					DestroyWindow(hwnd);
					return 0;
				}
			}
		}
		break;
		case WM_CLOSE:
		{
			DestroyWindow(hwnd);
			return 0;
		}
		break;
		case WM_DESTROY:
		{
			DestroyWindow(hwnd);
			return 0;
		}
		break;
		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;


}

/*************

	GUI CODE
	
**************/

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){

	/* console attached for debugging */
	//AllocConsole();
	//freopen( "CON", "w", stdout );
	
	WNDCLASSEX wc; // this is the main GUI window 
	MSG Msg;
	
	/* register the window class */
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = 0;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszMenuName = NULL; //MAKEINTRESOURCE(IDR_MYMENU);
	wc.lpszClassName = g_szClassName;
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	
	WNDCLASSEX wc2;
	/* register the second window class */
	wc2.cbSize = sizeof(WNDCLASSEX);
	wc2.style = 0;
	wc2.lpfnWndProc = WndProcSelection;
	wc2.cbClsExtra = 0;
	wc2.cbWndExtra = 0;
	wc2.hInstance = hInstance;
	wc2.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc2.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc2.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc2.lpszMenuName = NULL;
	wc2.lpszClassName = g_szClassName2;
	wc2.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	
	
	if(!RegisterClassEx(&wc)){
		cout << "error code: " << GetLastError() << endl;
		MessageBox(NULL, "window registration failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}
	
	if(!RegisterClassEx(&wc2)){
		cout << "error code: " << GetLastError() << endl;
		MessageBox(NULL, "window registration failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}
	
	/* create the window */
	hwnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		g_szClassName,
		"gifGetter",
		WS_TILEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 400, 400,
		NULL, NULL, hInstance, NULL
	);
	
	if(hwnd == NULL){
		MessageBox(NULL, "window creation failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}
	
	/* make a title label */
	CreateWindow(
		TEXT("STATIC"),
		TEXT(" gif getter \n nch "),
		WS_VISIBLE | WS_CHILD | SS_LEFT,
		10, 10,
		200, 40,
		hwnd, /* parent window */
		(HMENU)ID_TITLE_LABEL,
		hInstance,
		NULL
	);
	
	
	/* make text box for # FRAMES TO COLLECT (HWND textInputPriorityLabel) */
	CreateWindow(
		TEXT("STATIC"),
		TEXT("# frames to collect: "),
		WS_VISIBLE | WS_CHILD | SS_LEFT,
		10, 90,
		130, 20,
		hwnd, /* parent window */
		(HMENU)ID_NUMFRAMES_LABEL,
		hInstance,
		NULL
	);
	/* make text box  ADD NUMBER OF FRAMES TO COLLECT  (HWND textInput)*/
	CreateWindow(
		TEXT("edit"),
		TEXT(""),
		WS_VISIBLE | WS_CHILD | WS_BORDER,
		150, 90,  /* x, y coords */
		80, 20, /* width, height */
		hwnd,
		(HMENU)ID_NUMFRAMES_TEXTBOX,
		hInstance,
		NULL
	);
	/* prepopulate text input */
	SetDlgItemText(hwnd, ID_NUMFRAMES_TEXTBOX, "15");
	CreateWindow(
		TEXT("STATIC"),
		TEXT("1 <= frames <= 50"),
		WS_VISIBLE | WS_CHILD | SS_LEFT,
		240, 90,
		130, 20,
		hwnd, /* parent window */
		NULL,
		hInstance,
		NULL
	);
	
	
	/* make text box LABEL FOR DELAY (HWND textInputPriorityLabel) */
	CreateWindow(
		TEXT("STATIC"),
		TEXT("# ms delay: "),
		WS_VISIBLE | WS_CHILD | SS_LEFT,
		10, 120,
		100, 20,
		hwnd, /* parent window */
		(HMENU)ID_DELAY_LABEL,
		hInstance,
		NULL
	);
	CreateWindow(
		TEXT("edit"),
		TEXT(""),
		WS_VISIBLE | WS_CHILD | WS_BORDER,
		120, 120,  /* x, y coords */
		80, 20, /* width, height */
		hwnd,
		(HMENU)ID_DELAY_TEXTBOX,
		hInstance,
		NULL
	);
	/* prepopulate text input */
	SetDlgItemText(hwnd, ID_DELAY_TEXTBOX, "100");
	CreateWindow(
		TEXT("STATIC"),
		TEXT("10 <= ms <= 1000"),
		WS_VISIBLE | WS_CHILD | SS_LEFT,
		210, 120,
		130, 20,
		hwnd, /* parent window */
		NULL,
		hInstance,
		NULL
	);
	
	
	/* button to select area of screen  */
	CreateWindow(
		TEXT("button"),
		TEXT("select area"),
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		150, 220,
		80, 20, 
		hwnd,
		(HMENU)ID_SELECT_SCREENAREA_BUTTON,
		hInstance,
		NULL
	);
	
	/* button to start the screen capture */
	CreateWindow(
		TEXT("button"),
		TEXT("start"),
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		150, 250,
		80, 20, 
		hwnd,
		(HMENU)ID_START_BUTTON,
		hInstance,
		NULL
	);
	
	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);
	
	/* message loop */
	while(GetMessage(&Msg, NULL, 0, 0) > 0){
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
	return Msg.wParam;
}








