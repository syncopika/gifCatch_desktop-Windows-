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

// for improving GUI appearance
// these definitions have to be declared here 
#define _WIN32_WINNT 0x0601
#define _WIN32_IE 0x0900

// IDs for signalling certain progress messages
// see application-defined msgs
// https://docs.microsoft.com/en-us/windows/desktop/winmsg/about-messages-and-message-queues#application-defined-messages
#define ID_IN_PROGRESS 		 (WM_APP + 0)
#define ID_FINISHED 		 (WM_APP + 1)
#define ID_UNABLE_TO_OPEN	 (WM_APP + 2)
#define ID_NO_BMPS_FOUND 	 (WM_APP + 3)
#define ID_ASSEMBLING_GIF 	 (WM_APP + 4)
#define ID_COLLECTING_IMAGES (WM_APP + 5)

// define a color for screen selection 
#define COLOR RGB(255,130,140)

#include <stdlib.h>  // for atoi 

// this also brings in windows.h, gdiplus.h, and everything else 
#include "capture.hh"
#include "bmpHelper.hh"

// for improving GUI appearance
// defined here since it needs to come after windows.h
#include <commctrl.h> 

// for directory finding 
#include <dirent.h>

// give some identifiers for the GUI components 
#include "resources.h"

#define ID_MAIN_PAGE 9000
#define ID_SET_PARAMETERS_PAGE 9001

// struct to provide arguments needed for gif creation 
// notice how in c++ you can declare non-typedef'd structs without struct!
struct windowInfo {
	int numFrames;
	int timeDelay;
	int selectedFilter;
	std::string directory;
	std::string memeText;
	HWND mainWindow; // main window so the worker thread can post messages to its queue 
};

// struct that will hold currently set parameters for things like filters, selection screen color 
struct currentSettings {
	COLORREF selectionWindowColor;
};


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

// register 4 different windows 
const char g_szClassName[] = "mainGUI";
const char g_szClassName2[] = "mainPage";
const char g_szClassName3[] = "parametersPage";
const char g_szClassName4[] = "selectionWindow";

// handler variables for the windows 
HWND hwnd;	// this is the main GUI window handle (the parent window of the main and parameter pages)
HWND mainPage; // this is the main page of the GUI 
HWND parameterPage; // this is the window handle for the page where the user can set some parameters 
HWND selectionWindow;	// this is the handle for the rubber-banding selection window 


// use Tahoma font for the text 
// this HFONT object needs to be deleted (via DeleteObject) when program ends 
HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, 
      OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
      DEFAULT_PITCH | FF_DONTCARE, TEXT("Tahoma"));
	  
// default settings 
currentSettings currSettings {
	COLOR
};


/******************

	DO Win32 GUI STUFF HERE 

*****************/

void makeGif(windowInfo* args){
	
	HWND mainWindow = args->mainWindow;
	
	PostMessage(mainWindow, ID_IN_PROGRESS, 0, 0);
	
	int nFrames = args->numFrames;
	//std::cout << "frames: " << nFrames << std::endl;
	
	int tDelay = args->timeDelay;
	//std::cout << "delay: " << tDelay << std::endl;
	
	int currFilterIndex = args->selectedFilter;
	//std::cout << "currFilterIndex: " << currFilterIndex << std::endl;
	
	std::string theDir = args->directory;
	//std::cout << "directory: " << theDir << std::endl;
	
	std::string theText = args->memeText;
	//std::cout << "memetext: " << theText << std::endl;
	
	// indicate process started 
	PostMessage(mainWindow, ID_IN_PROGRESS, 0, 0);
	
	if(theDir != ""){
		// user wants to assemble a gif from some already made bmps!
		struct dirent *dir_entry;
		DIR *pd = 0;
		
		pd = opendir(theDir.c_str());
		if(pd == NULL){
			std::cout << "unable to open directory..." << std::endl;
			//PostMessage(mainWindow, ID_UNABLE_TO_OPEN, 0, 0);
			delete args;
			return;
		}
		
		// check directory contents 
		// indicate to user that images are being looked through 
		PostMessage(mainWindow, ID_COLLECTING_IMAGES, 0, 0);
		
		// images1 will hold any images numbered from 0 to 9. 
		// images2 will hold images numbered after 9. 
		std::vector<std::string> images1;
		std::vector<std::string> images2;
		
		// use this to keep track of filename lengths. ideally they should be consistent, with the only variation being the number at the end
		// i.e. screen0, screen1, screen2, ... -> this is the scheme I follow when generating screenshots
		int filenameLength = -1;
		while((dir_entry = readdir(pd)) != NULL){
			//images.push_back(std::string(dir_entry->d_name));
			std::string filename = dir_entry->d_name;
			if(filename.compare(".") != 0 && filename.compare("..") != 0){
				// look for bmp images only 
				int len = filename.size();
				if(len < 3){
					continue;
				}else{
					std::string last3chars = filename.substr(len - 3);
					if(last3chars.compare("bmp") == 0){		
						if(filenameLength == -1){
							filenameLength = len;
							// put first file in images1 
							images1.push_back(theDir + "\\" + filename);
						}else if(len > filenameLength){
							images2.push_back(theDir + "\\" + filename);
						}else{
							images1.push_back(theDir + "\\" + filename);
						}
					}
				}
			}
		}
		closedir(pd);
		
		// process images now 
		if(images1.size() == 0 && images2.size() == 0){
			// no bmp images found 
			PostMessage(mainWindow, ID_NO_BMPS_FOUND, 0, 0);
			delete args;
			return;
		}else{
			
			std::vector<std::string> allImages; // add images from images1 and images2 to here
			int images1len = images1.size();
			int images2len = images2.size();
			for(int i = 0; i < images1len; i++){
				allImages.push_back(images1[i]);
			}
			
			for(int j = 0; j < images2len; j++){
				allImages.push_back(images2[j]);
			}
			
			// now create gif from images 
			PostMessage(mainWindow, ID_ASSEMBLING_GIF, 0, 0);
			
			switch(currFilterIndex){
				case 0: assembleGif(nFrames, tDelay, allImages, getBMPImageData, theText);
						break;
				case 1: assembleGif(nFrames, tDelay, allImages, getBMPImageDataInverted, theText);
						break;
				case 2: assembleGif(nFrames, tDelay, allImages, getBMPImageDataSaturated, theText);
						break;
				case 3: assembleGif(nFrames, tDelay, allImages, getBMPImageDataWeird, theText);
						break;
				case 4: assembleGif(nFrames, tDelay, allImages, getBMPImageDataGrayscale, theText);
						break;		
				case 5: assembleGif(nFrames, tDelay, allImages, getBMPImageDataEdgeDetection, theText);
						break;	
				case 6: assembleGif(nFrames, tDelay, allImages, getBMPImageDataMosaic, theText);
						break;
				case 7: assembleGif(nFrames, tDelay, allImages, getBMPImageDataOutline, theText);
						break;
			}
			
			PostMessage(mainWindow, ID_FINISHED, 0, 0);
		}
	}else{
		// this applies to gif-generation from a specified part of the screen (not using already made images)
		switch(currFilterIndex){
			case 0: getSnapshots(nFrames, tDelay, x1, y1, (x2-x1), (y2-y1), getBMPImageData);
					break;
			case 1: getSnapshots(nFrames, tDelay, x1, y1, (x2-x1), (y2-y1), getBMPImageDataInverted);
					break;
			case 2: getSnapshots(nFrames, tDelay, x1, y1, (x2-x1), (y2-y1), getBMPImageDataSaturated);
					break;
			case 3: getSnapshots(nFrames, tDelay, x1, y1, (x2-x1), (y2-y1), getBMPImageDataWeird);
					break;
			case 4: getSnapshots(nFrames, tDelay, x1, y1, (x2-x1), (y2-y1), getBMPImageDataGrayscale);
					break;		
			case 5: getSnapshots(nFrames, tDelay, x1, y1, (x2-x1), (y2-y1), getBMPImageDataEdgeDetection);
					break;	
			case 6: getSnapshots(nFrames, tDelay, x1, y1, (x2-x1), (y2-y1), getBMPImageDataMosaic);
					break;
			case 7: getSnapshots(nFrames, tDelay, x1, y1, (x2-x1), (y2-y1), getBMPImageDataOutline);
					break;
		}
		PostMessage(mainWindow, ID_FINISHED, 0, 0);
	}
	
	// free memory associated with the arguments struct 
	delete args;
}


/*

	the function, which does the gif generation, that is executed by a new thread.
	pass it a pointer to a struct that contains parameters for creating the gif

*/
DWORD WINAPI processGifThread(LPVOID lpParam){
	makeGif((windowInfo*)lpParam);
	return 0;
}


/* 

    the window procedure for the GUI
    
*/
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
    
    switch(msg){
		
		case WM_CREATE:
		{
			// create the menu tabs (main page, parameter page tabs)
			HMENU hMenu;
			hMenu = CreateMenu();
			AppendMenu(hMenu, MF_STRING, ID_MAIN_PAGE, "Main");
			AppendMenu(hMenu, MF_STRING, ID_SET_PARAMETERS_PAGE, "Options");
			SetMenu(hwnd, hMenu);
		}
		break;
		
        case WM_COMMAND:
		{
            /* LOWORD takes the lower 16 bits of wParam => the element clicked on */
            switch(LOWORD(wParam)){
				
				case ID_MAIN_PAGE:
				{
					// go back to main page 
					ShowWindow(parameterPage, SW_HIDE);
					ShowWindow(mainPage, SW_SHOW);
					UpdateWindow(hwnd);
				}
				break;
				
				case ID_SET_PARAMETERS_PAGE:
				{
					// switch to parameters page 
					ShowWindow(mainPage, SW_HIDE);
					ShowWindow(parameterPage, SW_SHOW);
					UpdateWindow(hwnd);
				}
				break;
	
				case WM_CLOSE:
				{
					DeleteObject(hFont);
					DestroyWindow(hwnd);
				}
				break;
				
				case WM_DESTROY:
				{
					DeleteObject(hFont);
					PostQuitMessage(0);
				}
				break;
				
			}		
		}
		break; 
		
		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);

	}	
    return 0;
}

/*

	procedure for the main page

*/
LRESULT CALLBACK WndProcMainPage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
    
    switch(msg){
		
        case WM_COMMAND:
		{
            /* LOWORD takes the lower 16 bits of wParam => the element clicked on */
            switch(LOWORD(wParam)){
				
                case ID_SELECT_SCREENAREA_BUTTON:
                {
                    // make a new window to select the area 
                    int x = GetSystemMetrics(SM_CXSCREEN);
                    int y = GetSystemMetrics(SM_CYSCREEN);
                    selectionWindow = CreateWindowEx(
                                        WS_EX_LAYERED,
                                        g_szClassName4,
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
					
					// get the parameters 
					HWND frames = GetDlgItem(hwnd, ID_NUMFRAMES_TEXTBOX);
					HWND delay = GetDlgItem(hwnd, ID_DELAY_TEXTBOX);
					
					TCHAR numFrames[3];
					TCHAR timeDelay[5];
					
					GetWindowText(frames, numFrames, 3);
					GetWindowText(delay, timeDelay, 5);
					
					int nFrames = atoi(numFrames);
					int tDelay = atoi(timeDelay);
					
					// validate values!! 
					if(nFrames < 1){
						nFrames = 1;
					}else if(nFrames > 50){
						nFrames = 50;
					}
					
					if(tDelay < 10){
						tDelay = 10;
					}else if(tDelay > 1000){
						tDelay = 1000;
					}
					
					// also, get the currently selected filter
					HWND filterbox = GetDlgItem(hwnd, ID_FILTERS_COMBOBOX);
					int currFilterIndex = SendMessage(filterbox, CB_GETCURSEL, 0, 0);
					
					// also check if user has specified a directory to create the gif from 
					HWND directory = GetDlgItem(hwnd, ID_CHOOSE_DIR);
					int textLen = GetWindowTextLength(directory);
					TCHAR dir[textLen + 1]; // +1 for null term 
					GetWindowText(directory, dir, textLen + 1);
					std::string theDir = std::string(dir);
					
					// check if user wants to memefy! if there's text entered in the textbox for ID_MEMEFY_MSG,
					// pass it to assembleGif 
					HWND memefyText = GetDlgItem(hwnd, ID_MEMEFY_MSG);
					TCHAR mtext[textLen + 1];	
					GetWindowText(memefyText, mtext, textLen + 1);
					std::string theText = std::string(mtext);
					
					// set up arguments struct to pass to the thread that will generate the gif 
					// need to allocate on to heap otherwise this data will go out of scope and be unreachable from thread 
					windowInfo* gifParams = new windowInfo();
					gifParams->numFrames = nFrames;
					gifParams->timeDelay = tDelay;
					gifParams->selectedFilter = currFilterIndex;
					gifParams->directory = theDir;
					gifParams->memeText = theText;
					gifParams->mainWindow = hwnd;
					
					// start process in another thread
					CreateThread(NULL, 0, processGifThread, gifParams, 0, 0);
					
                }
                break;
            }
		}
		break;
		
		case ID_IN_PROGRESS:
		{
			SetDlgItemText(hwnd, ID_PROGRESS_MSG, "processing...");
		}
		break;
		
		case ID_FINISHED:
		{
			SetDlgItemText(hwnd, ID_PROGRESS_MSG, "processing successful!");
		}
		break;
		
		case ID_ASSEMBLING_GIF:
		{
			SetDlgItemText(hwnd, ID_PROGRESS_MSG, "assembling gif...");
		}
		break;
		
		case ID_UNABLE_TO_OPEN:
		{
			SetDlgItemText(hwnd, ID_PROGRESS_MSG, "unable to open directory");
		}
		break;
		
		case ID_NO_BMPS_FOUND:
		{
			SetDlgItemText(hwnd, ID_PROGRESS_MSG, "no bmp images were found");
		}
		break;
		
		case ID_COLLECTING_IMAGES:
		{
			SetDlgItemText(hwnd, ID_PROGRESS_MSG, "collecting images...");
		}
		break;
			
        case WM_CLOSE:
        {
			DeleteObject(hFont);
            DestroyWindow(hwnd);
        }
        break;
		
        case WM_DESTROY:
        {
            DeleteObject(hFont);
            PostQuitMessage(0);
        }
        break;
		
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

/*

	window procedure for the parameters page 

*/
LRESULT CALLBACK WndProcParameterPage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){

    switch(msg){
        case WM_LBUTTONDOWN:
        {
			case WM_COMMAND:
			{
				/* LOWORD takes the lower 16 bits of wParam => the element clicked on */
				switch(LOWORD(wParam)){
				}
			}
			break;
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

/*
    
    window procedure for the selection window 
    
*/

// reset helper function 
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
            
            // need button down to initiate mousemove step 
            bDrag = true;

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
                SetDCBrushColor(hdc, currSettings.selectionWindowColor); // set color to pinkish-red color 
                
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
                    if(y1 > 0){
                        y1 = y1 + GetSystemMetrics(SM_CYCAPTION);
                    }
					
					// don't forget about y2, the endpoint for the y-coord - needs to be adjusted too
					y2 = y2 + GetSystemMetrics(SM_CYCAPTION);
                    
                    reset(&ptCurr, &ptNew, &bDrag, &bDraw);
                    DestroyWindow(hwnd);
                    ReleaseCapture();
                    return 0;
                    
                }else if(response == IDNO){
                    // need to clear screen!!
                    HDC hdc = GetDC(hwnd);
                    SelectObject(hdc,GetStockObject(DC_BRUSH));
                    SetDCBrushColor(hdc, COLOR); 
                    SetROP2(hdc, R2_NOTXORPEN);
                    // erase old rectangle 
                    Rectangle(hdc, ptCurr.x, ptCurr.y, ptNew.x, ptNew.y);
                    ReleaseDC(hwnd, hdc);
                    // reset stuff
                    reset(&ptCurr, &ptNew, &bDrag, &bDraw);
                    
                }else{
                    // failure to show message box    
                    std::cout << "error with message box!!" << std::endl;
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


/*
	this function creates the UI for the main page/screen
	it takes a window handler (HWND) as an argument that the UI will be drawn on 
	and an HINSTANCE
*/
void createMainScreen(HWND hwnd, HINSTANCE hInstance){

    /* make a title label */
    HWND title;
    title = CreateWindow(
        TEXT("STATIC"),
        TEXT("nch 2018 | https://github.com/syncopika "),
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        10, 10,
        280, 40,
        hwnd, /* parent window */
        (HMENU)ID_TITLE_LABEL,
        hInstance,
        NULL
    );
    // send the gui the font to use 
    SendMessage(title, WM_SETFONT, (WPARAM)hFont, true);
    
    /* make text box for # FRAMES TO COLLECT (HWND textInputPriorityLabel) */
    HWND framesLabel = CreateWindow(
        TEXT("STATIC"),
        TEXT("# frames to get: "),
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        10, 60,
        110, 20,
        hwnd, /* parent window */
        (HMENU)ID_NUMFRAMES_LABEL,
        hInstance,
        NULL
    );
    SendMessage(framesLabel, WM_SETFONT, (WPARAM)hFont, true);
    
    /* make text box  ADD NUMBER OF FRAMES TO COLLECT  (HWND textInput)*/
    HWND editFrames = CreateWindow(
        TEXT("edit"),
        TEXT(""),
        WS_VISIBLE | WS_CHILD | WS_BORDER,
        110, 60,  /* x, y coords */
        80, 20, /* width, height */
        hwnd,
        (HMENU)ID_NUMFRAMES_TEXTBOX,
        hInstance,
        NULL
    );
    SendMessage(editFrames, WM_SETFONT, (WPARAM)hFont, true);
    /* prepopulate text input */
    SetDlgItemText(hwnd, ID_NUMFRAMES_TEXTBOX, "10");
    
    HWND frameLimit = CreateWindow(
        TEXT("STATIC"),
        TEXT("1 <= frames <= 50"),
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        210, 60,
        130, 20,
        hwnd, /* parent window */
        NULL,
        hInstance,
        NULL
    );
    SendMessage(frameLimit, WM_SETFONT, (WPARAM)hFont, true);
    
    /* make text box LABEL FOR DELAY (HWND textInputPriorityLabel) */
    HWND delayLabel = CreateWindow(
        TEXT("STATIC"),
        TEXT("# delay(ms): "),
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        10, 90,
        100, 20,
        hwnd, /* parent window */
        (HMENU)ID_DELAY_LABEL,
        hInstance,
        NULL
    );
    SendMessage(delayLabel, WM_SETFONT, (WPARAM)hFont, true);
    
    HWND editDelay = CreateWindow(
        TEXT("edit"),
        TEXT(""),
        WS_VISIBLE | WS_CHILD | WS_BORDER,
        110, 90,  /* x, y coords */
        80, 20, /* width, height */
        hwnd,
        (HMENU)ID_DELAY_TEXTBOX,
        hInstance,
        NULL
    );
    SendMessage(editDelay, WM_SETFONT, (WPARAM)hFont, true);
    /* prepopulate text input */
    SetDlgItemText(hwnd, ID_DELAY_TEXTBOX, "120");
    
    HWND delayLimit = CreateWindow(
        TEXT("STATIC"),
        TEXT("10 <= ms <= 1000"),
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        210, 90,
        130, 20,
        hwnd, /* parent window */
        NULL,
        hInstance,
        NULL
    );
    SendMessage(delayLimit, WM_SETFONT, (WPARAM)hFont, true);
	
	/* combobox to select image filter LABEL */
	HWND filterComboBoxLabel = CreateWindow(
		TEXT("STATIC"),
        TEXT("filter options: "),
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        10, 125,
        80, 20,
        hwnd, /* parent window */
        (HMENU)ID_FILTERS_LABEL,
        hInstance,
        NULL
	);
	SendMessage(filterComboBoxLabel, WM_SETFONT, (WPARAM)hFont, true);
	
	/* combobox to select image filter */
	HWND filterComboBox = CreateWindow(
		WC_COMBOBOX,
		TEXT(""),
		CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE, 
		110, 120, 
		80, 20,
		hwnd,
		(HMENU)ID_FILTERS_COMBOBOX,
		hInstance,
		NULL
	);
	SendMessage(filterComboBox, WM_SETFONT, (WPARAM)hFont, true);
	
	// filter options 
	SendMessage(filterComboBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"none");
	SendMessage(filterComboBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"inverted");
	SendMessage(filterComboBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"saturated");
	SendMessage(filterComboBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"weird");
	SendMessage(filterComboBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"grayscale");
	SendMessage(filterComboBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"edge_detect");
	SendMessage(filterComboBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"mosaic");
	SendMessage(filterComboBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"outline");
	
    // initially the filter is set to "none"
	SendMessage(filterComboBox, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
	
	
	/* let user select a directory of images to create gif from */
    HWND createGifFromDir = CreateWindow(
        TEXT("STATIC"),
        TEXT("specify full directory path of images to generate gif from: "),
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        10, 170,
        340, 20,
        hwnd, /* parent window */
        NULL,
        hInstance,
        NULL
    );
    SendMessage(createGifFromDir, WM_SETFONT, (WPARAM)hFont, true);
	
	HWND editImageDirectory = CreateWindow(
		TEXT("edit"),
		TEXT(""),
		WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
		10, 190,  /* x, y coords */
		280, 20, /* width, height */
		hwnd,
		(HMENU)ID_CHOOSE_DIR,
		hInstance,
		NULL
    );
    SendMessage(editImageDirectory, WM_SETFONT, (WPARAM)hFont, true);
	
	/* 
		let user memefy their gif. for now, it'll be a bit limited in that the text 
		will automatically be placed towards the bottom of the gif. 
		it will however, be centered (and so some calculations are needed)
		the amount of text will vary depending on gif size as well
		font will also be Impact and size will be determined by program 
	*/
	HWND memefyOption = CreateWindow(
        TEXT("STATIC"),
        TEXT("specify a message to show at bottom of gif: "),
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        10, 220,
        340, 20,
        hwnd,
        NULL,
        hInstance,
        NULL
    );
    SendMessage(memefyOption, WM_SETFONT, (WPARAM)hFont, true);
	
	HWND memefyMsg = CreateWindow(
		TEXT("edit"),
		TEXT(""),
		WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
		10, 240,
		280, 20, 
		hwnd,
		(HMENU)ID_MEMEFY_MSG,
		hInstance,
		NULL
	);
	SendMessage(memefyMsg, WM_SETFONT, (WPARAM)hFont, true);
	
	
    /* button to select area of screen  */
    HWND selectAreaButton = CreateWindow(
        TEXT("button"),
        TEXT("select area"),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        150, 280,
        80, 20, 
        hwnd,
        (HMENU)ID_SELECT_SCREENAREA_BUTTON,
        hInstance,
        NULL
    );
    SendMessage(selectAreaButton, WM_SETFONT, (WPARAM)hFont, true);
    
    /* button to start the screen capture */
    HWND startButton = CreateWindow(
        TEXT("button"),
        TEXT("start"),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        150, 310,
        80, 20, 
        hwnd,
        (HMENU)ID_START_BUTTON,
        hInstance,
        NULL
    );
    SendMessage(startButton, WM_SETFONT, (WPARAM)hFont, true);
    
    /* text indicator/message for gif processing progress */
    HWND progressBar = CreateWindow(
        TEXT("STATIC"),
        TEXT(""),
        WS_VISIBLE | WS_CHILD | WS_BORDER,
        80, 350,  /* x, y coords */
        220, 20, /* width, height */
        hwnd,
        (HMENU)ID_PROGRESS_MSG,
        hInstance,
        NULL
    );
    SendMessage(progressBar, WM_SETFONT, (WPARAM)hFont, true);
	
}

/*
	this function sets up[ the parameters page, where the user can change certain parameters
	like for image filters, or to change the color of the selection screen 
*/
void createParameterPage(HWND hwnd, HINSTANCE hInstance){
	
	HWND setColorLabel = CreateWindow(
	    TEXT("STATIC"),
        TEXT("choose selection screen color: "),
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        10, 10,
        180, 20,
        hwnd,
        NULL,
        hInstance,
        NULL
	);
	SendMessage(setColorLabel, WM_SETFONT, (WPARAM)hFont, true);
	
	
}

/*************

	MAIN METHOD FOR GUI
    
**************/

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){

    /* console attached for debugging */
    //AllocConsole();
    //freopen( "CON", "w", stdout );
    
    // for improving the gui appearance (buttons, that is. the font needs to be changed separately) 
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);
    
    WNDCLASSEX wc; // this is the main GUI window  
    MSG Msg;
	
	/* make a main window */ 
	wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
    wc.lpszMenuName = NULL; 
    wc.lpszClassName = g_szClassName;
    wc.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON));
	wc.hIconSm = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON), IMAGE_ICON, 16, 16, 0);
	
    // register the main screen, which is a child of the main window 
	WNDCLASSEX wc1;
    wc1.cbSize = sizeof(WNDCLASSEX);
    wc1.style = 0;
    wc1.lpfnWndProc = WndProcMainPage;
    wc1.cbClsExtra = 0;
    wc1.cbWndExtra = 0;
    wc1.hInstance = hInstance;
    wc1.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc1.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
    wc1.lpszMenuName = NULL; 
    wc1.lpszClassName = g_szClassName2;
	wc1.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc1.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	
	// register the second window class - this is the parameters page  
	WNDCLASSEX wc2;
    wc2.cbSize = sizeof(WNDCLASSEX);
    wc2.style = 0;
    wc2.lpfnWndProc = WndProcParameterPage;
    wc2.cbClsExtra = 0;
    wc2.cbWndExtra = 0;
    wc2.hInstance = hInstance;
    wc2.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc2.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
    wc2.lpszMenuName = NULL;
    wc2.lpszClassName = g_szClassName3;
	wc2.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc2.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	
    // register the third window class - this is the selection window 
	WNDCLASSEX wc3;
    wc3.cbSize = sizeof(WNDCLASSEX);
    wc3.style = 0;
    wc3.lpfnWndProc = WndProcSelection;
    wc3.cbClsExtra = 0;
    wc3.cbWndExtra = 0;
    wc3.hInstance = hInstance;
    wc3.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc3.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc3.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc3.lpszMenuName = NULL;
    wc3.lpszClassName = g_szClassName4;
    wc3.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    
    
    if(!RegisterClassEx(&wc)){
        std::cout << "error code: " << GetLastError() << std::endl;
        MessageBox(NULL, "window registration failed for the main GUI!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
	
	if(!RegisterClassEx(&wc1)){
        std::cout << "error code: " << GetLastError() << std::endl;
        MessageBox(NULL, "window registration failed for main page!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
	
	if(!RegisterClassEx(&wc2)){
        std::cout << "error code: " << GetLastError() << std::endl;
        MessageBox(NULL, "window registration failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
	
    if(!RegisterClassEx(&wc3)){
        std::cout << "error code: " << GetLastError() << std::endl;
        MessageBox(NULL, "window registration failed for selection screen!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
	
	// create the window
    hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        g_szClassName,
        "gifCatch",
        WS_TILEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 450,
        NULL, NULL, hInstance, NULL
    );
	
	// create the main screen 
	mainPage = CreateWindowEx(
        WS_EX_WINDOWEDGE, // border with raised edge 
        g_szClassName2,
        NULL,
        WS_CHILD,
        0, 0, 400, 450,
        hwnd, // parent window 
		NULL, 
		hInstance, NULL
    );
	
	// create the parameters page 
	parameterPage = CreateWindowEx(
        WS_EX_WINDOWEDGE,
        g_szClassName3,
        NULL,
        WS_CHILD,
        0, 0, 400, 450,
        hwnd, // parent window
		NULL, 
		hInstance, NULL
    );
	
    if(hwnd == NULL){
		//std::cout << "error code: " << GetLastError() << std::endl;
        MessageBox(NULL, "window creation failed for the main GUI!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
	
	if(mainPage == NULL){
        MessageBox(NULL, "window creation failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
	
	if(parameterPage == NULL){
        MessageBox(NULL, "window creation failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    
	// make and show main GUI window
    ShowWindow(hwnd, nCmdShow); // show the GUI 
    UpdateWindow(hwnd);
	
	// show the main screen on the GUI 
	createMainScreen(mainPage, hInstance); // create the main screen
	ShowWindow(mainPage, SW_SHOW);
	UpdateWindow(hwnd);
	
	// create the parameter page (but don't show it yet)
	createParameterPage(parameterPage, hInstance);
    
    /* message loop */
    while(GetMessage(&Msg, NULL, 0, 0) > 0){
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
}

