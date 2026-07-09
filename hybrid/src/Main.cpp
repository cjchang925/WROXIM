/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2015 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file contains the implementation of the top-level of Noxim
 */

#include "ConfigurationManager.h"
#include "NoC.h"
#include "GlobalStats.h"
#include "DataStructs.h"
#include "GlobalParams.h"
#include "taskMapping/TaskMapping.h" // Added by LGGM
#include "taskMapping/TgffMapping.h" // Added by ACAG

#include "OpticalPower.h" // Added by JengDe

#include <csignal>

using namespace std;

// need to be globally visible to allow "-volume" simulation stop
unsigned int drained_volume;
NoC *n;

void signalHandler(int signum)
{
	cout << "\b\b  " << endl;
	cout << endl;
	cout << "Current Statistics:" << endl;
	cout << "(" << sc_time_stamp().to_double() / GlobalParams::clock_period_ps << " sim cycles executed)" << endl;
	GlobalStats gs(n);
	gs.showStats(std::cout, GlobalParams::detailed);
}

int sc_main(int arg_num, char *arg_vet[])
{
	signal(SIGQUIT, signalHandler);

	// TEMP
	drained_volume = 0;

	// ****************
	// Modified by LGGM
	// ****************
	// Handle command-line arguments
	cout << endl
		 << "\t\tNoxim - the WNoC Simulator" << endl;
	cout << "\t\t(C) University of Catania" << endl
		 << endl;
	cout << "\t\tAdaptations by LGGM - UdeA" << endl
		 << endl;
	configure(arg_num, arg_vet);
	// ****************

	// Signals
	sc_clock clock("clock", GlobalParams::clock_period_ps, SC_PS);
	sc_clock clock_optical("clock_optical", GlobalParams::clock_period_optical_ps, SC_PS);
	sc_signal<bool> reset;

	// NoC instance
	n = new NoC("NoC");

	n->clock(clock);
	n->clock_optical(clock_optical);
	n->reset(reset);

	// *************
	// Added by LGGM
	// *************
	ios init_cout_format(NULL);
	init_cout_format.copyfmt(cout);	
	if (GlobalParams::traffic_distribution == TRAFFIC_TASKMAPPING)
	{
		tmInstance = new TaskMapping("TaskMapping");

		bool success = false;
		if (GlobalParams::tgffMappingEnabled)
		{
			GraphParser parser;
			cout << "Loading TGFF graph file \"" << GlobalParams::taskmapping_filename << "\"" << endl;
			success = parser.Parse(GlobalParams::taskmapping_filename.c_str());
			// parser.PrintParsedTables(); // Add by JengDe
		}
		else
		{
			cout << "Loading task mapping configuration from file \"" << GlobalParams::taskmapping_filename << "\"" << endl;
			success = tmInstance->createTaskGraphsFromFile(GlobalParams::taskmapping_filename.c_str());
		}

		if (!success)
		{
			cout << "Failed to load the task mapping configuration." << endl;
			exit(0);
			TM_ASSERT(false, "Failed to load the task mapping configuration.");
		}
		else
		{
			cout << "  Task mapping configuration loaded correctly." << endl;

			tmInstance->showTaskMappingConfiguration();
#if defined(DEFINE_TM_SIMEXECLOG) || defined(DEFINE_TM_SIMSTATSLOG)
			TM_SIMWRITELOG(endl
						   << "********************************" << endl
						   << "TaskMapping Simulation Execution" << endl
						   << "********************************" << endl
						   << endl);
#endif
		}
	}
	else
		tmInstance = NULL;
	// *************

	// Trace signals
	sc_trace_file *tf = NULL;
	if (GlobalParams::trace_mode)
	{
		tf = sc_create_vcd_trace_file(GlobalParams::trace_filename.c_str());
		sc_trace(tf, reset, "reset");
		sc_trace(tf, clock, "clock");
		cout << "In Main.cpp, sc_trace(tf, clock, \"clock\");" << endl;
		if (GlobalParams::architecture == ARCH_HYBRID)
		{
			for (int i = 0; i < GlobalParams::mesh_dim_x; i++)
			{
				for (int j = 0; j < GlobalParams::mesh_dim_y; j++)
				{
					char label[30];

					sprintf(label, "req(%02d)(%02d).east", i, j);
					sc_trace(tf, n->req[i][j].east, label);
					sprintf(label, "req(%02d)(%02d).west", i, j);
					sc_trace(tf, n->req[i][j].west, label);
					sprintf(label, "req(%02d)(%02d).south", i, j);
					sc_trace(tf, n->req[i][j].south, label);
					sprintf(label, "req(%02d)(%02d).north", i, j);
					sc_trace(tf, n->req[i][j].north, label);

					sprintf(label, "ack(%02d)(%02d).east", i, j);
					sc_trace(tf, n->ack[i][j].east, label);
					sprintf(label, "ack(%02d)(%02d).west", i, j);
					sc_trace(tf, n->ack[i][j].west, label);
					sprintf(label, "ack(%02d)(%02d).south", i, j);
					sc_trace(tf, n->ack[i][j].south, label);
					sprintf(label, "ack(%02d)(%02d).north", i, j);
					sc_trace(tf, n->ack[i][j].north, label);
				}
			}
		}

		else if (GlobalParams::architecture != ARCH_MESH)
		{
			sc_trace(tf, clock_optical, "clock_optical");
			cout << "In Main.cpp, sc_trace(tf, clock_optical, \"clock_optical\");" << endl;
			for (int i = 0; i < GlobalParams::mesh_dim_x; i++)
			{
				for (int j = 0; j < GlobalParams::mesh_dim_y; j++)
				{
					for (int k = 0; k < GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y; k++)
					{
						char label[50];

						sprintf(label, "req_opt(%02d)(%02d).to_central_router(%02d)", i, j, k);
						sc_trace(tf, n->req_opt[i][j].to_central_router[k], label);
						sprintf(label, "req_opt(%02d)(%02d).from_central_router(%02d)", i, j, k);
						sc_trace(tf, n->req_opt[i][j].from_central_router[k], label);

						sprintf(label, "ack_opt(%02d)(%02d).to_central_router(%02d)", i, j, k);
						sc_trace(tf, n->ack_opt[i][j].to_central_router[k], label);
						sprintf(label, "ack_opt(%02d)(%02d).from_central_router(%02d)", i, j, k);
						sc_trace(tf, n->ack_opt[i][j].from_central_router[k], label);

						sprintf(label, "bits_opt(%02d)(%02d).to_central_router(%02d)", i, j, k);
						sc_trace(tf, n->bits_opt[i][j].to_central_router[k], label);
						sprintf(label, "bits_opt(%02d)(%02d).from_central_router(%02d)", i, j, k);
						sc_trace(tf, n->bits_opt[i][j].from_central_router[k], label);
					}
				}
				cout << "In Main.cpp, sc_trace add req_opt, ack_opt, bits_opt" << endl;
			}
		}
		else
		{
			for (int i = 0; i < GlobalParams::mesh_dim_x; i++)
			{
				for (int j = 0; j < GlobalParams::mesh_dim_y; j++)
				{
					char label[30];

					sprintf(label, "req(%02d)(%02d).east", i, j);
					sc_trace(tf, n->req[i][j].east, label);
					sprintf(label, "req(%02d)(%02d).west", i, j);
					sc_trace(tf, n->req[i][j].west, label);
					sprintf(label, "req(%02d)(%02d).south", i, j);
					sc_trace(tf, n->req[i][j].south, label);
					sprintf(label, "req(%02d)(%02d).north", i, j);
					sc_trace(tf, n->req[i][j].north, label);

					sprintf(label, "ack(%02d)(%02d).east", i, j);
					sc_trace(tf, n->ack[i][j].east, label);
					sprintf(label, "ack(%02d)(%02d).west", i, j);
					sc_trace(tf, n->ack[i][j].west, label);
					sprintf(label, "ack(%02d)(%02d).south", i, j);
					sc_trace(tf, n->ack[i][j].south, label);
					sprintf(label, "ack(%02d)(%02d).north", i, j);
					sc_trace(tf, n->ack[i][j].north, label);
				}
			}
		}
	}
	// ****************
	// Modified by JengDe
	// ****************
	// Parse Path_input data to calculate power
	OpticalPower opticalpowerInfo;
	// if(GlobalParams::architecture != ARCH_MESH && GlobalParams::architecture != ARCH_HYBRID){
	if (GlobalParams::architecture != ARCH_MESH)
	{
		opticalpowerInfo.parsePathData();
		opticalpowerInfo.parseMasterSlaveData();
		opticalpowerInfo.parsePathLossFile();
	}
	// ****************

	// Reset the chip and run the simulation
	cout << endl;
	reset.write(1);
	cout << "Reset for " << (int)(GlobalParams::reset_time) << " cycles... " << endl;
	cout << "stop at " << GlobalParams::simulation_time + GlobalParams::reset_time << " cycles." << endl;
	srand(GlobalParams::rnd_generator_seed);
	sc_start(GlobalParams::reset_time, SC_NS);
	cout << "  Reset for " << (int)(GlobalParams::reset_time) << " cycles completed." << endl;

	reset.write(0);
	cout << "Now running for " << GlobalParams::simulation_time << " cycles... " << endl;
	sc_start(GlobalParams::simulation_time, SC_NS);

	// Close the simulation
	if (GlobalParams::trace_mode)
		sc_close_vcd_trace_file(tf);
	cout << "  Noxim simulation completed: " << sc_time_stamp().to_double() / GlobalParams::clock_period_ps << " cycles executed." << endl;
	cout << endl;

	// *************
	// Added by LGGM
	// *************
	if (tmInstance)
		tmInstance->showTaskMappingStatistics();
	// *************

	// ****************
	// Modified by LGGM
	// ****************
	// Show statistics
	if (tmInstance)
	{
		TM_SIMWRITELOG(endl
					   << "***************" << endl
					   << "WNoC Statistics" << endl
					   << "***************" << endl
					   << endl);
	}
	GlobalStats gs(n);
	if (tmInstance && tmInstance->taskmappinglog_file.is_open())
	{
		tmInstance->taskmappinglog_file.copyfmt(init_cout_format);
		gs.showStats(tmInstance->taskmappinglog_file, GlobalParams::detailed);
		TM_SIMSTATSLOG(endl);
		cout << "See the statistics in the file: " << GlobalParams::taskmappinglog_filename.c_str() << endl
			 << endl;
	}
	else
	{
		cout.copyfmt(init_cout_format);
		gs.showStats(std::cout, GlobalParams::detailed);
	}
	// ****************

	// ****************
	// Modified by JengDe
	// ****************
	// Show WRONoC statistics

	// if(GlobalParams::architecture == ARCH_HYBRID)
	//   if (tmInstance) {
	//     TM_SIMWRITELOG(endl  << "Architecture is HYBRID, no WRONoC statistics." << endl);
	//   }

	//  else if(GlobalParams::architecture != ARCH_MESH){
	if (GlobalParams::architecture != ARCH_MESH)
	{
		if (tmInstance)
		{
			TM_SIMWRITELOG(endl
						   << "***************" << endl
						   << "WRONoC Statistics" << endl
						   << "***************" << endl
						   << endl);
		}

		if (tmInstance && tmInstance->taskmappinglog_file.is_open())
		{
			tmInstance->taskmappinglog_file.copyfmt(init_cout_format);
			gs.showWRONoCStats(tmInstance->taskmappinglog_file);
			TM_SIMSTATSLOG(endl);
			cout << "See the WRONoC statistics in the file: " << GlobalParams::taskmappinglog_filename.c_str() << endl
				 << endl;
		}
		else
		{
			cout.copyfmt(init_cout_format);
			gs.showWRONoCStats(std::cout);
		}
	}
	// ****************

	if ((GlobalParams::max_volume_to_be_drained > 0) &&
		(sc_time_stamp().to_double() / GlobalParams::clock_period_ps - GlobalParams::reset_time >=
		 GlobalParams::simulation_time))
	{
		cout << endl
			 << "WARNING! the number of flits specified with -volume option" << endl
			 << "has not been reached. ( " << drained_volume << " instead of " << GlobalParams::max_volume_to_be_drained << " )" << endl
			 << "You might want to try an higher value of simulation cycles" << endl
			 << "using -sim option." << endl;

#ifdef TESTING
		cout << endl
			 << " Sum of local drained flits: " << gs.drained_total << endl
			 << endl
			 << " Effective drained volume: " << drained_volume;
#endif
	}

#ifdef DEADLOCK_AVOIDANCE
	cout << "***** WARNING: DEADLOCK_AVOIDANCE ENABLED!" << endl;
#endif
	return 0;
}
