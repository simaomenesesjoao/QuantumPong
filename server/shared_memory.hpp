
// Class containing all the memory objects required by other classes
// The objects are initialized in other classes
class shared_memory{
    public: 
    uint8_t *buffer;
    uint8_t *buffer_f;
    unsigned buffer_size;
    unsigned buffer_f_size;

    uint8_t *pot;
    uint8_t *pot_f;
    unsigned pot_size;
    unsigned pot_f_size;

    bool ready[2];
    bool is_paused;
    unsigned *V;

    bool connection_status[2];
    int socket[2];

    // connection player1, player2, engine, connection_handler
    sem_t *semaphores1[4];
    sem_t *semaphores2[4];
    std::mutex *mutexes[4];
    bool can_process[4];
    std::mutex mtx_ready;

};
