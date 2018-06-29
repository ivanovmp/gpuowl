#pragma once

#include "stats.h"
#include "clwrap.h"
#include "common.h"

#include <string>
#include <vector>
#include <memory>

class Kernel {
  Holder<cl_kernel> kernel;
  cl_queue queue;
  int workSize;
  int nArgs;
  std::string name;
  std::vector<std::string> argNames;
  u64 timeSum;
  u64 nCalls;
  bool doTime;
  int groupSize;
  Stats stats;

  int getArgPos(const std::string &name) {
    for (int i = 0; i < nArgs; ++i) { if (argNames[i] == name) { return i; } }
    return -1;
  }

  template<typename T> void setArgs(int pos, const T &arg) { ::setArg(kernel.get(), pos, arg); }
  
  template<typename T, typename... Args>
  void setArgs(int pos, const T &arg, Args &...tail) {
    setArgs(pos, arg);
    setArgs(pos + 1, tail...);
  }

  // void setArg(int pos, const Buffer &buf) { setArg(pos, buf.get()); }  

  
public:
  Kernel(cl_program program, cl_queue q, cl_device_id device, int workSize, const std::string &name, bool doTime) :
    kernel(makeKernel(program, name.c_str())),
    queue(q),
    workSize(workSize),
    nArgs(getKernelNumArgs(kernel.get())),
    name(name),
    doTime(doTime),
    groupSize(getWorkGroupSize(kernel.get(), device, name.c_str()))
  {
    assert((workSize % groupSize == 0) || (log("%s\n", name.c_str()), false));
    assert(nArgs >= 0);
    for (int i = 0; i < nArgs; ++i) { argNames.push_back(getKernelArgName(kernel.get(), i)); }
  }

  template<typename... Args>
  void operator()(Args &...args) {
    setArgs(0, args...);
    if (doTime) {
      finish(queue);
      Timer timer;
      run(queue, kernel.get(), groupSize, workSize, name);
      finish(queue);
      stats.add(timer.deltaMicros());
    } else {
      run(queue, kernel.get(), groupSize, workSize, name);
    }
  }
  
  string getName() { return name; }

  /*
  template<typename T> void setArg(const std::string &name, const T &arg) {
    int pos = getArgPos(name);
    if (pos < 0) { log("setArg '%s' on '%s'\n", name.c_str(), this->name.c_str()); }
    setArg(pos, arg);
    // assert(pos >= 0);
    // ::setArg(kernel.get(), pos, arg);
  }
  */
  
  // void setArg(const std::string &name, const Buffer &buf) { setArg(name, buf.get()); }
  
  // void finish() { queue.finish(); }

  StatsInfo resetStats() {
    StatsInfo ret = stats.getStats();
    stats.reset();
    return ret;
  }
};
