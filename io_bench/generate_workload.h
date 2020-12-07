#ifndef INCLUDE_GEN_WORKLOAD_H_
#define INCLUDE_GEN_WORKLOAD_H_

#include "db_bench.h"
#include "header.h"

namespace io_bench {

class Workload {
public:
    virtual uint64_t Get() = 0;
};

class DBBenchWorkload : public Workload {
public:
    DBBenchWorkload(uint32_t seed)
    {
        random_ = new Random(seed);
    }

    ~DBBenchWorkload()
    {
    }

public:
    uint64_t Get()
    {
        uint64_t _t = (uint64_t)random_->Next();
        return _t;
    }

private:
    Random* random_;
};

class YCSBWorkload : public Workload {
public:
    YCSBWorkload()
    {
    }

    ~YCSBWorkload()
    {
    }

public:
    uint64_t Get()
    {
        return 0;
    }
};
};

#endif