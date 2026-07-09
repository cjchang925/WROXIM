

#include "TxRxCircuit.h"

int calculateHammingRedundantBits(int flit_size) {
    // 2^r >= m + r + 1
    int m = flit_size;
    int r = ceil(log2(m + 1));

    while (pow(2, r) < m + r + 1) {
        r++; 
    }

    return r;
}

// Serializer
void Serializer::data_buffering() {
    if(reset.read()) {
        prev_ack.write(0);
        flit_busy.write(false);
        ack_busy.write(false);
        read_enable.write(1);
    }
    else {
        Flit received_flit = flit_in.read();
        bool received_req = req_rx.read();
        ACK received_ack = ack_in.read();
        if (current_level == 1 - req_rx.read() && !flit_busy.read()) {
            // flit_out.write(received_flit);
            // req_tx.write(received_req);
            // ack_out.write(received_ack);
            sc_biguint<130> temp_val = (sc_biguint<130>(1) << (GlobalParams::flit_size + 2)) - 1; // GlobalParams::flit_size+2 : data+req+ack
            sc_lv<130> input_vec = temp_val;
            fifo.write(input_vec);
            flit_busy.write(true);
            if (prev_ack.read() != received_ack.value) {
                prev_ack.write(received_ack.value);
            }
            cout << "In Serializer::data_buffering(), local_id = " << local_id
            << ", s_id = " << s_id << ", ready to serialize " << flit_in.read()
            << ", flit_busy.write(true), fifo.write(" << input_vec << ")" << endl;
            read_enable.write(0);
        }
        else if (prev_ack.read() != received_ack.value && !ack_busy.read() && !flit_busy.read()) {
            sc_lv<130> input_vec = 1;
            fifo.write(input_vec);
            ack_busy.write(true);
            prev_ack.write(received_ack.value);

            cout << "In Serializer::data_buffering(), local_id = " << local_id
            << ", s_id = " << s_id << ", ack_busy.write(true), fifo.write(" << input_vec << ")" << endl;
            cout << "In Serializer::data_buffering(), received_ack.value = " << received_ack.value
            << ", prev_ack.read() = " << prev_ack.read() << endl;

        }
    }
}

void Serializer::serialize() {
    if (reset.read()) {
        current_level = 0;
        shift_reg.write(0);
        bit_counter.write(0);
        read_enable.write(1);
    }
    else {
        Flit received_flit = flit_in.read();
        bool received_req = req_rx.read();
        ACK received_ack = ack_in.read();
        if (flit_busy.read() || ack_busy.read()) {
            if (fifo.num_available() > 0) {
                sc_lv<130> data;
                fifo.read(data);
                shift_reg.write(data);
                bit_counter.write(0);
                bits_out.write(sc_logic('0')); 
            }
            else {
                sc_logic bit_value = shift_reg.read()[bit_counter.read()];
                if (flit_busy.read()) {
                    if ((received_flit.flit_type == FLIT_TYPE_FLOWCONTROL)
                    || (received_flit.flit_type != FLIT_TYPE_FLOWCONTROL
                    && bit_counter.read() == GlobalParams::flit_size + 2 - 1)) {
                        shift_reg.write(0);
                        bit_counter.write(0);
                        flit_busy.write(false);
                        bits_out.write(bit_value);
                        flit_out.write(received_flit);
                        req_tx.write(received_req);
                        ack_out.write(received_ack);

                        current_level = 1 - current_level;

                        //power 
                        if (flit_in.read().flit_type == FLIT_TYPE_FLOWCONTROL) {
                            int src_mid = chipletOpticalPower.getMasterId(received_flit.src_id);
                            int dst_mid = chipletOpticalPower.getMasterId(received_flit.dst_id);
                            PathInfo tmpPathInfo = chipletOpticalPower.getPathInfo(src_mid, dst_mid);
                            chipletOpticalPower.addNewTxTime(src_mid, tmpPathInfo.Wavelength_type, (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps), -1);
                            chipletOpticalPower.addNewTxTime(src_mid, tmpPathInfo.Wavelength_type, -1, (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps));
                        }
                        else {
                            if (GlobalParams::use_TxRx_PreciseTiming) {
                                int src_mid = chipletOpticalPower.getMasterId(received_flit.src_id);
                                int dst_mid = chipletOpticalPower.getMasterId(received_flit.dst_id);
                                PathInfo tmpPathInfo = chipletOpticalPower.getPathInfo(src_mid, dst_mid);
                                chipletOpticalPower.addNewTxTime(src_mid, tmpPathInfo.Wavelength_type, -1, (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps));
                            }
                            else {
                                if (received_flit.flit_type == FLIT_TYPE_TAIL) {
                                    int src_mid = chipletOpticalPower.getMasterId(received_flit.src_id);
                                    int dst_mid = chipletOpticalPower.getMasterId(received_flit.dst_id);
                                    PathInfo tmpPathInfo = chipletOpticalPower.getPathInfo(src_mid, dst_mid);
                                    chipletOpticalPower.addNewTxTime(src_mid, tmpPathInfo.Wavelength_type, -1, (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps));
                                    cout << "In Serializer::serialize(), addNewTxTime: local_id = " << local_id
                                    << ", src_master_id = " << src_mid <<", Wavelength_type = " << tmpPathInfo.Wavelength_type
                                    << ", end_time = " << (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps) << endl << endl;
                                }
                            }
                        }
                        read_enable.write(1);
                    }
                    else {
                        bits_out.write(bit_value); 
                        flit_out.write(received_flit);
                        req_tx.write(received_req);
                        ack_out.write(received_ack);

                        //power
                        if(GlobalParams::use_TxRx_PreciseTiming){
                            if(bit_counter.read() == 0) {
                                int src_mid = chipletOpticalPower.getMasterId(received_flit.src_id);
                                int dst_mid = chipletOpticalPower.getMasterId(received_flit.dst_id);
                                PathInfo tmpPathInfo = chipletOpticalPower.getPathInfo(src_mid, dst_mid);
                                chipletOpticalPower.addNewTxTime(src_mid, tmpPathInfo.Wavelength_type, (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps), -1);
                            }
                        }
                        else {
                            if (bit_cnt == 0 && received_flit.flit_type == FLIT_TYPE_HEAD) {
                                int src_mid = chipletOpticalPower.getMasterId(received_flit.src_id);
                                int dst_mid = chipletOpticalPower.getMasterId(received_flit.dst_id);
                                PathInfo tmpPathInfo = chipletOpticalPower.getPathInfo(src_mid, dst_mid);
                                chipletOpticalPower.addNewTxTime(src_mid, tmpPathInfo.Wavelength_type, (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps), -1);
                                cout << "In Serializer::serialize(), addNewTxTime: local_id = " << local_id
                                << ", src_master_id = " << src_mid << ", Wavelength_type = " << tmpPathInfo.Wavelength_type
                                << ", start_time = " << (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps) << endl;
                            }
                        }

                        bit_counter.write(bit_counter.read() + 1);
                        read_enable.write(0);
                    }
                    cout << "In Serializer::serialize(), local_id = " << local_id
                    << ", s_id = " << s_id << ", flit_out.write(" << received_flit << ")"
                    << ", bits_out.write(" << bit_value << "), bit_counter = " << bit_counter << endl;
                }
                else if (ack_busy.read()) {
                    bits_out.write(bit_value); 
                    ack_out.write(received_ack);
                    shift_reg.write(0);
                    ack_busy.write(false);

                    //power
                    int src_mid = chipletOpticalPower.getMasterId(received_ack.src_id);
                    int dst_mid = chipletOpticalPower.getMasterId(received_ack.dst_id);
                    PathInfo tmpPathInfo = chipletOpticalPower.getPathInfo(src_mid, dst_mid);
                    chipletOpticalPower.addNewTxTime(src_mid, tmpPathInfo.Wavelength_type, (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps), -1);
                    chipletOpticalPower.addNewTxTime(src_mid, tmpPathInfo.Wavelength_type, -1, (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps));

                    cout << "In Serializer::serialize(), local_id = " << local_id
                    << ", s_id = "<< s_id << ", ack_out.write(" << received_ack
                    << "), bits_out.write(" << bit_value<<"), bit_counter = "<< bit_counter << endl;
                } 
            }
        }
        else {
            bits_out.write(sc_logic('0')); 
            // cout << "In Serializer::serialize(), local_id = " << local_id << ", s_id = "<<s_id<<", bits_out.write(sc_logic('0'))" << endl;
        }
    }
}

// Deserializer
void  Deserializer::send_flit() {
    if (reset.read()) {
        req_tx.write(0);
    } 
    else {
        Flit received_flit = flit_in.read();
        bool received_req = req_rx.read();
        ACK received_ack = ack_in.read();
        if (flit_ready.read() && fifo.num_available() > 0) {
            sc_lv<130> tmp;
            fifo.read(tmp);
            shift_reg.write(0);

            flit_out.write(received_flit);
            req_tx.write(received_req);
            ack_out.write(received_ack);
            
            flit_ready.write(false); 

            cout << "In Deserializer::send_flit(), local_id = " << local_id
            << ", r_id = " << r_id << ", flit_out.write(" << received_flit << ")" << endl;
        }
        else if (ack_ready.read() && fifo.num_available() > 0) {
            sc_lv<130> tmp;
            fifo.read(tmp);
            ack_out.write(received_ack);
            ack_ready.write(false);
            cout << "In Deserializer::send_flit(), ack_out.write(" << ack_out << ")" << endl;
        }
    }
}
void Deserializer::deserialize() {
    if (reset.read()) {
        current_level = 0;
        shift_reg.write(0);
        bit_counter.write(0);
        flit_ready.write(false);
        ack_ready.write(false);
        prev_ack.write(false);
    }
    else {
        if (bits_in.read() == '1' && current_level == 1 - req_rx.read()) {
            if ((flit_in.read().flit_type == FLIT_TYPE_FLOWCONTROL) || (flit_in.read().flit_type != FLIT_TYPE_FLOWCONTROL && bit_counter.read() == GlobalParams::flit_size + 2 - 1)) {
                fifo.write(shift_reg.read());
                bit_counter.write(0);
                flit_ready.write(true);
                current_level = 1 - current_level;
                if (prev_ack.read() != ack_in.read().value) {
                    prev_ack.write(ack_in.read().value);
                }
                //power
                if (flit_in.read().flit_type == FLIT_TYPE_FLOWCONTROL) {
                    int src_mid = chipletOpticalPower.getMasterId(flit_in.read().src_id);
                    int dst_mid = chipletOpticalPower.getMasterId(flit_in.read().dst_id);
                    PathInfo tmpPathInfo = chipletOpticalPower.getPathInfo(src_mid, dst_mid);
                    chipletOpticalPower.addNewTxTime(src_mid, tmpPathInfo.Wavelength_type, (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps), -1);
                    chipletOpticalPower.addNewTxTime(src_mid, tmpPathInfo.Wavelength_type, -1, (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps));
                }
                else {
                    if (GlobalParams::use_TxRx_PreciseTiming) {
                        int src_mid = chipletOpticalPower.getMasterId(flit_in.read().src_id);
                        int dst_mid = chipletOpticalPower.getMasterId(flit_in.read().dst_id);
                        PathInfo tmpPathInfo = chipletOpticalPower.getPathInfo(src_mid, dst_mid);
                        chipletOpticalPower.addNewRxTime(dst_mid, tmpPathInfo.Wavelength_type, -1, (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps));
                    }
                    else {
                        if(flit_in.read().flit_type == FLIT_TYPE_TAIL){
                            int src_mid = chipletOpticalPower.getMasterId(flit_in.read().src_id);
                            int dst_mid = chipletOpticalPower.getMasterId(flit_in.read().dst_id);
                            PathInfo tmpPathInfo = chipletOpticalPower.getPathInfo(src_mid, dst_mid);
                            chipletOpticalPower.addNewRxTime(dst_mid, tmpPathInfo.Wavelength_type, -1, (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps));
                            cout << "In Deserializer::deserialize(), addNewRxTime: local_id = " << local_id
                            << ", dst_master_id = " << dst_mid <<", Wavelength_type = " << tmpPathInfo.Wavelength_type
                            << ", end_time = " << (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps) << endl;
                        }
                    }
                }
                cout << "In Deserializer::deserialize(), local_id = " << local_id
                << ", r_id = " << r_id << ", flit_in.read() = " << flit_in.read()
                << ", bits_in.read() == '1', bit_counter = " << bit_counter << ", last bit" << endl;
            }
            else {
                sc_lv<130> reg_val = shift_reg.read();
                reg_val[bit_counter.read()] = bits_in.read();
                shift_reg.write(reg_val);
                
                //power
                if(GlobalParams::use_TxRx_PreciseTiming){
                    if (bit_counter.read() == 0) {
                        int src_mid = chipletOpticalPower.getMasterId(flit_in.read().src_id);
                        int dst_mid = chipletOpticalPower.getMasterId(flit_in.read().dst_id);
                        PathInfo tmpPathInfo = chipletOpticalPower.getPathInfo(src_mid, dst_mid);
                        chipletOpticalPower.addNewRxTime(dst_mid, tmpPathInfo.Wavelength_type, (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps), -1);
                    }
                }
                else {
                    if (bit_counter.read() == 0 && flit_in.read().flit_type == FLIT_TYPE_HEAD) {
                        int src_mid = chipletOpticalPower.getMasterId(flit_in.read().src_id);
                        int dst_mid = chipletOpticalPower.getMasterId(flit_in.read().dst_id);
                        PathInfo tmpPathInfo = chipletOpticalPower.getPathInfo(src_mid, dst_mid);
                        chipletOpticalPower.addNewRxTime(dst_mid, tmpPathInfo.Wavelength_type, (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps), -1);
                        cout << "In Deserializer::deserialize(), addNewRxTime: local_id = " << local_id
                        << ", dst_master_id = " << dst_mid << ", Wavelength_type = " << tmpPathInfo.Wavelength_type
                        << ", start_time = " << (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps) << endl;
                    }
                }
                bit_counter.write(bit_counter.read() + 1);
                cout << "In Deserializer::deserialize(), local_id = " << local_id
                << ", r_id = " << r_id << ", flit_in.read() = " << flit_in.read()
                << ", bits_in.read() == '1', bit_counter = " << bit_counter << endl;
            }
        }
        else if (bits_in.read() == '1' && ack_in.read().value != prev_ack.read()){
            prev_ack.write(ack_in.read().value);
            ack_ready.write(true);
            fifo.write(1);
            
            //power
            int src_mid = chipletOpticalPower.getMasterId(ack_in.read().src_id);
            int dst_mid = chipletOpticalPower.getMasterId(ack_in.read().dst_id);
            PathInfo tmpPathInfo = chipletOpticalPower.getPathInfo(src_mid, dst_mid);
            chipletOpticalPower.addNewRxTime(dst_mid, tmpPathInfo.Wavelength_type, (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps), -1);
            chipletOpticalPower.addNewRxTime(dst_mid, tmpPathInfo.Wavelength_type, -1, (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps));

            cout << "In Deserializer::deserialize(), local_id = " << local_id
            << ", r_id = " << r_id << ",  bits_in.read() == '1', receive ack_in.read() = "
            << ack_in.read() << endl;
        }
        else if (bits_in.read() == '1') {
            cout << "In Deserializer::deserialize(), missing bit, local_id = " << local_id
            << ", r_id = " << r_id << ", flit_in.read() = " << flit_in.read()
            << ", bits_in.read() == '1', current_level = " << current_level
            << ", req_rx.read() = " << req_rx.read() << endl;
        }
        
    }
}



// void Serializer::serialize()
// {
//     if(reset.read()){
//         bit_cnt = 0;
//         current_level = 0;
//         prev_ack = 0;
//         read_enable.write(1);
//     }
//     else {
//         Flit received_flit = flit_in.read();
//         bool received_req = req_rx.read();
//         ACK received_ack = ack_in.read();
//         if(bit_cnt < (GlobalParams::flit_size+2)-16 && current_level == 1 - req_rx.read()){ //calculateHammingRedundantBits(GlobalParams::flit_size)
//             flit_out.write(received_flit);
//             req_tx.write(received_req);
//             ack_out.write(received_ack);
//             sc_uint<16> flit_seq_value = (1 << 16) - 1;  // 65535
//             bits_out.write(flit_seq_value);
//             cout << "\nIn Serializer::serialize(), sc_time_stamp().to_double() = " << sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps << endl;
//             cout << "In Serializer::serialize(), received_flit = " << received_flit << ", bit_cnt = " << bit_cnt << endl << endl;
//             if(GlobalParams::use_TxRx_PreciseTiming){
//                 if(bit_cnt == 0){
//                     int src_mid = chipletOpticalPower.getMasterId(received_flit.src_id);
//                     int dst_mid = chipletOpticalPower.getMasterId(received_flit.dst_id);
//                     PathInfo tmpPathInfo = chipletOpticalPower.getPathInfo(src_mid, dst_mid);
//                     chipletOpticalPower.addNewTxTime(src_mid, tmpPathInfo.Wavelength_type, (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps), -1);
//                 }
//             }
//             else {
//                 if(bit_cnt == 0 && received_flit.flit_type == FLIT_TYPE_HEAD){
//                     int src_mid = chipletOpticalPower.getMasterId(received_flit.src_id);
//                     int dst_mid = chipletOpticalPower.getMasterId(received_flit.dst_id);
//                     PathInfo tmpPathInfo = chipletOpticalPower.getPathInfo(src_mid, dst_mid);
//                     chipletOpticalPower.addNewTxTime(src_mid, tmpPathInfo.Wavelength_type, (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps), -1);
//                     cout << "In serializer(), addNewTxTime: local_id = " << local_id << ", src_master_id = " << src_mid << ", Wavelength_type = " << tmpPathInfo.Wavelength_type << ", start_time = " << (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps) << endl;
//                 }
//             }
//             bit_cnt += 16;
//             read_enable.write(0);
//         }
//         else if(bit_cnt >= (GlobalParams::flit_size+2)-16 && current_level == 1 - req_rx.read()){    //(bit_cnt >= flit_size) : last data
//             sc_uint<16> flit_seq_value = (1 << 16) - 1;  // 65535
//             bits_out.write(flit_seq_value);
//             cout << "\nIn Serializer::serialize(), sc_time_stamp().to_double() = " << sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps << endl;
//             cout << "In Serializer::serialize(), last data received_flit = " << received_flit << ", bit_cnt = " << bit_cnt << endl << endl;
//             if(GlobalParams::use_TxRx_PreciseTiming){
//                 int src_mid = chipletOpticalPower.getMasterId(received_flit.src_id);
//                 int dst_mid = chipletOpticalPower.getMasterId(received_flit.dst_id);
//                 PathInfo tmpPathInfo = chipletOpticalPower.getPathInfo(src_mid, dst_mid);
//                 chipletOpticalPower.addNewTxTime(src_mid, tmpPathInfo.Wavelength_type, -1, (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps));
//             }
//             else {
//                 if(received_flit.flit_type == FLIT_TYPE_TAIL){
//                     int src_mid = chipletOpticalPower.getMasterId(received_flit.src_id);
//                     int dst_mid = chipletOpticalPower.getMasterId(received_flit.dst_id);
//                     PathInfo tmpPathInfo = chipletOpticalPower.getPathInfo(src_mid, dst_mid);
//                     chipletOpticalPower.addNewTxTime(src_mid, tmpPathInfo.Wavelength_type, -1, (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps));
//                     cout << "In serializer(), addNewTxTime: local_id = " << local_id << ", src_master_id = " << src_mid <<", Wavelength_type = " << tmpPathInfo.Wavelength_type << ", end_time = " << (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps) << endl << endl;
//                 }
//             }
//             bit_cnt = 0;
//             read_enable.write(1);
//             // req_tx.write(received_req);
//             current_level = 1 - current_level;
//             prev_ack = received_ack.value;
//         }
//         else if (prev_ack != received_ack.value) {    //only transmitts ack
//             sc_uint<16> ack_seq_value = (1 << 8) - 1;
//             bits_out.write(ack_seq_value);
//             ack_out.write(received_ack);
//             cout << "In serializer(), local_id = "<<local_id<<", s_id = "<<s_id<<", prev_ack != received_ack, prev_ack = "<<prev_ack<<", received_ack = "<<received_ack.value<<", bits_out.write("<< ack_seq_value<<")" << endl;
//             prev_ack = received_ack.value;
//             int src_mid = chipletOpticalPower.getMasterId(received_ack.src_id);
//             int dst_mid = chipletOpticalPower.getMasterId(received_ack.dst_id);
//             PathInfo tmpPathInfo = chipletOpticalPower.getPathInfo(src_mid, dst_mid);
//             chipletOpticalPower.addNewTxTime(src_mid, tmpPathInfo.Wavelength_type, (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps), -1);
//             chipletOpticalPower.addNewTxTime(src_mid, tmpPathInfo.Wavelength_type, -1, (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps));
//         }
//         else {
//             bits_out.write(0);
//             read_enable.write(1);
//         }
//     }
// }


// void deserializer::deserialize()
// {
//     if(reset.read()){
//         bit_cnt = 0;
//         current_level = 0;
//         prev_ack = 0;
//     }
//     else {
//         Flit received_flit = flit_in.read();
//         bool received_req = req_rx.read();
//         ACK received_ack = ack_in.read();
//         if(bit_cnt >= (GlobalParams::flit_size+2)-16 && bits_in.read() == (1<<16)-1 && (current_level == 1 - req_rx.read())){  // last data
//             flit_out.write(received_flit);
//             req_tx.write(received_req);
//             ack_out.write(received_ack);
//             prev_ack = received_ack.value;
            
//             cout << "\nIn Deserializer::deserialize(), sc_time_stamp().to_double() = " << sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps << endl;
//             cout << "In Deserializer::deserialize(), flit_out.write(received_flit) = " << received_flit << ", bit_cnt = " << bit_cnt << endl << endl;
//             if(GlobalParams::use_TxRx_PreciseTiming){
//                 int src_mid = chipletOpticalPower.getMasterId(received_flit.src_id);
//                 int dst_mid = chipletOpticalPower.getMasterId(received_flit.dst_id);
//                 PathInfo tmpPathInfo = chipletOpticalPower.getPathInfo(src_mid, dst_mid);
//                 chipletOpticalPower.addNewRxTime(dst_mid, tmpPathInfo.Wavelength_type, -1, (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps));
//             }
//             else {
//                 if(received_flit.flit_type == FLIT_TYPE_TAIL){
//                     int src_mid = chipletOpticalPower.getMasterId(received_flit.src_id);
//                     int dst_mid = chipletOpticalPower.getMasterId(received_flit.dst_id);
//                     PathInfo tmpPathInfo = chipletOpticalPower.getPathInfo(src_mid, dst_mid);
//                     chipletOpticalPower.addNewRxTime(dst_mid, tmpPathInfo.Wavelength_type, -1, (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps));
//                     cout << "In deserialize(), addNewRxTime: local_id = " << local_id << ", dst_master_id = " << dst_mid <<", Wavelength_type = " << tmpPathInfo.Wavelength_type << ", end_time = " << (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps) << endl;
//                 }
//             }
            

//             current_level = 1 - current_level;

//             bit_cnt = 0;
//         }
//         else if(bit_cnt < (GlobalParams::flit_size+2)-16 && bits_in.read() == (1<<16)-1 && (current_level == 1 - req_rx.read())){
//             cout << "\nIn Deserializer::deserialize(), sc_time_stamp().to_double() = " << sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps << endl;
//             cout << "In Deserializer::deserialize(), received_flit = "<<received_flit<<", bit_cnt = " << bit_cnt << endl << endl;
//             Flit received_flit = flit_in.read();
//             if(GlobalParams::use_TxRx_PreciseTiming){
//                 if(bit_cnt == 0){
//                     int src_mid = chipletOpticalPower.getMasterId(received_flit.src_id);
//                     int dst_mid = chipletOpticalPower.getMasterId(received_flit.dst_id);
//                     PathInfo tmpPathInfo = chipletOpticalPower.getPathInfo(src_mid, dst_mid);
//                     chipletOpticalPower.addNewRxTime(dst_mid, tmpPathInfo.Wavelength_type, (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps), -1);
//                 }
//             }
//             else {
//                 if(bit_cnt == 0 && received_flit.flit_type == FLIT_TYPE_HEAD){
//                     int src_mid = chipletOpticalPower.getMasterId(received_flit.src_id);
//                     int dst_mid = chipletOpticalPower.getMasterId(received_flit.dst_id);
//                     PathInfo tmpPathInfo = chipletOpticalPower.getPathInfo(src_mid, dst_mid);
//                     chipletOpticalPower.addNewRxTime(dst_mid, tmpPathInfo.Wavelength_type, (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps), -1);
//                     cout << "In deserialize(), addNewRxTime: local_id = " << local_id << ", dst_master_id = " << dst_mid << ", Wavelength_type = " << tmpPathInfo.Wavelength_type << ", start_time = " << (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps) << endl;
//                 }
//             }
            
//             bit_cnt += 16;
//         }
//         else if (bits_in.read() == (1<<8)-1 && prev_ack!=received_ack.value) {    //only transmitts ack
//             cout << "In deserialize(), local_id = "<<local_id<<", r_id = "<<r_id<<", bits_in.read() == (1<<8)-1, ack_out.write("<<received_ack.value<<")." << endl;
//             ack_out.write(received_ack);
//             prev_ack = received_ack.value;

//             int src_mid = chipletOpticalPower.getMasterId(received_ack.src_id);
//             int dst_mid = chipletOpticalPower.getMasterId(received_ack.dst_id);
//             PathInfo tmpPathInfo = chipletOpticalPower.getPathInfo(src_mid, dst_mid);
//             chipletOpticalPower.addNewRxTime(dst_mid, tmpPathInfo.Wavelength_type, (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps), -1);
//             chipletOpticalPower.addNewRxTime(dst_mid, tmpPathInfo.Wavelength_type, -1, (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps));
//         }
//     }

// }