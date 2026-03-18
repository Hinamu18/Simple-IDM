SRC=src/*.c 
LIBS= -lcurl -pthread -lncurses
output = -o IDM
FLAGS = -O3
all:
	gcc $(SRC) $(LIBS) $(output)
