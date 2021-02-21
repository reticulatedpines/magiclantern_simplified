project = 'ML Lua API'
file = { '../lua', '../../scripts/lib'}
title = 'ML Lua API'
format = 'markdown'
description = 'Magic Lantern Lua API Documentation'
no_space_before_args = true 
full_description = [[
Startup
-------
The scripting engine looks for scripts in `ML/SCRIPTS`. Scripts should end with `.LUA`.

There are two types of scripts "simple" and "complex". "Simple" scripts do not define
custom menus, don't leave any background tasks running, or any event or property handlers
registered when they finish loading. The user will be able to execute such scripts from
the 'Scripts' menu and they will not autorun when the camera starts. Also, such scripts
will be 'unloaded' when they're not actively running, which will help conserve memory.

"Complex" scripts are any scripts that define custom menus, start background task(s),
or leave event or property handlers registered when they finish loading. These scripts
cannot be unloaded; they will run in background until the camera is turned off.

Tasks, event handlers and property handlers are allowed in simple scripts
as long as they are cleaned up before the main task exits
(that is, the top-level code from your script).
See @{unload.lua} and @{api_test.lua} for examples.

Simple scripts are allowed to use the config_save event without unregistering it.
In this case, the event will be called after the main script body exits,
right before unloading the script.

Menus
-----
All script will appear in the 'Scripts' menu. Scripts can be run from here and autorun
can be toggled on or off. Scripts can also define their own menu(s) elsewhere if they
need extra settings or parameters. See: @{menu}, @{copy2m.lua}, @{menutest.lua},
@{recdelay.lua}.

State
-----
For "complex" scripts, the scripting engine will maintain your script's global state
for as long as the camera is powered on. So any global variables you declare will
persist until the camera is turned off. "Simple" scripts are unloaded when they finish
executing, so global state is lost when the script is executed again (if you need to
persist information from run to run, you can use file I/O).

Each script gets it's own Lua state (virtual machine). If you would like scripts to
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
keep the script from blocking itself).

See: @{task}, @{api_test.lua}, @{unload.lua}, @{recdelay.lua}, @{calc.lua}, @{editor.lua}.

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
Unhandled errors are logged to the "console". You can turn it on from Scripts > Show Console.
This is also where you will see the output of the print() function.
You can also implement your own error handling, see @{editor.lua}.

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
