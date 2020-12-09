SPDK_LINK_FLAGS := -Wl,--whole-archive -Lthird-party/dpdk -lspdk_env_dpdk  -lspdk_env_dpdk_rpc \
    -Lthird-party/spdk -ldpdk -lspdk_json -lspdk_jsonrpc -lspdk_log_rpc  -lspdk_app_rpc  -lspdk_rpc \
    -lspdk_bdev_rpc -lspdk_bdev_null -lspdk_bdev_malloc -lspdk_bdev_nvme -lspdk_bdev_zone_block -lspdk_bdev \
    -lspdk_event_bdev -lspdk_event_copy -lspdk_event_net -lspdk_event_vmd -lspdk_event \
    -lspdk_thread -lspdk_sock_posix -lspdk_sock -lspdk_notify -lspdk_net -lspdk_nvme \
    -lspdk_log -lspdk_trace -lspdk_util -lspdk_copy -lspdk_conf -lspdk_vmd

LINK_FLAGS := -Wl,--no-whole-archive -lpthread -lrt -lnuma -ldl -luuid -lm -lisal

all:
	g++ -std=c++11 -Iinclude -Iio -O0 io/main.cc io/posix_io_handle.cc io/spdk_io_handle.cc -o test $(SPDK_LINK_FLAGS) $(LINK_FLAGS)