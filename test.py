# Testing the pymite interpreter dynamic loading
collection = {
	'first'		: "hello",
	'second'	: "world",
	'third'		: "from",
}

def run_test():
	y = 30
	x = 10
	#collection['fourth'] = 'pymite'
	#collection['fifth'] = 'user script!'

	#for k,v in collection.iteritems():
	for k in collection.keys():
		new_item = "X=" + `x` + ": " + collection[k]
		#main.bmp_puts( x, y, new_item )
		print new_item
		y += 32
		x += 28 * 2

#run_test()

import main;

main.bmp_fill( 3, 0, 30, 720, 300 )

run_test()
