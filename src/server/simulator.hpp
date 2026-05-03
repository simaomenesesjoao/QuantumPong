#ifndef SIMULATOR_H
#define SIMULATOR_H 1

#define CL_HPP_TARGET_OPENCL_VERSION 210
#include <CL/opencl.hpp>
#include <atomic>
#include <cstdint>
#include <deque>
#include <variant>
#include <vector>
#include <mutex>

typedef cl_float2 float2;
typedef cl_int4   int4;

class connection_handler;

struct Point {
    unsigned int row, col;
};

struct PotentialShape {
    std::vector<Point> points;
    float height;
};

struct Array {
    unsigned int Nrows, Ncols;
    std::vector<std::vector<float>> array;

    Array(unsigned int Nrows, unsigned int Ncols)
        : Nrows{Nrows}, Ncols{Ncols},
          array{std::vector<std::vector<float>>(Nrows, std::vector<float>(Ncols, 0))} {}

          std::vector<float> to_buffer() const {
          std::vector<float> buffer(Nrows * Ncols);
        for (unsigned row = 0; row < Nrows; row++)
            for (unsigned col = 0; col < Ncols; col++)
                buffer.at(row * Ncols + col) = array.at(row).at(col);
        return buffer;
    }
};

struct PotentialShapes {
    std::vector<PotentialShape> shapes;
    bool dirty = false;

    Array to_array(unsigned int Nrows, unsigned int Ncols) const {
        Array a(Nrows, Ncols);
        for (const auto& shape : shapes)
            for (const auto& [row, col] : shape.points)
                if (row < Nrows && col < Ncols)
                    a.array.at(row).at(col) = shape.height;
        return a;
    }
};

// ---- Tagged shape variants for SET_FIELDS (V-targeted only for now) ----

struct ShapePolygon { float magnitude; std::vector<Point> pixels; };
struct ShapeRect    { float magnitude; int cx, cy, w, h; };
struct ShapeCircle  { float magnitude; int cx, cy, r; };
struct ShapeTilt    { float magnitude; int ax, ay, bx, by, halfW; };

using ShapeBody = std::variant<ShapePolygon, ShapeRect, ShapeCircle, ShapeTilt>;

struct Shape { ShapeBody body; };  // (room to add a `field` byte later)

// Absorbing scoring zone for puzzle mode. sign ∈ {-1, +1}; weight ≥ 0.
// `body` reuses ShapeBody (tilt is rasterized but not particularly meaningful).
struct ScoreZone {
    int8_t   sign;     // +1 = positive (green), -1 = negative (red)
    float    weight;   // multiplier on absorbed |ψ|²
    ShapeBody body;
};


struct NormOutput {
    float norm, max;
};


class simulator {
public:
    // ---- public state (read by streamer / game_engine threads) ----
    std::atomic<bool> streaming  { false };
    std::atomic<bool> paused     { true  };
    std::atomic<bool> frame_ready{ false };
    std::atomic<bool> absorbing  { true  };  // gate for absorb() in loop()
    uint8_t  *buffer_f      = nullptr;  // HEADER_LEN + N bytes; published atomically
    unsigned  buffer_f_size = 0;

    // ---- lifecycle ----
    simulator() = default;
    void init_cl();

    void start(uint16_t lx, uint16_t ly, uint8_t material,
               uint16_t x0, uint16_t y0,
               float kx, float ky, float sigma);

    void stop();    // halt without clearing state
    void resume();  // restart from current state

    // ---- in-game commands (connection thread; return immediately) ----
    void move_paddle(uint8_t player_id, float dx, float dy);
    void set_well(int16_t *points, size_t n_points, float strength);
    void set_potential_shapes(std::vector<PotentialShape> shapes);
    void set_shapes(std::vector<Shape> shapes);
    void set_uniform_b(float value);
    void set_score_zones(std::vector<ScoreZone> zones);

    // ---- sim thread entry point ----
    void loop();

    void finalize();

private:
    // ---- CL objects ----
    std::vector<cl::Platform> platforms;
    cl::Platform     platform;
    std::vector<cl::Device>  devices;
    cl::Device       device;
    cl::Context      context;
    cl::CommandQueue queue;   // in-order; no finish() in the hot path

    cl::Kernel cheb1, cheb2, colormap, copy, kreset;
    cl::Kernel set_sq_B_k;
    cl::Kernel kfill_rect;
    cl::Kernel kabsorb;
    cl::Kernel kabsorb_zone;

    cl::Buffer hops_buf, input_buf, output_buf, acc_buf;
    cl::Buffer pix_buf, score_buf, mag_buf;
    cl::Buffer pot_buf;    // static wells
    cl::Buffer paddle_buf; // moving paddles
    cl::Buffer scale_buf, max_buf, val_buf;

    // Puzzle-mode scoring zones (interior-grid Lx*Ly buffers).
    cl::Buffer zone_mask_buf;   // signed mask: +w / -w / 0
    cl::Buffer score_pos_buf;   // accumulated absorbed mass in positive zones
    cl::Buffer score_neg_buf;   // accumulated absorbed mass in negative zones

    std::vector<cl::Buffer> jn_bufs;

    // ---- grid / physics parameters ----
    bool cl_initialized = false;
    int  Lx = 0, Ly = 0, pad = 1, local_size = 16;
    unsigned Ncells = 0, N = 0, Npixels = 0;
    unsigned Nhops = 5, Ncheb = 10;
    float SCALE = 4.1f;
    float max   = 1.0f;
    float dt    = 2.0f;
    int   delay_simulation = 25000; // µs

    // ---- paddle state ----
    static constexpr int   PADDLE_W   = 100;
    static constexpr int   PADDLE_H   = 20;
    static constexpr float PADDLE_STR = 4.0f;

    // commanded positions (protected by cmd_mutex)
    int top_x = 0, top_y = 0;
    int bot_x = 0, bot_y = 0;

    // last positions actually applied to the GPU (read/written only by loop())
    int gpu_top_x = 0, gpu_top_y = 0;
    int gpu_bot_x = 0, gpu_bot_y = 0;

    // ---- pending well command (protected by cmd_mutex) ----
    struct PendingWell {
        bool  pending  = false;
        int   cx, cy, w, h;
        float strength;
    };
    PendingWell pending_well;

    // ---- pending potential shapes (protected by cmd_mutex) ----
    PotentialShapes pending_shapes;

    // ---- tagged shape list (protected by cmd_mutex) ----
    std::vector<Shape> pending_v_shapes;
    bool               pending_v_shapes_dirty = false;

    // ---- uniform B value (protected by cmd_mutex) ----
    float pending_b_value      = 0.f;
    bool  pending_b_dirty       = false;

    // ---- pending score zones (protected by cmd_mutex) ----
    std::vector<ScoreZone> pending_score_zones;
    bool                   pending_score_zones_dirty = false;
    bool                   has_score_zones           = false;

    // ---- dirty flag (protected by cmd_mutex) ----
    bool hops_dirty = false;  // set by move_paddle/set_well/set_potential_shapes; consumed by loop()

    // ---- score ----
    // In sandbox mode these hold per-half boundary-absorb sums (legacy paddle
    // game). In puzzle mode they hold positive- and negative-zone scores.
    float norm_top = 0, norm_bot = 0;
    float score_pos = 0, score_neg = 0;

    // ---- thread safety ----
    std::mutex iter_mutex;  // protects GPU queue; held by loop() per iteration and start()
    std::mutex cmd_mutex;   // lightweight; protects paddle state + dirty flags

    // ---- double-buffered async pixel readback ----
    float    *float_staging[2] = {};  // CPU staging for GPU readback
    int       pix_cur = 0;

    // ---- queue-depth tracking: one marker event per loop iteration ----
    std::deque<cl::Event> inflight;

    // ---- internals ----
    void alloc_buffers();
    void compile_kernels();
    void set_kernel_args();
    void init_tevop();
    void set_H();
    void initialize_wf(uint16_t x0, uint16_t y0, float kx, float ky, float sigma);
    void place_paddles();
    void update_hops();

    void fill_rect_gpu(cl::Buffer &buf, int cx, int cy, int w, int h, float val);
    void apply_pending_commands(const PotentialShapes& shapes);

    void iterate_time(unsigned niters);
    void update_pixel(uint8_t*);   // async; event stored in pix_ready[buf_idx]
    void absorb();
    void absorb_zone();
    NormOutput get_norm();
    void build_frame_header(uint8_t *buf);

    void release_buffers();
    void release_kernels();
};

#endif // SIMULATOR_H
