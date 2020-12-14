rw=('write' 'randwrite' 'read' 'randread')
num_rw=${#rw[@]}

# bs=('512B' '1KB' '4KB' '16KB' '64KB')
bs=('1KB' '4KB' '16KB')
num_bs=${#bs[@]}

iodepth=(1 4 16 32)
num_iodepth=${#iodepth[@]}

numjobs=(1 2 4 8)
num_numjobs=${#numjobs[@]}

directory='/home/hanshukai/mount/4800'

ioengine=libaio 

runtime=600

log_avg_msec=1000

random_distribution=random

# GB
filesize=80

nrfiles=1

time=$(date +%s)

mkdir $time

for ((i=0; i<${num_bs}; i+=1))
do
_bs=${bs[$i]}
for ((j=0; j<${num_rw}; j+=1))
do
_rw=${rw[$j]}
for ((k=0; k<${num_numjobs}; k++))
do
_numjobs=${numjobs[$k]}
_filesize=`expr $filesize / $_numjobs`
for ((l=0; l<${num_iodepth}; l++))
do
_iodepth=${iodepth[$l]}
_dir=$time/${_bs}_${_rw}_${_numjobs}_${_iodepth}
echo $_dir
mkdir $_dir
_log=${_dir}/log
_shell="-name=wjob -rw=${_rw} -direct=1 -bs=${_bs} -fallocate=1 -thread -directory=${directory} -ioengine=${ioengine} -iodepth=${_iodepth} -time_based --runtime=${runtime} -log_avg_msec=${log_avg_msec}\
 -write_bw_log=${_log} -write_iops_log=${_log} -write_lat_log=${_log} -numjobs=${_numjobs} \
 -random_distribution=${random_distribution} -filesize=${_filesize}GB -nrfiles=${nrfiles} --output=${_dir}/result.txt"
echo $_shell
fio $_shell
done
done
done
done