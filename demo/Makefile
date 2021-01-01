all: clean mmap rw rwv

clean:
	rm -rf mmap rw rwv

mmap:
	g++ -O0 mmap.cc -o mmap

rw:
	g++ -O0 read_write.cc -o rw

rwv:
	g++ -O0 readv_writev.cc -o rwv