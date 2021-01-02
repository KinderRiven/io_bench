all: clean barrier # mmap rw rwv aio lock

clean:
	rm -rf barrier # mmap rw rwv aio lock

mmap:
	g++ -O0 mmap.cc -o mmap

rw:
	g++ -O0 read_write.cc -o rw

rwv:
	g++ -O0 readv_writev.cc -o rwv

aio:
	g++ -O0 libaio.cc -o aio -lpthread -laio

lock:
	g++ -O0 flock.cc -o lock

barrier:
	g++ barrier.cc -o barrier