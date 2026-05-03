#define CL_HPP_TARGET_OPENCL_VERSION 210

#include <unistd.h>
#include <cmath>
#include <complex>
#include <fstream>
#include <iostream>
#include <cstring>
#include <chrono>
#include <iomanip>
#include <sstream>

static std::string sim_ts() {
    auto now = std::chrono::system_clock::now();
    auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{}; localtime_r(&t, &tm);
    std::ostringstream ss;
    ss << std::put_time(&tm, "%H:%M:%S") << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}
#include "../macros.hpp"
#include "simulator.hpp"

static inline cl::size_type rup(int n, int ls) {
    return (cl::size_type)(((n + ls - 1) / ls) * ls);
}

// ---------------------------------------------------------------------------
// Shape rasterizers — write the shape's value into `buf` (Nrows × Ncols,
// row-major). Replacement semantics: cells inside the shape are overwritten;
// cells outside are left untouched.
// ---------------------------------------------------------------------------

static void rasterize_polygon(std::vector<float>& buf, const ShapePolygon& s,
                              int Nrows, int Ncols) {
    for (const auto& [row, col] : s.pixels)
        if (row < (unsigned)Nrows && col < (unsigned)Ncols)
            buf[row * Ncols + col] = s.magnitude;
}

static void rasterize_rect(std::vector<float>& buf, const ShapeRect& s,
                           int Nrows, int Ncols) {
    int x0 = std::max(0,        s.cx - s.w/2);
    int x1 = std::min(Ncols-1,  s.cx + s.w/2);
    int y0 = std::max(0,        s.cy - s.h/2);
    int y1 = std::min(Nrows-1,  s.cy + s.h/2);
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++)
            buf[y * Ncols + x] = s.magnitude;
}

static void rasterize_circle(std::vector<float>& buf, const ShapeCircle& s,
                             int Nrows, int Ncols) {
    int x0 = std::max(0,        s.cx - s.r);
    int x1 = std::min(Ncols-1,  s.cx + s.r);
    int y0 = std::max(0,        s.cy - s.r);
    int y1 = std::min(Nrows-1,  s.cy + s.r);
    int r2 = s.r * s.r;
    for (int y = y0; y <= y1; y++) {
        int dy = y - s.cy;
        int dy2 = dy * dy;
        for (int x = x0; x <= x1; x++) {
            int dx = x - s.cx;
            if (dx*dx + dy2 <= r2) buf[y * Ncols + x] = s.magnitude;
        }
    }
}

static void rasterize_tilt(std::vector<float>& buf, const ShapeTilt& s,
                           int Nrows, int Ncols) {
    float ux = (float)(s.bx - s.ax), uy = (float)(s.by - s.ay);
    float L  = std::hypot(ux, uy);
    if (L < 1.f) return;
    ux /= L; uy /= L;
    float nx = -uy, ny = ux;
    float mx = (s.ax + s.bx) * 0.5f, my = (s.ay + s.by) * 0.5f;
    float halfL = L * 0.5f;
    float halfW = (float)s.halfW;

    // Loose AABB of the rotated rectangle = circumscribing box around the
    // four corners. Conservative bound via projected extents.
    float ext_x = std::abs(ux) * halfL + std::abs(nx) * halfW;
    float ext_y = std::abs(uy) * halfL + std::abs(ny) * halfW;
    int x0 = std::max(0,        (int)std::floor(mx - ext_x));
    int x1 = std::min(Ncols-1,  (int)std::ceil (mx + ext_x));
    int y0 = std::max(0,        (int)std::floor(my - ext_y));
    int y1 = std::min(Nrows-1,  (int)std::ceil (my + ext_y));

    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            float dx = (float)x - mx, dy = (float)y - my;
            float along  = dx * ux + dy * uy;
            float across = dx * nx + dy * ny;
            if (std::abs(along) <= halfL && std::abs(across) <= halfW)
                buf[y * Ncols + x] = s.magnitude * (along / L);  // ±mag/2 at ends
        }
    }
}

static void rasterize_shape(std::vector<float>& buf, const Shape& s,
                            int Nrows, int Ncols) {
    std::visit([&](const auto& body) {
        using T = std::decay_t<decltype(body)>;
        if constexpr (std::is_same_v<T, ShapePolygon>) rasterize_polygon(buf, body, Nrows, Ncols);
        else if constexpr (std::is_same_v<T, ShapeRect>)   rasterize_rect  (buf, body, Nrows, Ncols);
        else if constexpr (std::is_same_v<T, ShapeCircle>) rasterize_circle(buf, body, Nrows, Ncols);
        else if constexpr (std::is_same_v<T, ShapeTilt>)   rasterize_tilt  (buf, body, Nrows, Ncols);
    }, s.body);
}

// Rasterize a scoring zone into the mask: writes sign*weight at every cell
// inside the zone. Mirrors rasterize_shape but overrides the magnitude.
static void rasterize_zone(std::vector<float>& buf, const ScoreZone& z,
                           int Nrows, int Ncols) {
    float val = (float)z.sign * z.weight;
    std::visit([&](const auto& body) {
        using T = std::decay_t<decltype(body)>;
        if constexpr (std::is_same_v<T, ShapePolygon>) {
            ShapePolygon copy{val, body.pixels};
            rasterize_polygon(buf, copy, Nrows, Ncols);
        } else if constexpr (std::is_same_v<T, ShapeRect>) {
            ShapeRect copy{val, body.cx, body.cy, body.w, body.h};
            rasterize_rect(buf, copy, Nrows, Ncols);
        } else if constexpr (std::is_same_v<T, ShapeCircle>) {
            ShapeCircle copy{val, body.cx, body.cy, body.r};
            rasterize_circle(buf, copy, Nrows, Ncols);
        }
        // ShapeTilt intentionally ignored for zones — gradient-shaped absorbers
        // aren't part of the puzzle vocabulary.
    }, z.body);
}

// ---------------------------------------------------------------------------
// OpenCL one-time setup
// ---------------------------------------------------------------------------

void simulator::init_cl() {
    cl::Platform::get(&platforms);
    if (platforms.empty()) { std::cerr << "No OpenCL platforms\n"; exit(1); }
    platform = platforms[0];
    std::cout << "Platform: " << platform.getInfo<CL_PLATFORM_NAME>() << "\n";

    platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
    if (devices.empty()) { std::cerr << "No OpenCL devices\n"; exit(2); }
    device = devices[0];
    std::cout << "Device: " << device.getInfo<CL_DEVICE_NAME>() << "\n";

    context = cl::Context(device);
    queue   = cl::CommandQueue(context, device);
    cl_initialized = true;
}

// ---------------------------------------------------------------------------
// Buffer allocation / release
// ---------------------------------------------------------------------------

void simulator::alloc_buffers() {
    std::cout << "Alloc buffers::::\n";
    pad     = 1;
    Ncells  = (Lx + 2*pad) * (Ly + 2*pad);
    N       = Lx * Ly;
    Npixels = N;

    input_buf  = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(float2)*Ncells);
    output_buf = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(float2)*Ncells);
    acc_buf    = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(float2)*Ncells);
    score_buf  = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(float)*Ncells);
    hops_buf   = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(float2)*N*Nhops);
    pix_buf    = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(float)*Npixels);
    scale_buf  = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(float));
    max_buf    = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(float));
    val_buf    = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(float));
    mag_buf    = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(float)*Ncells);
    pot_buf    = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(float)*N);
    paddle_buf = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(float)*N);

    zone_mask_buf  = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(float)*N);
    score_pos_buf  = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(float)*N);
    score_neg_buf  = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(float)*N);

    max = 1.0f;
    queue.enqueueWriteBuffer(scale_buf, CL_TRUE, 0, sizeof(float), &SCALE);
    queue.enqueueWriteBuffer(max_buf,   CL_TRUE, 0, sizeof(float), &max);

    std::vector<float> zeros(Ncells, 0.0f);
    queue.enqueueWriteBuffer(score_buf,  CL_TRUE, 0, sizeof(float)*Ncells, zeros.data());
    queue.enqueueWriteBuffer(mag_buf,    CL_TRUE, 0, sizeof(float)*Ncells, zeros.data());
    queue.enqueueWriteBuffer(pot_buf,    CL_TRUE, 0, sizeof(float)*N,      zeros.data());
    queue.enqueueWriteBuffer(paddle_buf, CL_TRUE, 0, sizeof(float)*N,      zeros.data());
    queue.enqueueWriteBuffer(zone_mask_buf, CL_TRUE, 0, sizeof(float)*N,   zeros.data());
    queue.enqueueWriteBuffer(score_pos_buf, CL_TRUE, 0, sizeof(float)*N,   zeros.data());
    queue.enqueueWriteBuffer(score_neg_buf, CL_TRUE, 0, sizeof(float)*N,   zeros.data());

    // Staging buffers for double-buffered pixel readback
    for (int i = 0; i < 2; i++) {
        delete[] float_staging[i];
        float_staging[i] = new float[Npixels]();
    }

    delete[] buffer_f;
    buffer_f_size = HEADER_LEN + N * sizeof(float);
    buffer_f = new uint8_t[buffer_f_size]();
}

void simulator::release_buffers() {}

// ---------------------------------------------------------------------------
// Kernel compilation and argument wiring
// ---------------------------------------------------------------------------

void simulator::compile_kernels() {
    std::ifstream f("kernel.cpp");
    std::string src((std::istreambuf_iterator<char>(f)),
                     std::istreambuf_iterator<char>());

    cl::Program::Sources sources;
    sources.push_back(src);
    cl::Program program(context, sources);

    char opts[64];
    snprintf(opts, sizeof(opts), "-DLX=%d -DLY=%d -DPAD=%d -DNHOPS=%d",
             Lx, Ly, pad, Nhops);
    std::cout << "Compiling kernels: " << opts << "\n";
    if (program.build({device}, opts) != CL_SUCCESS) {
        std::cerr << "Kernel build error:\n"
                  << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device) << "\n";
        exit(1);
    }

    set_sq_B_k = cl::Kernel(program, "set_sq_B");
    kfill_rect = cl::Kernel(program, "fill_rect");
    cheb1      = cl::Kernel(program, "cheb1");
    cheb2      = cl::Kernel(program, "cheb2");
    colormap   = cl::Kernel(program, "colormap");
    copy       = cl::Kernel(program, "copy");
    kreset     = cl::Kernel(program, "reset");
    kabsorb    = cl::Kernel(program, "absorb");
    kabsorb_zone = cl::Kernel(program, "absorb_zone");
}

void simulator::set_kernel_args() {
    set_sq_B_k.setArg(0, hops_buf);
    set_sq_B_k.setArg(1, scale_buf);
    set_sq_B_k.setArg(2, mag_buf);
    set_sq_B_k.setArg(3, pot_buf);
    set_sq_B_k.setArg(4, paddle_buf);

    cheb1.setArg(0, input_buf);
    cheb1.setArg(1, output_buf);
    cheb1.setArg(2, hops_buf);
    cheb1.setArg(3, acc_buf);

    cheb2.setArg(2, hops_buf);
    cheb2.setArg(3, acc_buf);

    colormap.setArg(0, acc_buf);
    colormap.setArg(1, pix_buf);

    copy.setArg(0, acc_buf);
    copy.setArg(1, input_buf);

    kreset.setArg(0, acc_buf);
    kabsorb.setArg(0, input_buf);
    kabsorb.setArg(1, score_buf);

    kabsorb_zone.setArg(0, input_buf);
    kabsorb_zone.setArg(1, zone_mask_buf);
    kabsorb_zone.setArg(2, score_pos_buf);
    kabsorb_zone.setArg(3, score_neg_buf);

    kfill_rect.setArg(0, pot_buf);
    kfill_rect.setArg(1, val_buf);
}

// ---------------------------------------------------------------------------
// Chebyshev time-evolution operator coefficients
// ---------------------------------------------------------------------------

void simulator::init_tevop() {
    jn_bufs.clear();
    std::vector<float2> jn(Ncheb);
    std::complex<float> im(0, 1);
    for (unsigned i = 0; i < Ncheb; i++) {
        auto z = std::pow(im, (int)i) * (float)std::cyl_bessel_j((int)i, dt);
        if (i > 0) z *= 2.0f;
        jn[i].s[0] = std::real(z);
        jn[i].s[1] = std::imag(z);
    }
    for (unsigned i = 0; i < Ncheb; i++) {
        cl::Buffer b(context, CL_MEM_READ_ONLY, sizeof(float2));
        queue.enqueueWriteBuffer(b, CL_TRUE, 0, sizeof(float2), &jn[i]);
        jn_bufs.push_back(b);
    }
    cheb1.setArg(4, jn_bufs[0]);
    cheb1.setArg(5, jn_bufs[1]);
}

// ---------------------------------------------------------------------------
// Hamiltonian rebuild — async, no finish
// ---------------------------------------------------------------------------

void simulator::set_H() {
    cl::NDRange off(0, 0);
    cl::NDRange gs(rup(Lx, local_size), rup(Ly, local_size));
    cl::NDRange ls((cl::size_type)local_size, (cl::size_type)local_size);
    queue.enqueueNDRangeKernel(set_sq_B_k, off, gs, ls);
    // waits for this one to complete before it starts.
}

void simulator::update_hops() {
    set_H();
}

// ---------------------------------------------------------------------------
// Wavefunction initialisation
// ---------------------------------------------------------------------------

void simulator::initialize_wf(uint16_t x0, uint16_t y0, float kx, float ky, float sigma) {
    std::vector<float2> psi(Ncells);
    memset(psi.data(), 0, sizeof(float2)*Ncells);

    float norm2 = 0;
    for (int j = 0; j < Ly; j++) {
        for (int i = 0; i < Lx; i++) {
            unsigned n = (j + pad) * (Lx + 2*pad) + pad + i;
            float arg = std::exp(-0.5f * ((i-x0)*(i-x0) + (j-y0)*(j-y0)) / (sigma*sigma));
            std::complex<float> z = arg * std::exp(std::complex<float>(0, kx*i + ky*j));
            psi[n].s[0] = std::real(z);
            psi[n].s[1] = std::imag(z);
            norm2 += psi[n].s[0]*psi[n].s[0] + psi[n].s[1]*psi[n].s[1];
        }
    }
    float norm = std::sqrt(norm2);
    for (int j = 0; j < Ly; j++)
        for (int i = 0; i < Lx; i++) {
            unsigned n = (j + pad) * (Lx + 2*pad) + pad + i;
            psi[n].s[0] /= norm;
            psi[n].s[1] /= norm;
        }

    queue.enqueueWriteBuffer(input_buf, CL_TRUE, 0, sizeof(float2)*Ncells, psi.data());
    queue.enqueueWriteBuffer(max_buf,   CL_TRUE, 0, sizeof(float), &max);
}

// ---------------------------------------------------------------------------
// Paddle GPU helpers — async, no finish
// ---------------------------------------------------------------------------

void simulator::fill_rect_gpu(cl::Buffer &buf, int cx, int cy, int w, int h, float val) {
    int x0 = cx - w/2, y0 = cy - h/2;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    int x1 = std::min(x0 + w, Lx);
    int y1 = std::min(y0 + h, Ly);
    if (x1 <= x0 || y1 <= y0) return;

    queue.enqueueWriteBuffer(val_buf, CL_TRUE, 0, sizeof(float), &val);
    kfill_rect.setArg(0, buf);
    kfill_rect.setArg(1, val_buf);

    cl::NDRange off((cl::size_type)x0, (cl::size_type)y0);
    cl::NDRange gs((cl::size_type)(x1-x0), (cl::size_type)(y1-y0));
    queue.enqueueNDRangeKernel(kfill_rect, off, gs, cl::NullRange);
}

void simulator::place_paddles() {
    top_x = Lx/2; top_y = Ly - 50;
    bot_x = Lx/2; bot_y = 50;
    gpu_top_x = top_x; gpu_top_y = top_y;
    gpu_bot_x = bot_x; gpu_bot_y = bot_y;

    std::vector<float> zeros(N, 0.0f);
    queue.enqueueWriteBuffer(paddle_buf, CL_TRUE, 0, sizeof(float)*N, zeros.data());
    fill_rect_gpu(paddle_buf, top_x, top_y, PADDLE_W, PADDLE_H, PADDLE_STR);
    fill_rect_gpu(paddle_buf, bot_x, bot_y, PADDLE_W, PADDLE_H, PADDLE_STR);
    set_H();
}

// ---------------------------------------------------------------------------
// start() — full reinitialisation
// ---------------------------------------------------------------------------

void simulator::start(uint16_t lx, uint16_t ly, uint8_t /*material*/,
                      uint16_t x0, uint16_t y0,
                      float kx, float ky, float sigma) {
    paused    = true;
    streaming = false;
    std::lock_guard<std::mutex> lock(iter_mutex);

    bool dims_changed = (lx != (uint16_t)Lx || ly != (uint16_t)Ly);
    Lx = lx; 
    Ly = ly;

    if (!cl_initialized) {
        std::cout << "[" << sim_ts() << "] sim: init_cl\n" << std::flush;
        init_cl();
        std::cout << "[" << sim_ts() << "] sim: init_cl done\n" << std::flush;
    }

    if (dims_changed || buffer_f == nullptr) {
        std::cout << "[" << sim_ts() << "] sim: alloc_buffers\n" << std::flush;
        alloc_buffers();
        std::cout << "[" << sim_ts() << "] sim: compile_kernels\n" << std::flush;
        compile_kernels();
        std::cout << "[" << sim_ts() << "] sim: set_kernel_args + init_tevop\n" << std::flush;
        set_kernel_args();
        init_tevop();
    } else {
        std::vector<float> zeros(Ncells, 0.0f);
        queue.enqueueWriteBuffer(score_buf,  CL_TRUE, 0, sizeof(float)*Ncells, zeros.data());
        queue.enqueueWriteBuffer(mag_buf,    CL_TRUE, 0, sizeof(float)*Ncells, zeros.data());
        queue.enqueueWriteBuffer(pot_buf,    CL_TRUE, 0, sizeof(float)*N,      zeros.data());
        queue.enqueueWriteBuffer(paddle_buf, CL_TRUE, 0, sizeof(float)*N,      zeros.data());
        queue.enqueueWriteBuffer(zone_mask_buf, CL_TRUE, 0, sizeof(float)*N,   zeros.data());
        queue.enqueueWriteBuffer(score_pos_buf, CL_TRUE, 0, sizeof(float)*N,   zeros.data());
        queue.enqueueWriteBuffer(score_neg_buf, CL_TRUE, 0, sizeof(float)*N,   zeros.data());
    }

    has_score_zones = false;
    norm_top = 0;
    norm_bot = 0;
    score_pos = 0;
    score_neg = 0;
    std::cout << "[" << sim_ts() << "] sim: initialize_wf\n" << std::flush;
    initialize_wf(x0, y0, kx, ky, sigma);
    std::cout << "[" << sim_ts() << "] sim: place_paddles + set_H\n" << std::flush;
    place_paddles();

    std::cout << "[" << sim_ts() << "] sim: queue.finish (waiting for GPU init)\n" << std::flush;
    queue.finish();
    std::cout << "[" << sim_ts() << "] sim: ready, unpausing\n" << std::flush;

    paused    = false;
    streaming = true;
    std::cout << "[" << sim_ts() << "] simulator: started  Lx=" << Lx << " Ly=" << Ly << "\n";
}

void simulator::stop() {
    streaming = false;
    paused    = true;
    std::cout << "simulator: stopped\n";
}

void simulator::resume() {
    paused    = false;
    streaming = true;
    std::cout << "simulator: resumed\n";
}

// ---------------------------------------------------------------------------
// In-game commands — cmd_mutex only, no GPU ops
// ---------------------------------------------------------------------------

void simulator::move_paddle(uint8_t player_id, float dx, float dy) {
    const float step = 2.5f;
    int ddx = (int)(dx * step);
    int ddy = (int)(dy * step);

    std::lock_guard<std::mutex> lock(cmd_mutex);
    if (player_id == 0) {
        top_x = std::max(PADDLE_W/2, std::min(Lx - PADDLE_W/2, top_x + ddx));
        top_y = std::max(PADDLE_H/2, std::min(Ly - PADDLE_H/2, top_y + ddy));
    } else {
        bot_x = std::max(PADDLE_W/2, std::min(Lx - PADDLE_W/2, bot_x + ddx));
        bot_y = std::max(PADDLE_H/2, std::min(Ly - PADDLE_H/2, bot_y + ddy));
    }
    hops_dirty = true;
}

void simulator::set_well(int16_t *points, size_t n, float strength) {
    if (n < 3) return;
    int min_x = points[0], max_x = points[0];
    int min_y = points[1], max_y = points[1];
    for (size_t i = 1; i < n; i++) {
        if (points[i*2]   < min_x) min_x = points[i*2];
        if (points[i*2]   > max_x) max_x = points[i*2];
        if (points[i*2+1] < min_y) min_y = points[i*2+1];
        if (points[i*2+1] > max_y) max_y = points[i*2+1];
    }

    std::lock_guard<std::mutex> lock(cmd_mutex);
    pending_well.pending  = true;
    pending_well.cx       = (min_x + max_x) / 2;
    pending_well.cy       = (min_y + max_y) / 2;
    pending_well.w        = std::max(1, max_x - min_x);
    pending_well.h        = std::max(1, max_y - min_y);
    pending_well.strength = strength;
    hops_dirty = true;
}

void simulator::set_potential_shapes(std::vector<PotentialShape> shapes) {
    std::lock_guard<std::mutex> lock(cmd_mutex);
    pending_shapes.shapes = std::move(shapes);
    pending_shapes.dirty  = true;
    hops_dirty            = true;
}

void simulator::set_shapes(std::vector<Shape> shapes) {
    std::lock_guard<std::mutex> lock(cmd_mutex);
    pending_v_shapes       = std::move(shapes);
    pending_v_shapes_dirty = true;
    hops_dirty             = true;
}

void simulator::set_uniform_b(float value) {
    std::lock_guard<std::mutex> lock(cmd_mutex);
    pending_b_value  = value;
    pending_b_dirty  = true;
    hops_dirty       = true;
}

void simulator::set_score_zones(std::vector<ScoreZone> zones) {
    std::lock_guard<std::mutex> lock(cmd_mutex);
    pending_score_zones       = std::move(zones);
    pending_score_zones_dirty = true;
}

// ---------------------------------------------------------------------------
// Simulation loop
// ---------------------------------------------------------------------------

void simulator::iterate_time(unsigned niters) {
    cl::NDRange off((cl::size_type)pad, (cl::size_type)pad);
    cl::NDRange gs(rup(Lx, local_size), rup(Ly, local_size));
    cl::NDRange ls((cl::size_type)local_size, (cl::size_type)local_size);

    unsigned NPairs = Ncheb/2 - 1;
    for (unsigned n = 0; n < niters; n++) {
        queue.enqueueNDRangeKernel(kreset, off, gs, ls);
        queue.enqueueNDRangeKernel(cheb1,  off, gs, ls);
        for (unsigned i = 0; i < NPairs; i++) {
            cheb2.setArg(0, output_buf);
            cheb2.setArg(1, input_buf);
            cheb2.setArg(4, jn_bufs[2 + 2*i]);
            queue.enqueueNDRangeKernel(cheb2, off, gs, ls);

            cheb2.setArg(0, input_buf);
            cheb2.setArg(1, output_buf);
            cheb2.setArg(4, jn_bufs[3 + 2*i]);
            queue.enqueueNDRangeKernel(cheb2, off, gs, ls);
        }
        queue.enqueueNDRangeKernel(copy, off, gs, ls);
    }
}

// Enqueue colormap + async readback into float_staging[buf_idx].
// Event stored in pix_ready[buf_idx]; caller must wait before reading staging buffer.
void simulator::update_pixel(uint8_t* buffer) {
    cl::NDRange off((cl::size_type)pad, (cl::size_type)pad);
    cl::NDRange gs(rup(Lx, local_size), rup(Ly, local_size));
    cl::NDRange ls((cl::size_type)local_size, (cl::size_type)local_size);
    queue.enqueueNDRangeKernel(colormap, off, gs, ls);
    queue.enqueueReadBuffer(pix_buf, CL_FALSE, 0, sizeof(float)*Npixels, buffer);
}

void simulator::absorb() {
    unsigned aw = 50;
    cl::NDRange gs((cl::size_type)Lx, (cl::size_type)aw);
    queue.enqueueNDRangeKernel(kabsorb, cl::NDRange((cl::size_type)pad, (cl::size_type)pad), gs, cl::NullRange);
    queue.enqueueNDRangeKernel(kabsorb, cl::NDRange((cl::size_type)pad, (cl::size_type)(pad+Ly-aw)), gs, cl::NullRange);
}

void simulator::absorb_zone() {
    cl::NDRange off((cl::size_type)pad, (cl::size_type)pad);
    cl::NDRange gs(rup(Lx, local_size), rup(Ly, local_size));
    cl::NDRange ls((cl::size_type)local_size, (cl::size_type)local_size);
    queue.enqueueNDRangeKernel(kabsorb_zone, off, gs, ls);
}

NormOutput simulator::get_norm() {
    static unsigned gn_call = 0;
    static unsigned suppressed = 0;
    gn_call++;

    std::vector<float2> wavef(Ncells);
    std::vector<float>  score(Ncells);
    queue.enqueueReadBuffer(input_buf, CL_TRUE, 0, sizeof(float2)*Ncells, wavef.data());
    queue.enqueueReadBuffer(score_buf, CL_TRUE, 0, sizeof(float)*Ncells,  score.data());

    float norm2 = 0, mx = 0;
    bool any_nan = false;
    norm_top = 0; norm_bot = 0;
    int L = Lx + 2*pad;
    for (int r = 0; r < Ly; r++) {
        for (int c = 0; c < Lx; c++) {
            unsigned n = (r + pad)*L + pad + c;
            float re = wavef[n].s[0], im = wavef[n].s[1];
            if (std::isnan(re) || std::isnan(im)) any_nan = true;
            float amp2 = re*re + im*im;
            norm2 += amp2;
            if (amp2 > mx) mx = amp2;
            if (r > Ly/2) norm_top += score[n];
            else          norm_bot += score[n];
        }
    }

    // Puzzle-mode scoring: read interior-grid score buffers and sum them.
    // Only when the simulator currently has zones, to avoid unnecessary readbacks.
    if (has_score_zones) {
        std::vector<float> spos(N), sneg(N);
        queue.enqueueReadBuffer(score_pos_buf, CL_TRUE, 0, sizeof(float)*N, spos.data());
        queue.enqueueReadBuffer(score_neg_buf, CL_TRUE, 0, sizeof(float)*N, sneg.data());
        float sp = 0, sn = 0;
        for (unsigned i = 0; i < N; i++) { sp += spos[i]; sn += sneg[i]; }
        score_pos = sp;
        score_neg = sn;
    }
    // mx = 0.03;
    // norm2 = 1.0;

    // Divergence guard: warn loudly if ψ is heading to NaN or its norm has
    // grown well past the unitary expectation (≈ 1, drifting down via absorb).
    // Throttled: print first occurrence, suppress 50 (~5 s at 10 Hz), repeat.
    bool nan_or_inf  = any_nan || std::isnan(norm2) || std::isinf(norm2);
    bool norm_blowup = norm2 > 5.0f || mx > 100.0f;
    if (nan_or_inf || norm_blowup) {
        if (suppressed == 0) {
            std::cerr << "[" << sim_ts() << "] [WARN] sim diverging  "
                      << "gn_call=" << gn_call
                      << "  norm²=" << norm2
                      << "  max(|ψ|²)=" << mx
                      << (nan_or_inf ? "  NaN/inf in ψ" : "")
                      << "\n  hint: V/SCALE pushed spectral radius > 1; "
                      << "lower WELL_HEIGHT (client) or raise SCALE (simulator.hpp)\n"
                      << std::flush;
            suppressed = 50;
        } else {
            suppressed--;
        }
    } else {
        suppressed = 0;
    }

    float threshold = mx * 0.03f;
    max = threshold > 0 ? threshold : 1.0f;
    queue.enqueueWriteBuffer(max_buf, CL_FALSE, 0, sizeof(float), &max);
    return {norm2, max};
}

void simulator::build_frame_header(uint8_t *buf) {
    memset(buf, 0, HEADER_LEN);
    buf[0] = FRAME_WAVEFUNCTION;
    uint32_t ps = (uint32_t)(N * sizeof(float));
    memcpy(buf + 4, &ps, 4);

    int16_t p0x = (int16_t)gpu_top_x, p0y = (int16_t)gpu_top_y;
    int16_t p1x = (int16_t)gpu_bot_x, p1y = (int16_t)gpu_bot_y;
    memcpy(buf + 8,  &p0x, 2);
    memcpy(buf + 10, &p0y, 2);
    memcpy(buf + 12, &p1x, 2);
    memcpy(buf + 14, &p1y, 2);
    // In puzzle mode the two score floats carry zone accumulators; in sandbox
    // they carry the legacy paddle-game half-grid sums.
    float top_field = has_score_zones ? score_pos : norm_top;
    float bot_field = has_score_zones ? score_neg : norm_bot;
    memcpy(buf + 16, &top_field, 4);
    memcpy(buf + 20, &bot_field, 4);
    memcpy(buf + 24, &max, 4);
}

void simulator::loop() {
    int frame = 0;
    int tick  = 0;

    // Per-stage timing accumulators (microseconds), reset each log window.
    uint64_t prof_apply_us = 0, prof_norm_us = 0, prof_iter_us = 0,
             prof_wait_us  = 0, prof_publish_us = 0;
    auto prof_window_start = std::chrono::steady_clock::now();

    cl::Event pix_ready[2];
    while (true) {
        if (paused) { usleep(10000); continue; }
        std::cout << "+" << std::flush;


        int cur  = frame & 1;
        int prev = cur ^ 1;
        pix_ready[prev].wait();
        std::lock_guard<std::mutex> lock(iter_mutex);

        // Snapshot command state under lightweight mutex (no GPU ops here)
        int new_top_x, new_top_y, new_bot_x, new_bot_y;
        bool do_hops;
        PendingWell well;
        PotentialShapes shapes_snapshot;

        std::vector<Shape> v_shapes_snapshot;
        bool v_shapes_dirty = false;
        float b_value_snapshot = 0.f;
        bool  b_dirty = false;
        std::vector<ScoreZone> zones_snapshot;
        bool zones_dirty = false;
        {
            std::lock_guard<std::mutex> cmd_lock(cmd_mutex);
            new_top_x = top_x;
            new_top_y = top_y;
            new_bot_x = bot_x;
            new_bot_y = bot_y;
            do_hops   = hops_dirty;
            well      = pending_well;
            if (pending_shapes.dirty) {
                shapes_snapshot.shapes = std::move(pending_shapes.shapes);
                shapes_snapshot.dirty  = true;
                pending_shapes.shapes.clear();
                pending_shapes.dirty   = false;
            }
            if (pending_v_shapes_dirty) {
                v_shapes_snapshot = std::move(pending_v_shapes);
                v_shapes_dirty    = true;
                pending_v_shapes.clear();
                pending_v_shapes_dirty = false;
            }
            if (pending_b_dirty) {
                b_value_snapshot = pending_b_value;
                b_dirty          = true;
                pending_b_dirty  = false;
            }
            if (pending_score_zones_dirty) {
                zones_snapshot              = std::move(pending_score_zones);
                zones_dirty                 = true;
                pending_score_zones.clear();
                pending_score_zones_dirty   = false;
            }
            hops_dirty           = false;
            pending_well.pending = false;
        }

        using clk = std::chrono::steady_clock;
        auto t0 = clk::now();

        // Enqueue any pending GPU commands (paddle moves, well updates, H rebuild)

        if(shapes_snapshot.dirty){
            std::cout << "Updating shapes\n";
            const auto potential = shapes_snapshot.to_array(Ly, Lx).to_buffer();
            queue.enqueueWriteBuffer(pot_buf, CL_TRUE, 0, sizeof(float)*potential.size(), potential.data());
        }

        if (v_shapes_dirty) {
            std::cout << "Updating V shapes (n=" << v_shapes_snapshot.size() << ")\n";
            std::vector<float> V(Lx * Ly, 0.f);
            for (const Shape& s : v_shapes_snapshot)
                rasterize_shape(V, s, Ly, Lx);
            queue.enqueueWriteBuffer(pot_buf, CL_TRUE, 0,
                                     sizeof(float) * V.size(), V.data());
        }

        if (b_dirty) {
            std::cout << "Updating uniform B = " << b_value_snapshot << "\n";
            std::vector<float> B(Ncells, b_value_snapshot);
            queue.enqueueWriteBuffer(mag_buf, CL_TRUE, 0,
                                     sizeof(float) * B.size(), B.data());
        }

        if (zones_dirty) {
            std::cout << "Updating score zones (n=" << zones_snapshot.size() << ")\n";
            std::vector<float> mask(N, 0.f);
            for (const ScoreZone& z : zones_snapshot)
                rasterize_zone(mask, z, Ly, Lx);
            queue.enqueueWriteBuffer(zone_mask_buf, CL_TRUE, 0,
                                     sizeof(float) * mask.size(), mask.data());
            std::vector<float> zeros(N, 0.f);
            queue.enqueueWriteBuffer(score_pos_buf, CL_TRUE, 0,
                                     sizeof(float) * N, zeros.data());
            queue.enqueueWriteBuffer(score_neg_buf, CL_TRUE, 0,
                                     sizeof(float) * N, zeros.data());
            score_pos = 0;
            score_neg = 0;
            has_score_zones = !zones_snapshot.empty();
        }

        if (do_hops) set_H();

        auto t_apply = clk::now();

        // get_norm() does sync (CL_TRUE) readbacks of input/score buffers — this
        // is the one stage where the CPU actually waits on the GPU mid-iteration.
        uint64_t this_norm_us = 0;
        if (tick % 4 == 0) {
            auto tn0 = clk::now();
            const auto& [norm, mx] = get_norm();
            std::cout << "current norm/max: " << norm << " | " << mx << "\n";
            this_norm_us = std::chrono::duration_cast<std::chrono::microseconds>(clk::now() - tn0).count();
        }
        tick++;
        auto t_norm = clk::now();

        // GPU compute enqueue (all async — these calls just submit work).
        iterate_time(3);
        update_pixel(buffer_f + HEADER_LEN); // colormap + async DMA → float_staging[cur]
        if (absorbing.load(std::memory_order_relaxed)) absorb();
        if (has_score_zones) absorb_zone();

        auto t_iter = clk::now();

        // Wait on previous frame's readback and publish it.
        uint64_t this_wait_us = 0, this_publish_us = 0;
        if (frame > 0) {
            auto tw0 = clk::now();
            auto tw1 = clk::now();
            // memcpy(buffer_f + HEADER_LEN, float_staging[prev], N * sizeof(float));
            build_frame_header(buffer_f);
            // frame_ready.store(true);
            auto tw2 = clk::now();
            this_wait_us    = std::chrono::duration_cast<std::chrono::microseconds>(tw1 - tw0).count();
            this_publish_us = std::chrono::duration_cast<std::chrono::microseconds>(tw2 - tw1).count();
        }

        // Tag this iteration with a marker event for the queue-depth counter.
        cl::Event tag;
        queue.enqueueMarkerWithWaitList(nullptr, &tag);
        inflight.push_back(std::move(tag));

        queue.enqueueMarkerWithWaitList(nullptr, &pix_ready[cur]);

        // Accumulate stage timings for the rolling 30-frame window.
        prof_apply_us   += std::chrono::duration_cast<std::chrono::microseconds>(t_apply - t0).count();
        prof_norm_us    += this_norm_us;
        prof_iter_us    += std::chrono::duration_cast<std::chrono::microseconds>(t_iter - t_norm).count();
        prof_wait_us    += this_wait_us;
        prof_publish_us += this_publish_us;

        // Every 30 frames (~0.75 s), report queue depth + per-stage averages and reset.
        if (frame > 0 && frame % 30 == 0) {
            while (!inflight.empty()) {
                cl_int status = inflight.front().getInfo<CL_EVENT_COMMAND_EXECUTION_STATUS>();
                if (status != CL_COMPLETE) break;
                inflight.pop_front();
            }
            auto ms = [](uint64_t us) { return us / 1000.0 / 30.0; };  // avg ms / frame
            auto now_t = std::chrono::steady_clock::now();
            double elapsed_s = std::chrono::duration<double>(now_t - prof_window_start).count();
            double fps = 30.0 / elapsed_s;
            prof_window_start = now_t;
            std::cout << "[prof] f=" << frame
                      << " fps=" << fps
                      << " queue=" << inflight.size()
                      << "  apply=" << ms(prof_apply_us) << "ms"
                      << "  norm="  << ms(prof_norm_us)  << "ms"
                      << "  iter="  << ms(prof_iter_us)  << "ms"
                      << "  wait="  << ms(prof_wait_us)  << "ms"
                      << "  pub="   << ms(prof_publish_us) << "ms\n"
                      << std::flush;
            prof_apply_us = prof_norm_us = prof_iter_us = prof_wait_us = prof_publish_us = 0;
        }

        frame++;
        usleep(delay_simulation);
    }
}

void simulator::finalize() {
    for (int i = 0; i < 2; i++) {
        delete[] float_staging[i];
        float_staging[i] = nullptr;
    }
    delete[] buffer_f;
    buffer_f = nullptr;
}
