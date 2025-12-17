CFLAGS=`pkg-config --cflags notcurses` -Wall -Werror
LIBS=`pkg-config --libs notcurses` -lm
SRCFILES=$(wildcard src/*.c)
OBJFILES=$(addprefix build/, $(addsuffix .o,$(basename $(notdir $(SRCFILES)))))
NAME=calculette
RM=rm
CC=gcc


all: $(NAME) $(CLIENT_NAME)

$(NAME): $(OBJFILES)
	$(CC) $^ -o $(NAME) $(LIBS) -ggdb

build/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@ $(LIBS) -ggdb

clean:
	$(RM) $(wildcard $(OBJFILES) $(NAME))
