CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -D_XOPEN_SOURCE=700 -pedantic
TARGET  = tarsau
SRC     = tarsau.c

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) *.sau
	rm -rf test_dir test_extract

test: $(TARGET)
	@echo "Test 1: Arsivleme..."
	@echo "Merhaba" > t1.txt
	@./$(TARGET) -b t1.txt -o test.sau
	@echo "Test 2: Cikarma..."
	@mkdir -p test_extract
	@./$(TARGET) -a test.sau test_extract
	@diff t1.txt test_extract/t1.txt && echo "Test Basarili!"
	@rm -f t1.txt test.sau
	@rm -rf test_extract
