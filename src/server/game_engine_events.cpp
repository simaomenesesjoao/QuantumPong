


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


// void game_engine::on_add_mag(int *data){
//     // int player_number = data[1];
//     // int x = data[2];
//     // int y = data[3];

//     // if(VERBOSE>0){ std::cout << "p" << player_number << " on_add_pot\n";}

//     // int data_send[HEADER_LEN];
//     // data_send[0] = PAYLOAD_OFF;
//     // data_send[1] = EV_ADD_POT;
//     // data_send[2] = x/256;
//     // data_send[3] = x%256;
//     // data_send[4] = y/256;
//     // data_send[5] = y%256;

//     // for(unsigned player=0; player<2; player++)
//     //     sender(player, data_send, HEADER_LEN);
// }
