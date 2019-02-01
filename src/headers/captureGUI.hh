#include <string>
#include <windows.h>

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
	float saturationValue;
};

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
