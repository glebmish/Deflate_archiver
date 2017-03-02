all:
	g++ *.cpp deflate/*.cpp huffman/*.cpp util/*.cpp -o archiver.o

clean:
	rm *.o