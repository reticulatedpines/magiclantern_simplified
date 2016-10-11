-- A very simple script.

function main()
    menu.close()
    console.show()
    print "Hello, World!"
    print "Press any key to exit."
    key.wait()
    console.hide()
end

keymenu = menu.new
{
    name   = "Hello, World!",
    help   = "Prints 'Hello World' on the console",
    select = function(this) task.create(main) end,
}

