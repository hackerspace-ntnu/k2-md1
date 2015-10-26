#ifndef FREENECT_TYPES
#define FREENECT_TYPES

#include <stdexcept>

#include <stdio.h>

#include <string>
#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/frame_listener_impl.h>

namespace KineBot
{
typedef unsigned int uint;

/*!
 * \brief A structure containing the data for a Freenect device. Requires a valid Freenect device to be present on construction (selecting the default device) and for this device to initialize correctly. On destruction, this object will close the device.
 */
struct FreenectContext
{
    FreenectContext();
    ~FreenectContext();

    libfreenect2::Freenect2 manager;
    libfreenect2::Freenect2Device* device;

    libfreenect2::SyncMultiFrameListener* listener;
};

class FreenectListener : public libfreenect2::FrameListener
{
    public:
        bool onNewFrame(libfreenect2::Frame::Type type, libfreenect2::Frame *frame);
};

}

#endif
