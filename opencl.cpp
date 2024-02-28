// The GPU device I'm using uses openCL v2.1
#define CL_HPP_TARGET_OPENCL_VERSION 210

#include <complex>
#include <fstream>
#include <iostream>
#include "opencl.hpp"

void kpm::init_cl(){
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
};


void kpm::init_geometry(unsigned lx, unsigned ly, unsigned p, unsigned loc){
    geometry_init = true;
    Lx = lx;
    Ly = ly;
    pad = p;
    Ncells = (Lx+2*pad)*(Ly+2*pad);
    N = Lx*Ly;
    local = loc;
    //std::cout << "initializing geometry" << Lx << " " << Ly << " " << Ncells << "\n";

}


void kpm::init_window(unsigned width, unsigned height){
    window_init = true;
    Npixels = width*height;
}

void kpm::set_hamiltonian_sq(){
    Nhops = 5;
    SCALE = 4.1;

}

void kpm::init_buffers(){
    if(!geometry_init){
        std::cout << "Geometry must be initialized before buffers are initialized. Exiting.\n";
        exit(1);
    }

    if(!window_init){
        std::cout << "Window dimensions must be specified before buffers are initialized. Exiting.\n";
        exit(2);
    }

    input_buf  = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(float2)*Ncells);
    output_buf = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(float2)*Ncells);
    acc_buf    = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(float2)*Ncells);
    score_buf  = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(float)*Ncells);

    hops_buf   = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(float2)*N*Nhops);
    pix_buf    = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(int4)*Npixels);

    scale_buf  = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(float));
    max_buf    = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(float));
    val_buf    = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(float));

    max = 1.0;
    queue.enqueueWriteBuffer(scale_buf, CL_TRUE, 0, sizeof(float), &SCALE);
    queue.enqueueWriteBuffer(max_buf,   CL_TRUE, 0, sizeof(float), &max);


    // Initialize score to zero
    float  *score = new float[Ncells];
    for(unsigned n=0; n<Ncells; n++){
        score[n] = 0;
    }
    queue.enqueueWriteBuffer(score_buf, CL_TRUE, 0, sizeof(float)*Ncells, score);
    delete score;


}



void kpm::init_kernels(){
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
    //std::cout << programString << "\n";

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

    kset_local_pot = cl::Kernel(program, "set_local_pot");
    std::cout << "created kernel: " << set_sq.getInfo<CL_KERNEL_FUNCTION_NAME>() << "\n";

    kclear_local_pot = cl::Kernel(program, "clear_local_pot");
    std::cout << "created kernel: " << set_sq.getInfo<CL_KERNEL_FUNCTION_NAME>() << "\n";

    cheb1 = cl::Kernel(program, "cheb1");
    std::cout << "created kernel: " << cheb1.getInfo<CL_KERNEL_FUNCTION_NAME>() << "\n";

    cheb2 = cl::Kernel(program, "cheb2");
    std::cout << "created kernel: " << cheb2.getInfo<CL_KERNEL_FUNCTION_NAME>() << "\n";

    colormap = cl::Kernel(program, "colormap");
    std::cout << "created kernel: " << colormap.getInfo<CL_KERNEL_FUNCTION_NAME>() << "\n";

    copy = cl::Kernel(program, "copy");
    std::cout << "created kernel: " << copy.getInfo<CL_KERNEL_FUNCTION_NAME>() << "\n";
    
    kreset = cl::Kernel(program, "reset");
    std::cout << "created kernel: " << kreset.getInfo<CL_KERNEL_FUNCTION_NAME>() << "\n";

    kabsorb = cl::Kernel(program, "absorb");
    std::cout << "created kernel: " << kabsorb.getInfo<CL_KERNEL_FUNCTION_NAME>() << "\n";

    set_sq.setArg(0,  hops_buf);
    set_sq.setArg(1,  scale_buf);

    cheb1.setArg(0,  input_buf);
    cheb1.setArg(1, output_buf);
    cheb1.setArg(2,   hops_buf);
    cheb1.setArg(3,    acc_buf);

    cheb2.setArg(2,   hops_buf);
    cheb2.setArg(3,    acc_buf);

    colormap.setArg(0, acc_buf);
    colormap.setArg(1, pix_buf);
    colormap.setArg(2, max_buf);

    copy.setArg(0,     acc_buf); // from
    copy.setArg(1,   input_buf); // to

    kreset.setArg(0,    acc_buf);

    kabsorb.setArg(0, input_buf);
    kabsorb.setArg(1, score_buf);

    kset_local_pot.setArg(0, hops_buf);
    kset_local_pot.setArg(1, scale_buf);
    kset_local_pot.setArg(2, val_buf);

    kclear_local_pot.setArg(0, hops_buf);

}

void kpm::init_paddles(float pt_x, float pt_y, float pb_x, float pb_y, float paddle_w, float paddle_h){
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

void kpm::update_paddles(float pt_x, float pt_y, float pb_x, float pb_y){
    // Update the values of the paddle position
    top_player_prev_x = top_player_x;
    top_player_x = pt_x;

    bot_player_prev_x = bot_player_x;
    bot_player_x = pb_x;

    top_player_prev_y = top_player_y;
    top_player_y = pt_y;

    bot_player_prev_y = bot_player_y;
    bot_player_y = pb_y;

    global_size = cl::NDRange{paddle_width, paddle_height};
    local_size  = cl::NDRange{local, local};


    float val = 4;
    queue.enqueueWriteBuffer(val_buf, CL_TRUE, 0, sizeof(float), &val);

    // Update player on top
    offset = cl::NDRange{top_player_prev_x - paddle_width/2, top_player_prev_y - paddle_height/2};
    queue.enqueueNDRangeKernel(kclear_local_pot, offset, global_size, local_size);

    offset = cl::NDRange{top_player_x - paddle_width/2, top_player_y - paddle_height/2};
    queue.enqueueNDRangeKernel(kset_local_pot, offset, global_size, local_size);

    // Update player on bottom
    offset = cl::NDRange{bot_player_prev_x - paddle_width/2, bot_player_prev_y - paddle_height/2};
    queue.enqueueNDRangeKernel(kclear_local_pot, offset, global_size, local_size);

    offset = cl::NDRange{bot_player_x - paddle_width/2, bot_player_y - paddle_height/2};
    queue.enqueueNDRangeKernel(kset_local_pot, offset, global_size, local_size);

}

void kpm::set_local_pot(unsigned x, unsigned y, unsigned dx, unsigned dy, float v){
    val = v;

    queue.enqueueWriteBuffer(val_buf,   CL_TRUE, 0, sizeof(float), &val);

    offset      = cl::NDRange{x, y};
    global_size = cl::NDRange{dx, dy};
    local_size  = cl::NDRange{local, local};
    //std::cout << "local:"  << local << "\n";
    queue.enqueueNDRangeKernel(kset_local_pot, offset, global_size, local_size);
}

void kpm::reset_state(){


    // Initialize score to zero
    float  *score = new float[Ncells];
    for(unsigned n=0; n<Ncells; n++){
        score[n] = 0;
    }
    queue.enqueueWriteBuffer(score_buf, CL_TRUE, 0, sizeof(float)*Ncells, score);
    delete score;

}

void kpm::set_H(){//float d, int length, int width){
    // d, length, width is for the potential, to be implemented later
    //queue.enqueueWriteBuffer(  hops_buf, CL_TRUE, 0, sizeof(float2)*Nhops*WIDTH*WIDTH, hops);

    offset      = cl::NDRange{0, 0};
    global_size = cl::NDRange{Lx, Ly};
    local_size  = cl::NDRange{local, local};
    //std::cout << "local:"  << local << "\n";
    queue.enqueueNDRangeKernel(set_sq, offset, global_size, local_size);
}


void kpm::initialize_wf(unsigned i0, unsigned j0, float kx, float ky, float broad){


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
    for(unsigned j = 0; j < Ly; j++){
        for(unsigned i = 0; i < Lx; i++){
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
    std::cout << "norm2: " << norm2 << "\n";
    std::cout << "max: " << maxn << "\n";
    maxn = maxn/norm2;
    std::cout << "max: " << maxn << "\n";
    float norm = sqrt(norm2);

    // Normalize the wavefunction
    for(unsigned r = 0; r < Ly; r++){
        for(unsigned c = 0; c < Lx; c++){
            n = (r+pad)*(Lx+2*pad) + pad + c;

            input[n].x /= norm;
            input[n].y /= norm;
        }
    }

    max = maxn;
    queue.enqueueWriteBuffer(input_buf, CL_TRUE, 0, sizeof(float2)*Ncells, input);
    queue.enqueueWriteBuffer(max_buf,   CL_TRUE, 0, sizeof(float), &max);
    delete input;
}

void kpm::initialize_tevop(float dt, unsigned Npolys){
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


void kpm::iterate_time(unsigned niters){

    for(unsigned n=0; n<niters; n++){
        // Pairs of chebs
        unsigned NPairChebs = (Ncheb-2)/2;                                          
        //std::cout << "npairs: " << NPairChebs << "\n";
        offset      = cl::NDRange{pad, pad};
        global_size = cl::NDRange{Lx, Ly};
        local_size  = cl::NDRange{local, local};


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
}

void kpm::set_max(float m){
    max = m;
    queue.enqueueWriteBuffer(max_buf, CL_TRUE, 0, sizeof(float), &max);
}

float kpm::get_norm(float *maximum){
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
    for(unsigned r = 0; r < Ly; r++){
        for(unsigned c = 0; c < Lx; c++){
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
    for(unsigned r = 0; r < Ly; r++){
        for(unsigned c = 0; c < Lx; c++){
            n = (r+pad)*(Lx+2*pad) + pad + c;
            if(r<Ly/2) norm_top += score[n];
            if(r>Ly/2) norm_bot += score[n];
        }
    }


    delete wavef;
    delete score;

    return norm2;
}


void kpm::update_pixel(char *data){

    offset      = cl::NDRange{pad, pad};
    global_size = cl::NDRange{Lx, Ly};
    local_size  = cl::NDRange{local, local};

    queue.enqueueNDRangeKernel(colormap,  offset, global_size, local_size);

    int4 *array = new int4[Npixels];
    queue.enqueueReadBuffer(   pix_buf, CL_TRUE, 0, sizeof(int4)*Npixels, array);
    unsigned m;
    for(unsigned r = 0; r < Ly; r++){
        for(unsigned c = 0; c < Lx; c++){
            m = r*Lx + c;
            data[4*m+0] = array[m].x;
            data[4*m+1] = array[m].y;
            data[4*m+2] = array[m].z;
            data[4*m+3] = array[m].w;
        }
    }

    for(unsigned i=top_player_x-paddle_width/2; i<=top_player_x+paddle_width/2; i++){
        for(unsigned j=top_player_y-paddle_height/2; j<=top_player_y+paddle_height/2; j++){
            m = j*Lx + i;
            //data[4*m+0] = array[m].x;
            //data[4*m+1] = array[m].y;
            data[4*m+2] += 50;
            //data[4*m+3] = array[m].w;
        }
    }

    for(unsigned i=bot_player_x-paddle_width/2; i<=bot_player_x+paddle_width/2; i++){
        for(unsigned j=bot_player_y-paddle_height/2; j<=bot_player_y+paddle_height/2; j++){
            m = j*Lx + i;
            //data[4*m+0] = array[m].x;
            //data[4*m+1] = array[m].y;
            data[4*m+2] += 50;
            //data[4*m+3] = array[m].w;
        }
    }



    delete array;
}

void kpm::absorb(){
    psi2_top = 0;
    psi2_bot = 0;

    offset      = cl::NDRange{pad, pad};
    global_size = cl::NDRange{Lx, 20};
    local_size  = cl::NDRange{local, local};

    queue.enqueueNDRangeKernel(kabsorb,  offset, global_size, local_size);

    offset      = cl::NDRange{pad, Ly-20-pad};
    global_size = cl::NDRange{Lx, 20};
    local_size  = cl::NDRange{local, local};

    queue.enqueueNDRangeKernel(kabsorb,  offset, global_size, local_size);
    //float2 *array = new float2[Ncells];
    //queue.enqueueReadBuffer(input_buf, CL_TRUE, 0, sizeof(float2)*Ncells, array);
    //delete array;
}


void print_arr(float2 *arr, unsigned Lx, unsigned Ly, unsigned offset, int flag){

    unsigned n;
    for(unsigned j = 0; j < Ly; j++){
        for(unsigned i = 0; i < Lx; i++){
            n = (j+offset)*(Lx+2*offset) + offset + i;

            if(flag==0){ 
                printf("%6.2f ",arr[n].x);
            } else if(flag==1){ 
                printf("%6.2f ",arr[n].y);
            }
        }
        std::cout << "\n";
    }

}

void print_arr(int4 *arr, unsigned Lx, unsigned Ly, unsigned offset, int flag){

    unsigned n;
    for(unsigned j = 0; j < Ly; j++){
        for(unsigned i = 0; i < Lx; i++){
            n = (j+offset)*(Lx+2*offset) + offset + i;

            if(flag==0){ 
                printf("%6.2d ",arr[n].x);
            } else if(flag==1){ 
                printf("%6.2d ",arr[n].y);
            }
        }
        std::cout << "\n";
    }

}
