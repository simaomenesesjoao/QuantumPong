#ifndef MACROS_H
#define MACROS_H 1

#define VERBOSE 2

#define EV_GENERIC -1
#define EV_CONNECT 0
#define EV_DISCONNECT 1
#define EV_CHANGE_SCREEN 2
#define EV_SEND_INIT_INFO 3
#define EV_PRESSED_SPACE 4
#define EV_UPDATE_STATUS 5
#define EV_STREAM 6
#define EV_PLAYER_WON 7
#define EV_PRESSED_KEY 8
#define EV_MOUSEBUTTONDOWN 9
#define EV_SEND_POT 10
#define EV_ADD_MAG 11

// Variables
#define PAYLOAD_OFF 0
#define PAYLOAD_ON 1

#define EV_SDL_QUIT 0
#define EV_SDL_SPACE 3

#define HEADER_LEN 100

#define KEY_w 0
#define KEY_a 1
#define KEY_s 2
#define KEY_d 3

#define BUTTON_LEFT 1
#define BUTTON_RIGHT 3

#endif // MACROS_H