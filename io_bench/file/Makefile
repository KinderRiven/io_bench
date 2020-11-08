all: clean seqwrite rw

seqwrite:
	g++ -O0 seqwrite.cc -o seqwrite -lpthread

rw:
	g++ -O0 readwrite.cc -o readwrite -lpthread

clean:
	rm -rf seqwrite readwrite