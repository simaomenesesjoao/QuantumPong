

// void game_engine::on_disconnect(uint8_t *data){

//     Event<EV_DISCONNECT> event(data);
//     int player_number = event.player_number;

//     if(VERBOSE>0){
//         std::cout << "p" << player_number << " on_disconnect\n";
//     }


//     // Check if this function was already called
//     if(!connection_status[player_number]) return;

//     can_toggle = false;
//     ready[0] = 0;
//     ready[1] = 0;
//     paused = true;
//     streaming[player_number] = false;
//     connection_status[player_number] = false;
//     sockets[player_number] = -1;

//     // Tell the other player to update the screen

//     int screen = 1;
//     // Event<EV_CHANGE_SCREEN> event_change_screen(1-player_number, screen);
//     eq->add_event(Event<EV_CHANGE_SCREEN>(1-player_number, screen).buffer_b);

// }


// void game_engine::on_mousebuttondown(uint8_t *data){

//     Event<EV_MOUSEBUTTONDOWN> event(data);
//     int player_number = event.player_number;
//     int button = event.button_number;
//     int x = event.x;
//     int y = event.y;


//     if(VERBOSE>0){
//         std::cout << "p" << player_number << " on_mousebuttondown. button:" << button << " ";
//         std::cout << "xy: " << x << " " << y <<  "\n";
//     }

//     if(button == 1){
//         unsigned w = 70;
//         float v = 0.9;
//         physics->set_local_pot(x, y, w, w, v);
        
//     }

//     // if(button == 3){
//     //     unsigned w = 70;
//     //     float v = 0.005;
//     //     physics->set_local_B(x, y, v);

//     //     EventUnion<EV_ADD_POT> event;
//     //     event.event_data.event_ID = EV_ADD_POT;
//     //     event.event_data.has_payload = false;
//     //     event.event_data.player_number = player_number;
//     //     event.event_data.x = x;
//     //     event.event_data.y = y;
//     //     event.event_data.depth = v;
//     //     event.event_data.radius = w;

//     //     eq->add_event(event.buffer);
//     // }
// }

// void game_engine::on_add_pot(uint8_t *data){
//     if(VERBOSE>0){ std::cout << " on_add_pot\n";}

//     Event<EV_SEND_POT> event(data);
//     int x = event.x;
//     int y = event.y;
//     int dx = event.dx;
//     int dy = event.dy;
//     int Lx = physics->Lx;
//     int Ly = physics->Ly;
//     if(x+dx > Lx) dx = Lx - x; 
//     if(y+dy > Ly) dy = Ly - y;

//     if(x<0){
//         dx += x;
//         x = 0;
//     }
//     if(y<0){
//         dy += y;
//         y = 0;
//     }
//     int buf_size = dx*dy;

//     event.payload_size = buf_size;
//     event.dx = dx;
//     event.dy = dy;

//     // std::cout << "event contains \n";
//     // for(int i=0; i<HEADER_LEN;i++)
//     //     std::cout << (int)event.buffer_b[i] << " ";
//     // std::cout << "\n" << std::flush;

//     std::cout << "pot event contains:\n";
//     std::cout << event.x0 << " " << event.y0 << " " << event.x1 << " " << event.y1 << "\n";



//     uint8_t buffer[buf_size+HEADER_LEN];
//     for(unsigned i=0; i<HEADER_LEN; i++)
//         buffer[i] = event.buffer_b[i];

//     physics->get_pot(x, y, dx, dy, buffer+HEADER_LEN);

//     // std::cout << "potential gotten from physics:\n";
//     // int n;
//     // for(int x0=0; x0<dx; x0++){
//     //     for(int y0=0; y0<dy; y0++){
//     //         n = x0 + dx*y0;
//     //         std::cout << (int)buffer[n+HEADER_LEN] << " ";
//     //     }
//     //     std::cout << "\n";
//     // }

//     // When the simulator is set up, it sends out an event that the potential
//     // has been modified (because it has), but the players may not yet be connected
//     // so this event needs to be handled with care
//     for(unsigned player=0; player<2; player++)
//         if(connection_status[player])
//             sender2(player, buffer, buf_size+HEADER_LEN);

// }


// // void game_engine::on_add_mag(int *data){
// //     // int player_number = data[1];
// //     // int x = data[2];
// //     // int y = data[3];

// //     // if(VERBOSE>0){ std::cout << "p" << player_number << " on_add_pot\n";}

// //     // int data_send[HEADER_LEN];
// //     // data_send[0] = PAYLOAD_OFF;
// //     // data_send[1] = EV_ADD_POT;
// //     // data_send[2] = x/256;
// //     // data_send[3] = x%256;
// //     // data_send[4] = y/256;
// //     // data_send[5] = y%256;

// //     // for(unsigned player=0; player<2; player++)
// //     //     sender(player, data_send, HEADER_LEN);
// // }

