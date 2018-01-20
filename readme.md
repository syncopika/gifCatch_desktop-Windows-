## capture screenshots on your Windows desktop and create a gif!    
    
the executable in the gui folder lets you select what part of the screen to screenshot.    
this is a work-in-progress.     
    
things to do:    
- clean up gui (maybe eventually move to Qt)        
- can I get better quality images? different formats?    
- allow gif naming / output directory naming?    
- different colors for selection screen?    
    
thanks to Philip Goh (https://github.com/cotidian/Win32GrabScreen) for the screen capture code, and Charlie Tangora (https://github.com/ginsweater/gif-h/blob/master/gif.h) for the gif-making code.        
    
current screenshots:    
### adjust the settings to your liking (i.e. number of frames, the time delay between frames) and click on the 'select area' button to choose an area on the screen to screenshot. by default it should capture the whole screen.    
![start](screenshots/start.png "the gui")    
     
### screen selection (sorry, only red is available right now)    
![selecting an area to screenshot](screenshots/selection.png "selecting an area to screenshot")    
    
### after clicking the start button, a "processing..." message should appear    
![processing gif](screenshots/processing.png "processing the gif")    
    
### then if everything worked out, a successful processing message should show 
![done](screenshots/process_successful.png "finished processing")    
    
### you can find the finished gif (called "test" - can't choose name yet, sorry!) in the same folder as the program, as well as its frames in the "temp" directory.    
![find your gif](screenshots/done.png "find the gif and its frames")    
    
### here's my result:    
![gif result](screenshots/test.gif "Joey Alexander performing 'Pannonica'")    
    
feel free to check out my Chrome extension that does pretty much the same thing (but is not as neat): https://github.com/syncopika/gifCatch_extension    
