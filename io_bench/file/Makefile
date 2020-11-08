all: clean seqwrite randwrite randread

seqwrite:
	g++ -O0 seqwrite.cc -o seqwrite -lpthread

randwrite:
	g++ -O0 randwrite.cc -o randwrite -lpthread

randread:
	g++ -O0 randread.cc -o randread -lpthread

clean:
	rm -rf seqwrite randwrite