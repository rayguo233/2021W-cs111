CC = gcc
CFLAGS = -o
WARN = -Wall -Wextra
SRC = lab3.c ext2_fs.h
OUT = lab3a
TAR = lab3a-123456789.tar.gz
SUBMIT = Makefile README $(SRC)
CHECK_SCR = P3A_check.sh

build: $(OUT)
dist: $(TAR)
clean:
	rm -f $(OUT) $(TAR)
$(OUT): $(SRC)
	$(CC) $(WARN) $(CFLAGS) $(OUT) $(SRC)
$(TAR): $(SUBMIT)
	tar -czvf $(TAR) $(SUBMIT)
check: dist
	chmod +x $(CHECK_SCR)
	./$(CHECK_SCR) 123456789
t: $(OUT) trivial.img
	./$(OUT) trivial.img
e: $(OUT) EXT2_test.img
	./$(OUT) EXT2_test.img