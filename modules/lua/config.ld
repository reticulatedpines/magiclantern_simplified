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

Menus
-----
Scripts are not automatically visible in any specific menu. The script itself should
define it's own menu (if a menu is needed). See: @{menu}, @{calc.lua}, @{copy2m.lua},
@{menutest.lua}, @{recdelay.lua}, @{scrnshot.lua}, @{sokoban.lua}

State
-----
The scripting engine will maintain your script's global state for as long as the camera
is powered on. So any global variables you declare will persist until the camera is
turned off.

Each script get's it's own lua state (virtual machine). If you would like scripts to
share a state, have only one of the scripts in the `ML/SCRIPTS` folder, and have it
call the other scripts with dofile() or require()

Tasks (threading)
-----------------
"Preemptive" multithreading is not allowed within a single lua state, so only one
thread of execution is allowed to be running in your script at a given time.
Any attempts by other tasks to call functions in your scripts (e.g. via @{event}
handlers) will block until the current execution completes (or yields) or some timeout
is reached (this is achieved by the scripting backend making use of semaphores).
(Separate scripts may run at "the same time" since they have separate lua states)

The scripting engine will load and run scripts in a separate task created explicitly
to load scripts. If a script blocks during loading, it will prevent other subsequent
scripts from loading.

Different event and menu handlers get called from different ML or Canon tasks, and
scripts may start their own tasks. A script may yield it's execution to some other
task that is calling the script by calling @{task.yield}. Execution is automatically
yielded when the script returns from a call.

When writing a script, you should be mindful of the context (task) from which your
script is running, and consider when it might be appropriate to start something in a
separate task (to keep from blocking some ML or Canon task), or when to yield (to
keep the script from blocking itself)

See: @{task}

Library Scripts
---------------
You can write re-usable script code as a "library" by saving it as a .lua file in
`ML/SCRIPTS/LIB` The require function will search for libraries (or "packages") here.
As with regular scripts make sure to use valid 8.3 filenames. For a library called
`ML/SCRIPTS/LIB/MYLIB.LUA`, load it by calling `require("MYLIB")`. For example, see
@{config.lua} and @{keys.lua}, which are 'library' scripts included in this API.

Canon Properties
----------------
Canon firmware manages many settings via what it refers to as "properties". We can
set handlers to recieve notificatons when properties change and we can send requests
to change their values. One must be very careful when doing so, because Canon
firmware persists this properties in NVRAM, and setting invalid values can
'soft-brick' the camera.

'Safe' getters and setters for some of the more common of these properties are
available in sereveral of the modules, mainly @{camera} and @{lens}. You can
register handlers for properties with the @{property} module. Property handlers
are run in a seprate task, so you can take as much time as you like without worrying
about blocking Canon code, but doing so will block subsequent property handlers.

Errors
------
Unhandled errors are logged to the "console". You can turn it on from Debug >
Modules debug > Show Console. This is also where you will see the output of
the print() function. You can also implement your own error handling
see @{editor.lua}

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
    examples[1]..'/legacy/brack.c',
    examples[1]..'/legacy/clock.c',
    examples[1]..'/legacy/hello.c',
    examples[1]..'/legacy/hello.py',
    examples[1]..'/legacy/img.c',
    examples[1]..'/legacy/keys.c',
    examples[1]..'/legacy/sokoban.c',
    examples[1]..'/legacy/test.c'
}