# Testing the pymite interpreter dynamic loading
def run_test():
	y = 30
	x = 10
	collection = ["hello","world","from","pymite","user script!"]
	for item in collection:
		main.bmp_puts( x, y, item )
		y += 32
		x += 28 * 2

#run_test()

import main;
main.bmp_puts( 200, 40, "Hi there" );
#main.bmp_puts( 200, 60, "Hello %03d" % 5 );
#bmp_puts( 200, 60, 9999 );

run_test()
