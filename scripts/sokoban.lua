--[[
 Sokoban Game
 Ported to Lua from the original PicoC version
  
 This is a playable demo for Lua scripting.
 
 It should help you understand:
 - Basic key processing
 - Simple graphics
 - Lua programming basics
 
 Enjoy! 
]]

require("keys")

--printf
function printf(...)
    io.write(string.format(...))
end

-- http://www.sokobano.de/wiki/index.php?title=Sok_format

WALL = '#'
SPACE  = ' '
PLAYER  = '@'
BOX  = '$'

TARGET  = '.'
PLAYER_ON_TARGET  = '+'
BOX_ON_TARGET  = '*'

-- from Wikipedia
maze1 = {
    "  ##### ",
    "###   # ",
    "#.@$  # ",
    "### $.# ",
    "#.##$ # ",
    "# # . ##",
    "#$ *$$.#",
    "#   .  #",
    "########"
};

-- Level 1 from http://sneezingtiger.com/sokoban/levels/minicosmosText.html
maze2 = {
    "  ##### ",
    "###   # ",
    "# $ # ##",
    "# #  . #",
    "#    # #",
    "## #   #",
    " #@  ###",
    " #####  "
};

-- Level 2 from http://sneezingtiger.com/sokoban/levels/minicosmosText.html
maze3 = {
    "  ##### ",
    "###   # ",
    "# $ # ##",
    "# #  . #",
    "#    # #",
    "##$#.  #",
    " #@  ###",
    " #####  "
};

-- Level 23 from http://sneezingtiger.com/sokoban/levels/minicosmosText.html

maze4 = {
    "      #### ",
    "#######  # ",
    "#        # ",
    "#  $ #.# # ",
    "#  $## # ##",
    "###   @   #",
    "  ###  #  #",
    "    ##.  ##",
    "     ##### "
};


-- Level 1 from http://sneezingtiger.com/sokoban/levels/nabokosmosText.html
maze5 = {
    " #####  ",
    " #   ## ",
    "## * .##",
    "# $$*  #",
    "#  * . #",
    "## @ ###",
    " #####  "
};

-- Level 2 from http://sneezingtiger.com/sokoban/levels/nabokosmosText.html
maze6 = {
    "  ####  ",
    "###  ###",
    "#   *$ #",
    "# #  # #",
    "#   ** #",
    "###  #@#",
    "  # ** #",
    "  #  # #",
    "  # *. #",
    "  #  ###",
    "  ####  "
};

-- global variables
maze = nil
LINES = nil
COLUMNS = nil

function string_to_table(str)
    local t = {}
    for i = 1,#str,1 do
        t[i] = string.sub(str,i,i)
    end
    return t
end

function convert_maze(m)
    local r = {}
    for i,v in ipairs(m) do
        r[i] = string_to_table(v)
    end
    return r
end

function setup(a)
    if (a == 1) then
    
        maze = convert_maze(maze1);
        LINES = 9;
        COLUMNS = 8;
    
    elseif (a == 2) then
    
        maze = convert_maze(maze2);
        LINES = 8;
        COLUMNS = 8;
    
    elseif (a == 3) then
    
        maze = convert_maze(maze3);
        LINES = 8;
        COLUMNS = 8;
    
    elseif (a == 4) then
    
        maze = convert_maze(maze4);
        LINES = 9;
        COLUMNS = 11;
    
    elseif (a == 5) then
    
        maze = convert_maze(maze5);
        LINES = 7;
        COLUMNS = 8;
    
    elseif (a == 6) then
    
        maze = convert_maze(maze6);
        LINES = 11;
        COLUMNS = 8;
    end
end

targets = {}
function print_maze()

    for i = 1,LINES,1 do
    
        for j = 1,COLUMNS,1 do
            local c = ' '
            if targets[i*COLUMNS + j] ~= nil then c= c '*' end
            printf("%s%s", maze[i][j], c);
        end
        printf("\n");
    end
end

function draw_wall(x, y, a)

    display.rect(x, y, a, a, COLOR.BLACK);
end

function draw_space(x, y, a, on_target)

    display.rect(x, y, a, a, COLOR.gray(80), COLOR.gray(80));
    if (on_target) then
        display.circle(x + a//2, y + a//2, a//4, COLOR.ORANGE);
    end
end

function draw_box(x, y, a, on_target)

    local pad = 3;

    for i = 0, pad, 1 do
        display.rect(x + i, y + i, a - i*2, a - i*2, COLOR.gray(80));
    end
    local color = COLOR.YELLOW;
    if on_target then color = COLOR.ORANGE end
    display.rect(x + pad, y + pad, a - pad*2, a - pad*2, color,color);
    display.rect(x + pad, y + pad, a - pad*2, a - pad*2, COLOR.BLACK);
    pad = pad+1;
    display.rect(x + pad, y + pad, a - pad*2, a - pad*2, COLOR.BLACK);

    display.line(x + pad, y + pad, x + a - pad, y + a - pad, COLOR.BLACK);
    display.line(x + pad - 1, y + pad, x + a - pad - 1, y + a - pad, COLOR.BLACK);
    
    display.line(x + pad, y + a - pad, x + a - pad, y + pad, COLOR.BLACK);
    display.line(x + pad - 1, y + a - pad, x + a - pad - 1, y + pad, COLOR.BLACK);
end

function draw_player(x, y, a, on_target)

    draw_space(x, y, a, 0);
    
    local xc = x + a//2;
    local yc = y + a//2;
    local r = a * 3//8;
    local y_eye = yc - r * 3//8;
    local y_mouth = yc + r * 3//8;
    display.circle(xc, yc, r, COLOR.YELLOW, COLOR.YELLOW);
    display.circle(xc, yc, r, COLOR.BLACK);
    display.circle(xc, yc, r-1, COLOR.BLACK);
    display.circle(xc - r//3, y_eye, r//5, COLOR.BLACK, COLOR.BLACK);
    display.circle(xc + r//3, y_eye, r//5, COLOR.BLACK, COLOR.BLACK);
    
    local r_mouth = r*2//3;
    for t = 45, 180-45, 2 do
    
        local xp = xc + r_mouth * math.cos(t*math.pi/180);
        local yp = yc - r/8 + r_mouth * math.sin(t*math.pi/180);
        display.pixel(xp, yp, COLOR.BLACK);
        display.pixel(xp, yp+1, COLOR.BLACK);
    end
end

function draw_maze()

    display.rect(0, 0, 720, 480, COLOR.BLACK, COLOR.BLACK);
    local a = math.min(400 // LINES, 700 // COLUMNS);
    local total_w = a * COLUMNS;
    local total_h = a * LINES;
    local x0 = 360 - total_w // 2;
    local y0 = 240 - total_h // 2;
    
    for i = 1, LINES, 1 do
    
        for j = 1, COLUMNS, 1 do
        
            -- top-left corner of current cell
            local x = x0 + (j-1) * a;
            local y = y0 + (i-1) * a;
            
            if maze[i][j] == BOX then
                draw_box(x, y, a, targets[i*COLUMNS + j]);
            elseif maze[i][j] == SPACE then
                draw_space(x, y, a, targets[i*COLUMNS + j]);
            elseif maze[i][j] == WALL then
                draw_wall(x, y, a);
            elseif maze[i][j] == PLAYER then
                draw_player(x, y, a, targets[i*COLUMNS + j]);
            end
            
        end
    end
    
    for i = 1, 5, 1 do
        display.rect(x0-i, y0-i, total_w+2*i, total_h+2*i, COLOR.WHITE);
    end
end

function victory()

    beep();
    draw_maze();
    display.print("YOU WIN :)", 300, 200, FONT.LARGE );
    
    local level_menu = sokoban_menu.submenu["Level"]
    if level_menu.value < level_menu.max then
        level_menu.value = level_menu.value + 1
    end
end

function split_target()

    for i = 1, LINES, 1 do
    
        for j = 1, COLUMNS, 1 do
        
            if (maze[i][j] == TARGET) then
            
                targets[i*COLUMNS + j] = true;
                maze[i][j] = SPACE;
            
            elseif (maze[i][j] == BOX_ON_TARGET) then
            
                targets[i*COLUMNS + j] = true;
                maze[i][j] = BOX;
            
            elseif (maze[i][j] == PLAYER_ON_TARGET) then
            
                targets[i*COLUMNS + j] = true;
                maze[i][j] = PLAYER;
            
            else targets[i*COLUMNS + j] = false; end
        end
    end
end

function get_current_pos()

    for i = 1, LINES, 1 do
    
        for j = 1, COLUMNS, 1 do
        
            if (maze[i][j] == PLAYER) then
            
                return i,j;
            end
        end
    end
    -- error
    return -1,-1
end

function move(dl, dc)

    local l,c 
    l,c = get_current_pos();
    
    if (maze[l][c] ~= PLAYER) then printf("whoops"); return; end
    
    if (maze[l+dl][c+dc] == SPACE) then-- what's in the destination box?
    
        -- simple, just move
        maze[l][c] = SPACE;
        maze[l+dl][c+dc] = PLAYER;
        
    elseif (maze[l+dl][c+dc] == BOX) then -- move only if we can push the box in a free space
        
        if (maze[l+dl*2][c+dc*2] == SPACE) then
        
            maze[l+dl*2][c+dc*2] = BOX;
            maze[l+dl][c+dc] = PLAYER;
            maze[l][c] = SPACE;
        end
    end
    
end

function check_solution()

    for i = 1, LINES, 1 do
    
        for j = 1, COLUMNS, 1 do
        
            if ((targets[i*COLUMNS + j]) and (maze[i][j] ~= BOX)) then
                return 0;-- at least one box not on target position
            end
        end
    end
    return 1; -- looks alright
end

function main()

    setup(sokoban_menu.submenu["Level"].value)
    
    menu.block(true);
    local status,error = xpcall(function()
        split_target();
        keys:start()
        draw_maze();
        while true do
            if menu.visible == false then return end
            -- print_maze();
            local key = keys:getkey();
            if key == KEY.LEFT or key == KEY.WHEEL_LEFT then
                move(0, -1);
            elseif key == KEY.RIGHT or key == KEY.WHEEL_RIGHT then
                move(0, 1);
            elseif key == KEY.UP or key == KEY.WHEEL_UP then
                move(-1, 0);
            elseif key == KEY.DOWN or key == KEY.WHEEL_DOWN then
                move(1, 0);
            elseif key == KEY.SET or key == KEY.UNPRESS_SET then
            elseif key == KEY.Q or key == KEY.TRASH or key == KEY.MENU then
                printf("Exiting...\n");
                menu.block(false);
                keys:stop()
                return;
            end

            display.draw(draw_maze)
            if check_solution() ~= 0 then break end

            task.yield(100)
        end
        
        printf("You win!\n");
        victory();
        
        task.yield(5000)
    end,debug.traceback)
    if status == false then
        print(error)
    end
    menu.block(false);
    keys:stop()
end

sokoban_menu = menu.new
{
    parent  = "Games",
    name    = "Sokoban",
    help    = "A simple game in Lua",
    submenu = 
    {
        {
            name = "Play",
            select = function(this) task.create(main) end,
        },
        {
            name = "Level",
            value = 1,
            min = 1,
            max = 6
        }
    }
}
