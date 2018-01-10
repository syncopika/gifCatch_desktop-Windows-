# python script to generate a gif from temp folder of images

import os
import sys
import imageio

# go to the temp directory 
path = "temp"
dir = os.listdir(path)

# set up list for images 
images = []

for file in dir:
    images.append(imageio.imread(path + '/' + file))
	#print(file)

# set duration for each frame  
kargs = {'duration': .2}
	
# save gif as 'test.gif', place in current directory 
imageio.mimsave('test.gif', images, 'GIF', **kargs)
