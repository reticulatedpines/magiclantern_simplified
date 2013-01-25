printf("Let's fake some buttons :)\n");

// click means "press and release"
// press means "press and hold" (you need to call unpress too)

msleep(2000);

console_hide();

click(MENU);
msleep(1000);

click(LEFT);
msleep(1000);

click(RIGHT);
msleep(1000);

// notice the difference between press and click
press(RIGHT);
msleep(1000);
unpress(RIGHT);
msleep(1000);

click(MENU);
msleep(1000);

click(PLAY);
msleep(1000);

click(ZOOM_IN);
msleep(1000);

click(PLAY);
msleep(1000);

click(PLAY);
msleep(1000);

press(SHOOT_HALF);
press(SHOOT_FULL);
msleep(1000);
unpress(SHOOT_FULL);
unpress(SHOOT_HALF);

msleep(2000);
console_show();
printf("That's all, folks!\n");


