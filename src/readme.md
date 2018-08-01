as you can see, the executable is available here ^^.    
     
a bit of an explanation for the files:    
- **Application.manifest**: needed to update the GUI appearance depending on what version of Windows 
- **capture.cpp / capture.hh**: the code needed to capture the screen     
- **bmpHelper.cpp / bmpHelper.hh**: contains bmp-data handling functions, filter functions 
- **gif.h**: the code for creating a gif
- **captureGUI.cpp**: the code for the GUI    
- **resources.h**: defines GUI components with integer IDs    
- **resources.rc**: connects the icon and application manifest
- **makefile**: takes care of the compilation process 