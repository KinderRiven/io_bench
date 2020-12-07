#ifndef INCLUDE_GEN_WORKLOAD_H_
#define INCLUDE_GEN_WORKLOAD_H_

namespace io_bench {

class Workload {
public:
};

class DBBenchWorkload : public Workload {
public:
    DBBenchWorkload();
};

class YCSBWorkload : public Workload {
public:
    YCSBWorkload();
};
};

#endif