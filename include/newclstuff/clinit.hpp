#define __CL_ENABLE_EXCEPTIONS
#include "/home/test/c/tests/cl/cl.hpp"
#include <iostream>
#include <fstream>

std::vector<cl::Device> devices;
cl::Context context;
cl::CommandQueue queue;

void initCL() {
	try {
		std::vector<cl::Platform> platforms;
		// create platform
		cl::Platform::get(&platforms);
		platforms[1].getDevices(CL_DEVICE_TYPE_ALL, &devices);
		// create context

		/*cl_context_properties props[] = 
			{
				CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(), 
				CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(), 
				CL_CONTEXT_PLATFORM, (cl_context_properties)platforms[1](), 
				0};
				context = cl::Context(devices, props);*/
		context = cl::Context(devices);
		// create command queue
		queue = cl::CommandQueue(context, devices[0]);
	} catch (cl::Error e) {
		std::cout << std::endl << e.what() << " : " << e.err() << std::endl;
	}
}

cl::Program createProgram(std::string name, const char*args = NULL) {
	std::ifstream cl_file(name.c_str());
	std::string cl_string(std::istreambuf_iterator<char>(cl_file), (std::istreambuf_iterator<char>()));
	cl::Program::Sources source(1, std::make_pair(cl_string.c_str(), cl_string.length() + 1));
	
	// create program
	cl::Program program(context, source);
	// compile opencl source
	try {
		program.build(devices, args);
	} catch (cl::Error&e) {
		//std::cout << "Build Status: " << program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(devices[0]) << std::endl;
		//std::cout << "Build Options:\n" << program.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(devices[0]) << std::endl;
		std::cout << "Build Error:\n" << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]) << std::endl;
		//throw std::runtime_error(std::string("Build Error:")+program.getBuildInfo<CL_PROGRAM_BUILD_LOG >(devices[0]));		
		}
	return program;
}
