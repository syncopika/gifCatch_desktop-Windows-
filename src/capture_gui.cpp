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
#include "headers/capture_gui.hh"

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
const char g_szClassName5[] = "aboutPage";

// handler variables for the windows 
HWND hwnd;	// this is the main GUI window handle (the parent window of the main and parameter pages)
HWND mainPage; // this is the main page of the GUI 
HWND parameterPage; // this is the window handle for the page where the user can set some parameters 
HWND selectionWindow; // this is the handle for the rubber-banding selection window 
HWND aboutPage; // an about page


// use Tahoma font for the text 
// this HFONT object needs to be deleted (via DeleteObject) when program ends 
HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, 
      OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
      DEFAULT_PITCH | FF_DONTCARE, TEXT("Tahoma"));
	  
	  
// filters map - order matters!
std::map<int, std::string> filterMap = {
	{0, "none"},
	{1, "inverted"},
	{2, "saturated"},
	{3, "weird"},
	{4, "grayscale"},
	{5, "edge_detect"},
	{6, "mosaic"},
	{7, "outline"},
	{8, "voronoi"},
	{9, "blur"}
};

// default settings 
windowInfo* gifParams = new windowInfo(); // from capture.hh

/***

	functions to make creating window wlements easier

***/
void createEditBox(std::string defaultText, int width, int height, int xCoord, int yCoord, HWND parent, HINSTANCE hInstance, HMENU elementId, HFONT hFont){
	HWND editBox = CreateWindow(
		TEXT("edit"),
		defaultText.c_str(),
		WS_VISIBLE | WS_CHILD | WS_BORDER, 
		xCoord, yCoord,  /* x, y coords */
		width, height, /* width, height */
		parent,
		elementId,
		hInstance,
		NULL
	);
	SendMessage(editBox, WM_SETFONT, (WPARAM)hFont, true);
}

void createLabel(std::string defaultText, int width, int height, int xCoord, int yCoord, HWND parent, HINSTANCE hInstance, HMENU elementId, HFONT hFont){
	HWND label = CreateWindow(
	    TEXT("STATIC"),
        defaultText.c_str(),
        WS_VISIBLE | WS_CHILD | SS_LEFT,
		xCoord, yCoord,  /* x, y coords */
		width, height, /* width, height */
		parent,
		elementId,
		hInstance,
        NULL
	);
	SendMessage(label, WM_SETFONT, (WPARAM)hFont, true);
}

void createCheckBox(std::string defaultText, int width, int height, int xCoord, int yCoord, HWND parent, HINSTANCE hInstance, HMENU elementId, HFONT hFont){
	HWND checkBox = CreateWindow(
		TEXT("button"),
		defaultText.c_str(),
		BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE,
		xCoord, yCoord,  /* x, y coords */
		width, height, /* width, height */
		parent,
		elementId,
		hInstance,
		NULL
	);
	SendMessage(checkBox, WM_SETFONT, (WPARAM)hFont, true);
}

/******************

	DO Win32 GUI STUFF HERE 

*****************/

void makeGif(windowInfo* args){
	
	HWND mainWindow = args->mainWindow;
	
	PostMessage(mainWindow, ID_IN_PROGRESS, 0, 0);
	
	int nFrames = args->numFrames;
	
	int tDelay = args->timeDelay;
	
	std::string theDir = args->directory;
	
	std::string theText = args->memeText;
	
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
			
			assembleGif(nFrames, tDelay, allImages, getBMPImageData, args);
			
			PostMessage(mainWindow, ID_FINISHED, 0, 0);
		}
	}else{
		// this applies to gif-generation from a specified part of the screen (not using already made images)
		getSnapshots(nFrames, tDelay, x1, y1, (x2-x1), (y2-y1), getBMPImageData, args);
		
		PostMessage(mainWindow, ID_FINISHED, 0, 0);
	}
}


/***

	this function, which does the gif generation, is executed by a new thread.
	pass it a pointer to a struct that contains parameters for creating the gif

***/
DWORD WINAPI processGifThread(LPVOID lpParam){
	makeGif((windowInfo*)lpParam);
	return 0;
}


/***

    the window procedure for the GUI (i.e. switching between main page, parameters page, closing the window)
    
***/
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
    
    switch(msg){
		
		case WM_CREATE:
		{
			// create the menu tabs (main page, parameter page tabs)
			HMENU hMenu;
			hMenu = CreateMenu();
			AppendMenu(hMenu, MF_STRING, ID_MAIN_PAGE, "Main");
			AppendMenu(hMenu, MF_STRING, ID_SET_PARAMETERS_PAGE, "Options");
			AppendMenu(hMenu, MF_STRING, ID_SET_ABOUT_PAGE, "About");
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
					ShowWindow(aboutPage, SW_HIDE);
					ShowWindow(parameterPage, SW_HIDE);
					ShowWindow(mainPage, SW_SHOW);
					UpdateWindow(hwnd);
				}
				break;
				
				case ID_SET_PARAMETERS_PAGE:
				{
					// switch to parameters page 
					ShowWindow(mainPage, SW_HIDE);
					ShowWindow(aboutPage, SW_HIDE);
					ShowWindow(parameterPage, SW_SHOW);
					UpdateWindow(hwnd);
				}
				break;
				
				case ID_SET_ABOUT_PAGE:
				{
					ShowWindow(mainPage, SW_HIDE);
					ShowWindow(parameterPage, SW_HIDE);
					ShowWindow(aboutPage, SW_SHOW);
					UpdateWindow(hwnd);
				}
				break;
				
				case WM_CLOSE:
				{
					DeleteObject(hFont);
					delete gifParams;
					DestroyWindow(hwnd);
				}
				break;
				
				case WM_DESTROY:
				{
					DeleteObject(hFont);
					delete gifParams;
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

/***

	procedure for the main page (i.e. select area, start)

***/
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
					
					// because numFrames and timeDelay are initialized as arrays, we can use sizeof to get the number of bytes they occupy
					GetWindowText(frames, numFrames, sizeof(numFrames));
					GetWindowText(delay, timeDelay, sizeof(timeDelay));
					
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
		
		case ID_PROCESS_FRAME:
		{
			// for this particular message we want to know which frame is being processed, 
			// so we can use WPARAM as an int 
			int currFrame = (int)wParam;
			//std::cout << "curr frame: " << currFrame << std::endl;
			std::string msg = "processing frame: " + int_to_string(currFrame);
			SetDlgItemText(hwnd, ID_PROGRESS_MSG, msg.c_str());
		}
		break;
			
        case WM_CLOSE:
        {
			DeleteObject(hFont);
			delete gifParams;
            DestroyWindow(hwnd);
        }
        break;
		
        case WM_DESTROY:
        {
            DeleteObject(hFont);
			delete gifParams;
            PostQuitMessage(0);
        }
        break;
		
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

/***

	window procedure for the parameters page 

***/

// return the selected color for the selection screen 
COLORREF getSelectedColor(HWND selectBox){
	int currColorIndex = SendMessage(selectBox, CB_GETCURSEL, 0, 0);
	//std::cout << "currColorIndex: " << currColorIndex << std::endl;
	if(currColorIndex == 1){
		return (COLORREF)RGB(140,180,255);
	}else if(currColorIndex == 2){
		return (COLORREF)RGB(140,255,180);
	}
	
	return COLOR;
}

LRESULT CALLBACK WndProcParameterPage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){

    switch(msg){
        case WM_LBUTTONDOWN:
        {
			case WM_COMMAND:
			{
				/* LOWORD takes the lower 16 bits of wParam => the element clicked on */
				switch(LOWORD(wParam)){
					// there should be a save button option
					case ID_SAVE_PARAMETERS:
					{
						// get the selected color for the screen! 
						HWND colorSelect = GetDlgItem(hwnd, ID_SELECTION_COLOR);
						gifParams->selectionWindowColor = getSelectedColor(colorSelect);
						
						// get the saturation value 
						HWND saturation = GetDlgItem(hwnd, ID_SET_SATURATION);
						TCHAR saturationValue[5];
						GetWindowText(saturation, saturationValue, sizeof(saturationValue));
						
						// need to validate!!
						gifParams->saturationValue = atof(saturationValue);
						
						// get the mosaic chunk size value
						HWND mosaic = GetDlgItem(hwnd, ID_SET_MOSAIC);
						TCHAR mosaicValue[5];
						GetWindowText(mosaic, mosaicValue, sizeof(mosaicValue));
						gifParams->mosaicChunkSize = atoi(mosaicValue);
						
						// get the outline size value 
						HWND outline = GetDlgItem(hwnd, ID_SET_OUTLINE);
						TCHAR outlineValue[5];
						GetWindowText(outline, outlineValue, sizeof(outlineValue));
						gifParams->outlineColorDiffLimit = atoi(outlineValue);
						
						// get the Voronoi neighbor constant value
						HWND voronoi = GetDlgItem(hwnd, ID_SET_VORONOI);
						TCHAR voronoiValue[5];
						GetWindowText(voronoi, voronoiValue, sizeof(voronoiValue));
						gifParams->voronoiNeighborConstant = atoi(voronoiValue);
						
						// get the blur factor value
						HWND blur = GetDlgItem(hwnd, ID_SET_BLUR);
						TCHAR blurValue[5];
						GetWindowText(blur, blurValue, sizeof(blurValue));
						gifParams->blurFactor = atoi(blurValue);
						
						// get the value of 'show cursor' checkbox 
						HWND getCursorBox = GetDlgItem(hwnd, ID_GET_CURSOR);
						int getCursorVal = SendMessage(getCursorBox, BM_GETCHECK, 0, 0);
						if(getCursorVal == BST_CHECKED){
							gifParams->getCursor = true;
						}else{
							gifParams->getCursor = false;
						}
					}
				}
			}
			break;
		}
		break;
		
        case WM_CLOSE:
        {
            DestroyWindow(hwnd);
			delete gifParams;
            return 0;
        }
        break;
		
        case WM_DESTROY:
        {
            DestroyWindow(hwnd);
			delete gifParams;
            return 0;
        }
        break;
		
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;

}

/***
    
    window procedure for the selection window 
    
***/

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
                SetDCBrushColor(hdc, gifParams->selectionWindowColor);
                
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
					// note that this can be a bit tricky because normally I'd expect to draw the selection rectangle
					// from top left to bottom right. however, you should allow for rectangles to be drawn starting from 
					// wherever the user wants. if you just assume one way (the expected way), you can get negative widths and heights!
					// therefore, for x1 and x2, x1 should be the min of the 2, and x2 the max. likewise for y1 and y2.
                    x1 = std::min(ptCurr.x, ptNew.x);
                    y1 = std::min(ptCurr.y, ptNew.y);
                    x2 = std::max(ptCurr.x, ptNew.x);
                    y2 = std::max(ptCurr.y, ptNew.y);
                    
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
                    SetDCBrushColor(hdc, gifParams->selectionWindowColor); 
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

/***
	procedure for the about page 
***/
LRESULT CALLBACK WndProcAboutPage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){

    switch(msg){
        case WM_LBUTTONDOWN:
        {
			case WM_COMMAND:
			{
				switch(LOWORD(wParam)){
					// nothing to do :)
				}
			}
			break;
		}
		break;
		
        case WM_CLOSE:
        {
            DestroyWindow(hwnd);
			delete gifParams;
            return 0;
        }
        break;
		
        case WM_DESTROY:
        {
            DestroyWindow(hwnd);
			delete gifParams;
            return 0;
        }
        break;
		
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;

}


/***
	this function creates the UI for the main page (the first screen you see)
	it takes a window handler (HWND) as an argument that the UI will be drawn on 
	and an HINSTANCE
***/
void createMainScreen(HWND hwnd, HINSTANCE hInstance){
    
    /* make text box for # FRAMES TO COLLECT (HWND textInputPriorityLabel) */
    HWND framesLabel = CreateWindow(
        TEXT("STATIC"),
        TEXT("# frames to get: "),
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        10, 20,
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
        110, 20,  /* x, y coords */
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
        210, 20,
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
        TEXT("# delay (ms): "),
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        10, 50,
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
        110, 50,  /* x, y coords */
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
        210, 50,
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
        10, 85,
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
		110, 80, 
		80, 20,
		hwnd,
		(HMENU)ID_FILTERS_COMBOBOX,
		hInstance,
		NULL
	);
	SendMessage(filterComboBox, WM_SETFONT, (WPARAM)hFont, true);
	
	// add filter options to dropdown
	std::map<int, std::string>::iterator it = filterMap.begin();
	while(it != filterMap.end()){
		SendMessage(filterComboBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)(it->second).c_str());
		it++;
	}
	
    // initially the filter is set to "none"
	SendMessage(filterComboBox, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
	
	/* let user select a directory of images to create gif from */
    HWND createGifFromDir = CreateWindow(
        TEXT("STATIC"),
        TEXT("specify full directory path of images to generate gif from: "),
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        10, 130,
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
		10, 150,  /* x, y coords */
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
        10, 190,
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
		10, 210,
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
        150, 260,
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
        150, 290,
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
        80, 330,  /* x, y coords */
        220, 20, /* width, height */
        hwnd,
        (HMENU)ID_PROGRESS_MSG,
        hInstance,
        NULL
    );
    SendMessage(progressBar, WM_SETFONT, (WPARAM)hFont, true);
	
}

/***
	this function sets up the parameters page, where the user can change certain parameters
	like for image filters, or to change the color of the selection screen 
***/
void createParameterPage(HWND hwnd, HINSTANCE hInstance){
		
	createLabel("choose selection screen color: ", 180, 20, 10, 12, hwnd, hInstance, NULL, hFont);
		
	HWND setColorBox = CreateWindow(
		WC_COMBOBOX,
		TEXT(""),
		CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE, 
		210, 10,  /* x, y coords */
		85, 20, /* width, height */
		hwnd,
		(HMENU)ID_SELECTION_COLOR,
		hInstance,
		NULL
	);
	SendMessage(setColorBox, WM_SETFONT, (WPARAM)hFont, true);
	SendMessage(setColorBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"light red");
	SendMessage(setColorBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"light blue");
	SendMessage(setColorBox, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"light green");
	SendMessage(setColorBox, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
	
	// set saturation value (float) for saturation filter
	createLabel("set saturation value: ", 200, 20, 10, 45, hwnd, hInstance, NULL, hFont);
	createEditBox("2.1", 50, 20, 210, 43, hwnd, hInstance, (HMENU)ID_SET_SATURATION, hFont);
	
	// set mosaic chunk size for the mosaic filter
	createLabel("set mosaic chunk size: ", 200, 20, 10, 85, hwnd, hInstance, NULL, hFont);
	createEditBox("10", 50, 20, 210, 83, hwnd, hInstance, (HMENU)ID_SET_MOSAIC, hFont);
	
	// set the difference limit allowed betweeen 2 pixel colors (int) for outline filter
	createLabel("set outline difference limit: ", 200, 20, 10, 125, hwnd, hInstance, NULL, hFont);
	createEditBox("10", 50, 20, 210, 123, hwnd, hInstance, (HMENU)ID_SET_OUTLINE, hFont);
	
	// set neighbor constant for Voronoi
	createLabel("set Voronoi neighbor constant: ", 180, 20, 10, 165, hwnd, hInstance, NULL, hFont);
	createEditBox("30", 50, 20, 210, 165, hwnd, hInstance, (HMENU)ID_SET_VORONOI, hFont);
	
	// set blur factor 
	createLabel("set blur factor: ", 170, 20, 10, 205, hwnd, hInstance, NULL, hFont);
	createEditBox("3", 50, 20, 210, 205, hwnd, hInstance, (HMENU)ID_SET_BLUR, hFont);

	// set whether the gif should capture the cursor or not
	createCheckBox("capture screen cursor", 180, 50, 10, 230, hwnd, hInstance, NULL, hFont);
	
	HWND saveParameters = CreateWindow(
		TEXT("button"),
        TEXT("save"),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        150, 310,
        80, 20, 
        hwnd,
        (HMENU)ID_SAVE_PARAMETERS,
        hInstance,
        NULL
	);
	SendMessage(saveParameters, WM_SETFONT, (WPARAM)hFont, true);
}

/***
	set up the about page (sets the text that's on the page)
***/
void createAboutPage(HWND hwnd, HINSTANCE hInstance){
    HWND title;
    title = CreateWindow(
        TEXT("STATIC"),
        TEXT(" \n    An application for catching and creating gifs!\n    Thanks for checking it out! :)\n\n\n    (c) nch 2019 | https://github.com/syncopika\n\n"),
        WS_VISIBLE | WS_CHILD,
        0, 0,
        400, 450,
        hwnd, /* parent window */
        NULL,
        hInstance,
        NULL
    );
    // send the gui the font to use 
    SendMessage(title, WM_SETFONT, (WPARAM)hFont, true);
}

/*************

	MAIN METHOD FOR GUI
    
**************/

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){

    /* console attached for debugging */
    //AllocConsole();
    //freopen("CON", "w", stdout);
	
	// add some default parameters to gifParams immediately
	gifParams->filters = &filterMap;
	gifParams->selectionWindowColor = COLOR;
	gifParams->saturationValue = 2.1;
	gifParams->mosaicChunkSize = 30;
	gifParams->outlineColorDiffLimit = 10;
	gifParams->voronoiNeighborConstant = 30;
	gifParams->blurFactor = 3;
	gifParams->getCursor = false;
    
    // for improving the gui appearance (buttons, that is. the font needs to be changed separately) 
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);
    
	MSG Msg;
	
	// make a main window
	WNDCLASSEX wc; // this is the main GUI window 
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
	
    // register the main screen (the contents i.e. text boxes, labels, etc.), which is a child of the main window 
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
	
	// register the second window class - this is the parameters/options page  
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
	
	// this is the about page window class 
	WNDCLASSEX wc4;
    wc4.cbSize = sizeof(WNDCLASSEX);
    wc4.style = 0;
    wc4.lpfnWndProc = WndProcAboutPage; 
    wc4.cbClsExtra = 0;
    wc4.cbWndExtra = 0;
    wc4.hInstance = hInstance;
    wc4.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc4.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc4.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc4.lpszMenuName = NULL;
    wc4.lpszClassName = g_szClassName5;
    wc4.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    
    
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
	
	if(!RegisterClassEx(&wc4)){
        std::cout << "error code: " << GetLastError() << std::endl;
        MessageBox(NULL, "window registration failed for selection screen!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
	
	// create the window
    hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        g_szClassName,
        "gifCatch",
        (WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX) & ~WS_MAXIMIZEBOX, // this combo disables maximizing and resizing the window
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
	
	aboutPage = CreateWindowEx(
        WS_EX_WINDOWEDGE,
        g_szClassName5,
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
	
	if(aboutPage == NULL){
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
	
	// create the parameter and about pages (but don't show yet)
	createParameterPage(parameterPage, hInstance);
	createAboutPage(aboutPage, hInstance);
    
    /* message loop */
    while(GetMessage(&Msg, NULL, 0, 0) > 0){
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
}

