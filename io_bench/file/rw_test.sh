# ./readwrite [device_mount_path] [device_capcity|GB] [n_rthread] [r_bs|B] [n_wthread] [w_bs|B] [time|sec]
run_time=60
./readwrite /home/hanshukai/dir1 1740 0 4096 1 4096 $((run_time))
./readwrite /home/hanshukai/dir1 1740 1 4096 0 4096 $((run_time))
./readwrite /home/hanshukai/dir1 1740 1 4096 1 4096 $((run_time))
./readwrite /home/hanshukai/dir1 1740 1 4096 1 $((16*1024)) $((run_time))
./readwrite /home/hanshukai/dir1 1740 1 4096 1 $((64*1024*1024)) $((run_time))