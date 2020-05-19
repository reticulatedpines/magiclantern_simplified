TOOL = dissect_fw3

SRC = $(TOOL).c

$(TOOL): $(SRC)
	gcc -o $(TOOL) $(SRC)

install: $(TOOL)
	cp $(TOOL) $(HOME)/bin

clean:
	rm $(TOOL)
