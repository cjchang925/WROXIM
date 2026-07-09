/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2010 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file contains the declaration of the router
 */

#ifndef __NOXIMROUTER_H__
#define __NOXIMROUTER_H__

#include <systemc.h>
#include "DataStructs.h"
#include "Buffer.h"
#include "Stats.h"
#include "GlobalRoutingTable.h"
#include "LocalRoutingTable.h"
#include "ReservationTable.h"
#include "Utils.h"

#include "OpticalPower.h"
#include "DualClockBuffer.h"

#include "routingAlgorithms/RoutingAlgorithm.h"
#include "routingAlgorithms/RoutingAlgorithms.h"
#include "selectionStrategies/SelectionStrategy.h"
#include "selectionStrategies/SelectionStrategy.h"
#include "selectionStrategies/Selection_NOP.h"
#include "selectionStrategies/Selection_BUFFER_LEVEL.h"

using namespace std;

extern unsigned int drained_volume;

SC_MODULE(Router)
{
	friend class Selection_NOP;
	friend class Selection_BUFFER_LEVEL;

	// I/O Ports
	sc_in_clk clock;   // The input clock for the router
	sc_in<bool> reset; // The reset signal for the router

	// number of ports: 4 mesh directions + local + wireless
	sc_in<Flit> flit_rx[DIRECTIONS + 2]; // The input channels
	sc_in<bool> req_rx[DIRECTIONS + 2];	 // The requests associated with the input channels
	sc_out<bool> ack_rx[DIRECTIONS + 2]; // The outgoing ack signals associated with the input channels

	sc_out<Flit> flit_tx[DIRECTIONS + 2]; // The output channels
	sc_out<bool> req_tx[DIRECTIONS + 2];  // The requests associated with the output channels
	sc_in<bool> ack_tx[DIRECTIONS + 2];	  // The outgoing ack signals associated with the output channels

	sc_out<int> free_slots[DIRECTIONS + 1];
	sc_in<int> free_slots_neighbor[DIRECTIONS + 1];

	// Neighbor-on-Path related I/O
	sc_out<NoP_data> NoP_data_out[DIRECTIONS];
	sc_in<NoP_data> NoP_data_in[DIRECTIONS];

	int rtable_cnt[DIRECTIONS + 2];
	int cur_path[DIRECTIONS + 2];

	// Registers

	/*
	   Coord position;                     // Router position inside the mesh
	 */
	int cnt = 0;
	int local_id;	  // Unique ID
	int routing_type; // Type of routing algorithm
	int selection_type;
	Buffer buffer[DIRECTIONS + 2];		   // Buffer for each input channel
	bool current_level_rx[DIRECTIONS + 2]; // Current level for Alternating Bit Protocol (ABP)
	bool current_level_tx[DIRECTIONS + 2]; // Current level for Alternating Bit Protocol (ABP)
	Stats stats;						   // Statistics
	Power power;
	LocalRoutingTable routing_table;	// Routing table
	ReservationTable reservation_table; // Switch reservation table
	int start_from_port;				// Port from which to start the reservation cycle
	unsigned long routed_flits;
	RoutingAlgorithm *routingAlgorithm;
	SelectionStrategy *selectionStrategy;

	// Functions

	void rxProcess(); // The receiving process
	void txProcess(); // The transmitting process
	void perCycleUpdate();
	void configure(const int _id, const double _warm_up_time,
				   const unsigned int _max_buffer_size,
				   GlobalRoutingTable &grt);

	unsigned long getRoutedFlits(); // Returns the number of routed flits
	unsigned int getFlitsCount();	// Returns the number of flits into the router

	// Constructor

	SC_CTOR(Router)
	{
		SC_METHOD(rxProcess);
		sensitive << reset;
		sensitive << clock.pos();

		SC_METHOD(txProcess);
		sensitive << reset;
		sensitive << clock.pos();

		SC_METHOD(perCycleUpdate);
		sensitive << reset;
		sensitive << clock.pos();

		routingAlgorithm = RoutingAlgorithms::get(GlobalParams::routing_algorithm);

		if (routingAlgorithm == 0)
		{
			cerr << " FATAL: invalid routing -routing " << GlobalParams::routing_algorithm << ", check with noxim -help" << endl;
			exit(-1);
		}

		selectionStrategy = SelectionStrategies::get(GlobalParams::selection_strategy);

		if (selectionStrategy == 0)
		{
			cerr << " FATAL: invalid selection strategy -sel " << GlobalParams::selection_strategy << ", check with noxim -help" << endl;
			exit(-1);
		}
	}

private:
	// performs actual routing + selection
	int route(const RouteData &route_data);

	// wrappers
	int selectionFunction(const vector<int> &directions,
						  const RouteData &route_data);
	vector<int> routingFunction(const RouteData &route_data);

	NoP_data getCurrentNoPData();
	void NoP_report() const;
	int NoPScore(const NoP_data &nop_data, const vector<int> &nop_channels) const;
	int reflexDirection(int direction) const;
	int getNeighborId(int _id, int direction) const;

public:
	unsigned int local_drained;

	bool inCongestion();
	void ShowBuffersStats(std::ostream & out);
};

//! -------------------------------------------------- WRONoC --------------------------------------------------
SC_MODULE(Router_ONoC)
{
	friend class Selection_NOP;
	friend class Selection_BUFFER_LEVEL;

	// I/O Ports
	sc_in_clk clock; // The input clock for the router
	sc_in_clk clock_optical;
	sc_in<bool> reset; // The reset signal for the router

	// number of ports: local + direction_central_router
	sc_in<Flit> *flit_rx; // The input channels
	sc_in<bool> *req_rx;  // The requests associated with the input channels
	sc_out<ACK> *ack_rx;  // The outgoing ack signals associated with the input channels
	sc_out<bool> ack_rx_local;

	sc_out<Flit> *flit_tx; // The output channels
	sc_out<bool> *req_tx;  // The requests associated with the output channels
	sc_in<ACK> *ack_tx;	   // The outgoing ack signals associated with the output channels
	sc_in<bool> ack_tx_local;

	sc_in<bool> *read_enable;

	// sc_out <int> free_slots[DIRECTIONS + 1];
	// sc_in <int> free_slots_neighbor_from_central_tile;

	// Neighbor-on-Path related I/O
	// sc_out < NoP_data > NoP_data_out_to_central_tile;
	// sc_in < NoP_data > NoP_data_in_from_central_tile;

	// Registers

	/*
	   Coord position;                     // Router position inside the mesh
	 */
	int local_id;	  // Unique ID
	int routing_type; // Type of routing algorithm
	int selection_type;
	DualClockBuffer *rx_buffer; // Buffer for each rx channel
	DualClockBuffer *tx_buffer; // Buffer for each tx channel
	bool *current_level_rx;		// Current level for Alternating Bit Protocol (ABP)
	bool *current_level_tx;		// Current level for Alternating Bit Protocol (ABP)
	Stats stats;				// Statistics
	Power power;

	RouterOpticalPower opticalPower; //

	LocalRoutingTable routing_table; // Routing table
	// ReservationTable reservation_table;	// Switch reservation table
	int start_from_port; // Port from which to start the reservation cycle
	unsigned long routed_flits;
	RoutingAlgorithm *routingAlgorithm;
	SelectionStrategy *selectionStrategy;

	int *next_flit_state;
	bool *flowControl_signal; // Control own router's output flow
	bool *flowControl_table;  // Maintain other router's flow control

	double *buffer_full_counter; // Count how many cycles the buffer is full

	// Functions

	void rxProcess_PE();			// The receiving process from local PE
	void rxProcess_centralRouter(); // The receiving process from central router
	void txProcess_PE();			// The transmitting process to local PE
	void txProcess_centralRouter(); // The transmitting process to central router
	void perCycleUpdate();			//* WRONoC doesn't need to Update NoP data;
	void configure(const int _id, const double _warm_up_time,
				   const unsigned int _max_buffer_size,
				   GlobalRoutingTable &grt);

	unsigned long getRoutedFlits(); // Returns the number of routed flits
	unsigned int getFlitsCount();	// Returns the number of flits into the router

	// Constructor

	SC_CTOR(Router_ONoC)
	{
		// cout << "In Router_ONoC constructor, before new flit_rx port.-----------------" << endl;
		//* -----------
		int num_ports = GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y;
		flit_rx = new sc_in<Flit>[num_ports + 1];
		req_rx = new sc_in<bool>[num_ports + 1];
		ack_rx = new sc_out<ACK>[num_ports + 1];

		flit_tx = new sc_out<Flit>[num_ports + 1];
		req_tx = new sc_out<bool>[num_ports + 1];
		ack_tx = new sc_in<ACK>[num_ports + 1];

		read_enable = new sc_in<bool>[num_ports];

		current_level_rx = new bool[num_ports + 1];
		current_level_tx = new bool[num_ports + 1];
		//* -----------
		// cout << "In Router_ONoC constructor, After new flit_rx port." << endl;

		next_flit_state = new int[num_ports];
		flowControl_table = new bool[num_ports];
		flowControl_signal = new bool[num_ports];

		buffer_full_counter = new double[num_ports];

		rx_buffer = new DualClockBuffer[num_ports];
		tx_buffer = new DualClockBuffer[num_ports];
		for (int i = 0; i < num_ports; i++)
		{
			rx_buffer[i].SetMaxBufferSize(GlobalParams::buffer_depth);
			tx_buffer[i].SetMaxBufferSize(GlobalParams::buffer_depth);
		}

		SC_METHOD(rxProcess_PE);
		sensitive << reset;
		sensitive << clock.pos();

		SC_METHOD(txProcess_PE);
		sensitive << reset;
		sensitive << clock.pos();

		SC_METHOD(rxProcess_centralRouter);
		sensitive << reset;
		sensitive << clock_optical.pos();

		SC_METHOD(txProcess_centralRouter);
		sensitive << reset;
		sensitive << clock_optical.pos();

		SC_METHOD(perCycleUpdate);
		sensitive << reset;
		sensitive << clock.pos();

		routingAlgorithm = RoutingAlgorithms::get(GlobalParams::routing_algorithm);

		if (routingAlgorithm == 0)
		{
			cerr << " FATAL: invalid routing -routing " << GlobalParams::routing_algorithm << ", check with noxim -help" << endl;
			exit(-1);
		}

		selectionStrategy = SelectionStrategies::get(GlobalParams::selection_strategy);

		if (selectionStrategy == 0)
		{
			cerr << " FATAL: invalid selection strategy -sel " << GlobalParams::selection_strategy << ", check with noxim -help" << endl;
			exit(-1);
		}
	}

private:
	// performs actual routing + selection
	int route(const RouteData &route_data);

	// wrappers
	int selectionFunction(const vector<int> &directions,
						  const RouteData &route_data);
	vector<int> routingFunction(const RouteData &route_data);

	// NoP_data getCurrentNoPData();
	// void NoP_report() const;
	// int NoPScore(const NoP_data & nop_data, const vector <int> & nop_channels) const;
	int reflexDirection(int direction) const;
	int getNeighborId(int _id, int direction) const;

public:
	unsigned int local_drained;

	// bool inCongestion();
	void ShowBuffersStats(std::ostream & out);
};

#endif
