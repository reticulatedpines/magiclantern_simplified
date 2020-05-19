TOOL = check_sum

SRC = $(TOOL).c

$(TOOL): $(SRC)
	gcc -o $(TOOL) $(SRC)

install: $(TOOL)
	cp $(TOOL) $(HOME)/bin

clean:
	rm $(TOOL)
