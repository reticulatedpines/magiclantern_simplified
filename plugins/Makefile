DIRS=$(shell ls -d -- */)

all:
	+$(foreach d,$(DIRS),make -C ${d};)

clean:
	+$(foreach d,$(DIRS),make -C ${d} clean;)
