#define __CL_ENABLE_EXCEPTIONS
#include "CL/cl.hpp"
#include <iostream>
#include <fstream>
using namespace std;

static std::vector<cl::Device> devices;
static cl::Context context;
static cl::CommandQueue queue;

void initCL() {
  try {
    std::vector<cl::Platform> platforms;
    // create platform
    cl::Platform::get(&platforms);
    platforms[0].getDevices(CL_DEVICE_TYPE_ALL, &devices);
    // create context
    context = cl::Context(devices);
    // create command queue
    queue = cl::CommandQueue(context, devices[0]);
  } catch (cl::Error e) {
    std::cout << std::endl << e.what() << " : " << e.err() << std::endl;
  }
}

cl::Program createProgram(std::string name) {
  std::ifstream cl_file(name.c_str());
  std::string cl_string(std::istreambuf_iterator<char>(cl_file), (std::istreambuf_iterator<char>()));
  cl::Program::Sources source(1, std::make_pair(cl_string.c_str(), cl_string.length() + 1));
  // create program
  cl::Program program(context, source);
  // compile opencl source
  try {
    program.build(devices);
  } catch (cl::Error&e) {
    //std::cout << "Build Status: " << program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(devices[0]) << std::endl;
    //std::cout << "Build Options:\n" << program.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(devices[0]) << std::endl;
    std::cout << "Build Error:\n" << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]) << std::endl;
    //throw std::runtime_error(std::string("Build Error:")+program.getBuildInfo<CL_PROGRAM_BUILD_LOG >(devices[0]));		
  }
  return program;
}
