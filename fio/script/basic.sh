rw=('write') # 'randwrite' 'read' 'randread')
num_rw=${#rw[@]}

bs=('512B') # '1KB' '4KB' '16KB' '64KB')
num_bs=${#bs[@]}

directory='/home/hanshukai/mount/4800'

ioengine=libaio 

iodepth=8

runtime=10

log_avg_msec=5000

numjobs=4

random_distribution=random

filesize=80GB

nrfiles=1

time=$(date +%s)

mkdir $time

for ((i=0; i<${num_bs}; i+=1))
do
_bs=${bs[$i]}
for ((j=0; j<${num_rw}; j+=1))
do
_rw=${rw[$j]}
_dir=$time/${_bs}_${_rw}_${iodepth}_${numjobs}
echo $_dir
mkdir $_dir
_log=${_dir}/log
_shell="-name=wjob -rw=${_rw} -bs=${_bs} -fallocate=1 -thread -directory=${directory} -ioengine=${ioengine} -iodepth=${iodepth} -time_based -log_avg_msec=${log_avg_msec}\
 -write_bw_log=${_log} -write_iops_log=${_log} -write_lat_log=${_log} -numjobs=${numjobs} \
 -random_distribution=${random_distribution} -filesize=${filesize} -nrfiles=${nrfiles} --output=${_dir}/result.txt"
echo $_shell
fio $_shell
done
done