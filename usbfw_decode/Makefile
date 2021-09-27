all:
	gcc -c decode.c
	gcc -c decode_utils.c -DENCODE_SUPPORT
	gcc decode.o decode_utils.o -o decode

clean:
	rm -f *.o decode
