-- Script unload test
--
-- Scripts can be unloaded if, when the main function exits:
-- * no other background tasks started by this script are (still) running
-- * no other event handlers are (still) active
-- * no property handlers are (still) active
-- * no menu entries are created (even if you remove them - it's not implemented)
-- * no LVInfo entries are registered (they can't be removed yet)
--
-- Otherwise, the script will keep running in background (just like modules).
--
-- Note: task support is extremely hackish and has many limitations.
-- For example, (m)sleep does not allow other tasks from this script to run.
-- To allow multitasking within the same script, you must call task.yield()
-- however, only one task is allowed call task.yield(), as switching between tasks
-- will corrupt Lua internals.
--
-- Still, this half-broken multitasking has a good use case:
-- starting some long-running action from a menu or event handler
-- (these functions must return quickly).
--
-- If you are familiar with multitasking in Lua, your help is welcome.

-- close ML menu and open the console
menu.close()
console.show()
sleep(0.5)

io.write("Unload test...\n")

function my_task()
    io.write("User task here.\n")
    sleep(2)
    io.write("User task exiting.\n")
end

-- note: with the task.yield restriction, it's hard to imagine a way
-- to let the user task finish after the main task
-- because once the main task allowed the child task to take control,
-- it cannot advance until the child task returns.

event.keypress = function(key)
    if key == KEY.PLAY then
        -- start a user task when pressing PLAY
        task.create(my_task)
        return false
    end
end

-- give control to other tasks 
io.write("Press PLAY to start a new task (within the next 10 seconds).\n")
task.yield(10000)

-- unregister the hook (so we can unload the script)
event.keypress = nil
io.write("You can no longer start a new task with PLAY.\n")

-- if our user tasks return earlier than our main task
-- the script can be considered "simple" and unloaded
io.write("Main task exiting.\n")
