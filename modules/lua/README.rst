Lua scripting
=============

Module for running Lua scripts on the camera.

The scripting engine looks for scripts (\*.LUA) in ML/SCRIPTS.
Any scripts found get "loaded" at startup by the scripting engine simply running them.
Therefore the main body of the script should simply *define* the script's menu and behavior,
not actually *execute* anything.

The scripting engine will maintain your script's global state from run to run,
so any global variables you declare will persist until the camera is turned off.

API documentation: http://davidmilligan.github.io/ml-lua/

:Authors: dmilligan
:License: GPL
:Summary: Lua scripting
:Forum: http://www.magiclantern.fm/forum/index.php?topic=14828.0