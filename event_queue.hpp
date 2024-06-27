#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H 1

#include <mutex>
#include "macros.hpp"


class event_queue{
    public:
        unsigned tail, head, queue_size;
        uint8_t **events;
        std::mutex mutex;

    event_queue(unsigned size);
    ~event_queue();
    
    int add_event(uint8_t *);
    int print_queue_data();
    void read(int*, uint8_t**);

};



// template <int ID>
// class Event{
//     buffer_b;
// };

template <int ID> class Event{
    public:
        union {
            struct {
                int event_ID;
                int payload_size;
                int player_number;
            };
            uint8_t buffer_b[HEADER_LEN];
        };


        Event(int pnum){
            event_ID = ID;
            payload_size = PAYLOAD_OFF;
            player_number = pnum;           
        }
        
        Event(uint8_t *buf){
            for(unsigned i=0; i<HEADER_LEN; i++){
                buffer_b[i] = buf[i];
            }
        }
        ~Event(){};
};

template <> class Event <EV_CONNECT>{
    public:
        union {
            struct {
                int event_ID;
                int has_payload;
                int player_number;

                int socket;
            };
            uint8_t buffer_b[HEADER_LEN];
        };


        Event(int pnum, int sock){
            event_ID = EV_CONNECT;
            has_payload = PAYLOAD_OFF;
            player_number = pnum;
            socket = sock;
            
        }
        
        Event(uint8_t *buf){
            for(unsigned i=0; i<HEADER_LEN; i++){
                buffer_b[i] = buf[i];
            }
        }
        ~Event(){};
};

template <> class Event <EV_DISCONNECT>{
    public:
        union {
            struct {
                int event_ID;
                int has_payload;
                int player_number;

            };
            uint8_t buffer_b[HEADER_LEN];
        };


        Event(int pnum){
            event_ID = EV_DISCONNECT;
            has_payload = PAYLOAD_OFF;
            player_number = pnum;
            
        }
        
        Event(uint8_t *buf){
            for(unsigned i=0; i<HEADER_LEN; i++){
                buffer_b[i] = buf[i];
            }
        }
        ~Event(){};
};

template <> class Event <EV_CHANGE_SCREEN>{
    public:
        union {
            struct {
                int event_ID;
                int has_payload;
                int player_number;
                int screen_number;

            };
            uint8_t buffer_b[HEADER_LEN];
        };


        Event(int pnum, int screen){
            event_ID = EV_CHANGE_SCREEN;
            has_payload = PAYLOAD_OFF;
            player_number = pnum;
            screen_number = screen;
        }
        
        Event(uint8_t *buf){
            for(unsigned i=0; i<HEADER_LEN; i++){
                buffer_b[i] = buf[i];
            }
        }
        ~Event(){}; 
};

template <> class Event <EV_SEND_INIT_INFO>{
    public:
        union {
            struct {
                int event_ID;
                int has_payload;
                int player_number;

            };
            uint8_t buffer_b[HEADER_LEN];
            int buffer_i[HEADER_LEN];
        };


        Event(int pnum){
            event_ID = EV_SEND_INIT_INFO;
            has_payload = PAYLOAD_OFF;
            player_number = pnum;
        }
        
        Event(uint8_t *buf){
            for(unsigned i=0; i<HEADER_LEN; i++){
                buffer_i[i] = (int)buf[i];
                buffer_b[i] = buf[i];
            }
        }

        Event(int *buf){
            for(unsigned i=0; i<HEADER_LEN; i++){
                buffer_i[i] = buf[i];
                buffer_b[i] = buf[i];
            }
        }
        ~Event(){};
};

template <> class Event <EV_START_GAME>{
    public:
        union {
            struct {
                int event_ID;
                int has_payload;
                int player_number;
            };
            uint8_t buffer_b[HEADER_LEN];
        };

        Event(){
            event_ID = EV_START_GAME;
            has_payload = PAYLOAD_OFF;
            player_number = 0; // Not important here
        }
        
        Event(uint8_t *buf){
            for(unsigned i=0; i<HEADER_LEN; i++){
                buffer_b[i] = buf[i];
            }
        }

        ~Event(){};
};

// template <> class Event <EV_QUIT_GAME>{
//     public:
//         union {
//             struct {
//                 int event_ID;
//                 int has_payload;
//                 int player_number;
//             };
//             uint8_t buffer_b[HEADER_LEN];
//         };

//         Event(int pnum){
//             event_ID = EV_QUIT_GAME;
//             has_payload = PAYLOAD_OFF;
//             player_number = pnum; // Not important here
//         }
        
//         Event(uint8_t *buf){
//             for(unsigned i=0; i<HEADER_LEN; i++){
//                 buffer_b[i] = buf[i];
//             }
//         }

//         ~Event(){};
// };

template <> class Event <EV_PRESSED_SPACE>{
    public:
        union {
            struct {
                int event_ID;
                int has_payload;
                int player_number;
            };
            uint8_t buffer_b[HEADER_LEN];
        };

        Event(int pnum){
            event_ID = EV_PRESSED_SPACE;
            has_payload = PAYLOAD_OFF;
            player_number = pnum;
        }
        
        Event(uint8_t *buf){
            for(unsigned i=0; i<HEADER_LEN; i++){
                buffer_b[i] = buf[i];
            }
        }

        ~Event(){};
};

template <> class Event <EV_UPDATE_STATUS>{
    public:
        union {
            struct {
                int event_ID;
                int has_payload;
                int player_number;

                bool ready[2];
                bool connection_status[2];

            };
            uint8_t buffer_b[HEADER_LEN];
        };


        Event(int pnum, bool *ready_stat, bool *connection_stat){
            event_ID = EV_UPDATE_STATUS;
            has_payload = PAYLOAD_OFF;
            player_number = pnum;
            for(unsigned i=0; i<2; i++){
                ready[i] = ready_stat[i];
                connection_status[i] = connection_stat[i];
            }
            
        }
        
        Event(uint8_t *buf){
            for(unsigned i=0; i<HEADER_LEN; i++){
                buffer_b[i] = buf[i];
            }
        }
        ~Event(){}; 
};

template <> class Event <EV_STREAM>{
    public:
        union {
            struct {
                int event_ID;
                int payload_size;
                int player_number;

                int x0,y0,x1,y1;
            };
            uint8_t buffer_b[HEADER_LEN];
        };


        Event(int N, int x0p, int y0p, int x1p, int y1p){
            event_ID = EV_STREAM;
            payload_size = N;
            player_number = 0; // not really important
            x0 = x0p;
            y0 = y0p;
            x1 = x1p;
            y1 = y1p;            
        }
        
        Event(uint8_t *buf){
            for(unsigned i=0; i<HEADER_LEN; i++){
                buffer_b[i] = buf[i];
            }
        }

        ~Event(){};
};

template <> class Event <EV_PLAYER_WON>{
    public:
        union {
            struct {
                int event_ID;
                int has_payload;
                int player_number;

            };
            uint8_t buffer_b[HEADER_LEN];
        };


        Event(int pnum){
            event_ID = EV_PLAYER_WON;
            has_payload = PAYLOAD_OFF;
            player_number = pnum;
            
        }
        
        Event(uint8_t *buf){
            for(unsigned i=0; i<HEADER_LEN; i++){
                buffer_b[i] = buf[i];
            }
        }

        ~Event(){};
};

template <> class Event <EV_PRESSED_KEY>{
    public:
        union {
            struct {
                int event_ID;
                int has_payload;
                int player_number;
                int keycode;
            };
            uint8_t buffer_b[HEADER_LEN];
        };


        Event(int pnum, int key){
            event_ID = EV_PRESSED_KEY;
            has_payload = PAYLOAD_OFF;
            player_number = pnum;
            keycode = key;            
        }
        
        Event(uint8_t *buf){
            for(unsigned i=0; i<HEADER_LEN; i++){
                buffer_b[i] = buf[i];
            }
        }

        ~Event(){};
};

template <> class Event <EV_MOUSEBUTTONDOWN>{
    public:
        union {
            struct {
                int event_ID;
                int has_payload;
                int player_number;
                int button_number;
                int x,y;
            };
            uint8_t buffer_b[HEADER_LEN];
        };


        Event(int pnum, int button, int xp, int yp){
            event_ID = EV_MOUSEBUTTONDOWN;
            has_payload = PAYLOAD_OFF;
            player_number = pnum;
            button_number = button;   
            x = xp;
            y = yp;         
        }
        
        Event(uint8_t *buf){
            for(unsigned i=0; i<HEADER_LEN; i++){
                buffer_b[i] = buf[i];
            }
        }

        ~Event(){};
};

template <> class Event <EV_SEND_POT>{
    public:
        union {
            struct {
                int event_ID;
                int payload_size;
                int player_number;

                int x0,y0,x1,y1; // paddle positions
                int x, y, dx, dy;
                
            };
            uint8_t buffer_b[HEADER_LEN];
        };


        Event(int xp, int yp, int ddx, int ddy, int x0p, int y0p, int x1p, int y1p){
            event_ID = EV_SEND_POT;
            payload_size = ddx*ddy;
            player_number = 0; // not really important
            x = xp;
            y = yp;
            dx = ddx;
            dy = ddy;

            // Paddle positions 
            x0 = x0p;
            y0 = y0p;
            x1 = x1p;
            y1 = y1p;     

        }
        
        Event(uint8_t *buf){
            for(unsigned i=0; i<HEADER_LEN; i++){
                buffer_b[i] = buf[i];
            }
        }

        ~Event(){};
};

#endif // EVENT_QUEUE_H