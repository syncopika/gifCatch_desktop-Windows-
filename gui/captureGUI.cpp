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


// use Tahoma font for the text 
// this HFONT object needs to be deleted (via DeleteObject) when program ends 
HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, 
      OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
      DEFAULT_PITCH | FF_DONTCARE, TEXT("Tahoma"));


/******************

DO Win32 GUI STUFF HERE 

*****************/

/* 

    the window procedure for the GUI
    
*/
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
    
    switch(msg){

        case WM_COMMAND:
            /* LOWORD takes the lower 16 bits of wParam => the element clicked on */
            switch(LOWORD(wParam)){
                case ID_SELECT_SCREENAREA_BUTTON:
                {
                    // make a new window to select the area 
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
                    
                    // indicate process started 
                    SetDlgItemText(hwnd, ID_PROGRESS_MSG, "processing...");
                    
                    // collect snapshots given the current dimensions 
                    // if no window selection occurred, screenshot whole screen
					// also, get the currently selected filter and apply it 
					HWND filterbox = GetDlgItem(hwnd, ID_FILTERS_COMBOBOX);
					int currFilterIndex = SendMessage(filterbox, CB_GETCURSEL, 0, 0);
					
					// also check if user has specified a directory to create the gif from 
					// if so, don't get any screenshots 
					HWND directory = GetDlgItem(hwnd, ID_CHOOSE_DIR);
					TCHAR dir[38];	// max is 38 chars!!!
					GetWindowText(directory, dir, 38);
					std::string theDir = std::string(dir);
					//std::cout << "directory path: " + theDir << std::endl;
					//std::cout << "is directory path empty string?: " << theDir.compare("") << std::endl;
					
					if(theDir.compare("") != 0){
						struct dirent *dir_entry;
						DIR *pd = 0;
						
						pd = opendir(theDir.c_str());
						if(pd == NULL){
							std::cout << "unable to open directory..." << std::endl;
							SetDlgItemText(hwnd, ID_PROGRESS_MSG, "unable to open directory");
							break;
						}
						
						// check directory contents 
						// indicate to user that images are being looked through 
						SetDlgItemText(hwnd, ID_PROGRESS_MSG, "collecting images from directory...");
						
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
							SetDlgItemText(hwnd, ID_PROGRESS_MSG, "no bmp images were found");
							break;
						}else{
							
							std::vector<std::string> allImages; // add images from images1 and images2 to here
							int i, j;
							int images1len = images1.size();
							int images2len = images2.size();
							for(i = 0; i < images1len; i++){
								allImages.push_back(images1[i]);
								//std::cout << images1[i] << std::endl;
							}
							
							for(j = 0; j < images2len; j++){
								allImages.push_back(images2[j]);
								//std::cout << images2[j] << std::endl;
							}
							
							// now create gif from images 
							SetDlgItemText(hwnd, ID_PROGRESS_MSG, "assembling gif...");
							assembleGif(nFrames, tDelay, allImages, getBMPImageData); // implement apply filters later 
						}
						
					}else{
					
						// capture screenshots 				
						if(currFilterIndex == 0){
							// no filter
							getSnapshots(nFrames, tDelay, x1, y1, (x2-x1), (y2-y1), getBMPImageData);
						}else if(currFilterIndex == 1){
							// color inversion filter
							getSnapshots(nFrames, tDelay, x1, y1, (x2-x1), (y2-y1), getBMPImageDataInverted);
						}else{
							// saturation filter
							getSnapshots(nFrames, tDelay, x1, y1, (x2-x1), (y2-y1), getBMPImageDataSaturated);
						}
					}
						
					// if at this point, task is done 
					SetDlgItemText(hwnd, ID_PROGRESS_MSG, "processing successful!");
                    
                }
                break;
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
                    SetDCBrushColor(hdc, RGB(255,0,0)); 
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
    
    /* register the window class */
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
    wc.lpszMenuName = NULL; 
    wc.lpszClassName = g_szClassName;
    wc.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON));
	wc.hIconSm = (HICON)LoadImage(GetModuleHandle(NULL),  MAKEINTRESOURCE(IDI_ICON), IMAGE_ICON, 16, 16, 0);
    
    /* register the second window class */
	WNDCLASSEX wc2;
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
        std::cout << "error code: " << GetLastError() << std::endl;
        MessageBox(NULL, "window registration failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    
    if(!RegisterClassEx(&wc2)){
        std::cout << "error code: " << GetLastError() << std::endl;
        MessageBox(NULL, "window registration failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    
    /* create the window */
    hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        g_szClassName,
        "gifCatch",
        WS_TILEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 400,
        NULL, NULL, hInstance, NULL
    );
    
    if(hwnd == NULL){
        MessageBox(NULL, "window creation failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    
    
    /* make a title label */
    HWND title;
    title = CreateWindow(
        TEXT("STATIC"),
        TEXT(" gifCatch \n nch 2018 "),
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        10, 10,
        200, 40,
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
	// for now provide these 3 options: none, inverted, and saturated 
	SendMessage(filterComboBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"none");
	SendMessage(filterComboBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"inverted");
	SendMessage(filterComboBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"saturated");
    // initially the filter is set to "none"
	SendMessage(filterComboBox, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
	
	
	/* let user select a directory of images to create gif from */
    HWND createGifFromDir = CreateWindow(
        TEXT("STATIC"),
        TEXT("specify full directory path of images to generate gif from: "),
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        10, 150,
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
		WS_VISIBLE | WS_CHILD | WS_BORDER,
		10, 180,  /* x, y coords */
		280, 20, /* width, height */
		hwnd,
		(HMENU)ID_CHOOSE_DIR,
		hInstance,
		NULL
    );
    SendMessage(editImageDirectory, WM_SETFONT, (WPARAM)hFont, true);
	
	
    /* button to select area of screen  */
    HWND selectAreaButton = CreateWindow(
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
    SendMessage(selectAreaButton, WM_SETFONT, (WPARAM)hFont, true);
    
    /* button to start the screen capture */
    HWND startButton = CreateWindow(
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
    SendMessage(startButton, WM_SETFONT, (WPARAM)hFont, true);
    
    /* text indicator/message for gif processing progress */
    HWND progressBar = CreateWindow(
        TEXT("STATIC"),
        TEXT(""),
        WS_VISIBLE | WS_CHILD | WS_BORDER,
        80, 300,  /* x, y coords */
        220, 20, /* width, height */
        hwnd,
        (HMENU)ID_PROGRESS_MSG,
        hInstance,
        NULL
    );
    SendMessage(progressBar, WM_SETFONT, (WPARAM)hFont, true);
    
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    
    /* message loop */
    while(GetMessage(&Msg, NULL, 0, 0) > 0){
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
}

