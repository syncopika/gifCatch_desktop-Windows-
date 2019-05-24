## capture screenshots on your Windows desktop and create a gif!    
    
This application lets you select a part of the screen to screenshot, specify the number of screenshots to take (with adjustable delay between shots), and creates a gif from them.    
    
I use MinGW-64 gcc(8.1.0) to make/compile this project. Currently a work-in-progress.     
    
#### this is the current look of the application:     
![current look of the gui](screenshots/current.png "current look")    
![current look of the gui](screenshots/current_parameters_page.png "current look (parameters page)")    
 
There are adjustable settings, such as the <b>number of frames</b>, the <b>time delay between frames</b>, and some <b>color filter choices</b>.  You can also choose between red (![#FF828C](https://placehold.it/15/FF828C/000000?text=+)), blue (![#8CB4FF](https://placehold.it/15/8CB4FF/000000?text=+)), or green (![#8CFFB4](https://placehold.it/15/8CFFB4/000000?text=+)) for the selection area screen when choosing a place on the screen to capture.
The screenshots below show an earlier version that only had dark red as an option.    
    
For the filters, you can currently adjust the amount of saturation with a float value, change the mosaic filter chunk size with an int value, and change the outline filter's allowable limit for the difference between 2 pixels' color. I have not yet implemented any input validation for these parameters.    
    
Additionally, you can create a gif from a folder of .bmp images (24-bit and 32-bit!) by specifying the full path to a directory. This feature is particularly useful if you have some bmp images that you have edited and want to put them together in a gif.
If recreating a gif from pre-collected bmps, you also have the option of adding a caption! (but currently only Impact font is used, with size 32, and placement is near the bottom of the gif, centered)    
    
things to do:    
- can I get better quality images? different formats (right now it just outputs bmps and uses bmps for gif creation)?    
- allow gif naming / output directory naming?      
- more filters!    
- finish parameters/options page    
- remove flickering when dragging selection area screen?    

      
- - -    
**screenshots (some of these show use of an older version of the application, but the functionality is the same):**
### adjust the settings to your liking and click on the 'select area' button to choose an area on the screen to screenshot. by default it should capture the whole screen.      
![start](screenshots/start.png "the gui")    
     
### select a part of the screen by clicking down anywhere on the screen and dragging.    
![selecting an area to screenshot](screenshots/selection.png "selecting an area to screenshot")    
    
### after clicking the start button, a "processing..." message should appear    
![processing gif](screenshots/processing.png "processing the gif")    
    
### then if everything worked out, a successful processing message should show 
![done](screenshots/process_successful.png "finished processing")    
    
### you can find the finished gif (will be automatically named the current date and time, i.e. "19-06-2018_200620.gif") in the same folder as the application, as well as its frames in the "temp" directory.    
![find your gif](screenshots/done.png "find the gif and its frames")    
    
### here's my result:    
![gif result](screenshots/test.gif "f22! wow!'")    
    
### with inversion and saturation filters:    
![gif inverted](screenshots/test_inverted.gif)    
![gif saturated](screenshots/test_saturated.gif)    
    
### with saturation and a caption:    
![gif with caption](screenshots/caption_demo.gif)    
     
check out my Chrome extension that does pretty much the same thing (but is not as neat, and the timing of screenshots is a bit worse I think): https://github.com/syncopika/gifCatch_extension    
    
- - -    
## acknowledgements
Thanks very much to Philip Goh (https://github.com/cotidian/Win32GrabScreen) for the screen capture code, and Charlie Tangora (https://github.com/ginsweater/gif-h/blob/master/gif.h) for the gif-making code.    

gifs were taken from this video by Tonkatsu298: https://www.youtube.com/watch?v=D8gwnKApqCE   
