#ifndef MACROS_H
#define MACROS_H 1

// Commands: gateway → C++ server
#define CMD_START         0x01  // full reinit + start streaming
#define CMD_STOP          0x02  // halt simulation, preserve state
#define CMD_RESUME        0x03  // restart from current state (no reinit)
#define CMD_MOVE_PADDLE   0x04  // move a paddle by normalised direction
#define CMD_SET_WELL      0x05  // add a static potential well
#define CMD_SET_POTENTIAL_SHAPES 0x06  // (legacy) polygon-only V shapes
#define CMD_SET_ABSORBING        0x07  // enable/disable boundary absorption
#define CMD_SET_FIELDS           0x08  // tagged-union shape list (V, all kinds)
#define CMD_SET_UNIFORM_B        0x09  // fill mag_buf with a uniform value
#define CMD_SET_SCORE_ZONES      0x0A  // puzzle-mode positive/negative absorbing zones

// Frames: C++ server → gateway
#define FRAME_WAVEFUNCTION 0x10

// Header layout (32 bytes, little-endian):
//   [0]    cmd/frame id  uint8
//   [1]    pad           uint8
//   [2-3]  pad           uint16
//   [4-7]  payload_size  uint32
//   [8-31] data          uint8[24]  (command-specific)
#define HEADER_LEN 32

#endif // MACROS_H
