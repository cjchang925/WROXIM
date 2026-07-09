/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2010 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file contains the declaration of the global params needed by Noxim
 * to forward configuration to every sub-block
 */

#ifndef __NOXIMGLOBALPARAMS_H__
#define __NOXIMGLOBALPARAMS_H__ 

#include <map>
#include <utility>
#include <vector>
#include <string>

using namespace std;

#define CONFIG_FILENAME        "config.yaml"
#define POWER_CONFIG_FILENAME  "power.yaml"

#define OPTICAL_POWER_CONFIG_FILENAME   "power_optical.yaml"

// *************
// Added by LGGM
// *************
#define TM_TASKMAPPING_FILE	"taskmapping.map"
#define TM_TASKMAPPINGLOG_FILE	"taskmapping.log"
#define TM_CMDLINE		"-taskmapping"
#define TM_CMDLINE_LOG		"-taskmappinglog"
#define TM_NOT_FILE		"filenull"

// *************

// Define the directions as numbers
#define DIRECTIONS             4
#define DIRECTION_NORTH        0
#define DIRECTION_EAST         1
#define DIRECTION_SOUTH        2
#define DIRECTION_WEST         3
#define DIRECTION_LOCAL        4
#define DIRECTION_HUB          5
#define DIRECTION_ONOC         5
#define DIRECTION_WIRELESS     747

//! -------------------------------------------------- WRONoC --------------------------------------------------
#define DIRECTIONS_ONOC             2
#define DIRECTION_LOCAL_ONOC        0
#define DIRECTION_CENTRAL_ROUTER    1
//! ------------------------------------------------------------------------------------------------------------

// Generic not reserved resource
#define NOT_RESERVED          -2

// To mark invalid or non exhistent values
#define NOT_VALID             -1

// Routing algorithms
#define ROUTING_DYAD           "DYAD"
#define ROUTING_TABLE_BASED    "TABLE_BASED"

// Selection strategies
#define SEL_RANDOM             0
#define SEL_BUFFER_LEVEL       1
#define SEL_NOP                2
#define INVALID_SELECTION     -1

// Traffic distribution
#define TRAFFIC_RANDOM         "TRAFFIC_RANDOM"
#define TRAFFIC_TRANSPOSE1     "TRAFFIC_TRANSPOSE1"
#define TRAFFIC_TRANSPOSE2     "TRAFFIC_TRANSPOSE2"
#define TRAFFIC_HOTSPOT        "TRAFFIC_HOTSPOT"
#define TRAFFIC_TABLE_BASED    "TRAFFIC_TABLE_BASED"
#define TRAFFIC_BIT_REVERSAL   "TRAFFIC_BIT_REVERSAL"
#define TRAFFIC_SHUFFLE        "TRAFFIC_SHUFFLE"
#define TRAFFIC_BUTTERFLY      "TRAFFIC_BUTTERFLY"
#define TRAFFIC_LOCAL	       "TRAFFIC_LOCAL"
#define TRAFFIC_ULOCAL	       "TRAFFIC_ULOCAL"
#define TRAFFIC_TASKMAPPING    "TRAFFIC_TASKMAPPING"		// Added by LGGM

// Verbosity levels
#define VERBOSE_OFF            "VERBOSE_OFF"
#define VERBOSE_LOW            "VERBOSE_LOW"
#define VERBOSE_MEDIUM         "VERBOSE_MEDIUM"
#define VERBOSE_HIGH           "VERBOSE_HIGH"


// Wireless MAC constants
#define RELEASE_CHANNEL 1
#define HOLD_CHANNEL 	2

#define TOKEN_HOLD             "TOKEN_HOLD"
#define TOKEN_MAX_HOLD         "TOKEN_MAX_HOLD"
#define TOKEN_PACKET           "TOKEN_PACKET"

#define ARCH_MESH              "MESH"  
//* WRONoC architecture
#define ARCH_LAMBDA            "LAMBDA"    
#define ARCH_GWOR              "GWOR"  
#define ARCH_LIGHT             "LIGHT" 
#define ARCH_SNAKE             "SNAKE" 
//* Hybrid NoC architecture
#define ARCH_HYBRID            "HYBRID"

//* network protocol
#define ALTERNETING_BIT_PROTOCOL "ABP"
#define ON_OFF_FLOWCONTROL     "ON_OFF_FLOWCONTROL"

typedef struct {
    pair<double, double> ber;
    int dataRate;
    vector<string> macPolicy;
} ChannelConfig;

typedef struct {
    vector<int> attachedNodes;
    vector<int> rxChannels;
    vector<int> txChannels;
    int toTileBufferSize;
    int fromTileBufferSize;
    int txBufferSize;
    int rxBufferSize;
} HubConfig;

typedef struct {
    map<pair <int, int>, double> front;
    map<pair <int, int>, double> pop;
    map<pair <int, int>, double> push;
    map<pair <int, int>, double> leakage;
} BufferPowerConfig;

typedef map<double, pair <double, double> > LinkBitLinePowerConfig;

typedef struct {
    map<pair<double, double>, pair<double, double> > crossbar_pm;
    map<int, pair<double, double> > network_interface;
    map<string, pair<double, double> > routing_algorithm_pm;
    map<string, pair<double, double> > selection_strategy_pm;
} RouterPowerConfig;

typedef struct {
    pair<double, double> transceiver_leakage;
    pair<double, double> transceiver_biasing;
    double rx_dynamic;
    double rx_snooping;
    double default_tx_energy;
    map<pair <int, int>, double> transmitter_attenuation_map;
} HubPowerConfig;

// *************
// Added by LGGM
// *************
typedef struct {
    double running_mode;
    double idle_mode;
} PEPowerConfig;
// *************

// ****************
// Modified by LGGM
// ****************
typedef struct {
    BufferPowerConfig bufferPowerConfig;
    LinkBitLinePowerConfig linkBitLinePowerConfig;
    RouterPowerConfig routerPowerConfig;
    HubPowerConfig hubPowerConfig;
    PEPowerConfig pePowerConfig;
} PowerConfig;
// ****************

// *************
// Added by JengDe
// *************
typedef struct {
    double active_mode;
    double idle_mode;
} LaserConfig;
typedef struct {
    double active_mode;
    double idle_mode;
} SerializerConfig;
typedef struct {
    double active_mode;
    double idle_mode;
} DriverConfig;
typedef struct {
    double active_mode;
    double idle_mode;
} RxComparatorConfig;
typedef struct {
    double active_mode;
    double idle_mode;
} TIAConfig;
// typedef struct {
//     double active_mode;
//     double idle_mode;
// } ArbitrationFlowControlConfig;
typedef struct {
    double active_mode;
    double idle_mode;
} MRRConfig;
typedef struct {
    LaserConfig laserConfig;
    SerializerConfig serializerConfig;
    DriverConfig driverConfig;
    RxComparatorConfig rxComparatorConfig;
    TIAConfig tiaConfig;
    // ArbitrationFlowControlConfig arbitrationFlowControlConfig;
    MRRConfig mrrConfig;
} OpticalPowerConfig;

// *************



struct GlobalParams {
    static string verbose_mode;
    static int trace_mode;
    static string trace_filename;
    static int mesh_dim_x;
    static int mesh_dim_y;
    static double r2r_link_length;
    static double r2h_link_length;
    static int buffer_depth;
    static int flit_size;
    static int min_packet_size;
    static int max_packet_size;
    static string routing_algorithm;
    static string routing_table_filename;
    static string selection_strategy;
    static double packet_injection_rate;
    static double probability_of_retransmission;
    static double locality;
    static string traffic_distribution;
    static string traffic_table_filename;
    static string config_filename;
    static string power_config_filename;
    static string taskmapping_filename;			// Added by LGGM
    static string taskmappinglog_filename;		// Added by LGGM
    static bool tgffMappingEnabled;		        // Added by ACAG
    static string optical_power_config_filename;    //WRONoC Added by JengDe
    static int clock_period_optical_ps;     //WRONoC Added by JengDe
    static int clock_period_ps;
    static int simulation_time;
    static int reset_time;
    static int stats_warm_up_time;
    static int rnd_generator_seed;
    static bool detailed;
    static vector <pair <int, double> > hotspots;
    static double dyad_threshold;
    static unsigned int max_volume_to_be_drained;
    static bool show_buffer_stats;
    static bool use_winoc;
    static bool use_powermanager;
    static ChannelConfig default_channel_configuration;
    static map<int, ChannelConfig> channel_configuration;
    static HubConfig default_hub_configuration;
    static map<int, HubConfig> hub_configuration;
    static map<int, int> hub_for_tile;
    static PowerConfig power_configuration;
    static OpticalPowerConfig optical_power_configuration; //WRONoC Added by JengDe

    static string network_protocol; //WRONoC Added by JengDe
    static bool use_TxRx_PreciseTiming; //WRONoC Added by JengDe

    static string architecture; 
};

#endif
