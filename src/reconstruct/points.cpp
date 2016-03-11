#include <iostream>

#include <future>
#include <vector>
#include <functional>

#include <sys/mman.h>
#include <sys/stat.h>

#define MULTITHREADED_OPTIMIZATIONS

#include "../../deps/bat/bat.hpp"
#include "reconstruct/trackball.hpp"
#include "reconstruct/dataio.hpp"

struct matrix_unit
{
    float rotmat[9];
    float pos[3];
};

#ifdef MULTITHREADED_OPTIMIZATIONS
template<typename T>
inline void parallel_for(T punits,T datapoints,std::function<void(T,T)> fun)
{
    std::vector<std::future<void>> futuristic;

    T rest = datapoints%punits;
    T unit_points = (datapoints-rest)/punits;

    T index = 0;

    for(T i=0;i<punits;i++)
    {
        T unit_actual_points = unit_points;
        if(i==0)
            unit_actual_points += rest;
        futuristic.push_back(std::async(std::launch::async,fun,index,index+unit_actual_points));
        index += unit_actual_points;
    }

    for(std::future<void>& f : futuristic)
        f.get();
}
#endif


using namespace std;

int N_FRAMES = 50;
#define USE_COLOR 1

struct Point {
    Color col;
    float x, y, z;
    Point() {}
    Point(float x_, float y_, float z_, Color&col) : x(x_),y(y_),z(z_),col(col) {}
};

int showall = 0, showframe = 0, frame = 0;

void renderPoints(Surface&sf, float *zbuf, Point const*point, int const& points, vec3 const& pos, vec3 const& side, vec3 const& up, vec3 const& viewdir, float const&pers) {
#ifdef MULTITHREADED_OPTIMIZATIONS
    const int core_count = 64;

    std::function<void(int,int)> clear_framebuffer = [sf,zbuf](int start ,int end)
    {
        for(int i=start;i<end;i++)
        {
            sf.pixels[i] = 0x503080;
            zbuf[i] = 1e9;
        }
    };

    parallel_for(core_count,sf.w*sf.h,clear_framebuffer);

#else
    for (int i = 0; i < sf.w*sf.h; i++) {
        sf.pixels[i] = 0x503080;
        zbuf[i] = 1e9;
    }
#endif

    float ccx = sf.w*.5-.5, ccy = sf.h*.5-.5;
    int inc = !showall*19+1;

#ifdef MULTITHREADED_OPTIMIZATIONS

    std::function<void(int,int)> draw_fun = [ccx,ccy,sf,zbuf,point,pos,side,up,viewdir,pers,inc](int start, int end)
    {
        for(int i=start;i<end;i+=inc)
        {
            float px = point[i].x, py = point[i].y, pz = point[i].z;

            px -= pos.x;
            py -= pos.y;
            pz -= pos.z;
            float x = side.x*px+side.y*py+side.z*pz;
            float y = up.x*px+up.y*py+up.z*pz;
            float z = viewdir.x*px+viewdir.y*py+viewdir.z*pz, iz = 1.f/z;
            int ix = x*pers*iz+ccx;
            int iy = y*pers*iz+ccy;
            if (z > 0 and ix >= 0 and iy >= 0 and ix < sf.w and iy < sf.h and zbuf[ix+iy*sf.w] > z) {
                zbuf[ix+iy*sf.w] = z;
                sf.pixels[ix+iy*sf.w] = point[i].col;//(int)(sin(z/5)*127+127)*0x10101;
            }
        }
    };

    parallel_for(core_count,points,draw_fun);

#else
    for (int i = 0; i < points; i += inc) {
        float px = point[i].x, py = point[i].y, pz = point[i].z;

        px -= pos.x;
        py -= pos.y;
        pz -= pos.z;
        float x = side.x*px+side.y*py+side.z*pz;
        float y = up.x*px+up.y*py+up.z*pz;
        float z = viewdir.x*px+viewdir.y*py+viewdir.z*pz, iz = 1.f/z;
        int ix = x*pers*iz+ccx;
        int iy = y*pers*iz+ccy;
        if (z > 0 and ix >= 0 and iy >= 0 and ix < sf.w and iy < sf.h and zbuf[ix+iy*sf.w] > z) {
          zbuf[ix+iy*sf.w] = z;
          sf.pixels[ix+iy*sf.w] = point[i].col;//(int)(sin(z/5)*127+127)*0x10101;
        }
    }
#endif

}

int main(int argc, char**argv) {
    if (argc > 1) {
        sscanf(argv[1], "%d", &N_FRAMES);
    } else {
        cout << "Using " << N_FRAMES << " frames by default, change by running " << argv[0] << " <N frames>" << endl;
    }


    int sw = 1920/2, sh = 1080/2;
    int Iw = 1920/2, Ih = 1080/2;
    int Dw = 512, Dh = 424;
    MyScreen screen(sw, sh);
    Surface sf(sw, sh);
    Clock clock;

    float*zbuf = new float[sw*sh];
    Surface I(Dw, Dh);

    float ccx = Iw*.5-.5, ccy = Ih*.5-.5, pers = 1081.37f/2;
    float Dcx = 255.559, Dcy = 207.671, Dpers = 365.463;

    float rotmat[9];
    float px, py, pz;

//    FILE*fp = fopen("views.txt", "r");

    /* Get a file handle for the matrix file */
    int view_fd = open("views.txt",O_RDONLY);

    /* Terminate in error if not found */
    if(view_fd<=0)
        exit(-1);

    size_t view_size = 0;
    {
        struct stat view_s;
        stat("views.txt",&view_s);
        view_size = view_s.st_size;
    }

    /* Memory map the matrix file */
    void* view_ptr = mmap(nullptr,view_size,PROT_READ,MAP_PRIVATE|MAP_POPULATE,view_fd,0);

    const char* matrixdata_source = (const char*)view_ptr;

    if(!view_ptr)
        exit(-1);

    /* Matrix entry processor, parses the text into matrices */
    std::function<void(const char*,size_t,matrix_unit&)> matrix_parse = [](
            const char* data_offset,
            size_t max_size,
            matrix_unit& output)
    {
        size_t i=0,j=0,k=0;

        /* Read tokens into memory reference given to us */
        const char* t1; const char* t2 = data_offset;
        for(j=0;j<3;j++)
        {
            for(k=0;k<3;k++)
            {
                t1 = strstr(t2," ");
                output.rotmat[j*3+k] = strtof(t1,nullptr);

                /* Parsing error */
                if(t1>t2)
                    break;
            }
            output.pos[k] = strtof(t1,nullptr);
            t2 = strstr(t2,"\n");
        }
    };

    /* Allocate space for matrix data */
    std::vector<matrix_unit> matrix_data_backing;
    matrix_data_backing.resize(N_FRAMES);

    matrix_unit* matrix_data = &matrix_data_backing[0];

    /* Matrix file processor, delegates task to the function above */
    std::function<void(size_t,size_t)> matrix_processor = [matrix_data,view_size,matrixdata_source,matrix_parse](size_t start, size_t end)
    {
        const char* data_source = matrixdata_source;

        for(size_t i=start;i<end;i++)
        {
            /* We jump to an offset in the file, where we find the textual representation of a matrix */
            const char* data_offset = data_source;
            while(i<i*5&&data_offset&&(data_offset-data_source)<view_size)
            {
                data_offset = strstr(data_offset,"\n");
                i++;
            }
            matrix_parse(data_offset,view_size,matrix_data[i]);
        }
    };

    parallel_for(64UL,(long unsigned)N_FRAMES,matrix_processor);

    float ifx = 1.f/Dpers;
    int points = 0;
    Point*point = new Point[300000000];
    int*pstart = new int[N_FRAMES+1];
    for (int i = 0; i < N_FRAMES; i++) {
        float dx, dy, dz, dxyz[3];

        matrix_unit& mdata = matrix_data[i];
        dx = mdata.pos[0];
        dy = mdata.pos[1];
        dz = mdata.pos[2];
        //if (i != 0 and i != 70) continue;

        //cout << dx << ' ' << dy << ' ' << dz << endl;
        //for (int k = 0; k < 9; k++) cout << rotmat[k] << endl;

        loadZB(zbuf, Dw, Dh, i);
#if defined USE_COLOR
        loadSurface(I, Dw, Dh, i);
#endif

        pstart[i] = points;
        for (int l = 0; l < Dh; l++) {
            for (int k = 0; k < Dw; k++) {
                float tz = zbuf[k+l*Dw];
                //if (tz < 500 && tz > 10) cout << tz << endl;
                if (tz > 1e8) continue;
                float tx = (k-Dcx)*tz*ifx, ty = (l-Dcy)*tz*ifx;
                float x = tx*mdata.rotmat[0]+ty*mdata.rotmat[1]+tz*mdata.rotmat[2]+dx*1000.f;
                float y = tx*mdata.rotmat[3]+ty*mdata.rotmat[4]+tz*mdata.rotmat[5]+dy*1000.f;
                float z = tx*mdata.rotmat[6]+ty*mdata.rotmat[7]+tz*mdata.rotmat[8]+dz*1000.f;
#if defined USE_COLOR
                Color col = I.pixels[k+l*Dw];
                if (col == 0) continue;
#else
                Color col = 0xffffff;
#endif	//if (rand()%10 == 0) 
                point[points++] = Point(x, y, z, col);
            }
        }
    }
    pstart[N_FRAMES] = points;
    //cout << points << endl;
    munmap(view_ptr,view_size);

    MyTrackball track(screen);
    track.speed = 30;

    while (1) {
        while (screen.gotEvent()) {
            Event e = screen.getEvent();
            if (e.type == KeyPress and e.key == K_ESCAPE) return 0;
            else if (e.type == KeyPress) {
                if (e.key == K_SPACE) showall ^= 1;
                else if (e.key == K_TAB) showframe ^= 1;
                else if (e.key == K_LEFT) frame -= 1;
                else if (e.key == K_RIGHT) frame += 1;
            }
            track.handleEvent(e);
        }
        track.update();
        vec3 side = up^viewdir, pos = track.pos;

        if (showframe) {
            frame = (frame%N_FRAMES+N_FRAMES)%N_FRAMES;
            renderPoints(sf, zbuf, point+pstart[frame], pstart[frame+1]-pstart[frame], pos, side, up, viewdir, pers);
        } else
            renderPoints(sf, zbuf, point, points, pos, side, up, viewdir, pers);
        //for (int i = 0; i < sw*sh; i += 2)
        //	sf.pixels[i] = I[i]*0x10101;
        screen.putSurface(sf);
        clock.tick(30);
    }
}

