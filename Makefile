NAME     := kindling
FLAGS    := -lm -lncurses -ljansson -lcurl -ljpeg
BIN_DIR  := /usr/local/bin
MAN_DIR  := /usr/local/share/man/man1
MAN_DATE := $(date +"%b %Y")

all: build

build: $(NAME).c
	gcc -Wall -Wextra -o $(NAME) $< $(FLAGS)

test: build
	./$(NAME)

install: $(NAME)
	install -m 0755 $(NAME) $(BIN_DIR)
	install -m 0755 $(NAME).1 $(MAN_DIR)

uninstall:
	rm -f $(BIN_DIR)/$(NAME) $(MAN_DIR)/$(NAME).1

clean:
	rm -f $(NAME)