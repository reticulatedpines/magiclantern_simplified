printf("Let's fake some buttons :)\n");

// click means "press and release"
// press means "press and hold" (you need to call unpress too)

sleep(2);

console_hide();

click(MENU);
sleep(1);

click(LEFT);
sleep(1);

click(RIGHT);
sleep(1);

// notice the difference between press and click
press(RIGHT);
sleep(1);
unpress(RIGHT);
sleep(1);

click(MENU);
sleep(1);

click(PLAY);
sleep(1);

click(ZOOM_IN);
sleep(1);

click(PLAY);
sleep(1);

click(PLAY);
sleep(1);

press(SHOOT_HALF);
press(SHOOT_FULL);
sleep(1);
unpress(SHOOT_FULL);
unpress(SHOOT_HALF);

sleep(2);
console_show();
printf("That's all, folks!\n");


