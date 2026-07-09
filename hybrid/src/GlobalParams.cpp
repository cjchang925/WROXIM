/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2015 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file contains the implementation of the command line parser
 */

#include "GlobalParams.h"

string GlobalParams::verbose_mode;
int GlobalParams::trace_mode;
string GlobalParams::trace_filename;
int GlobalParams::mesh_dim_x;
int GlobalParams::mesh_dim_y;
double GlobalParams::r2r_link_length;
double GlobalParams::r2h_link_length;
int GlobalParams::buffer_depth;
int GlobalParams::flit_size;
int GlobalParams::min_packet_size;
int GlobalParams::max_packet_size;
string GlobalParams::routing_algorithm;
string GlobalParams::routing_table_filename;
string GlobalParams::selection_strategy;
double GlobalParams::packet_injection_rate;
double GlobalParams::probability_of_retransmission;
double GlobalParams::locality;
string GlobalParams::traffic_distribution;
string GlobalParams::traffic_table_filename;
string GlobalParams::config_filename;
string GlobalParams::power_config_filename;
string GlobalParams::taskmapping_filename;		// Added by LGGM
string GlobalParams::taskmappinglog_filename;	// Added by LGGM
bool GlobalParams::tgffMappingEnabled;		    // Added by ACAG

string GlobalParams::optical_power_config_filename;// WRONoC Added by JengDe
int GlobalParams::clock_period_optical_ps;  // WRONoC Added by JengDe

int GlobalParams::clock_period_ps;
int GlobalParams::simulation_time;
int GlobalParams::reset_time;
int GlobalParams::stats_warm_up_time;
int GlobalParams::rnd_generator_seed;
bool GlobalParams::detailed;
double GlobalParams::dyad_threshold;
unsigned int GlobalParams::max_volume_to_be_drained;
vector <pair <int, double> > GlobalParams::hotspots;
bool GlobalParams::show_buffer_stats;
bool GlobalParams::use_winoc;
bool GlobalParams::use_powermanager;
ChannelConfig GlobalParams::default_channel_configuration;
map<int, ChannelConfig> GlobalParams::channel_configuration;
HubConfig GlobalParams::default_hub_configuration;
map<int, HubConfig> GlobalParams::hub_configuration;
map<int, int> GlobalParams::hub_for_tile;
PowerConfig GlobalParams::power_configuration;
OpticalPowerConfig GlobalParams::optical_power_configuration; // WRONoC Added by JengDe

string GlobalParams::network_protocol; // WRONoC Added by JengDe
bool GlobalParams::use_TxRx_PreciseTiming; //WRONoC Added by JengDe

string GlobalParams::architecture;
