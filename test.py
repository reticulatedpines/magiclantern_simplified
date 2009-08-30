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

main.bmp_fill( 3, 0, 30, 720, 300 )
main.bmp_puts( 200, 40, "Hi there" )

run_test()
