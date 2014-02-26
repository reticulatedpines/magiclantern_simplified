Advanced Intervalometer
========

Advanced ramping and exposure control for the intervalometer

:Author: David Milligan
:License: GPL 3.0
:Website: bitbucket.org/dmilligan/magic-lantern


Create keyframes from current camera settings and specify the 
frame at which the keyframe is applied. You will select which 
parameters you would like to set in the keyframe and which you 
would like to ignore. The module ramps (linearly) the vaules 
of selected parameters from one keyframe to the next while the
intervalometer is running. 

You should create an initial keyframe at 1, that has the 
settings you are going to start with, that way the module 
knows what to ramp from. If you don't do this the module won't 
ramp to your first keyframe, it will simply set the values 
when it gets there (it doesn't know how to calculate the ramp, 
b/c it doesn't know what the values started as).

To ramp basic expo controls (Av, Tv, ISO) set them like normal 
in the canon GUI as if you were taking a picture, then go into 
the ML menu under 'Shoot' -> 'Intervalometer' -> 
'Advanced Intervalometer' -> 'New Keyframe' and create a new
keyframe. Turn on or off to specify which parameters are to be 
included in the keyframe. The current values are displayed on 
the right for you, but you cannot change them from here.

For Focus you do specify the offset to focus to from the 'New 
Keyframe' menu. The module will calucate the number of focus 
steps it need to take each frame to arrive at your offset that 
you specify. It steps from wherever the lens is focused at the 
start. Negative values mean focus closer, positve is towards 
infinity. You need to be in LiveView for the focusing to work 
and make sure there is enough time after the exposure is taken, 
before the next one starts for the focusing to take place (I 
recommend at least 2s, maybe more depending on camera and lens)

You can ramp the intervalometer period as well, this you also 
specify from within the new keyframe menu

Once you have created keyframes you can view them with the 'List 
Keyframes' menu. You can also save your keyframe sequence to 
file, and then reload them later. If not saved, any keyframes
you created will be lost when the camera is turned off.
Keyframes are saved and loaded from a file called "SEQ.TXT" in
the ML/SETTINGS directory.

When you have created keyframes and are ready to begin, make 
sure the advanced intervalometer is turned on, then turn the 
intervalometer on as usual from the ML Shoot menu.

This module is compatible with AutoETTR so long as you don't try
to ramp the parameters that AutoETTR is trying to change. You
can ramp Av and let AutoETTR take care of Tv and ISO, or you can
ramp Av and Tv and let AuttoETTR only set ISO, or put AutoETTR to
link to Canon shutter (then your Tv will basically be setting
the slowest Tv parameter of AutoETTR)