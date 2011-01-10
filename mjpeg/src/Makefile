CFLAGS=-O0 -Wall -g
mjpeg : conv.o huffman.o idct.o iqzz.o main.o screen.o skip_segment.o unpack_block.o upsampler.o
	gcc -Bstatic -o $@ $^ -lSDL
clean :
	rm -f conv.o
