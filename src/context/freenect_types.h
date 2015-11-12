#ifndef FREENECT_TYPES
#define FREENECT_TYPES

#include <libfreenect2/libfreenect2.hpp>

namespace KineBot
{
typedef unsigned int uint;

typedef void(*FreenectFrameProcessFunction)(libfreenect2::Frame**,size_t);

/*!
 * \brief A structure containing the data for a Freenect device. Requires a valid Freenect device to be present on construction (selecting the default device) and for this device to initialize correctly. On destruction, this object will close the device.
 */
struct FreenectContext;

extern FreenectContext* freenect_alloc();

extern void freenect_launch_async(FreenectContext* kctxt);

extern void freenect_process_frame(FreenectContext* context, FreenectFrameProcessFunction fun, int numFrames);

extern void freenect_exit_async(FreenectContext* kctxt);

}

#endif
