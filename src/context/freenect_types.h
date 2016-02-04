#ifndef FREENECT_TYPES
#define FREENECT_TYPES

#include <cstddef>

namespace KineBot
{
using uint = unsigned int;
using uint8 = unsigned char;

template<typename T>
struct vec2
{
    vec2(T x, T y):x(x),y(y){}
    union{
        T x;
        T u;
    };
    union{
        T y;
        T v;
    };
};

template<typename T>
struct vec3
{
    union{
        T x;
        T r;
    };
    union{
        T y;
        T g;
    };
    union{
        T z;
        T b;
    };
};

struct rgb
{
    union{
        struct
        {
            uint8 r;
            uint8 g;
            uint8 b;
        };
        uint i;
    };
};

struct ksize
{
    uint w;
    uint h;
};

struct ColorDepthFrame
{
    ksize d;
    ksize c;
    float* depth;
    rgb* color;

    rgb& get_color(vec2<int> const& v)
    {
        return color[v.y*c.w+v.x];
    }
    float& get_depth(vec2<int> const& v)
    {
        return depth[v.y*d.w+v.x];
    }
};

using FreenectFrameProcessFunction = void(*)(ColorDepthFrame&);

/*!
 * \brief A structure containing the data for a Freenect device.
 * Requires a valid Freenect device to be present on construction (selecting the default device)
 *  and for this device to initialize correctly. On destruction, this object will close the device.
 */
struct FreenectContext;

extern FreenectContext* freenect_alloc();
extern void freenect_free(FreenectContext* kctxt);

extern void freenect_launch_async(FreenectContext* kctxt);

extern void freenect_process_frame(FreenectContext* context, FreenectFrameProcessFunction fun, size_t numFrames);

extern void freenect_exit_async(FreenectContext* kctxt);

}

#endif
