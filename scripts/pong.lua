--[[
 Pong Game
 
 Simple game to demonstrate Lua scripting capabilities.
 
 This game should help you understand:
 - Animation without double-buffering
 - Audible feedback (beeps)
 - Basic key processing
 - Lua programming basics

 Enjoy! 
]]

require("keys")

score_right = 0     -- human player score
score_left = 0      -- computer opponent score

-- ball position
ball_x = 320
ball_y = 240

-- ball speed
game_speed = 0
counter = 0
ball_dx = 0
ball_dy = 0

-- paddle y-pos
left_paddle_y = 240
right_paddle_y = 240

-- human player speed/direction, from key presses
right_paddle_delta = 0

-- computer AI
AI_maxspeed = 0
AI_bias = 0

-- store previous drawing state, for erasing
prev_ball_x = 0
prev_ball_y = 0
prev_left_paddle_y = 0
prev_right_paddle_y = 0

ball_radius = 10
paddle_size = 70

function coerce(x, lo, hi)
    return math.max(math.min(x, hi), lo)
end

function beep_border_bounce()
    beep(1, 100, 330)      -- short beep at 330 Hz
end

function beep_paddle_hit()
    beep(1, 100, 400)      -- short beep at 400 Hz
end

function beep_paddle_miss()
    beep(1, 100, 1000)     -- short beep at 1000 Hz
end

function game_pause()
    while keys:getkey() ~= KEY.MENU do
        display.print("Game paused, press MENU to resume.", 0, 0)
        task.yield(50)
    end

    display.clear()
end

function draw()
    -- to prevent flicker without using the expensive double-buffering routines,
    -- we erase the game items from previous position, then draw them in the new position
    
    -- draw the ball
    display.circle(prev_ball_x, prev_ball_y, ball_radius, COLOR.TRANSPARENT, COLOR.TRANSPARENT)
    display.circle(ball_x, ball_y, ball_radius, COLOR.WHITE, COLOR.WHITE)
    display.circle(ball_x, ball_y, ball_radius, COLOR.BLACK)

    -- draw right paddle
    if prev_right_paddle_y ~= right_paddle_y then
        display.rect(700, prev_right_paddle_y-paddle_size/2, 10, paddle_size, COLOR.TRANSPARENT, COLOR.TRANSPARENT)
    end
    display.rect(700+1, right_paddle_y-paddle_size/2+1, 10-2, paddle_size-2, COLOR.WHITE, COLOR.WHITE)
    display.rect(700, right_paddle_y-paddle_size/2, 10, paddle_size, COLOR.BLACK)
    
    -- draw left paddle
    if prev_left_paddle_y ~= left_paddle_y then
        display.rect(10, prev_left_paddle_y-paddle_size/2, 10, paddle_size, COLOR.TRANSPARENT, COLOR.TRANSPARENT)
    end
    display.rect(10+1, left_paddle_y-paddle_size/2+1, 10-2, paddle_size-2, COLOR.WHITE, COLOR.WHITE)
    display.rect(10, left_paddle_y-paddle_size/2, 10, paddle_size, COLOR.BLACK)
    
    -- show the score
    display.print(score_left, 350 - 50, 20, FONT.LARGE)
    display.print(score_right, 350 + 50, 20, FONT.LARGE)

    -- store previous coords
    prev_ball_x = ball_x
    prev_ball_y = ball_y
    prev_left_paddle_y = left_paddle_y
    prev_right_paddle_y = right_paddle_y
end

function main()
    -- this game runs on top of the menu backend
    -- we will hook all the keys and prevent menu redraws
    -- don't forget to restore them when the game ends
    keys:start()
    menu.block(true)
    
    -- wait until menu redrawing finishes, then clear the screen once
    -- during the game, we will use only incremental redraws
    -- that don't require double buffering
    sleep(0.5)
    display.clear()
    
    game_speed = 3
    counter = 0

    ball_dx = game_speed
    ball_dy = math.random(-game_speed, game_speed)

    AI_maxspeed = game_speed

    while menu.visible do
        --gradually increase speed
        counter = counter + 1
        if counter == 1000 then 
            counter = 0
            game_speed = game_speed + 1 
            AI_maxspeed = game_speed
        end
        local key = keys:getkey()
        if key == KEY.UP then
            -- up, down: move right paddle (smooth movement)
            right_paddle_delta = -2*game_speed
        elseif key == KEY.DOWN then
            right_paddle_delta = 2*game_speed
        elseif key == KEY.UNPRESS_UDLR then
            right_paddle_delta = 0
        elseif key == KEY.WHEEL_DOWN or key == KEY.WHEEL_RIGHT then
            -- scrollwheels: move right paddle (discrete increments)
            right_paddle_y = right_paddle_y + 5*game_speed
        elseif key == KEY.WHEEL_UP or key == KEY.WHEEL_LEFT then
            right_paddle_y = right_paddle_y - 5*game_speed
        elseif key == KEY.MENU then
            -- MENU: pause/resume game
            game_pause()
        end
        
        -- update ball position
        ball_x = ball_x + ball_dx
        ball_y = ball_y + ball_dy
        
        -- update right paddle position
        right_paddle_y = coerce(right_paddle_y + right_paddle_delta, 20 + paddle_size/2, 460-paddle_size/2)

        -- computer AI controls left paddle
        -- algorithm: just follow the ball with some hysteresis, limit max speed and add a little randomness
        -- that is, a slightly less-than-perfect player, so you still get a chance to win the game :P
        AI_bias = coerce(AI_bias + math.random(-1, 1), -10, 10)
        local left_paddle_delta = ball_y - left_paddle_y + AI_bias
        if left_paddle_delta > 0 then left_paddle_delta = math.max(left_paddle_delta-20, 0) end
        if left_paddle_delta < 0 then left_paddle_delta = math.min(left_paddle_delta+20, 0) end
        left_paddle_delta = coerce(left_paddle_delta, -AI_maxspeed, AI_maxspeed)
        left_paddle_y = coerce(left_paddle_y + left_paddle_delta, 20 + paddle_size/2, 460-paddle_size/2)
        
        if ball_x < 20 + ball_radius then
            -- left bounce
            
            if math.abs(left_paddle_y - ball_y) < paddle_size / 2 + ball_radius then
                -- paddle hit the ball?
                ball_x = 20 + ball_radius
                ball_dx = -ball_dx 
                ball_dy = (ball_y - left_paddle_y) * game_speed / 25
                beep_paddle_hit()
            else 
                -- nope, missed
                draw()
                beep_paddle_miss()
                task.yield(500)
                score_right = score_right + 1
                ball_x = 360
                ball_y = 240
                right_paddle_delta = 0
                ball_dx = game_speed
                ball_dy = math.random(-game_speed, game_speed)
            end
        
        elseif ball_x > 700 - ball_radius then
            -- right bounce
            
            if math.abs(right_paddle_y - ball_y) < paddle_size / 2 + ball_radius then
                -- paddle hit the ball?
                ball_x = 700-ball_radius
                ball_dx = -ball_dx
                ball_dy = (ball_y - right_paddle_y) * game_speed / 25
                beep_paddle_hit()
            else
                -- nope, missed
                draw()
                beep_paddle_miss()
                task.yield(500)
                score_left = score_left + 1
                ball_x = 360
                ball_y = 240
                right_paddle_delta = 0
                ball_dx = -game_speed
                ball_dy = math.random(-game_speed, game_speed)
            end
        end
        
        if ball_y < ball_radius then
            -- top bounce
            ball_y = ball_radius
            ball_dy = -ball_dy 
            beep_border_bounce()
        elseif ball_y > 480-ball_radius then
            -- bottom bounce
            ball_y = 480 - ball_radius
            ball_dy = -ball_dy 
            beep_border_bounce()
        end
        
        -- update the screen
        draw()

        -- be friendly with other tasks
        task.yield(20)
    end
    
    -- quit game
    menu.block(false)
    keys:stop()
end

main()
