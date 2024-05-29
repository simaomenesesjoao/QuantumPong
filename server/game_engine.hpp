

class game_engine{
    public:
        std::string event_names[100];
        event_queue *eq;
        simulator *physics;
        //connection_handler *conn_handler;
        std::mutex mutex;
        std::vector<std::thread> threads;

        // Game state
        bool *connection_status;
        bool ready[2], streaming[2];
        int sockets[2], ready_to_toggle[2];
        bool all_ready, paused, can_toggle, game_finished;


    game_engine();
    void game_loop();
    void listener(int);
    void streamer(int);
    void sender(int, int*, unsigned);
    void set_event_names();
    void print_state();

    void toggle_ready(int);

    void on_connect(int*);
    void on_disconnect(int*);
    void on_change_screen(int*);
    void on_send_init_info(int*);
    void on_pressed_space(int*);
    void on_update_status(int*);
    void on_player_won(int*);


};
