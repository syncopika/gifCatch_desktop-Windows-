#ifndef CAPTURE_GUI_H
#define CAPTURE_GUI_H

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

// for mapping color filters to their dropdown box index 
#include <map>

// give some identifiers for the GUI components 
#include "resources.h"

// define a default color for screen selection (light red)
#define COLOR RGB(255,130,140)


void reset(POINT *p1, POINT *p2, bool *drag, bool *draw);
void makeGif(windowInfo* args);
COLORREF getSelectedColor(HWND selectBox);

DWORD WINAPI processGifThread(LPVOID lpParam);

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProcMainPage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProcParameterPage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProcSelection(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

void createMainScreen(HWND hwnd, HINSTANCE hInstance);
void createParameterPage(HWND hwnd, HINSTANCE hInstance);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

#endif
