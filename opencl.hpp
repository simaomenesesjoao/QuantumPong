#include <CL/opencl.hpp>
typedef cl_float2 float2;
typedef cl_int4 int4;

class kpm {
    public:
    //private:
    std::vector<cl::Platform> platforms;
    cl::Platform platform;

    std::vector<cl::Device> devices;
    cl::Device device;

    cl::Context context;
    cl::Kernel cheb1, cheb2, colormap, copy, kreset, set_sq, kset_local_pot, kclear_local_pot, kabsorb;

    cl::CommandQueue queue;

    cl::NDRange offset;
    cl::NDRange global_size;
    cl::NDRange local_size;

    bool geometry_init, window_init;

    cl::Buffer hops_buf, input_buf, output_buf, acc_buf, pix_buf, score_buf;
    cl::Buffer scale_buf, max_buf, val_buf;
    std::vector<cl::Buffer> jn_bufs;
    unsigned Ncheb;


    unsigned Lx, Ly, pad, local;
    unsigned Ncells, N;

    unsigned width, height;
    unsigned Npixels;

    unsigned Nhops;
    float SCALE, max, val;
    float psi2_top, psi2_bot;
    float norm_top, norm_bot;

    unsigned top_player_x, bot_player_x, top_player_prev_x, bot_player_prev_x;
    unsigned top_player_y, bot_player_y, top_player_prev_y, bot_player_prev_y;
    unsigned paddle_width, paddle_height;

    // Methods
    void set_geometry(unsigned, unsigned);
    void init_cl();

    void init_geometry(unsigned Lx, unsigned Ly, unsigned pad, unsigned local);
    void init_window(unsigned width, unsigned height);
    void set_hamiltonian_sq();
    void set_local_pot(unsigned, unsigned, unsigned, unsigned, float);

    void init_kernels();
    void init_buffers();

    void set_H();

    void initialize_wf(unsigned i, unsigned j, float kx, float ky, float broad);
    void initialize_tevop(float dt, unsigned Ncheb);
    void iterate_time(unsigned);
    void update_pixel(char*);
    float get_norm(float*);
    void set_max(float);
    void absorb();

    void init_paddles(float, float, float, float, float, float);
    void reset_state();
    void update_paddles(float, float, float, float);
    
};


void print_arr(float2 *arr, unsigned Lx, unsigned Ly, unsigned offset, int flag);
void print_arr(int4 *arr, unsigned Lx, unsigned Ly, unsigned offset, int flag);

