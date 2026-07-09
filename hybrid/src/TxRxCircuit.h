
#ifndef __TXRXCIRCUIT_H__
#define __TXRXCIRCUIT_H__

#include <systemc.h>
#include <csignal>
#include "DataStructs.h"
#include "OpticalPower.h"
#include "Utils.h"

using namespace std;

int calculateHammingRedundantBits(int flit_size);

SC_MODULE(Serializer)
{
    // sc_in_clk clk;
    sc_in_clk clk_optical;
    sc_in<bool> reset;


    sc_in<Flit> flit_in;    //from router buffer
    sc_in<bool> req_rx;     //from router buffer
    sc_in<ACK> ack_in;

    // sc_out<sc_uint<16>> bits_out;  // Send 16 bits per cycle
    sc_out<Flit> flit_out;
    sc_out<bool> req_tx;
    sc_out<ACK> ack_out;
    sc_out<sc_logic> bits_out;

    sc_out<bool> read_enable;   //to Router

    
    sc_signal<sc_lv<130>> shift_reg;
    sc_signal<int> bit_counter;
    sc_signal<bool> flit_busy;
    sc_signal<bool> ack_busy;
    sc_signal<bool>  prev_ack;

    sc_fifo<sc_lv<130>> fifo;
    
    int local_id;
    bool current_level;
    
    int bit_cnt;

    int s_id;


    ChipletOpticalPower chipletOpticalPower;

    SC_CTOR(Serializer) : fifo(2){

        SC_METHOD(data_buffering);
        sensitive << reset;
        sensitive << clk_optical.pos();
        
        SC_METHOD(serialize);
        sensitive << reset;
        sensitive << clk_optical.pos();

    }

    void data_buffering();
    void serialize();
    
};
SC_MODULE(Deserializer) 
{
    // sc_in_clk clk;
    sc_in_clk clk_optical;
    sc_in<bool> reset;

    sc_in<Flit> flit_in;    //from router buffer
    sc_in<bool> req_rx;     //from router buffer
    sc_in<ACK> ack_in;
    sc_in<sc_logic> bits_in;  // Send 16 bits per cycle

    sc_out<Flit> flit_out;
    sc_out<bool> req_tx;
    sc_out<ACK> ack_out;

    int local_id;
    bool current_level;
    sc_signal<bool>  prev_ack;

    sc_signal<sc_lv<130>> shift_reg;
    sc_signal<int> bit_counter;
    sc_signal<bool> flit_ready;
    sc_signal<bool> ack_ready;
    

    sc_fifo<sc_lv<130>> fifo;

    int r_id;


    ChipletOpticalPower chipletOpticalPower;

    SC_CTOR(Deserializer) : fifo(2){

        SC_METHOD(deserialize);
        sensitive << reset;
        sensitive << clk_optical.pos();

        SC_METHOD(send_flit);
        sensitive << reset;
        sensitive << clk_optical.pos();
    }

    void send_flit();
    void deserialize();
};

#endif