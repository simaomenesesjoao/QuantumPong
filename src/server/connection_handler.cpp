#include <unistd.h>
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <chrono>
#include <iomanip>
#include <sstream>

static std::string now_ts() {
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
#include "connection_handler.hpp"

void connection_handler::init(unsigned port) {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); exit(1); }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,  &opt, sizeof(opt));
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT,  &opt, sizeof(opt));

    address.sin_family      = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port        = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind"); exit(1);
    }
    if (listen(server_fd, 16) < 0) { perror("listen"); exit(1); }
    std::cout << "connection_handler: listening on port " << port << "\n" << std::flush;
}

void connection_handler::send(const uint8_t *data, size_t len) {
    std::lock_guard<std::mutex> lock(send_mutex);
    if (gateway_socket < 0) return;
    ::send(gateway_socket, data, len, MSG_NOSIGNAL);
}

void connection_handler::accept_and_listen(simulator *sim) {
  while (true) {
    std::cout << "[" << now_ts() << "] connection_handler: waiting for accept()\n" << std::flush;
    gateway_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
    if (gateway_socket < 0) { perror("accept"); return; }
    std::cout << "[" << now_ts() << "] connection_handler: gateway connected\n" << std::flush;

    uint8_t header[HEADER_LEN];
    while (true) {
        ssize_t r = recv(gateway_socket, header, HEADER_LEN, MSG_WAITALL);
        if (r <= 0) break;

        uint8_t  cmd = header[0];
        uint32_t payload_size;
        memcpy(&payload_size, header + 4, 4);

        switch (cmd) {
            case CMD_START: {
                std::cout << "Start\n";
                uint16_t lx, ly, x0, y0;
                uint8_t  material;
                float    kx, ky, sigma;
                memcpy(&lx,       header + 8,  2);
                memcpy(&ly,       header + 10, 2);
                material = header[12];
                memcpy(&x0,       header + 14, 2);
                memcpy(&y0,       header + 16, 2);
                // header[18-19] = pad
                memcpy(&kx,       header + 20, 4);
                memcpy(&ky,       header + 24, 4);
                memcpy(&sigma,    header + 28, 4);
                sim->start(lx, ly, material, x0, y0, -kx, -ky, sigma);
                break;
            }
            case CMD_STOP:
                std::cout << "Stop\n";
                sim->stop();
                break;
            case CMD_RESUME:
                std::cout << "Resume\n";
                sim->resume();
                break;
            case CMD_MOVE_PADDLE: {
                std::cout << "Move paddle\n";
                uint8_t player_id = header[8];
                float dx, dy;
                memcpy(&dx, header + 12, 4);
                memcpy(&dy, header + 16, 4);
                sim->move_paddle(player_id, dx, dy);
                break;
            }
            case CMD_SET_WELL: {
                std::cout << "Set well\n";
                float strength;
                memcpy(&strength, header + 8, 4);
                if (payload_size > 0 && payload_size < 1024*1024) {
                    uint8_t *payload = new uint8_t[payload_size];
                    recv(gateway_socket, payload, payload_size, MSG_WAITALL);
                    size_t n = payload_size / 4;
                    sim->set_well(reinterpret_cast<int16_t*>(payload), n, strength);
                    delete[] payload;
                }
                break;
            }
            case CMD_SET_ABSORBING: {
                bool on = header[8] != 0;
                std::cout << "Set absorbing: " << (on ? "ON" : "OFF") << "\n";
                sim->absorbing.store(on, std::memory_order_relaxed);
                break;
            }
            case CMD_SET_UNIFORM_B: {
                float value;
                memcpy(&value, header + 8, 4);
                std::cout << "Set uniform B = " << value << "\n";
                sim->set_uniform_b(value);
                break;
            }
            case CMD_SET_FIELDS: {
                uint32_t num_shapes;
                memcpy(&num_shapes, header + 8, 4);
                std::cout << "Set fields: n=" << num_shapes
                          << " payload=" << payload_size << "\n";
                std::vector<Shape> shapes;
                if (payload_size > 0 && payload_size < 16 * 1024 * 1024) {
                    std::vector<uint8_t> payload(payload_size);
                    if (recv(gateway_socket, payload.data(), payload_size, MSG_WAITALL)
                            != (ssize_t)payload_size) break;

                    size_t off = 0;
                    shapes.reserve(num_shapes);
                    for (uint32_t s = 0; s < num_shapes; s++) {
                        if (off + 2 > payload.size()) break;
                        uint8_t kind  = payload[off++];
                        /* uint8_t field = */ payload[off++];   // reserved (V only for now)

                        Shape sh{};
                        if (kind == 0) {            // polygon
                            if (off + 8 > payload.size()) break;
                            float magnitude;
                            uint32_t n_pts;
                            memcpy(&magnitude, payload.data() + off,     4);
                            memcpy(&n_pts,     payload.data() + off + 4, 4);
                            off += 8;
                            size_t bytes = (size_t)n_pts * 2 * sizeof(int16_t);
                            if (off + bytes > payload.size()) break;
                            ShapePolygon body{ magnitude, {} };
                            body.pixels.reserve(n_pts);
                            for (uint32_t p = 0; p < n_pts; p++) {
                                int16_t row, col;
                                memcpy(&row, payload.data() + off + p*4,     2);
                                memcpy(&col, payload.data() + off + p*4 + 2, 2);
                                if (row >= 0 && col >= 0)
                                    body.pixels.push_back({(unsigned)row, (unsigned)col});
                            }
                            off += bytes;
                            sh.body = std::move(body);
                        } else if (kind == 1) {     // rect
                            if (off + 12 > payload.size()) break;
                            float magnitude;
                            int16_t cx, cy, w, h;
                            memcpy(&magnitude, payload.data() + off,     4);
                            memcpy(&cx,        payload.data() + off + 4, 2);
                            memcpy(&cy,        payload.data() + off + 6, 2);
                            memcpy(&w,         payload.data() + off + 8, 2);
                            memcpy(&h,         payload.data() + off + 10, 2);
                            off += 12;
                            sh.body = ShapeRect{ magnitude, cx, cy, w, h };
                        } else if (kind == 2) {     // circle
                            if (off + 10 > payload.size()) break;
                            float magnitude;
                            int16_t cx, cy, r;
                            memcpy(&magnitude, payload.data() + off,     4);
                            memcpy(&cx,        payload.data() + off + 4, 2);
                            memcpy(&cy,        payload.data() + off + 6, 2);
                            memcpy(&r,         payload.data() + off + 8, 2);
                            off += 10;
                            sh.body = ShapeCircle{ magnitude, cx, cy, r };
                        } else if (kind == 3) {     // tilt
                            if (off + 14 > payload.size()) break;
                            float magnitude;
                            int16_t ax, ay, bx, by, halfW;
                            memcpy(&magnitude, payload.data() + off,     4);
                            memcpy(&ax,        payload.data() + off + 4, 2);
                            memcpy(&ay,        payload.data() + off + 6, 2);
                            memcpy(&bx,        payload.data() + off + 8, 2);
                            memcpy(&by,        payload.data() + off + 10, 2);
                            memcpy(&halfW,     payload.data() + off + 12, 2);
                            off += 14;
                            sh.body = ShapeTilt{ magnitude, ax, ay, bx, by, halfW };
                        } else {
                            std::cout << "  unknown shape kind=" << (int)kind << "\n";
                            break;
                        }
                        shapes.push_back(std::move(sh));
                    }
                }
                sim->set_shapes(std::move(shapes));
                break;
            }
            case CMD_SET_SCORE_ZONES: {
                uint32_t num_zones;
                memcpy(&num_zones, header + 8, 4);
                std::cout << "Set score zones: n=" << num_zones
                          << " payload=" << payload_size << "\n";
                std::vector<ScoreZone> zones;
                if (payload_size > 0 && payload_size < 16 * 1024 * 1024) {
                    std::vector<uint8_t> payload(payload_size);
                    if (recv(gateway_socket, payload.data(), payload_size, MSG_WAITALL)
                            != (ssize_t)payload_size) break;

                    size_t off = 0;
                    zones.reserve(num_zones);
                    for (uint32_t z = 0; z < num_zones; z++) {
                        // Per-zone prefix: u8 kind, i8 sign, f32 weight
                        if (off + 6 > payload.size()) break;
                        uint8_t kind = payload[off];
                        int8_t  sign = (int8_t)payload[off + 1];
                        float   weight;
                        memcpy(&weight, payload.data() + off + 2, 4);
                        off += 6;

                        ScoreZone zn{ sign, weight, ShapeBody{} };
                        if (kind == 0) {            // polygon
                            if (off + 4 > payload.size()) break;
                            uint32_t n_pts;
                            memcpy(&n_pts, payload.data() + off, 4);
                            off += 4;
                            size_t bytes = (size_t)n_pts * 2 * sizeof(int16_t);
                            if (off + bytes > payload.size()) break;
                            ShapePolygon body{ 0.f, {} };
                            body.pixels.reserve(n_pts);
                            for (uint32_t p = 0; p < n_pts; p++) {
                                int16_t row, col;
                                memcpy(&row, payload.data() + off + p*4,     2);
                                memcpy(&col, payload.data() + off + p*4 + 2, 2);
                                if (row >= 0 && col >= 0)
                                    body.pixels.push_back({(unsigned)row, (unsigned)col});
                            }
                            off += bytes;
                            zn.body = std::move(body);
                        } else if (kind == 1) {     // rect
                            if (off + 8 > payload.size()) break;
                            int16_t cx, cy, w, h;
                            memcpy(&cx, payload.data() + off,     2);
                            memcpy(&cy, payload.data() + off + 2, 2);
                            memcpy(&w,  payload.data() + off + 4, 2);
                            memcpy(&h,  payload.data() + off + 6, 2);
                            off += 8;
                            zn.body = ShapeRect{ 0.f, cx, cy, w, h };
                        } else if (kind == 2) {     // circle
                            if (off + 6 > payload.size()) break;
                            int16_t cx, cy, r;
                            memcpy(&cx, payload.data() + off,     2);
                            memcpy(&cy, payload.data() + off + 2, 2);
                            memcpy(&r,  payload.data() + off + 4, 2);
                            off += 6;
                            zn.body = ShapeCircle{ 0.f, cx, cy, r };
                        } else {
                            std::cout << "  unknown zone kind=" << (int)kind << "\n";
                            break;
                        }
                        zones.push_back(std::move(zn));
                    }
                }
                sim->set_score_zones(std::move(zones));
                break;
            }
            case CMD_SET_POTENTIAL_SHAPES: {
                uint32_t num_shapes;
                memcpy(&num_shapes, header + 8, 4);
                std::cout << "Set potential shapes: n=" << num_shapes
                          << " payload=" << payload_size << "\n";
                std::vector<PotentialShape> shapes;
                if (payload_size > 0 && payload_size < 16 * 1024 * 1024) {
                    std::vector<uint8_t> payload(payload_size);
                    if (recv(gateway_socket, payload.data(), payload_size, MSG_WAITALL)
                            != (ssize_t)payload_size) break;

                    size_t off = 0;
                    shapes.reserve(num_shapes);
                    for (uint32_t s = 0; s < num_shapes; s++) {
                        if (off + 8 > payload.size()) break;
                        uint32_t n_points;
                        float    height;
                        memcpy(&n_points, payload.data() + off,     4);
                        memcpy(&height,   payload.data() + off + 4, 4);
                        off += 8;

                        size_t bytes = (size_t)n_points * 2 * sizeof(int16_t);
                        if (off + bytes > payload.size()) break;

                        std::vector<Point> pts;
                        pts.reserve(n_points);
                        for (uint32_t p = 0; p < n_points; p++) {
                            int16_t row, col;
                            memcpy(&row, payload.data() + off + p*4,     2);
                            memcpy(&col, payload.data() + off + p*4 + 2, 2);
                            if (row >= 0 && col >= 0)
                                pts.push_back({(unsigned)row, (unsigned)col});
                        }
                        off += bytes;
                        shapes.push_back({std::move(pts), height});
                    }
                } else if (num_shapes == 0) {
                    // empty list: clear all shapes
                }
                sim->set_potential_shapes(std::move(shapes));
                break;
            }
            default:
                std::cout << "[" << now_ts() << "] Unknown cmd=0x" << std::hex << (int)cmd << std::dec << "\n";
                if (payload_size > 0 && payload_size < 1024*1024) {
                    uint8_t *buf = new uint8_t[payload_size];
                    recv(gateway_socket, buf, payload_size, MSG_WAITALL);
                    delete[] buf;
                }
                break;
        }
    }

    std::cout << "[" << now_ts() << "] connection_handler: gateway disconnected\n" << std::flush;
    sim->stop();
    close(gateway_socket);
    gateway_socket = -1;
  } // accept loop
}
