/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2010 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file represents the top-level testbench
 */

#ifndef __NOXIMNOC_H__
#define __NOXIMNOC_H__

#include <systemc.h>
#include "Tile.h"
#include "GlobalRoutingTable.h"
#include "GlobalTrafficTable.h"
#include "Hub.h"
#include "Channel.h"
#include "TokenRing.h"

using namespace std;

template <typename T>
struct sc_signal_NSWE
{
    sc_signal<T> east;
    sc_signal<T> west;
    sc_signal<T> south;
    sc_signal<T> north;
};

template <typename T>
struct sc_signal_NSWEH
{
    sc_signal<T> east;
    sc_signal<T> west;
    sc_signal<T> south;
    sc_signal<T> north;
    sc_signal<T> to_hub;
    sc_signal<T> from_hub;
};

//WRONoC
template <typename T>
struct sc_signal_OPTICAL{
    sc_signal<T> *to_central_router;
    sc_signal<T> *from_central_router;
};

SC_MODULE(NoC)
{
    // I/O Ports
    sc_in_clk clock;		// The input clock for the NoC
    sc_in_clk clock_optical;// WRONoC
    sc_in < bool > reset;	// The reset signal for the NoC

    // Signals
    sc_signal_NSWEH<bool> **req;
    sc_signal_NSWEH<bool> **ack;
    sc_signal_NSWEH<Flit> **flit;
    sc_signal_NSWE<int> **free_slots;

    // NoP
    sc_signal_NSWE<NoP_data> **nop_data;

    // Matrix of tiles
    Tile ***t;

    map<int, Hub*> hub;
    map<int, Channel*> channel;

    TokenRing* token_ring;

    // Global tables
    GlobalRoutingTable grtable;
    GlobalTrafficTable gttable;

    //WRONoC
    sc_signal_OPTICAL<bool> **req_opt;
    sc_signal_OPTICAL<ACK> **ack_opt;
    sc_signal_OPTICAL<Flit> **flit_opt;
    sc_signal_OPTICAL<sc_logic> **bits_opt;


    Tile_Central *t_central;
    Tile_ONoC ***t_ONoC;
    Tile_Hybrid ***t_Hybrid;


    

    // Constructor

    SC_CTOR(NoC) {

        
        // Build the Mesh
        if(GlobalParams::architecture==ARCH_MESH){
            buildMesh();
            cout << "Building MESH architecture.";
        }
        else if(GlobalParams::architecture==ARCH_LAMBDA) {   //! Build the WRONoC LAMBDA-architecture
            buildOpticalNoC();
            cout << "Finish Building LAMBDA architecture." << endl;
        }
        else if(GlobalParams::architecture==ARCH_HYBRID){
            buildHybridNoC();
            cout << "Finish Building HYBRID architecture." << endl;
        }
        else{
            cerr << "Error: unknown architecture type." << endl;
            exit(-1);
        }
    }

    // Support methods
    Tile *searchNode(const int id) const;
    Tile_ONoC *searchNode_ONoC(const int id) const;
    Tile_Hybrid *searchNode_Hybrid(const int id) const;
  private:

    void buildMesh();
    // WRONoC
    void buildOpticalNoC();
    //build Hybrid NoC
    void buildHybridNoC();
};

//Hub * dd;

#endif
