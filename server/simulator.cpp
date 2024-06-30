// The GPU device I'm using uses openCL v2.1
#define CL_HPP_TARGET_OPENCL_VERSION 210

#include <unistd.h>
#include <complex>
#include <fstream>
#include <chrono>
// #include <mutex>
#include <iostream>
// #include <CL/opencl.hpp>
// #include <algorithm>
#include "../macros.hpp"
// #include "event_queue.hpp"
#include "simulator.hpp"

simulator::simulator(event_queue *evq){
    eq = evq;
}

void simulator::init_cl(){
    // If showcase is true, then several behaviours change, for cinematic reasons
    showcase = false;
    pressed_showcase = false;


    // Get the platforms
    cl::Platform::get(&platforms);
    if(platforms.size() == 0){
        std::cout << "No platforms were found. Exiting.\n";
        exit(1);
    }

    // Choose a platform
    platform = platforms[0];
    std::cout << "Using platform: " << platform.getInfo<CL_PLATFORM_NAME>() << "\n";

    // Get the devices
    platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
    if(devices.size() == 0){
        std::cout << "This platform has no devices. Exiting.\n";
        exit(2);
    }

    // Choose a device
    device = devices[0];
    std::cout << "Using device: " << device.getInfo<CL_DEVICE_NAME>() << "\n";
    std::cout << "driver version: " << device.getInfo<CL_DRIVER_VERSION>() << "\n";

    context = cl::Context(device);


    queue = cl::CommandQueue(context, device);
}

void simulator::init_geometry(unsigned lx, unsigned ly, unsigned p, unsigned loc){
    geometry_init = true;
    Lx = lx;
    Ly = ly;
    pad = p;
    Ncells = (Lx+2*pad)*(Ly+2*pad);
    N = Lx*Ly;
    local = loc;
    //std::cout << "initializing geometry" << Lx << " " << Ly << " " << Ncells << "\n";

}

void simulator::init_window(unsigned width, unsigned height){
    window_init = true;
    Npixels = width*height;
}

void simulator::set_hamiltonian_sq(){
    Nhops = 5;
    SCALE = 4.1;

}

void simulator::init_buffers(){
    if(!geometry_init){
        std::cout << "Geometry must be initialized before buffers are initialized. Exiting.\n";
        exit(1);
    }

    if(!window_init){
        std::cout << "Window dimensions must be specified before buffers are initialized. Exiting.\n";
        exit(2);
    }

    std::cout << "Initializing GPU buffers. Ncells: " << Ncells <<  " N: ";
    std::cout << N << " Npixels: " << Npixels << " " << Nhops << "\n";

    input_buf  = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(float2)*Ncells);
    output_buf = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(float2)*Ncells);
    acc_buf    = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(float2)*Ncells);
    score_buf  = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(float)*Ncells);

    hops_buf   = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(float2)*N*Nhops);
    pix_buf    = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(int4)*Npixels);

    scale_buf    = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(float));
    changed_buf  = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(bool));
    max_buf      = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(float));

    val_buf    = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(float));
    valB_buf   = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(float));
    mag_buf    = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(float)*Ncells);
    pot_buf    = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(float)*N);

    max = 1.0;
    valB = 0.0;
    val = 0.0;
    radB = 200;

    queue.enqueueWriteBuffer(scale_buf, CL_TRUE, 0, sizeof(float), &SCALE);
    queue.enqueueWriteBuffer(max_buf,   CL_TRUE, 0, sizeof(float), &max);
    queue.enqueueWriteBuffer(valB_buf,  CL_TRUE, 0, sizeof(float), &valB);
    queue.enqueueWriteBuffer(val_buf,   CL_TRUE, 0, sizeof(float), &val);

    potential_changed = false;
    absorb_on = true;
    // running = true;
    modifier = 1.0;


    // Initialize score to zero
    float  *score = new float[Ncells];
    for(unsigned n=0; n<Ncells; n++){
        score[n] = 0;
    }
    queue.enqueueWriteBuffer(score_buf, CL_TRUE, 0, sizeof(float)*Ncells, score);
    queue.enqueueWriteBuffer(mag_buf,   CL_TRUE, 0, sizeof(float)*Ncells, score);
    queue.enqueueWriteBuffer(pot_buf,   CL_TRUE, 0, sizeof(float)*N, score);
    delete[] score;

    buffer_size = N; 
    buffer_f_size = N+HEADER_LEN;

    buffer_f = new uint8_t[buffer_f_size];
    buffer = buffer_f+HEADER_LEN;

    for(unsigned i=0; i<buffer_size; i++){
        buffer[i] = 0;
    }
    

}

void simulator::finalize(){
    delete[] buffer_f;
}

void simulator::init_kernels(){
    if(!geometry_init){
        std::cout << "Geometry must be initialized before kernels are compiled. Exiting.\n";
        exit(1);
    }

    //if(!window_init){
        //std::cout << "Window dimensions must be specified before kernels are compiled. Exiting.\n";
        //exit(2);
    //}

    // Load the program from a file and into a string
    // The second line is saying to create an iterator from the stream, 
    // and to stop at the default constructed iterator end.
    // The parenthesis inside the second argument are necessary, because otherwise
    // the compiler thinks I'm declaring a function which returns a string,
    // instead of declaring a string!
    std::ifstream programFile("kernel.cpp");
    std::string programString(std::istreambuf_iterator<char>(programFile), (std::istreambuf_iterator<char>()));

    // Print out the kernel string

    // Build the program. The build method requires the sources to be inside a vector
    std::vector<std::string> sourceStrings;
    sourceStrings.push_back({programString.c_str(), programString.length()});

    cl::Program::Sources source(sourceStrings);
    cl::Program program(context, source);

    // Compile the program, and pass compilation options to it
    char options[50];
    //std::cout << "Lx,Ly,pad,nhops" << Lx << " " << Ly << " " << pad << " " << Nhops << "\n";
    sprintf(options, "-DLX=%d -DLY=%d -DPAD=%d -DNHOPS=%d", Lx, Ly, pad, Nhops);
    std::cout << "options: " << options << "\n";
    if(program.build({device}, options) != CL_SUCCESS){
        std::cout << "Error building: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device) << "\n";
        exit(1);
    }


    // create the kernel
    set_sq = cl::Kernel(program, "set_sq");
    std::cout << "created kernel: " << set_sq.getInfo<CL_KERNEL_FUNCTION_NAME>() << "\n";

    set_sq_B = cl::Kernel(program, "set_sq_B");
    std::cout << "created kernel: " << set_sq_B.getInfo<CL_KERNEL_FUNCTION_NAME>() << "\n";

    kset_local_pot = cl::Kernel(program, "set_local_pot");
    std::cout << "created kernel: " << kset_local_pot.getInfo<CL_KERNEL_FUNCTION_NAME>() << "\n";

    kset_local_pot_rect = cl::Kernel(program, "set_local_pot_rect");
    std::cout << "created kernel: " << kset_local_pot_rect.getInfo<CL_KERNEL_FUNCTION_NAME>() << "\n";

    kset_local_B = cl::Kernel(program, "set_local_B");
    std::cout << "created kernel: " << kset_local_B.getInfo<CL_KERNEL_FUNCTION_NAME>() << "\n";

    kclear_local_pot = cl::Kernel(program, "clear_local_pot");
    std::cout << "created kernel: " << kclear_local_pot.getInfo<CL_KERNEL_FUNCTION_NAME>() << "\n";

    cheb1 = cl::Kernel(program, "cheb1");
    std::cout << "created kernel: " << cheb1.getInfo<CL_KERNEL_FUNCTION_NAME>() << "\n";

    cheb2 = cl::Kernel(program, "cheb2");
    std::cout << "created kernel: " << cheb2.getInfo<CL_KERNEL_FUNCTION_NAME>() << "\n";

    colormap = cl::Kernel(program, "colormap");
    std::cout << "created kernel: " << colormap.getInfo<CL_KERNEL_FUNCTION_NAME>() << "\n";

    colormapV = cl::Kernel(program, "colormapV");
    std::cout << "created kernel: " << colormapV.getInfo<CL_KERNEL_FUNCTION_NAME>() << "\n";

    colormapB = cl::Kernel(program, "colormapB");
    std::cout << "created kernel: " << colormapB.getInfo<CL_KERNEL_FUNCTION_NAME>() << "\n";

    copy = cl::Kernel(program, "copy");
    std::cout << "created kernel: " << copy.getInfo<CL_KERNEL_FUNCTION_NAME>() << "\n";
    
    kreset = cl::Kernel(program, "reset");
    std::cout << "created kernel: " << kreset.getInfo<CL_KERNEL_FUNCTION_NAME>() << "\n";

    kabsorb = cl::Kernel(program, "absorb");
    std::cout << "created kernel: " << kabsorb.getInfo<CL_KERNEL_FUNCTION_NAME>() << "\n";

    set_sq.setArg(0,  hops_buf);
    set_sq.setArg(1,  scale_buf);

    set_sq_B.setArg(0,  hops_buf);
    set_sq_B.setArg(1,  scale_buf);
    set_sq_B.setArg(2,  mag_buf);
    set_sq_B.setArg(3,  pot_buf);

    cheb1.setArg(0,  input_buf);
    cheb1.setArg(1, output_buf);
    cheb1.setArg(2,   hops_buf);
    cheb1.setArg(3,    acc_buf);

    cheb2.setArg(2,   hops_buf);
    cheb2.setArg(3,    acc_buf);

    colormap.setArg(0, acc_buf);
    colormap.setArg(1, pix_buf);
    colormap.setArg(2, max_buf);

    colormapV.setArg(0, pot_buf);
    colormapV.setArg(1, pix_buf);

    colormapB.setArg(0, mag_buf);
    colormapB.setArg(1, pix_buf);

    copy.setArg(0,     acc_buf); // from
    copy.setArg(1,   input_buf); // to

    kreset.setArg(0,    acc_buf);

    kabsorb.setArg(0, input_buf);
    kabsorb.setArg(1, score_buf);

    kset_local_pot.setArg(0, pot_buf);
    kset_local_pot.setArg(1, val_buf);
    kset_local_pot.setArg(2, changed_buf);

    kset_local_pot_rect.setArg(0, hops_buf);
    kset_local_pot_rect.setArg(1, pot_buf);
    kset_local_pot_rect.setArg(2, val_buf);
    kset_local_pot_rect.setArg(3, scale_buf);
    kset_local_pot_rect.setArg(4, changed_buf);

    kclear_local_pot.setArg(0, hops_buf);
    kclear_local_pot.setArg(1, pot_buf);
    kclear_local_pot.setArg(2, changed_buf);

    kset_local_B.setArg(0, mag_buf);
    kset_local_B.setArg(1, valB_buf);


}

void simulator::init_paddles(int pt_x, int pt_y, int pb_x, int pb_y, int paddle_w, int paddle_h){
    top_player_x = pt_x;
    top_player_prev_x = pt_x;
    bot_player_x = pb_x;
    bot_player_prev_x = pb_x;
    
    top_player_y = pt_y;
    top_player_prev_y = pt_y;
    bot_player_y = pb_y;
    bot_player_prev_y = pb_y;

    paddle_width = paddle_w;
    paddle_height = paddle_h;
}

void simulator::update_paddles(int pt_x, int pt_y, int pb_x, int pb_y){
    // std::cout << "Entered update_paddles\n" << std::flush;
    // Update the values of the paddle position

    cl::NDRange offset;
    cl::NDRange global_size;
    cl::NDRange local_size;
    
    if(pb_x < 0) pb_x = 0;
    if(pt_x < 0) pt_x = 0;
    if(pb_y < 0) pb_y = 0;
    if(pt_y < 0) pt_y = 0;

    top_player_prev_x = top_player_x;
    top_player_x = pt_x; 

    bot_player_prev_x = bot_player_x;
    bot_player_x = pb_x; 

    top_player_prev_y = top_player_y;
    top_player_y = pt_y;

    bot_player_prev_y = bot_player_y;
    bot_player_y = pb_y;



    // Apply the paddle constraints in the x and y directions
    if(pt_x <    paddle_width/2) top_player_x =    paddle_width/2; 
    if(pt_x > Lx-paddle_width/2) top_player_x = Lx-paddle_width/2;
    if(pb_x <    paddle_width/2) bot_player_x =    paddle_width/2; 
    if(pb_x > Lx-paddle_width/2) bot_player_x = Lx-paddle_width/2;

    if(pt_y <    paddle_height/2) top_player_y =    paddle_height/2; 
    if(pt_y > Ly-paddle_height/2) top_player_y = Ly-paddle_height/2;
    if(pb_y <    paddle_height/2) bot_player_y =    paddle_height/2; 
    if(pb_y > Ly-paddle_height/2) bot_player_y = Ly-paddle_height/2;

    // Use the event class to convert the data into the buffer
    
    Event<EV_STREAM> event(N, top_player_x, top_player_y, bot_player_x, bot_player_y);


    std::cout << "simulator - update_paddles: buffer_f\n";
    for(unsigned i=0; i<HEADER_LEN; i++){
        buffer_f[i] = event.buffer_b[i];
        std::cout << (int)buffer_f[i] << " ";
    }
    std::cout << "\n";

    global_size = cl::NDRange{(cl::size_type)paddle_width, (cl::size_type)paddle_height};
    local_size  = cl::NDRange{(cl::size_type)local, (cl::size_type)local};

    // Load the value of the potential and change status into the GPU
    float val = 4;
    potential_changed = false;
    queue.enqueueWriteBuffer(val_buf, CL_TRUE, 0, sizeof(float), &val);
    queue.enqueueWriteBuffer(changed_buf, CL_TRUE, 0, sizeof(bool), &potential_changed);

    // Update player on top. Remove potential from previous position and add potential to new one
    cl::size_type startx, starty;

    startx = top_player_prev_x - paddle_width/2;
    starty = top_player_prev_y - paddle_height/2;
    offset = cl::NDRange{startx, starty};
    queue.enqueueNDRangeKernel(kclear_local_pot, offset, global_size, local_size);
    
    startx = top_player_x - paddle_width/2;
    starty = top_player_y - paddle_height/2;
    offset = cl::NDRange{startx, starty};
    queue.enqueueNDRangeKernel(kset_local_pot_rect, offset, global_size, local_size);

    // same thing for the player on the bottom
    startx = bot_player_prev_x - paddle_width/2;
    starty = bot_player_prev_y - paddle_height/2;
    offset = cl::NDRange{startx, starty};
    queue.enqueueNDRangeKernel(kclear_local_pot, offset, global_size, local_size);

    startx = bot_player_x - paddle_width/2;
    starty = bot_player_y - paddle_height/2;
    offset = cl::NDRange{startx, starty};
    queue.enqueueNDRangeKernel(kset_local_pot_rect, offset, global_size, local_size);


    // CHANGE: Does the GPU finish the previous instructions before this runs?
    queue.enqueueReadBuffer(changed_buf, CL_TRUE, 0, sizeof(bool), &potential_changed);
    if(potential_changed){
        queue.enqueueWriteBuffer(changed_buf, CL_TRUE, 0, sizeof(bool), &potential_changed);
        std::cout << "CHANGED POTENTIAL\n";
        int dd = 10;
        int x0 = paddle_width/2;
        int y0 = paddle_height/2;

        int dx = paddle_width;
        int dy = paddle_height;

        int xb = bot_player_prev_x;
        if(bot_player_x  < bot_player_prev_x) xb  = bot_player_x;
        if(bot_player_x != bot_player_prev_x) dx += dd;

        int yb = bot_player_prev_y;
        if(bot_player_y  < bot_player_prev_y) yb  = bot_player_y;
        if(bot_player_y != bot_player_prev_y) dy += dd;

        Event<EV_SEND_POT> event1(xb-x0, yb-y0, dx, dy,
         top_player_x, top_player_y, bot_player_x, bot_player_y);
        // Event<EV_SEND_POT> event2(top_player_x-x0, top_player_y-y0, paddle_width, paddle_height);
        // Event<EV_STREAM> event(N, top_player_x, top_player_y, bot_player_x, bot_player_y);
        
        eq->add_event(event1.buffer_b);
        // eq->add_event(event2.buffer_b);
    }

    potential_changed = false;
}

void simulator::set_local_B(unsigned x, unsigned y, float v){
    valB = v;

    cl::NDRange offset;
    cl::NDRange global_size;
    cl::NDRange local_size;

    if(radB > 2*Ly+2*Lx) radB = 2*Ly+2*Lx;
    unsigned dx = radB;
    unsigned dy = radB;


    queue.enqueueWriteBuffer(valB_buf,   CL_TRUE, 0, sizeof(float), &valB);

    offset      = cl::NDRange{(cl::size_type)(x-dx/2), (cl::size_type)(y-dy/2)};
    global_size = cl::NDRange{(cl::size_type)(dx), (cl::size_type)(dy)};
    local_size  = cl::NDRange{(cl::size_type)(local), (cl::size_type)(local)};
    queue.enqueueNDRangeKernel(kset_local_B, offset, global_size, local_size);

    offset      = cl::NDRange{(cl::size_type)(0), (cl::size_type)(0)};
    global_size = cl::NDRange{(cl::size_type)(Lx), (cl::size_type)(Ly)};
    local_size  = cl::NDRange{(cl::size_type)(local), (cl::size_type)(local)};
    queue.enqueueNDRangeKernel(set_sq_B, offset, global_size, local_size);
}

void simulator::set_local_pot(unsigned x, unsigned y, unsigned dx, unsigned dy, float v){
    std::cout << "simulator: set_local_pot:\n";

    cl::NDRange offset;
    cl::NDRange global_size;
    cl::NDRange local_size;

    val = v;
    potential_changed = false;
    queue.enqueueWriteBuffer(val_buf,   CL_TRUE, 0, sizeof(float), &val);
    queue.enqueueWriteBuffer(changed_buf, CL_TRUE, 0, sizeof(bool), &potential_changed);

    offset      = cl::NDRange{(cl::size_type)(x-dx/2), (cl::size_type)(y-dy/2)};
    global_size = cl::NDRange{(cl::size_type)(dx), (cl::size_type)(dy)};
    local_size  = cl::NDRange{(cl::size_type)(local), (cl::size_type)(local)};
    queue.enqueueNDRangeKernel(kset_local_pot, offset, global_size, local_size);

    offset      = cl::NDRange{(cl::size_type)(0), (cl::size_type)(0)};
    global_size = cl::NDRange{(cl::size_type)(Lx), (cl::size_type)(Ly)};
    local_size  = cl::NDRange{(cl::size_type)(local), (cl::size_type)(local)};
    queue.enqueueNDRangeKernel(set_sq_B, offset, global_size, local_size);

     // CHANGE: Does the GPU finish the previous instructions before this runs?
    queue.enqueueReadBuffer(changed_buf, CL_TRUE, 0, sizeof(bool), &potential_changed);
    if(potential_changed){
        potential_changed = false;
        queue.enqueueWriteBuffer(changed_buf, CL_TRUE, 0, sizeof(bool), &potential_changed);
        std::cout << "CHANGED POTENTIAL\n";

        Event<EV_SEND_POT> event(x-dx/2,y-dy/2,dx,dy,
         top_player_x, top_player_y, bot_player_x, bot_player_y);
        
        eq->add_event(event.buffer_b);
    }
}

void simulator::initialize_pot_from(){

    cl::NDRange offset;
    cl::NDRange global_size;
    cl::NDRange local_size;

    // Initialize container to zeros
    float *text_pot = new float[N];
    for(int i=0; i<N; i++) text_pot[i] = 0;

    std::ifstream inputFile("other/qp_pot.txt");
    int num;
    int ncols = 350;
    int nrows = 200;
    int n;
    int offset_c = 30;
    int offset_r = 200;
    std::cout << "before loop\n" << std::flush;
    for(int r=0; r<nrows; r++){
        for(int c=0; c<ncols; c++){
            n = c+offset_c + Lx*(r+offset_r);
            inputFile >> num;
            text_pot[n] = -(float)num*4;
        }
    }
    std::cout << "after loop\n" << std::flush;
    queue.enqueueWriteBuffer(pot_buf,   CL_TRUE, 0, sizeof(float)*N, text_pot);
    delete[] text_pot;

    offset      = cl::NDRange{(cl::size_type)(0), (cl::size_type)(0)};
    global_size = cl::NDRange{(cl::size_type)(Lx), (cl::size_type)(Ly)};
    local_size  = cl::NDRange{(cl::size_type)(local), (cl::size_type)(local)};
    queue.enqueueNDRangeKernel(set_sq_B, offset, global_size, local_size);
    inputFile.close();
}

void simulator::clear_wf_away_from_pot(char *data, unsigned vis_width, unsigned vis_height){
    // Initialize container to zeros
    float *text_pot = new float[N];
    for(int i=0; i<N; i++) text_pot[i] = 0;

    std::ifstream inputFile("other/qp_pot.txt");
    int num;
    int ncols = 350;
    int nrows = 200;
    int n;
    int offset_c = 30;
    int offset_r = 200;
    for(int r=0; r<nrows; r++){
        for(int c=0; c<ncols; c++){
            n = c+offset_c + Lx*(r+offset_r);
            inputFile >> num;
            text_pot[n] = (float)num;
        }
    }
    inputFile.close();

    int m;
    for(int r = 0; r < Ly; r++){
        for(int c = 0; c < Lx; c++){
            m = r*vis_width + c;
            n = r*Lx + c;
            if(text_pot[n] < 0.5){
                data[4*m+0] = 0.0;
                data[4*m+1] = 0.0;
                data[4*m+2] = 0.0;
                data[4*m+3] = 0.0;
            }
        }
    }
    delete[] text_pot;

}

void simulator::reset_state(){

    // Initialize score to zero
    float  *score = new float[Ncells];
    for(unsigned n=0; n<Ncells; n++){
        score[n] = 0;
    }
    queue.enqueueWriteBuffer(score_buf, CL_TRUE, 0, sizeof(float)*Ncells, score);
    queue.enqueueWriteBuffer(mag_buf,   CL_TRUE, 0, sizeof(float)*Ncells, score);
    queue.enqueueWriteBuffer(pot_buf,   CL_TRUE, 0, sizeof(float)*N, score);
    delete[] score;

    absorb_on = true;
    // running = true;
    paused = true;
    modifier = 1.0;
    radB = 200;

    unsigned ix = Lx/2;
    unsigned iy = Ly/2;
    float kx = 0.5;
    float ky = 1.0;
    float broad = 20.0;
    int paddle_x = 100;
    int paddle_y_bot = 50;
    int paddle_y_top = Ly-50;
    int paddle_width = 100;
    int paddle_height = 20;

    set_H();
    initialize_wf(ix, iy, kx, ky, broad);
    init_paddles(paddle_x, paddle_y_top, paddle_x, paddle_y_bot, paddle_width, paddle_height);
    update_paddles(paddle_x, paddle_y_top, paddle_x, paddle_y_bot);

}

void simulator::set_H(){//float d, int length, int width){
    // d, length, width is for the potential, to be implemented later
    //queue.enqueueWriteBuffer(  hops_buf, CL_TRUE, 0, sizeof(float2)*Nhops*WIDTH*WIDTH, hops);

    cl::NDRange offset;
    cl::NDRange global_size;
    cl::NDRange local_size;

    offset      = cl::NDRange{(cl::size_type)(0), (cl::size_type)(0)};
    global_size = cl::NDRange{(cl::size_type)(Lx), (cl::size_type)(Ly)};
    local_size  = cl::NDRange{(cl::size_type)(local), (cl::size_type)(local)};
    //std::cout << "local:"  << local << "\n";
    //queue.enqueueNDRangeKernel(set_sq, offset, global_size, local_size);
    queue.enqueueNDRangeKernel(set_sq_B, offset, global_size, local_size);
}

void simulator::initialize_wf(unsigned i0, unsigned j0, float kx, float ky, float broad){

    //std::cout << "N:" << N << "\n";
    float2  *input = new float2[Ncells];

    for(unsigned n=0; n<Ncells; n++){
        input[n].x = 0;
        input[n].y = 0;
    }

    queue.enqueueWriteBuffer(output_buf, CL_TRUE, 0, sizeof(float2)*Ncells, input);

    unsigned n;
    float norm2 = 0;
    float inc;
    float maxn = 0;

    std::complex<float> z;
    std::complex<float> im(0.0, 1.0);
    float arg;
    for(int j = 0; j < Ly; j++){
        for(int i = 0; i < Lx; i++){
            n = (j+pad)*(Lx+2*pad) + pad + i;

            arg = exp(-0.5*((i-i0)*(i-i0) + (j-j0)*(j-j0))/broad/broad);
            z = arg*exp(std::complex<float>(0,kx*i + ky*j));

            inc = std::real(z)*std::real(z) + std::imag(z)*std::imag(z);
            if(inc >= maxn) maxn=inc;
            norm2 += inc;

            input[n].x = std::real(z);
            input[n].y = std::imag(z);
        }
    }
    maxn = maxn/norm2;
    float norm = sqrt(norm2);

    // Normalize the wavefunction
    for(int r = 0; r < Ly; r++){
        for(int c = 0; c < Lx; c++){
            n = (r+pad)*(Lx+2*pad) + pad + c;

            input[n].x /= norm;
            input[n].y /= norm;
        }
    }

    max = maxn;
    queue.enqueueWriteBuffer(input_buf, CL_TRUE, 0, sizeof(float2)*Ncells, input);
    queue.enqueueWriteBuffer(max_buf,   CL_TRUE, 0, sizeof(float), &max);
    delete[] input;
}

void simulator::initialize_tevop(float dt, unsigned Npolys){
    // Determines the coefficients for the Chebyshev expansion of the time
    // evolution operator, and pushes them to the GPU
    Ncheb = Npolys; // Make sure this is an even number


    // Get the Chebyshev coefficients for the time evolution operator
    float2 jn[Ncheb];

    std::complex<float> z, im;
    im = std::complex<float>(0,1);

    for(unsigned i=0; i<Ncheb; i++){
        z = std::pow(im,i)*std::cyl_bessel_j(i, dt);
        if(i>0) 
            z *= 2;
        jn[i].x = std::real(z);
        jn[i].y = std::imag(z);
    }

    // Send the Bessel moments to the GPU
    // declaration: std::vector<cl::Buffer> jn_bufs;
    for(unsigned i=0; i<Ncheb; i++){
        cl::Buffer s(context, CL_MEM_READ_ONLY,  sizeof(float2));
        jn_bufs.push_back(s);
        queue.enqueueWriteBuffer(s, CL_TRUE, 0, sizeof(float2), &jn[i]);
    }

    cheb1.setArg(4,  jn_bufs[0]);
    cheb1.setArg(5,  jn_bufs[1]);

}

void simulator::iterate_time(unsigned niters){
    // std::cout << "Entered iterate_time\n" << std::flush;

    cl::NDRange offset;
    cl::NDRange global_size;
    cl::NDRange local_size;

    for(unsigned n=0; n<niters; n++){
        // Pairs of chebs
        unsigned NPairChebs = (Ncheb-2)/2;                                          
        //std::cout << "npairs: " << NPairChebs << "\n";
        offset      = cl::NDRange{(cl::size_type)(pad), (cl::size_type)(pad)};
        global_size = cl::NDRange{(cl::size_type)(Lx), (cl::size_type)(Ly)};
        local_size  = cl::NDRange{(cl::size_type)(local), (cl::size_type)(local)};


        // Reset the accumulator vector
        queue.enqueueNDRangeKernel(kreset, offset, global_size, local_size);

        // Zeroth and first Cheb iteration: Act with Hamiltonian. output = H@input
        queue.enqueueNDRangeKernel(cheb1, offset, global_size, local_size);

        for(unsigned i=0; i<NPairChebs; i++){

            cheb2.setArg(0, output_buf);
            cheb2.setArg(1,  input_buf);
            cheb2.setArg(4,  jn_bufs[2+2*i]);
            queue.enqueueNDRangeKernel(cheb2, offset, global_size, local_size);

            cheb2.setArg(0,  input_buf);
            cheb2.setArg(1, output_buf);
            cheb2.setArg(4,  jn_bufs[3+2*i]);
            queue.enqueueNDRangeKernel(cheb2, offset, global_size, local_size);
        }

        queue.enqueueNDRangeKernel(copy, offset, global_size, local_size);
    }

    // std::cout << "Left iterate_time\n" << std::flush;
}

void simulator::set_max(float m){
    max = m;
    queue.enqueueWriteBuffer(max_buf, CL_TRUE, 0, sizeof(float), &max);
}

float simulator::get_norm(float *maximum, float *threshhold){
    float2 *wavef = new float2[Ncells]; // input
    float  *score = new float[Ncells]; // score
    queue.enqueueReadBuffer(input_buf, CL_TRUE, 0, sizeof(float2)*Ncells, wavef);
    queue.enqueueReadBuffer(score_buf, CL_TRUE, 0, sizeof(float)*Ncells, score);

    float norm2 = 0;
    float norm;
    float inc;
    unsigned n;
    float2 a;
    *maximum = 0;
    for(int r = 0; r < Ly; r++){
        for(int c = 0; c < Lx; c++){
            n = (r+pad)*(Lx+2*pad) + pad + c;
            a = wavef[n];

            inc    = a.x*a.x + a.y*a.y;
            norm   = sqrt(inc);
            norm2 += inc;
            if(norm > *maximum) *maximum = norm;
        }
    }

    norm_top = 0;
    norm_bot = 0;
    for(int r = 0; r < Ly; r++){
        for(int c = 0; c < Lx; c++){
            n = (r+pad)*(Lx+2*pad) + pad + c;
            if(r>Ly/2) norm_top += score[n];
            if(r<Ly/2) norm_bot += score[n];
        }
    }

    // Get the histogram of wavefunction values 
    unsigned Nbins = 500;
    float *hist = new float[Nbins];
    for(unsigned i=0; i<Nbins; i++) 
        hist[i] = 0;

    int bin;
    for(int r = 0; r < Ly; r++){
        for(int c = 0; c < Lx; c++){
            n = (r+pad)*(Lx+2*pad) + pad + c;
            a = wavef[n];

            inc    = a.x*a.x + a.y*a.y;
            norm   = sqrt(inc);

            bin = (int)(norm/(*maximum*1.01)*Nbins);
            hist[bin] += 1;
        }
    }

    for(unsigned i=0; i<Nbins-1; i++){
        n = Nbins-2-i;
        hist[n] += hist[Nbins-1-i];
    }

    // Normalize by nonzero elements
    for(unsigned i=2; i<Nbins; i++){
        hist[i] /= hist[1];
    }

    unsigned threshhold_index = 0;
    for(unsigned i=0; i<Nbins-2; i++){
        n = Nbins-i-1;
        if(hist[n]>0.03){
            threshhold_index = n;
            break;
        }
    }
    *threshhold = *maximum*threshhold_index/Nbins*modifier;


    delete[] hist;
    delete[] wavef;
    delete[] score;
    return norm2;
}

void simulator::get_pot(int x, int y, int dx, int dy, uint8_t *buffer){
    std::cout << "simulator::get_pot\n" << std::flush;
    int ddx = dx;
    int ddy = dy;
    if(x + ddx > Lx) ddx = Lx-x;
    if(y + ddy > Ly) ddy = Ly-y;

    cl::NDRange offset;
    cl::NDRange global_size;
    cl::NDRange local_size;

    // Get local potential
    offset      = cl::NDRange{(cl::size_type)(0), (cl::size_type)(0)};
    global_size = cl::NDRange{(cl::size_type)(Lx), (cl::size_type)(Ly)};
    local_size  = cl::NDRange{(cl::size_type)(local), (cl::size_type)(local)};

    int4 *array = new int4[Npixels];
    queue.enqueueNDRangeKernel(colormapV,  offset, global_size, local_size);
    queue.enqueueReadBuffer(   pix_buf, CL_TRUE, 0, sizeof(int4)*Npixels, array);

    int n;
    // for(int i=0; i<Lx; i++){
    //     for(int j=0; j<Ly; j++){
    //         n = i + Lx*j;
    //         std::cout << array[n].x << " ";
    //     }
    //     std::cout << "\n";
    // }
    
    // std::cout << "x,y: " << x << " " << y << "\n";
    int m;
    for(int i=0; i<ddx; i++){
        for(int j=0; j<ddy; j++){
            n = i + dx*j;
            m = i+x + Lx*(j+y);
            buffer[n] = array[m].x;
            // std::cout << array[m].x << " ";
        }
        // std::cout << "\n";
    }
    // int n;
    // for(int x0=0; x0<dx; x0++){
    //     for(int y0=0; y0<dy; y0++){
    //         n = x0 + dx*y0;
    //         std::cout << (int)buffer[n+HEADER_LEN] << " ";
    //     }
    //     std::cout << "\n";
    // }

    delete[] array;
}

void simulator::update_pixel(){

    cl::NDRange offset;
    cl::NDRange global_size;
    cl::NDRange local_size;
    // Update the pixels on the screen
    // if showcase==true, then the potentials and the paddles wont be drawn

    // Get wavefunction
    offset      = cl::NDRange{(cl::size_type)(pad), (cl::size_type)(pad)};
    global_size = cl::NDRange{(cl::size_type)(Lx), (cl::size_type)(Ly)};
    local_size  = cl::NDRange{(cl::size_type)(local), (cl::size_type)(local)};

    queue.enqueueNDRangeKernel(colormap,  offset, global_size, local_size);

    int4 *array = new int4[Npixels];
    queue.enqueueReadBuffer(   pix_buf, CL_TRUE, 0, sizeof(int4)*Npixels, array);

    // Plot the wavefunction
    int m, n;
    for(int r = 0; r < Ly; r++){
        for(int c = 0; c < Lx; c++){
            m = r*Lx + c;
            n = r*Lx + c;
            // buffer[4*m+0] = array[n].x;
            buffer[m] = array[n].y;
            // buffer[4*m+2] = array[n].z;
            // buffer[4*m+3] = array[n].w;
        }
    }

    // Get magnetic field
    // queue.enqueueNDRangeKernel(colormapB,  offset, global_size, local_size);
    // queue.enqueueReadBuffer(   pix_buf, CL_TRUE, 0, sizeof(int4)*Npixels, array);

    // for(int r = 0; r < Ly; r++){
    //     for(int c = 0; c < Lx; c++){
    //         m = r*vis_width + c;
    //         n = r*Lx + c;
    //         data[4*m+2] = std::min(255, data[4*m+2] + array[n].z); // Edit the red channel
    //     }
    // }





    // Get local potential
    // offset      = cl::NDRange{(cl::size_type)(0), (cl::size_type)(0)};
    // global_size = cl::NDRange{(cl::size_type)(Lx), (cl::size_type)(Ly)};
    // local_size  = cl::NDRange{(cl::size_type)(local), (cl::size_type)(local)};

    // queue.enqueueNDRangeKernel(colormapV,  offset, global_size, local_size);
    // queue.enqueueReadBuffer(   pix_buf, CL_TRUE, 0, sizeof(int4)*Npixels, array);

    // // Plot the potential
    // if(!showcase)
    // for(int r = 0; r < Ly; r++){
    //     for(int c = 0; c < Lx; c++){
    //         m = r*vis_width + c;
    //         n = r*Lx + c;
    //         data[4*m+0] = std::min(255, data[4*m+0] + array[n].x);
    //     }
    // }
    delete[] array;


    // // Draw the paddles
    // if(!showcase)
    // for(int i=top_player_x-paddle_width/2; i<top_player_x+paddle_width/2; i++){
    //     for(int j=top_player_y-paddle_height/2; j<top_player_y+paddle_height/2; j++){
    //         m = j*vis_width + i;
    //         data[4*m+0] = 0;
    //         data[4*m+1] += 122;
    //         data[4*m+2] = 255;

    //     }
    // }

    // if(!showcase)
    // for(int i=bot_player_x-paddle_width/2; i<bot_player_x+paddle_width/2; i++){
    //     for(int j=bot_player_y-paddle_height/2; j<bot_player_y+paddle_height/2; j++){
    //         m = j*vis_width + i;
    //         data[4*m+0] = 255;
    //         data[4*m+1] += 122;
    //         data[4*m+2] = 0;
    //     }
    // }


    // int pixels_top = (int)(norm_top*Ly);
    // int pixels_bot = (int)(norm_bot*Ly);


    // // Clean p
    // for(int c = Lx; c < Lx + 30; c++){
    //     for(int r = 0; r < Ly; r++){
    //         m = r*vis_width + c;
    //         data[4*m+0] = 50;
    //         data[4*m+1] = 50;
    //         data[4*m+2] = 50;
    //         data[4*m+3] = 0;
    //     }
    // }

    // for(int c = Lx; c < Lx + 30; c++){
    //     for(int r = 0; r < pixels_bot; r++){
    //         m = r*vis_width + c;
    //         data[4*m+0] = 255;
    //         data[4*m+1] = 122;
    //         data[4*m+2] = 0;
    //         data[4*m+3] = 0;
    //     }

    //     for(int r = Ly-pixels_top; r < Ly; r++){
    //         m = r*vis_width + c;
    //         n = r*Lx + c;
    //         data[4*m+0] = 0;
    //         data[4*m+1] = 122;
    //         data[4*m+2] = 255;
    //         data[4*m+3] = 0;
    //     }

    //     m = Ly/2*vis_width + c;
    //     data[4*m+0] = 255;
    //     data[4*m+1] = 255;
    //     data[4*m+2] = 255;
    //     data[4*m+3] = 0;
    // }


    // for(int r = 0; r < Ly; r++){
    //     for(int c = 0; c < Lx; c++){
    //         m = r*vis_width + c;

    //         if(data[4*m+0]>255) data[4*m+0] = 255;
    //         if(data[4*m+1]>255) data[4*m+1] = 255;
    //         if(data[4*m+2]>255) data[4*m+2] = 255;
    //     }
    // }

}

void simulator::absorb(){
    unsigned absorption_width = 20;
    
    cl::NDRange offset;
    cl::NDRange global_size;
    cl::NDRange local_size;

    offset      = cl::NDRange{(cl::size_type)(pad), (cl::size_type)(pad)};
    global_size = cl::NDRange{(cl::size_type)(Lx), (cl::size_type)(absorption_width)};
    local_size  = cl::NDRange{(cl::size_type)(local), (cl::size_type)(local)};

    queue.enqueueNDRangeKernel(kabsorb,  offset, global_size, local_size);

    offset      = cl::NDRange{(cl::size_type)(pad), (cl::size_type)(Ly-absorption_width-pad)};
    global_size = cl::NDRange{(cl::size_type)(Lx), (cl::size_type)(absorption_width)};
    local_size  = cl::NDRange{(cl::size_type)(local), (cl::size_type)(local)};

    queue.enqueueNDRangeKernel(kabsorb,  offset, global_size, local_size);
}

void simulator::init(unsigned Lx, unsigned Ly, int delay){
    delay_simulation = delay;
    unsigned pad = 1;
    unsigned local = 2;

    // Time evolution operator parameters
    float dt = 2.0;
    unsigned Ncheb = 10;

    // initial wavefunction parameters: center, momentum, spread
    unsigned ix = Lx/2;
    unsigned iy = Ly/2;
    float kx = 0.5;
    float ky = 1.0;
    float broad = 20.0;

    showcase = false;

    init_cl();
    init_geometry(Lx, Ly, pad, local);
    init_window(Lx, Ly);
    set_hamiltonian_sq();
    init_buffers();
    init_kernels();
    initialize_tevop(dt, Ncheb);
    set_H();
    if(showcase) initialize_pot_from();
    initialize_wf(ix, iy, kx, ky, broad);

    int paddle_x = 100;
    int paddle_y_bot = 50;
    int paddle_y_top = Ly-50;
    int paddle_width = 100;
    int paddle_height = 20;
    init_paddles(paddle_x, paddle_y_top, paddle_x, paddle_y_bot, paddle_width, paddle_height);
    update_paddles(paddle_x, paddle_y_top, paddle_x, paddle_y_bot);
}

void simulator::loop(){

    unsigned Ntimes = 200000;
    
    float max = 0;
    float threshhold = 0;
    
    auto start = std::chrono::system_clock::now();

    // CHANGE: Replace this with while true
    bool processed_event = false;
    for(unsigned j=0; j<Ntimes; j++){
        
        // CHANGE: the colormap will be implemented client-side. This function can be replaced
        get_norm(&max, &threshhold);

        if(norm_top > 0.5 && !processed_event){
            eq->add_event(Event<EV_PLAYER_WON>(0).buffer_b);
            processed_event = true;

        } else if(norm_bot > 0.5 && !processed_event){
            eq->add_event(Event<EV_PLAYER_WON>(1).buffer_b);
            processed_event = true;

        } else {
            
            // set_max(threshhold);
            
            // if(engine.pressed_showcase) engine.clear_wf_away_from_pot(visual.image->data, visual.width, visual.height);
            if(!paused){
                
                iterate_time(3);

                // CHANGE: Replace this function name and functionality, since the 
                // colormap will be done client-side
                update_pixel();
            }
            if(absorb_on) absorb();

            
        }

        // visual.update();
        //if(j%10==0){
            //std::cout << "time, norm, max, norm_top, norm_bot: " << j << " " << norm << " " << max << " " << engine.norm_top << " " << engine.norm_bot << "\n";
        //}
        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = end-start;
        double elapsed_ms = elapsed_seconds.count()*1000;
        // std::cout << "simulation time: " << elapsed_ms << "ms" << std::endl;
        
        if(elapsed_ms*1000 < delay_simulation){
            usleep(delay_simulation-elapsed_ms*1000);
        }
        start = std::chrono::system_clock::now();
    }
}