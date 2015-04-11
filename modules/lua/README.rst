LUA Module
=================

Module for running LUA Scripts


:Authors: dmilligan
:License: GPL
:Summary: LUA

The scripting engine looks for scripts in ML/SCRIPTS. Scripts should end with .LUA. Any scripts found get "loaded" at startup by the scripting engine simply running them. Therefore the main body of the script should simply *define* the script's menu and behavior, not actually *execute* anything.

The scripting engine will maintain your script's global state from run to run. So any global variables you declare will persist until the camera is turned off.

See http://davidmilligan.github.io/ml-lua/ for complete API documentation