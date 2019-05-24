the executable is available here! :)    
     
a bit of an explanation for the files:    
- **Application.manifest**: needed to update the GUI appearance depending on what version of Windows 
- **capture.cpp / capture.hh**: the code needed to capture the screen and getting/filtering BMP image data. contains a struct definition that holds various parameters     
- **bmpHelper.cpp / bmpHelper.hh**: contains filter functions, as well as some auxiliary functions to do stuff with bmps like get the width and height.
- **gif.h**: the code for creating a gif
- **captureGUI.cpp / captureGUI.hh**: the code for the GUI (i.e. the main window, the options window, the selection window)    
- **resources.h**: defines GUI components with integer IDs    
- **resources.rc**: connects the icon and application manifest
- **makefile**: takes care of the compilation process 