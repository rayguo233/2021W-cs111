SRC = lab3b.py
TAR = lab3b-123456789.tar.gz
SUBMIT = Makefile README $(SRC)
CHECK_SCR = P3B_check.sh
LINK = lab3b

default: $(LINK)

dist: $(TAR)

clean:
	rm -rf $(TAR) CS111_test.rayguo functions.sh $(LINK)

$(TAR): $(SUBMIT)
	tar -czvf $(TAR) $(SUBMIT)

$(LINK): $(SRC)
	rm -f $(LINK)
	ln -s $(SRC) $(LINK)
	chmod +x $(LINK)
	echo "Success"

check: dist
	chmod +x $(CHECK_SCR)
	./$(CHECK_SCR) 123456789
