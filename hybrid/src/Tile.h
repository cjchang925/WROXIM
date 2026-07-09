/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2010 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file contains the declaration of the tile
 */

#ifndef __NOXIMTILE_H__
#define __NOXIMTILE_H__

#include <systemc.h>
#include "Router.h"
#include "ProcessingElement.h"
#include "TxRxCircuit.h"

#include <csignal>

using namespace std;

SC_MODULE(Tile)
{
    SC_HAS_PROCESS(Tile);

    // I/O Ports
    sc_in_clk clock;		                // The input clock for the tile
    sc_in <bool> reset;	                        // The reset signal for the tile

    int local_id; // Unique ID

    sc_in <Flit> flit_rx[DIRECTIONS];   // The input channels
    sc_in <bool> req_rx[DIRECTIONS];    // The requests associated with the input channels
    sc_out <bool> ack_rx[DIRECTIONS];   // The outgoing ack signals associated with the input channels

    sc_out <Flit> flit_tx[DIRECTIONS];	// The output channels
    sc_out <bool> req_tx[DIRECTIONS];   // The requests associated with the output channels
    sc_in <bool> ack_tx[DIRECTIONS];    // The outgoing ack signals associated with the output channels

    // hub specific ports
    // sc_in <Flit> hub_flit_rx;	// The input channels
    // sc_in <bool> hub_req_rx;	        // The requests associated with the input channels
    // sc_out <bool> hub_ack_rx;	        // The outgoing ack signals associated with the input channels

    // sc_out <Flit> hub_flit_tx;	// The output channels
    // sc_out <bool> hub_req_tx;	        // The requests associated with the output channels
    // sc_in <bool> hub_ack_tx;	        // The outgoing ack signals associated with the output channels

    sc_signal <Flit> hub_flit_rx;	// The input channels
    sc_signal <bool> hub_req_rx;	        // The requests associated with the input channels
    sc_signal <bool> hub_ack_rx;	        // The outgoing ack signals associated with the input channels

    sc_signal <Flit> hub_flit_tx;	// The output channels
    sc_signal <bool> hub_req_tx;	        // The requests associated with the output channels
    sc_signal <bool> hub_ack_tx;
    

    // NoP related I/O and signals
    sc_out <int> free_slots[DIRECTIONS];
    sc_in <int> free_slots_neighbor[DIRECTIONS];
    sc_out < NoP_data > NoP_data_out[DIRECTIONS];
    sc_in < NoP_data > NoP_data_in[DIRECTIONS];

    sc_signal <int> free_slots_local;
    sc_signal <int> free_slots_neighbor_local;

    // Signals required for Router-PE connection
    sc_signal <Flit> flit_rx_local;	
    sc_signal <bool> req_rx_local;     
    sc_signal <bool> ack_rx_local;

    sc_signal <Flit> flit_tx_local;
    sc_signal <bool> req_tx_local;
    sc_signal <bool> ack_tx_local;


    // Instances
    Router *r;		                // Router instance
    ProcessingElement *pe;	                // Processing Element instance

    // Constructor

    Tile(sc_module_name nm, int id): sc_module(nm) {
		local_id = id;
		
		// Router pin assignments
		r = new Router("Router");
		r->clock(clock);
		r->reset(reset);
		for (int i = 0; i < DIRECTIONS; i++) {
			r->flit_rx[i] (flit_rx[i]);
			r->req_rx[i] (req_rx[i]);
			r->ack_rx[i] (ack_rx[i]);

			r->flit_tx[i] (flit_tx[i]);
			r->req_tx[i] (req_tx[i]);
			r->ack_tx[i] (ack_tx[i]);

			r->free_slots[i] (free_slots[i]);
			r->free_slots_neighbor[i] (free_slots_neighbor[i]);

			// NoP 
			r->NoP_data_out[i] (NoP_data_out[i]);
			r->NoP_data_in[i] (NoP_data_in[i]);
		}
		
		// local
		r->flit_rx[DIRECTION_LOCAL] (flit_tx_local);
		r->req_rx[DIRECTION_LOCAL] (req_tx_local);
		r->ack_rx[DIRECTION_LOCAL] (ack_tx_local);

		r->flit_tx[DIRECTION_LOCAL] (flit_rx_local);
		r->req_tx[DIRECTION_LOCAL] (req_rx_local);
		r->ack_tx[DIRECTION_LOCAL] (ack_rx_local);


		// // hub related
		r->flit_rx[DIRECTION_HUB] (hub_flit_rx);
		r->req_rx[DIRECTION_HUB] (hub_req_rx);
		r->ack_rx[DIRECTION_HUB] (hub_ack_rx);

		r->flit_tx[DIRECTION_HUB] (hub_flit_tx);
		r->req_tx[DIRECTION_HUB] (hub_req_tx);
		r->ack_tx[DIRECTION_HUB] (hub_ack_tx);


		// Processing Element pin assignments
		pe = new ProcessingElement("ProcessingElement");
		pe->clock(clock);
		pe->reset(reset);

		pe->flit_rx(flit_rx_local);
		pe->req_rx(req_rx_local);
		pe->ack_rx(ack_rx_local);

		pe->flit_tx(flit_tx_local);
		pe->req_tx(req_tx_local);
		pe->ack_tx(ack_tx_local);

		// NoP
		//
		r->free_slots[DIRECTION_LOCAL] (free_slots_local);
		r->free_slots_neighbor[DIRECTION_LOCAL] (free_slots_neighbor_local);
		pe->free_slots_neighbor(free_slots_neighbor_local);

    }

};

//! -------------------------------------------------- WRONoC --------------------------------------------------
//! -------------------------------------------------- WRONoC --------------------------------------------------
//! -------------------------------------------------- WRONoC --------------------------------------------------
//! -------------------------------------------------- WRONoC --------------------------------------------------
//! -------------------------------------------------- WRONoC --------------------------------------------------

//! Central Router (without PE, receives Flits from all tiles and transmits them to all tiles)
SC_MODULE(Tile_Central) {

    SC_HAS_PROCESS(Tile_Central);

    sc_in_clk clock;
    sc_in<bool> reset;
    
    int local_id;
    sc_in<Flit>** flit_rx;
    sc_in<bool>** req_rx;
    sc_in<sc_logic>** bits_in;
    sc_in<ACK>** ack_rx;
    sc_out<Flit>** flit_tx;
    sc_out<bool>** req_tx;
    sc_out<sc_logic>** bits_out;
    sc_out<ACK>** ack_tx;



    Tile_Central(sc_module_name nm, int id): sc_module(nm) {
        local_id = id;
        int num_ports = GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y;
        flit_rx = new sc_in<Flit>*[num_ports];
        req_rx = new sc_in<bool>*[num_ports];
        bits_in = new sc_in<sc_logic>*[num_ports];
        ack_rx = new sc_in<ACK>*[num_ports];

        flit_tx = new sc_out<Flit>*[num_ports];
        req_tx = new sc_out<bool>*[num_ports];
        bits_out = new sc_out<sc_logic>*[num_ports];
        ack_tx = new sc_out<ACK>*[num_ports];
        for (int i = 0; i < num_ports; i++) {
            flit_rx[i] = new sc_in<Flit>[num_ports];
            req_rx[i] = new sc_in<bool>[num_ports];
            bits_in[i] = new sc_in<sc_logic>[num_ports];
            ack_rx[i] = new sc_in<ACK>[num_ports];

            flit_tx[i] = new sc_out<Flit>[num_ports];
            req_tx[i] = new sc_out<bool>[num_ports];
            bits_out[i] = new sc_out<sc_logic>[num_ports];
            ack_tx[i] = new sc_out<ACK>[num_ports];
        }


        SC_METHOD(transfer_flit);
        for (int i = 0; i < num_ports; ++i) {
            for (int j = 0; j < num_ports; ++j){
                sensitive << flit_rx[i][j];
                sensitive << req_rx[i][j];
                sensitive << bits_in[i][j];
                sensitive << ack_rx[i][j];
            }
        }
        dont_initialize();
    }


    
    void transfer_flit() {
        int num_ports = GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y;
        cout << "\nIn Central_Tile, transfering flit, req, ack..." << endl;
        for (int i = 0; i < num_ports; i++) {
            for (int j = 0; j < num_ports; j++) {
                bool send = false;
                if ((flit_rx[i][j].event() || req_rx[i][j].event() || bits_in[i][j].event())) {
                    send = true;
                    Flit received_flit = flit_rx[i][j].read();
                    int dst_id = received_flit.dst_id;
                    int src_id = received_flit.src_id;
                    if (dst_id == 0 && src_id == 0) {
                        cout << "In Tile_Central, only transfer ack signal." << endl;
                        send = false;
                    }
                    else if (dst_id >= 0 && dst_id < num_ports) {
                        flit_tx[dst_id][src_id].write(received_flit);
                        req_tx[dst_id][src_id].write(req_rx[i][j].read());
                        bits_out[dst_id][src_id].write(bits_in[i][j].read());
                        ack_tx[dst_id][src_id].write(ack_rx[i][j].read());
                        if (flit_rx[i][j].event())  
                            cout << "flit_rx[" << i << "][" << j << "].event() = true, flit_rx[" 
                                << i << "][" << j << "].read() = " << flit_rx[i][j].read() << endl;
                        else  
                            cout << "flit_rx[" << i << "][" << j << "].event() = false, flit_rx["
                                << i << "][" << j << "].read() = " << flit_rx[i][j].read() << endl;
                        if (req_rx[i][j].event())  
                            cout << "req_rx[" << i << "][" << j << "].event() = true, req_rx["
                                << i << "][" << j << "].read() = " << req_rx[i][j].read() << endl;
                        else 
                            cout << "req_rx[" << i << "][" << j << "].event() = false, req_rx["
                                << i << "][" << j << "].read() = " << req_rx[i][j].read() << endl;
                        if (bits_in[i][j].event()) 
                            cout << "bits_in[" << i << "][" << j << "].event() = true, bits_in["
                                << i << "][" << j << "].read() = " << bits_in[i][j].read() << endl;
                        else 
                            cout << "bits_in[" << i << "][" << j << "].event() = false, bits_in["
                                << i << "][" << j << "].read() = " << bits_in[i][j].read() << endl;
                        cout << "ack_tx[" << dst_id << "][" << src_id << "].write(" << ack_rx[i][j].read() << ");" << endl;
                        cout << "In Tile_Central, transfer_flit: src_id -> dst_id = " 
                            << src_id << " -> " << dst_id << endl << endl;
                    }
                    else {
                        cout << "In Tile_Central, transfer_flit: src_id -> dst_id = " << src_id << " -> " << dst_id << endl << endl;
                        SC_REPORT_ERROR("Tile_Central", "Invalid destination ID");
                    }

                }
                if ((ack_rx[i][j].event() || bits_in[i][j].event()) && !send){
                     
                    if (bits_in[i][j].event()) 
                        cout << "bits_in[" << i << "][" << j << "].event() = true, bits_in[" 
                            << i << "][" << j << "].read() = " << bits_in[i][j].read() << endl;
                    else 
                        cout << "bits_in[" << i << "][" << j << "].event() = false, bits_in[" 
                            << i << "][" << j << "].read() = " << bits_in[i][j].read() << endl;
                    if (ack_rx[i][j].event()) 
                        cout << "ack_rx[" << i << "][" << j << "].event() = true, ack_rx[" 
                            << i << "][" << j << "].read() = " << ack_rx[i][j].read() << endl;
                    else 
                        cout << "ack_rx[" << i << "][" << j << "].event() = false, ack_rx[" 
                            << i << "][" << j << "].read() = " << ack_rx[i][j].read() << endl;
                    
                    ACK received_ack = ack_rx[i][j].read();
                    int dst_id = received_ack.dst_id;
                    int src_id = received_ack.src_id;
                    cout << "In Tile_Central, transfer_ack: src_id -> dst_id = " << src_id << " -> " << dst_id << endl << endl;
                    if (dst_id >= 0 && dst_id < num_ports) {
                        bits_out[dst_id][src_id].write(bits_in[i][j].read());
                        ack_tx[dst_id][src_id].write(received_ack);
                    } else {
                        SC_REPORT_ERROR("Tile_Central", "Invalid destination ID by ack_rx[i][j].event()");
                    }
                }
            }
        }
    }
};

SC_MODULE(Tile_ONoC)
{
    SC_HAS_PROCESS(Tile_ONoC);

    // I/O Ports
    sc_in_clk clock;    // The input clock for the tile
    sc_in_clk clock_optical;
    sc_in <bool> reset; // The reset signal for the tile

    int local_id;   // Unique ID

    sc_in <Flit> *flit_rx_from_central_tile;    // The input channels
    sc_in <bool> *req_rx_from_central_tile;     // The requests associated with the input channels
    sc_in <sc_logic> *bits_in_from_central_tile;
    sc_out <ACK> *ack_rx_to_central_tile;       // The outgoing ack signals associated with the input channels

    sc_out <Flit> *flit_tx_to_central_tile;     // The output channels
    sc_out <bool> *req_tx_to_central_tile;      // The requests associated with the output channels
    sc_out <sc_logic> *bits_out_to_central_tile;
    sc_in <ACK> *ack_tx_from_central_tile;      // The outgoing ack signals associated with the output channels




    // NoP related I/O and signals
    // sc_out <int> free_slots_to_central_tile;
    // sc_in <int> free_slots_neighbor_from_central_tile;
    // sc_out < NoP_data > NoP_data_out_to_central_tile;
    // sc_in < NoP_data > NoP_data_in_from_central_tile;

    // sc_signal <int> free_slots_local;
    sc_signal <int> free_slots_neighbor_local;

    // Signals required for Router-PE connection
    sc_signal <Flit> flit_rx_local;	
    sc_signal <bool> req_rx_local;     
    sc_signal <bool> ack_rx_local;

    sc_signal <Flit> flit_tx_local;
    sc_signal <bool> req_tx_local;
    sc_signal <bool> ack_tx_local;

    sc_signal <Flit> *flit_to_serializer;
    sc_signal <bool> *req_to_serializer;
    sc_signal <ACK> *ack_to_serializer;
    sc_signal <Flit> *flit_from_deserializer;
    sc_signal <bool> *req_from_deserializer;
    sc_signal <ACK> *ack_from_deserializer;

    sc_signal <bool> *read_enable_from_serializer;
    // sc_signal <bool> read_enable_from_router;

    sc_signal <ACK> dummy_signal;


    // Instances
    Router_ONoC *r;		                // Router instance
    ProcessingElement *pe;	                // Processing Element instance

    Serializer **serializer;
    Deserializer **deserializer;

    // Constructor

    Tile_ONoC(sc_module_name nm, int id): sc_module(nm) {

        local_id = id;
        int num_ports = GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y;
        flit_rx_from_central_tile = new sc_in <Flit>[num_ports];
		req_rx_from_central_tile = new sc_in <bool>[num_ports];
        bits_in_from_central_tile = new sc_in <sc_logic>[num_ports];
        ack_rx_to_central_tile = new sc_out <ACK>[num_ports];
        
        flit_tx_to_central_tile = new sc_out <Flit>[num_ports];
		req_tx_to_central_tile = new sc_out <bool>[num_ports];
        bits_out_to_central_tile = new sc_out <sc_logic>[num_ports];
        ack_tx_from_central_tile = new sc_in <ACK>[num_ports];

        flit_from_deserializer = new sc_signal <Flit> [num_ports];
        req_from_deserializer = new sc_signal <bool> [num_ports];
        ack_from_deserializer = new sc_signal <ACK> [num_ports];
        flit_to_serializer = new sc_signal <Flit> [num_ports];
        req_to_serializer = new sc_signal <bool> [num_ports];
        ack_to_serializer = new sc_signal <ACK> [num_ports];

        read_enable_from_serializer = new sc_signal <bool> [num_ports];

        serializer = new Serializer*[num_ports];

        // cout << "Tile_ONoC's all signals and ports have been initialized." << endl;

        for (int i = 0; i < num_ports; i++) {
            char s_name[20];
            sprintf(s_name, "Serializer_[%02d]", i);
            serializer[i] = new Serializer(s_name);
            serializer[i]->clk_optical(clock_optical);
            serializer[i]->reset(reset);
            
            serializer[i]->flit_in(flit_to_serializer[i]);
            serializer[i]->req_rx(req_to_serializer[i]);
            serializer[i]->ack_in(ack_to_serializer[i]);


            serializer[i]->flit_out(flit_tx_to_central_tile[i]);
            serializer[i]->req_tx(req_tx_to_central_tile[i]);
            serializer[i]->ack_out(ack_rx_to_central_tile[i]);

            serializer[i]->bits_out(bits_out_to_central_tile[i]);

            serializer[i]->read_enable(read_enable_from_serializer[i]);

            serializer[i]->local_id = id;
            serializer[i]->s_id = i;
        }
        // cout << "Tile_ONoC's all serializers have been initialized." << endl;
       
        deserializer = new Deserializer*[num_ports];
        for (int i = 0; i < num_ports; i++) {
            char s_name[20];
            sprintf(s_name, "Deserializer_[%02d]", i);
            deserializer[i] = new Deserializer(s_name);
            deserializer[i]->clk_optical(clock_optical);
            deserializer[i]->reset(reset);

            deserializer[i]->flit_in(flit_rx_from_central_tile[i]);
            deserializer[i]->req_rx(req_rx_from_central_tile[i]);
            deserializer[i]->ack_in(ack_tx_from_central_tile[i]);
            deserializer[i]->bits_in(bits_in_from_central_tile[i]);

            deserializer[i]->flit_out(flit_from_deserializer[i]);
            deserializer[i]->req_tx(req_from_deserializer[i]);
            deserializer[i]->ack_out(ack_from_deserializer[i]);

            deserializer[i]->local_id = id;
            deserializer[i]->r_id = i;
        }
        // cout << "Tile_ONoC's all serializers and deserializers have been initialized." << endl;


		// Router pin assignments
		r = new Router_ONoC("Router_ONoC");
		r->clock(clock);
        r->clock_optical(clock_optical);
		r->reset(reset);

        //To/From central router
        for (int i=0;i<GlobalParams::mesh_dim_x*GlobalParams::mesh_dim_y;i++) {
            r->flit_rx[i+1] (flit_from_deserializer[i]);
            r->req_rx[i+1] (req_from_deserializer[i]);
            r->ack_tx[i+1] (ack_from_deserializer[i]);
        }
        
        
        for (int i=0;i<GlobalParams::mesh_dim_x*GlobalParams::mesh_dim_y;i++) {
            r->flit_tx[i+1] (flit_to_serializer[i]);
            r->req_tx[i+1] (req_to_serializer[i]);
            r->ack_rx[i+1] (ack_to_serializer[i]);

            r->read_enable[i](read_enable_from_serializer[i]);
        }

		// local
		r->flit_rx[DIRECTION_LOCAL_ONOC] (flit_tx_local);   //flit_rx[0]
		r->req_rx[DIRECTION_LOCAL_ONOC] (req_tx_local);     //req_rx[0]
        r->ack_rx[DIRECTION_LOCAL_ONOC] (dummy_signal);
		r->ack_rx_local (ack_tx_local);     //ack_rx[0]

		r->flit_tx[DIRECTION_LOCAL_ONOC] (flit_rx_local);
		r->req_tx[DIRECTION_LOCAL_ONOC] (req_rx_local);
        r->ack_tx[DIRECTION_LOCAL_ONOC] (dummy_signal);
		r->ack_tx_local (ack_rx_local);
        
        // r->read_enable_to_PE (read_enable_from_router);

		// Processing Element pin assignments
		pe = new ProcessingElement("ProcessingElement");
		pe->clock(clock);
		pe->reset(reset);

		pe->flit_rx(flit_rx_local);
		pe->req_rx(req_rx_local);   
		pe->ack_rx(ack_rx_local);   

		pe->flit_tx(flit_tx_local);
		pe->req_tx(req_tx_local);   
		pe->ack_tx(ack_tx_local);   

		// NoP
		//

		pe->free_slots_neighbor(free_slots_neighbor_local); //* not used in WRONoC //dummy_signal
    }
    ~Tile_ONoC() {
        delete[] flit_rx_from_central_tile;
        delete[] req_rx_from_central_tile;
        delete[] bits_in_from_central_tile;
        delete[] flit_from_deserializer;
        delete[] req_from_deserializer;
        for (int i = 0; i < GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y; i++) {
            delete deserializer[i];
        }
        delete[] deserializer;
        delete serializer;
        delete r;
        delete pe;
    }
};

SC_MODULE(Tile_Hybrid)
{
    SC_HAS_PROCESS(Tile_Hybrid);

    // I/O Ports
    sc_in_clk clock;		                // The input clock for the tile
    sc_in_clk clock_optical;
    sc_in <bool> reset;	                        // The reset signal for the tile


    // ONOC parts
    sc_in <Flit> *flit_rx_from_central_tile;	// The input channels
    sc_in <bool> *req_rx_from_central_tile;	        // The requests associated with the input channels
    sc_in <sc_logic> *bits_in_from_central_tile;
    sc_out <ACK> *ack_rx_to_central_tile;	        // The outgoing ack signals associated with the input channels

    sc_out <Flit> *flit_tx_to_central_tile;	// The output channels
    sc_out <bool> *req_tx_to_central_tile;	        // The requests associated with the output channels
    sc_out <sc_logic> *bits_out_to_central_tile;
    sc_in <ACK> *ack_tx_from_central_tile;	        // The outgoing ack signals associated with the output channels

    //sc_signal <int> free_slots_neighbor_local_o;

    // Signals required for Router-PE connection
    sc_signal <Flit> flit_rx_local_o;	
    sc_signal <bool> req_rx_local_o;     
    sc_signal <bool> ack_rx_local_o;

    sc_signal <Flit> flit_tx_local_o;
    sc_signal <bool> req_tx_local_o;
    sc_signal <bool> ack_tx_local_o;

    sc_signal <Flit> *flit_to_serializer;
    sc_signal <bool> *req_to_serializer;
    sc_signal <ACK> *ack_to_serializer;
    sc_signal <Flit> *flit_from_deserializer;
    sc_signal <bool> *req_from_deserializer;
    sc_signal <ACK> *ack_from_deserializer;

    sc_signal <bool> *read_enable_from_serializer;
    // sc_signal <bool> read_enable_from_router;

    sc_signal <ACK> dummy_signal;

    // Instances
    Router_ONoC *r_o;		                // Router instance
    

    Serializer **serializer;
    Deserializer **deserializer;

    // ENOC parts
    int local_id; // Unique ID

    sc_in <Flit> flit_rx[DIRECTIONS];	// The input channels
    sc_in <bool> req_rx[DIRECTIONS];	        // The requests associated with the input channels
    sc_out <bool> ack_rx[DIRECTIONS];	        // The outgoing ack signals associated with the input channels

    sc_out <Flit> flit_tx[DIRECTIONS];	// The output channels
    sc_out <bool> req_tx[DIRECTIONS];	        // The requests associated with the output channels
    sc_in <bool> ack_tx[DIRECTIONS];	        // The outgoing ack signals associated with the output channels

    // hub specific ports
    //sc_in <Flit> hub_flit_rx;	// The input channels
    //sc_in <bool> hub_req_rx;	        // The requests associated with the input channels
    //sc_out <bool> hub_ack_rx;	        // The outgoing ack signals associated with the input channels

    //sc_out <Flit> hub_flit_tx;	// The output channels
    //sc_out <bool> hub_req_tx;	        // The requests associated with the output channels
    //sc_in <bool> hub_ack_tx;	        // The outgoing ack signals associated with the output channels

    // enoc -> onoc NoP related ports
    //sc_in <Flit> flit_rx_2onoc;	// The input channels
    //sc_in <bool> hub_req_rx_2onoc;	        // The requests associated with the input channels
    //sc_out <bool> hub_ack_rx_2onoc;	        // The outgoing ack signals associated with the input channels

    //sc_out <Flit> hub_flit_tx_2onoc;	// The output channels
    //sc_out <bool> hub_req_tx_2onoc;	        // The requests associated with the output channels
    //sc_in <bool> hub_ack_tx_2onoc;	        // The outgoing ack signals associated with the output channels


    // NoP related I/O and signals
    sc_out <int> free_slots[DIRECTIONS];
    sc_in <int> free_slots_neighbor[DIRECTIONS];
    sc_out < NoP_data > NoP_data_out[DIRECTIONS];
    sc_in < NoP_data > NoP_data_in[DIRECTIONS];

    sc_signal <int> free_slots_local;
    sc_signal <int> free_slots_neighbor_local;

    // Signals required for Router-PE connection
    sc_signal <Flit> flit_rx_local;	
    sc_signal <bool> req_rx_local;     
    sc_signal <bool> ack_rx_local;

    sc_signal <Flit> flit_tx_local;
    sc_signal <bool> req_tx_local;
    sc_signal <bool> ack_tx_local;


    // Instances
    Router *r;		                // Router instance
    ProcessingElement *pe;	                // Processing Element instance

    // Constructor

    Tile_Hybrid(sc_module_name nm, int id): sc_module(nm) {
		local_id = id;
        // ONoC part
        int num_ports = GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y;
        // central tile to Rx interface
        flit_rx_from_central_tile = new sc_in <Flit>[num_ports];
		req_rx_from_central_tile = new sc_in <bool>[num_ports];
        bits_in_from_central_tile = new sc_in <sc_logic>[num_ports];
        ack_rx_to_central_tile = new sc_out <ACK>[num_ports];
        // central tile to Tx interface
        flit_tx_to_central_tile = new sc_out <Flit>[num_ports];
		req_tx_to_central_tile = new sc_out <bool>[num_ports];
        bits_out_to_central_tile = new sc_out <sc_logic>[num_ports];
        ack_tx_from_central_tile = new sc_in <ACK>[num_ports];
        // TxRx interface
        flit_from_deserializer = new sc_signal <Flit> [num_ports];
        req_from_deserializer = new sc_signal <bool> [num_ports];
        ack_from_deserializer = new sc_signal <ACK> [num_ports];
        flit_to_serializer = new sc_signal <Flit> [num_ports];
        req_to_serializer = new sc_signal <bool> [num_ports];
        ack_to_serializer = new sc_signal <ACK> [num_ports];

        read_enable_from_serializer = new sc_signal <bool> [num_ports];

        serializer = new Serializer*[num_ports];
        
        for(int i = 0; i < num_ports; i++) {
            char s_name[20];
            sprintf(s_name, "Serializer_[%02d]", i);
            serializer[i] = new Serializer(s_name);
            serializer[i]->clk_optical(clock_optical);
            serializer[i]->reset(reset);
            
            serializer[i]->flit_in(flit_to_serializer[i]);
            serializer[i]->req_rx(req_to_serializer[i]);
            serializer[i]->ack_in(ack_to_serializer[i]);


            serializer[i]->flit_out(flit_tx_to_central_tile[i]);
            serializer[i]->req_tx(req_tx_to_central_tile[i]);
            serializer[i]->ack_out(ack_rx_to_central_tile[i]);

            serializer[i]->bits_out(bits_out_to_central_tile[i]);

            serializer[i]->read_enable(read_enable_from_serializer[i]);

            serializer[i]->local_id = id;
            serializer[i]->s_id = i;
        }
        // cout << "Tile_ONoC's all serializers have been initialized." << endl;
        deserializer = new Deserializer*[num_ports];
        for(int i = 0; i < num_ports; i++){
            char s_name[20];
            sprintf(s_name, "Deserializer_[%02d]", i);
            deserializer[i] = new Deserializer(s_name);
            deserializer[i]->clk_optical(clock_optical);
            deserializer[i]->reset(reset);

            deserializer[i]->flit_in(flit_rx_from_central_tile[i]);
            deserializer[i]->req_rx(req_rx_from_central_tile[i]);
            deserializer[i]->ack_in(ack_tx_from_central_tile[i]);
            deserializer[i]->bits_in(bits_in_from_central_tile[i]);

            deserializer[i]->flit_out(flit_from_deserializer[i]);
            deserializer[i]->req_tx(req_from_deserializer[i]);
            deserializer[i]->ack_out(ack_from_deserializer[i]);

            deserializer[i]->local_id = id;
            deserializer[i]->r_id = i;
        }
        // onoc router part
        r_o = new Router_ONoC("Router_ONoC");
        r_o->clock(clock);
        r_o->clock_optical(clock_optical);
		r_o->reset(reset);

        //To/From central router
        for(int i = 0; i < num_ports; i++){
            r_o->flit_rx[i+1] (flit_from_deserializer[i]);
            r_o->req_rx[i+1] (req_from_deserializer[i]);
            r_o->ack_tx[i+1] (ack_from_deserializer[i]);
        }
        
        
        for(int i = 0; i < num_ports; i++){
            r_o->flit_tx[i+1] (flit_to_serializer[i]);
            r_o->req_tx[i+1] (req_to_serializer[i]);
            r_o->ack_rx[i+1] (ack_to_serializer[i]);

            r_o->read_enable[i](read_enable_from_serializer[i]);
        }

        // local
		r_o->flit_rx[DIRECTION_LOCAL_ONOC] (flit_tx_local_o);   //flit_rx[0]
		r_o->req_rx[DIRECTION_LOCAL_ONOC] (req_tx_local_o);     //req_rx[0]
        r_o->ack_rx[DIRECTION_LOCAL_ONOC] (dummy_signal);
		r_o->ack_rx_local (ack_tx_local_o);     //ack_rx[0]

		r_o->flit_tx[DIRECTION_LOCAL_ONOC] (flit_rx_local_o);
		r_o->req_tx[DIRECTION_LOCAL_ONOC] (req_rx_local_o);
        r_o->ack_tx[DIRECTION_LOCAL_ONOC] (dummy_signal);
		r_o->ack_tx_local (ack_rx_local_o);


        
        //ENOC part
		// Router pin assignments
		r = new Router("Router");
		r->clock(clock);
		r->reset(reset);
		for (int i = 0; i < DIRECTIONS; i++) {
			r->flit_rx[i] (flit_rx[i]);
			r->req_rx[i] (req_rx[i]);
			r->ack_rx[i] (ack_rx[i]);

			r->flit_tx[i] (flit_tx[i]);
			r->req_tx[i] (req_tx[i]);
			r->ack_tx[i] (ack_tx[i]);

			r->free_slots[i] (free_slots[i]);
			r->free_slots_neighbor[i] (free_slots_neighbor[i]);

			// NoP 
			r->NoP_data_out[i] (NoP_data_out[i]);
			r->NoP_data_in[i] (NoP_data_in[i]);
		}
		
		// local
		r->flit_rx[DIRECTION_LOCAL] (flit_tx_local);
		r->req_rx[DIRECTION_LOCAL] (req_tx_local);
		r->ack_rx[DIRECTION_LOCAL] (ack_tx_local);

		r->flit_tx[DIRECTION_LOCAL] (flit_rx_local);
		r->req_tx[DIRECTION_LOCAL] (req_rx_local);
		r->ack_tx[DIRECTION_LOCAL] (ack_rx_local);


		// hub related
		r->flit_rx[DIRECTION_HUB] (flit_rx_local_o);
		r->req_rx[DIRECTION_HUB] (req_rx_local_o);
		r->ack_rx[DIRECTION_HUB] (ack_rx_local_o);

		r->flit_tx[DIRECTION_HUB] (flit_tx_local_o);
		r->req_tx[DIRECTION_HUB] (req_tx_local_o);
		r->ack_tx[DIRECTION_HUB] (ack_tx_local_o);


		// Processing Element pin assignments
		pe = new ProcessingElement("ProcessingElement");
		pe->clock(clock);
		pe->reset(reset);

		pe->flit_rx(flit_rx_local);
		pe->req_rx(req_rx_local);
		pe->ack_rx(ack_rx_local);

		pe->flit_tx(flit_tx_local);
		pe->req_tx(req_tx_local);
		pe->ack_tx(ack_tx_local);

		// NoP
		//
		r->free_slots[DIRECTION_LOCAL] (free_slots_local);
		r->free_slots_neighbor[DIRECTION_LOCAL] (free_slots_neighbor_local);
		pe->free_slots_neighbor(free_slots_neighbor_local);

    }
    
};


#endif
