all: clean lock_test # barrier # mmap rw rwv aio flock

clean:
	rm -rf lock_test # barrier # mmap rw rwv aio lock

lock_test:
	g++ -O0 lock_test.cc -o lock_test -lpthread

mmap:
	g++ -O0 mmap.cc -o mmap

rw:
	g++ -O0 read_write.cc -o rw

rwv:
	g++ -O0 readv_writev.cc -o rwv

aio:
	g++ -O0 libaio.cc -o aio -lpthread -laio

flock:
	g++ -O0 flock.cc -o flock

barrier:
	g++ -O3 barrier.cc -o barrier -lpthread