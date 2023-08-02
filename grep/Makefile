CC=gcc
CFLAGS=-Wall -Werror -Wextra
GREP=s21_grep.c
all: s21_grep

s21_grep:
	$(CC) $(CFLAGS) $(GREP) -o s21_grep

clean:
	rm s21_grep

rebuild: clean s21_grep
