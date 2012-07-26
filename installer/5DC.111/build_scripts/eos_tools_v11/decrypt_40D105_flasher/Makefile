TOOL = decrypt_40D105_flasher

SRC = $(TOOL).c

$(TOOL): $(SRC)
	gcc -o $(TOOL) $(SRC)

install: $(TOOL)
	cp $(TOOL) $(HOME)/bin

clean:
	rm $(TOOL)
