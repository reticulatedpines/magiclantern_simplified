Lua Scripting API
================================================================================

The scripting engine looks for scripts in ML/SCRIPTS. Scripts should end with .LUA. Any scripts found get "loaded" at startup by the scripting engine simply running them. Therefore the main body of the script should simply *define* the script's menu and behavior, not actually *execute* anything.

The scripting engine will maintain your script's global state from run to run. So any global variables you declare will persist until the camera is turned off.

There are two ways to *define* the script's behavior. For simple scripts that don't need any menu options, simply declare a 'main' function, and optionally define two global variables 'script_name' and 'script_help'. If you don't define 'script_name', then the script's name is simply the filename of the script.

::
    
    script_name = "My Lua Script"
    script_help = "Run my awesome lua script"
    function main()
        print("Hello World!")
    end

Menu API
--------------------------------------------------------------------------------

In a more complex scenario you can define your script's menu by declaring a table named 'menu'

::
    
    menu =
    {
        parent = "LUA",
        name = "Run Test Script",
        help = "Run the test script.",
        submenu =
        { 
            {
                name = "Run",
                help = "Run this script.",
                icon_type = ICON_TYPE.ACTION,
                update = "",
            },
            {
                name = "param1",
                help = "help for param1",
                min = 0,
                max = 100,
                warning = function() return "this param doesn't work right :P" end,
            },
            {
                name = "param2",
                help = "help for param2",
                min = 0,
                max = 10,
                value = 5,
                depends_on = DEPENDS_ON.LIVEVIEW,
                info = function() return "click it baby!" end,
            },
            {
                name = "dec test",
                min = 0,
                max = 10000,
                unit = UNIT.DEC,
            },
            {
                name = "choices test",
                choices = { "choice1", "choice2", "choice3" },
            }
        },
        update = function() return menu.submenu[5].value end,
    }
    
    menu.submenu[1].select = function()
        console.show()
        for i = 1, #(menu.submenu), 1 do
            if menu.submenu[i].value ~= nil then
                print(menu.submenu[i].name.." = "..menu.submenu[i].value)
            end
        end
        print("script run finished!")
    end
    
The following table describes all the fields you can define in your 'menu' table

===============  ========  =====================================================
Field            Type      Description 
===============  ========  =====================================================
parent           string    The top-level menu that this menu should appear under
name             string    Name for the menu item
help             string    Help text for the menu item (line 1)
help2            string    Help text for the menu item (line 2)
min              integer   The minimum value the menu item can have
max              integer   The maximum value the menu item can have
value            integer   The current value of the menu item (automatically updated by the scripting backend, in the declaration, use this to specify a default value)
choices          table     An array of strings that represent choices the user can choose from. When used, min and max will be automatic and value will be the string that is selected
depends_on       integer   Dependencies for this menu item. Possible values are specified in the 'DEPENDS_ON' global. If the dependecies are not met, the item will be greyed out and a warning will appear at the bottom of the screen.
works_best_in    integer   Suggested operating mode for this menu item. Possible values are specified in the "WORKS_BEST_IN' global.
icon_type        integer   The type of icon to use for this menu item. Possible values are specified in the 'ICON_TYPE' global
unit             integer   The unit for the menu item's value. Possible values are specified in the 'UNIT' global
select           function  A function that will be called when the user selects the menu item. This function can take one parameter that is the delta for the menu's value
update           function  A function that will be called when the menu item is displayed. This function should return a string and that string will be displayed as the value of the menu item. Use this function to override the default display functionality.
warning          function  A function that returns a string to warn the user (e.g. whenever certain settings are invalid). Return nil when there should be no warning
info             function  A function that returns a string that will be displayed at the bottom of the screen
submenu          table     An array of menu tables that show up as a submenu for this menu item.
===============  ========  =====================================================

Events API
--------------------------------------------------------------------------------

Your script can repsond to events by defining functions in the 'events' table. Event handler functions can take one integer parameter, and must return a boolean that specifies whether or not the backend should continue executing event handlers for this particular event.

Event handlers will not run if there's already a script or another event handler actively executing at the same time.

The table below describes the events you can respond to.

=====================  =========================================================
Event                  Description
=====================  =========================================================
pre_shoot              called before image is taken
post_shoot             called after image is taken
seconds_clock          called every second
vsync                  called for every LiveView frame; can do display tricks; must not do any heavy processing!!!
keypress               when a key was pressed, this cbr gets the translated key as ctx 
vsync_setparam         called from every LiveView frame; can change FRAME_ISO, FRAME_SHUTTER_TIMER, just like for HDR video 
custom_picture_taking  special types of picture taking (e.g. silent pics); so intervalometer and other photo taking routines should use that instead of regular pics
intervalometer         called after a picture is taken with the intervalometer
=====================  =========================================================

Global functions
--------------------------------------------------------------------------------

=========================  =====================================================
Function                   Description
=========================  =====================================================
msleep(ms)                 Pauses for ms miliseconds and allows other tasks to run.
shoot([wait],[af])         Takes a picture.
call(funcname, [arg])      Calls an eventproc (a function from the camera firmware which can be called by name). See Eventprocs. Dangerous.
beep([numtimes])           Plays a beep through the camera speaker
=========================  =====================================================

Console Library
--------------------------------------------------------------------------------

===================  ===========================================================
Field                Description
===================  ===========================================================
console.show()       Shows the console.
console.hide()       Hides the console.
console.write(text)  Writes some text to the console.
===================  ===========================================================

Camera Library
--------------------------------------------------------------------------------

=========================  =====================================================
Field                      Description
=========================  =====================================================
camera.shoot([wait],[af])  Takes a picture.
camera.shutter             get/set the shutter speed in apex units x10.
camera.aperture            get/set the aperture in apex units x10.
camera.iso                 get/set the ISO in apex units x10.
camera.ec                  get/set the expsosure compensation in apex units x10.
camera.flash_ec            get/set the flash expsosure compensation in apex units x10.
camera.mode get            the current camera mode. Possible values defined in MODE global.
camera.af_mode             get the current auto focus mode.
camera.metering_mode       get the current metering mode.
camera.drive_mode          get the current drive mode.
camera.model               get the model name of the camera.
camera.firmware            get the Canon firmware version string.
camera.temperature         get the temperature from the efic chip in raw units
camera.state               get the current Canon GUI state of the camera (PLAY, QR, LV, etc)
=========================  =====================================================

Lens Library
--------------------------------------------------------------------------------

=================================================  =============================
Field                                              Description
=================================================  =============================
lens.focus(steps,[step_size],[wait],[extra_delay]  Moves the focus motor a specified number of steps. Only works in LV.
lens.name                                          get the name of the lens.
lens.focal_length                                  get the focal length of the lens (in mm)
lens.focal_distance                                get the current focal distance (in cm)
lens.hyperfocal                                    get the hyperfocal distance of the lens (in cm)
lens.dof_near                                      get the distance to the DOF near (in cm)
lens.dof_far                                       get the distance to the DOF far (in cm)
lens.af                                            true => auto focus; false => manual focus
=================================================  =============================

LiveView Library
--------------------------------------------------------------------------------

===============  ===============================================================
Field            Description
===============  ===============================================================
lv.start()       Enter LiveView.
lv.pause()       Pause LiveView (but leave shutter open).
lv.resume()      Resume LiveView (if paused)
lv.stop()        Exit LiveView.
lv.enabled       get/set whether or not LV is running.
===============  ===============================================================

Movie Library
--------------------------------------------------------------------------------

===============  ===============================================================
Field            Description
===============  ===============================================================
movie.start()    Start recording a movie.
movie.stop()     Stops recording a movie.
movie.recording  get/set whether or not a movie is currently recording
===============  ===============================================================

Constants
--------------------------------------------------------------------------------

==============  ================================================================
Constant        Description
==============  ================================================================
MODE.P          Program Mode
MODE.TV         Shutter Priority Mode
MODE.AV         Aperture Priority Mode
MODE.M Manual   Mode
MODE.BULB Bulb  Mode
MODE.ADEP ADEP  Mode
MODE.C          Custom Mode
MODE.C2         C2
MODE.C3         C3
MODE.CA         Creative Auto Mode
MODE.AUTO Full  Auto Mode
MODE.NOFLASH    No flash Mode
MODE.PORTRAIT   Portrait Mode
MODE.LANDSCAPE  Landscape Mode
MODE.MACRO      Macro Mode
MODE.SPORTS     Sports Mode
MODE.NIGHT      Night Mode
MODE.MOVIE      Movie Mode
==============  ================================================================

================  ==============================================================
Constant          Description
================  ==============================================================
UNIT.EV           1/8 EV units
UNIT.x10 x10      Fixed Point
UNIT.PERCENT      Percentage
UNIT.PERCENT_x10  x10 fixed point percentage
UNIT.ISO          ISO
UNIT.HEX          Hexadecimal
UNIT.DEC          Decimal
UNIT.TIME         Time
================  ==============================================================

==============================  ================================================
Constant                        Description
==============================  ================================================
ICON_TYPE.AUTO
ICON_TYPE.BOOL
ICON_TYPE.DICE
ICON_TYPE.PERCENT
ICON_TYPE.ALWAYS_ON
ICON_TYPE.ACTION
ICON_TYPE.BOOL_NEG
ICON_TYPE.DISABLE_SOME_FEATURE
ICON_TYPE.SUBMENU
ICON_TYPE.DICE_OFF
ICON_TYPE.PERCENT_OFF
ICON_TYPE.PERCENT_LOG
ICON_TYPE.PERCENT_LOG_OFF
==============================  ================================================

==============================  ================================================
Constant                        Description
==============================  ================================================
DEPENDS_ON.GLOBAL_DRAW
DEPENDS_ON.LIVEVIEW
DEPENDS_ON.NOT_LIVEVIEW
DEPENDS_ON.MOVIE_MODE
DEPENDS_ON.PHOTO_MODE
DEPENDS_ON.AUTOFOCUS
DEPENDS_ON.MANUAL_FOCUS
DEPENDS_ON.CFN_AF_HALFSHUTTER
DEPENDS_ON.CFN_AF_BACK_BUTTON
DEPENDS_ON.EXPSIM
DEPENDS_ON.NOT_EXPSIM
DEPENDS_ON.CHIPPED_LENS
DEPENDS_ON.M_MODE
DEPENDS_ON.MANUAL_ISO
DEPENDS_ON.SOUND_RECORDING
DEPENDS_ON.NOT_SOUND_RECORDING
==============================  ================================================

