project = 'ML Lua API'
file = { '../lua', '../../scripts/lib'}
title = 'ML Lua API'
format = 'markdown'
description = 'Magic Lantern Lua API Documentation'
full_description = [[
Startup
-------
The scripting engine looks for scripts in `ML/SCRIPTS`. Scripts should end with `.LUA`.
Any scripts found get "loaded" at startup by the scripting engine simply running them.
Therefore the main body of the script should simply *define* the script's menu and
behavior, not actually *execute* anything.

The scripting engine will maintain your script's global state from run to run. So any
global variables you declare will persist until the camera is turned off.

Standard Libraries
------------------
All of the standard Lua libraries are available except @{os} (replaced by @{dryos}):

* @{coroutine}
* @{package}
* @{string}
* @{table}
* @{math}
* @{io}
* @{debug}

]]
--TODO: figure out how to not hardcode these
examples = { '../../scripts' }
    examples.exclude = {
    examples[1]..'/brack.c',
    examples[1]..'/clock.c',
    examples[1]..'/hello.c',
    examples[1]..'/hello.py',
    examples[1]..'/img.c',
    examples[1]..'/keys.c',
    examples[1]..'/sokoban.c',
    examples[1]..'/test.c'
}