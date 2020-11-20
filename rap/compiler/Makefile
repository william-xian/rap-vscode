

#C_ARGS=-pipe  -O -W -Wall -Wpointer-arith -Wno-unused-parameter -Werror -g -lpcre -DDebug -I./ 
C_ARGS=-lpcre -DDebug -I./ 
INC=deed.h  rap_array.h  rapc.h  rap_hash.h  rap_palloc.h  rap_rbtree.h  rap_string.h
SRC=rap_array.c  rapc.c  rap_hash.c  rap_palloc.c  rap_rbtree.c  rap_string.c

build: $(INC) $(SRC)
	gcc -o rapc $(C_ARGS) $^

run: build
	./rapc ../demo/Main.rap
