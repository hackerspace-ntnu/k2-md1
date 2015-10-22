#ifndef FREENECT_TYPES
#define FREENECT_TYPES

#include <stdio.h>

#include <string>
#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/frame_listener_impl.h>

namespace KineBot
{
typedef unsigned int uint;

struct FreenectContext
{
    FreenectContext(uint filter);
    ~FreenectContext();

    libfreenect2::Freenect2 manager;
    libfreenect2::Freenect2Device* device;
};

class FreenectListener : public libfreenect2::FrameListener
{
public:
    bool onNewFrame(libfreenect2::Frame::Type type, libfreenect2::Frame *frame);
};

}

#endif
