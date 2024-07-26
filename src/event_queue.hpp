#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H 1

#include <mutex>
#include "macros.hpp"


class event_queue{

    private:
        std::mutex mutex;
        unsigned tail, head, queue_size, queue_width;
        int event_queue_verbose;
        

    public:
        
        uint8_t **events;
        
        event_queue(unsigned, unsigned, int event_queue_verbose = 0);
        ~event_queue();
        
        void add_event(uint8_t *);
        void pop_oldest(int*, uint8_t*);

};


class buffer{

    private:
        std::mutex mutex, mutex_lock;
        unsigned tail, head, queue_size, queue_width;
        int buffer_verbose;
        bool locked; 
        uint8_t **data;

    public:
        
        
        uint8_t *write_data, *read_data;
        
        
        ~buffer();
        
        buffer(unsigned, unsigned, int buffer_verbose = 0);
        void peek(uint8_t*);
        void update_tail();
        void update_tail_by1();
        void lock();
        void unlock();
        void reset();

};


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
                int payload_size;
                int player_number;

                int socket;
            };
            uint8_t buffer_b[HEADER_LEN];
        };


        Event(int pnum, int sock){
            event_ID = EV_CONNECT;
            payload_size = PAYLOAD_OFF;
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
                int payload_size;
                int player_number;

            };
            uint8_t buffer_b[HEADER_LEN];
        };


        Event(int pnum){
            event_ID = EV_DISCONNECT;
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

template <> class Event <EV_SEND_INIT_INFO>{
    public:
        union {
            struct {
                int event_ID;
                int payload_size;
                int player_number;

            };
            uint8_t buffer_b[HEADER_LEN];
            int buffer_i[HEADER_LEN];
        };


        Event(int pnum){
            event_ID = EV_SEND_INIT_INFO;
            payload_size = PAYLOAD_OFF;
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

template <> class Event <EV_UPDATE_STATUS>{
    public:
        union {
            struct {
                int event_ID;
                int payload_size;
                int player_number;
                
                int state_p1;
                int state_p2;

            };
            uint8_t buffer_b[HEADER_LEN];
        };


        Event(int pnum, int playerState, int otherPlayerState){
            event_ID = EV_UPDATE_STATUS;
            payload_size = PAYLOAD_OFF;
            player_number = pnum;


            if(pnum == 0){
                state_p1 = playerState;
                state_p2 = otherPlayerState;
            } else {
                state_p2 = playerState;
                state_p1 = otherPlayerState;
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
                float score_top;
                float score_bot;
            };
            uint8_t buffer_b[HEADER_LEN];
        };


        Event(int N, int x0p, int y0p, int x1p, int y1p, float sc_t, float sc_b){
            event_ID = EV_STREAM;
            payload_size = N;
            player_number = 0; // not really important
            x0 = x0p;
            y0 = y0p;
            x1 = x1p;
            y1 = y1p;            
            score_top = sc_t;
            score_bot = sc_b;
        }
        
        Event(uint8_t *buf){
            for(unsigned i=0; i<HEADER_LEN; i++){
                buffer_b[i] = buf[i];
            }
        }

        ~Event(){};
};

template <> class Event <EV_PADDLE_UPDATE>{
    public:
        union {
            struct {
                int event_ID;
                int payload_size;
                int player_number;

                int x, y;
            };
            uint8_t buffer_b[HEADER_LEN];
        };


        Event(int pNum, int xp, int yp){ 
            event_ID = EV_PADDLE_UPDATE;
            payload_size = PAYLOAD_OFF;
            player_number = pNum; 
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

template <> class Event <EV_PRESSED_KEY>{
    public:
        union {
            struct {
                int event_ID;
                int payload_size;
                int player_number;
                int keycode;
            };
            uint8_t buffer_b[HEADER_LEN];
        };


        Event(int pnum, int key){
            event_ID = EV_PRESSED_KEY;
            payload_size = PAYLOAD_OFF;
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
                int payload_size;
                int player_number;
                int button_number;
                int x,y;
            };
            uint8_t buffer_b[HEADER_LEN];
        };


        Event(int pnum, int button, int xp, int yp){
            event_ID = EV_MOUSEBUTTONDOWN;
            payload_size = PAYLOAD_OFF;
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

template <> class Event <EV_SEND_MAG>{
    public:
        union {
            struct {
                int event_ID;
                int payload_size;
                int player_number;

                int x, y, dx, dy;
                
            };
            uint8_t buffer_b[HEADER_LEN];
        };


        Event(int xp, int yp, int ddx, int ddy){
            event_ID = EV_SEND_MAG;
            payload_size = ddx*ddy;
            player_number = 0; // not really important
            x = xp;
            y = yp;
            dx = ddx;
            dy = ddy;  

        }
        
        Event(uint8_t *buf){
            for(unsigned i=0; i<HEADER_LEN; i++){
                buffer_b[i] = buf[i];
            }
        }

        ~Event(){};
};

#endif // EVENT_QUEUE_H