# echo 1 > /proc/sys/vm/drop_caches

for ((i=2;i<16;i+=2))
do
    ./lock_test $i > $i
done