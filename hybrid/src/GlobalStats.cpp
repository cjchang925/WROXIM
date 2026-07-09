/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2015 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file contains the implementaton of the global statistics
 */

#include "GlobalStats.h"
using namespace std;

GlobalStats::GlobalStats(const NoC * _noc)
{
    noc = _noc;

#ifdef TESTING
    drained_total = 0;
#endif
}

double GlobalStats::getAverageDelay()
{
    unsigned int total_packets = 0;
    double avg_delay = 0.0;

    for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	for (int x = 0; x < GlobalParams::mesh_dim_x; x++) {
		if(GlobalParams::architecture == ARCH_MESH){
			unsigned int received_packets =
			noc->t[x][y]->r->stats.getReceivedPackets();

			if (received_packets) {
				avg_delay +=
					received_packets *
					noc->t[x][y]->r->stats.getAverageDelay();
				total_packets += received_packets;
			}
		}
		else if(GlobalParams::architecture == ARCH_HYBRID){
			unsigned int received_packets =
			noc->t_Hybrid[x][y]->r->stats.getReceivedPackets();

			if (received_packets) {
				avg_delay +=
					received_packets *
					noc->t_Hybrid[x][y]->r->stats.getAverageDelay();
				total_packets += received_packets;
			}
		}
		else{
			//ONoC
			unsigned int received_packets =
			noc->t_ONoC[x][y]->r->stats.getReceivedPackets();

			if (received_packets) {
				avg_delay += received_packets * noc->t_ONoC[x][y]->r->stats.getAverageDelay();
				total_packets += received_packets;
			}
		}
	    
	}
	// cout << "In getAverageDelay, total_packets = " << total_packets << endl;
    avg_delay /= (double) total_packets;

    return avg_delay;
}



double GlobalStats::getAverageDelay(const int src_id,
					 const int dst_id)
{	

	if(GlobalParams::architecture == ARCH_HYBRID){
		Tile *tile;
		tile = noc->searchNode(dst_id);
		assert(tile != NULL);
    	return tile->r->stats.getAverageDelay(src_id);
	}	
	else if(GlobalParams::architecture != ARCH_MESH){
		Tile_ONoC *tile;
		tile = noc->searchNode_ONoC(dst_id);
		assert(tile != NULL);
    	return tile->r->stats.getAverageDelay(src_id);
	}
	else {
		Tile *tile;
		tile = noc->searchNode(dst_id);
		assert(tile != NULL);
    	return tile->r->stats.getAverageDelay(src_id);
	}
    
}

// *************
// Added by LGGM
// *************
vector < vector < double > > GlobalStats::getAverageDelayMtx()
{
    vector < vector < double > > mtx;

    mtx.resize(GlobalParams::mesh_dim_y);
    for (int y = 0; y < GlobalParams::mesh_dim_y; y++) {
	mtx[y].resize(GlobalParams::mesh_dim_x);
	
	for (int x = 0; x < GlobalParams::mesh_dim_x; x++) {
	    unsigned int received_packets = noc->t[x][y]->r->stats.getReceivedPackets();

	    if (received_packets) {
			mtx[y][x] = noc->t[x][y]->r->stats.getAverageDelay();
	    }
	    else {
			mtx[y][x] = 0.0;
	    }
	}
    }
    return mtx;
}
// *************

double GlobalStats::getMaxDelay()
{
// ****************
// Modified by LGGM
    // double maxd = 0.0;
    double maxd = -1.0;		// Original
// ****************

    for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	for (int x = 0; x < GlobalParams::mesh_dim_x; x++) {
	    Coord coord;
	    coord.x = x;
	    coord.y = y;
	    int node_id = coord2Id(coord);
	    double d = getMaxDelay(node_id);
	    if (d > maxd)
		maxd = d;
	}

    return maxd;
}

double GlobalStats::getMaxDelay(const int node_id)
{
    Coord coord = id2Coord(node_id);

	if(GlobalParams::architecture == ARCH_HYBRID){
		unsigned int received_packets =
		noc->t_Hybrid[coord.x][coord.y]->r->stats.getReceivedPackets();

		if (received_packets)
		return noc->t_Hybrid[coord.x][coord.y]->r->stats.getMaxDelay();
		else
	// ****************
	// Modified by LGGM
		// return 0.0;
		return -1.0;	// Original
	// ****************
	}	

	else if(GlobalParams::architecture != ARCH_MESH){
		unsigned int received_packets =
		noc->t_ONoC[coord.x][coord.y]->r->stats.getReceivedPackets();

		if (received_packets)
		return noc->t_ONoC[coord.x][coord.y]->r->stats.getMaxDelay();
		else
	// ****************
	// Modified by LGGM
		// return 0.0;
		return -1.0;	// Original
	// ****************
	}
	else{
		unsigned int received_packets =
		noc->t[coord.x][coord.y]->r->stats.getReceivedPackets();

		if (received_packets)
		return noc->t[coord.x][coord.y]->r->stats.getMaxDelay();
		else
	// ****************
	// Modified by LGGM
		// return 0.0;
		return -1.0;	// Original
	// ****************
	}	
    
}

double GlobalStats::getMaxDelay(const int src_id, const int dst_id)
{
    Tile *tile = noc->searchNode(dst_id);

    assert(tile != NULL);

    return tile->r->stats.getMaxDelay(src_id);
}

vector < vector < double > > GlobalStats::getMaxDelayMtx()
{
    vector < vector < double > > mtx;

    mtx.resize(GlobalParams::mesh_dim_y);
    for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	mtx[y].resize(GlobalParams::mesh_dim_x);

    for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	for (int x = 0; x < GlobalParams::mesh_dim_x; x++) {
	    Coord coord;
	    coord.x = x;
	    coord.y = y;
	    int id = coord2Id(coord);
	    mtx[y][x] = getMaxDelay(id);
	}

    return mtx;
}

double GlobalStats::getAverageThroughput(const int src_id, const int dst_id)
{
    Tile *tile = noc->searchNode(dst_id);

    assert(tile != NULL);

    return tile->r->stats.getAverageThroughput(src_id);
}

/*
double GlobalStats::getAverageThroughput()
{
    unsigned int total_comms = 0;
    double avg_throughput = 0.0;

    for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	for (int x = 0; x < GlobalParams::mesh_dim_x; x++) {
	    unsigned int ncomms =
		noc->t[x][y]->r->stats.getTotalCommunications();

	    if (ncomms) {
		avg_throughput +=
		    ncomms * noc->t[x][y]->r->stats.getAverageThroughput();
		total_comms += ncomms;
	    }
	}

    avg_throughput /= (double) total_comms;

    return avg_throughput;
}
*/

// *************
// Added by LGGM
// *************
vector < vector < double > > GlobalStats::getAverageThroughputMtx()
{
    vector < vector < double > > mtx;

    mtx.resize(GlobalParams::mesh_dim_y);
    for (int y = 0; y < GlobalParams::mesh_dim_y; y++) {
	mtx[y].resize(GlobalParams::mesh_dim_x);
	
	for (int x = 0; x < GlobalParams::mesh_dim_x; x++) {
	    unsigned int ncomms = noc->t[x][y]->r->stats.getTotalCommunications();

	    if (ncomms) {
		mtx[y][x] = noc->t[x][y]->r->stats.getAverageThroughput();
	    }
	    else {
		mtx[y][x] = 0.0;
	    }
	}
    }
    return mtx;
}
// *************

double GlobalStats::getAggregatedThroughput()
{
    int total_cycles = GlobalParams::simulation_time - GlobalParams::stats_warm_up_time;

    return (double)getReceivedFlits()/(double)(total_cycles);
}

unsigned int GlobalStats::getReceivedPackets()
{
    unsigned int n = 0;

    for (int y = 0; y < GlobalParams::mesh_dim_y; y++){
		for (int x = 0; x < GlobalParams::mesh_dim_x; x++){
			if(GlobalParams::architecture==ARCH_HYBRID){
				n += noc->t_Hybrid[x][y]->r->stats.getReceivedPackets();
			}
			else if(GlobalParams::architecture!=ARCH_MESH)
				n += noc->t_ONoC[x][y]->r->stats.getReceivedPackets();
			else
				n += noc->t[x][y]->r->stats.getReceivedPackets();
		}
	}
    return n;
}

// *************
// Added by LGGM
// *************
vector < vector < double > > GlobalStats::getReceivedPacketsMtx()
{
    vector < vector < double > > mtx;

    mtx.resize(GlobalParams::mesh_dim_y);
    for (int y = 0; y < GlobalParams::mesh_dim_y; y++) {
	mtx[y].resize(GlobalParams::mesh_dim_x);
	
	for (int x = 0; x < GlobalParams::mesh_dim_x; x++) {
	    mtx[y][x] = noc->t[x][y]->r->stats.getReceivedPackets();
	}
    }
    return mtx;
}
// *************

unsigned int GlobalStats::getReceivedFlits()
{
    unsigned int n = 0;

    for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	for (int x = 0; x < GlobalParams::mesh_dim_x; x++) {

		if(GlobalParams::architecture==ARCH_HYBRID){
			n += noc->t_Hybrid[x][y]->r->stats.getReceivedFlits();
		}
		else if(GlobalParams::architecture==ARCH_MESH)
			n += noc->t[x][y]->r->stats.getReceivedFlits();
		else
	    	n += noc->t_ONoC[x][y]->r->stats.getReceivedFlits();
	    	
#ifdef TESTING
	    drained_total += noc->t[x][y]->r->local_drained;
#endif
	}
	cout << "In getReceivedFlits, n = " << n << endl;
    return n;
}

double GlobalStats::getThroughput()
{

    int number_of_ip = GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y;

    return (double)getAggregatedThroughput()/(double)(number_of_ip);
}

// Only accounting IP that received at least one flit
double GlobalStats::getActiveThroughput()
{
    int total_cycles =
	GlobalParams::simulation_time -
	GlobalParams::stats_warm_up_time;


    unsigned int n = 0;
    unsigned int trf = 0;
    for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	for (int x = 0; x < GlobalParams::mesh_dim_x; x++) {
	    unsigned int rf;

		if(GlobalParams::architecture==ARCH_HYBRID){
			rf = noc->t_Hybrid[x][y]->r->stats.getReceivedFlits();
		}
		else if(GlobalParams::architecture==ARCH_MESH){
			rf = noc->t[x][y]->r->stats.getReceivedFlits();
		}
		else {
			rf = noc->t_ONoC[x][y]->r->stats.getReceivedFlits();
		}
	    if (rf != 0)
		n++;

	    trf += rf;
	}

    return (double) trf / (double) (total_cycles * n);

}

vector < vector < unsigned long > > GlobalStats::getRoutedFlitsMtx()
{

    vector < vector < unsigned long > > mtx;

    mtx.resize(GlobalParams::mesh_dim_y);
    for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	mtx[y].resize(GlobalParams::mesh_dim_x);

    for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	for (int x = 0; x < GlobalParams::mesh_dim_x; x++){
		if(GlobalParams::architecture==ARCH_HYBRID)
			mtx[y][x] = noc->t_Hybrid[x][y]->r->getRoutedFlits();
		
		else if(GlobalParams::architecture==ARCH_MESH)
			mtx[y][x] = noc->t[x][y]->r->getRoutedFlits();
		else
	    	mtx[y][x] = noc->t_ONoC[x][y]->r->getRoutedFlits();
	    	
	}

    return mtx;
}
//---------------------wireless packets sent
unsigned int GlobalStats::getWirelessPackets()
{
	if(GlobalParams::architecture != ARCH_MESH){
		return 0;
	}
    unsigned int packets = 0;

    // Wireless noc
    for (map<int, HubConfig>::iterator it = GlobalParams::hub_configuration.begin();
            it != GlobalParams::hub_configuration.end();
            ++it)
    {
	int hub_id = it->first;

	map<int,Hub*>::const_iterator i = noc->hub.find(hub_id);
	Hub * h = i->second;

	packets+= h->wireless_communications_counter;
    }
    return packets;
}

double GlobalStats::getDynamicPower()
{
    double power = 0.0;

    // Electric noc
    for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	for (int x = 0; x < GlobalParams::mesh_dim_x; x++){
		if(GlobalParams::architecture == ARCH_HYBRID)
			power += noc->t_Hybrid[x][y]->r->power.getDynamicPower();
		else if(GlobalParams::architecture != ARCH_MESH)
	    		power += noc->t_ONoC[x][y]->r->power.getDynamicPower();
		else
	    		power += noc->t[x][y]->r->power.getDynamicPower();
	}
	cout << "DynamicPower = " << power << endl;
	return power;
	// if(GlobalParams::architecture != ARCH_MESH){
	// 	cout << "DynamicPower = " << power << endl;
	// 	return power;
	// }
	// else if(GlobalParams::architecture == ARCH_HYBRID){
	// 	cout << "DynamicPower = " << power << endl;
	// 	return power;
	// }

    // // Wireless noc
    // for (map<int, HubConfig>::iterator it = GlobalParams::hub_configuration.begin();
    //         it != GlobalParams::hub_configuration.end();
    //         ++it)
    // {
	// int hub_id = it->first;

	// map<int,Hub*>::const_iterator i = noc->hub.find(hub_id);
	// Hub * h = i->second;

	// power+= h->power.getDynamicPower();
    // }
    // return power;
}

double GlobalStats::getStaticPower()
{
    double power = 0.0;

    for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	for (int x = 0; x < GlobalParams::mesh_dim_x; x++){
		if(GlobalParams::architecture == ARCH_HYBRID)
			power += noc->t_Hybrid[x][y]->r->power.getStaticPower();
		else if(GlobalParams::architecture != ARCH_MESH)
	    		power += noc->t_ONoC[x][y]->r->power.getStaticPower();
		else
	    		power += noc->t[x][y]->r->power.getStaticPower();
	}
	cout << "StaticPower = " << power << endl;
	return power;
	// if(GlobalParams::architecture != ARCH_MESH){
	// 	cout << "StaticPower = " << power << endl;
	// 	return power;
	// }
    // Wireless noc
    // for (map<int, HubConfig>::iterator it = GlobalParams::hub_configuration.begin();
    //         it != GlobalParams::hub_configuration.end();
    //         ++it)
    // {
	// int hub_id = it->first;

	// map<int,Hub*>::const_iterator i = noc->hub.find(hub_id);
	// Hub * h = i->second;

	// power+= h->power.getStaticPower();
    // }
    // return power;
}

void GlobalStats::showStats(std::ostream & out, bool detailed)
{
    if (detailed) {
		out << "detailed = [" << endl;
		for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
			for (int x = 0; x < GlobalParams::mesh_dim_x; x++)
			noc->t[x][y]->r->stats.showStats(y * GlobalParams:: mesh_dim_x + x, out, true);
		out << "];" << endl;

		// *************
		// Added by LGGM
		// *************
		
		// show AverageDelay matrix
		vector < vector < double > > mad_mtx = getAverageDelayMtx();

		out << endl << "average_delay = [" << endl;
		for (unsigned int y = 0; y < mad_mtx.size(); y++) {
			out << "   ";
			for (unsigned int x = 0; x < mad_mtx[y].size(); x++)
			out << setw(12) << mad_mtx[y][x];
			out << endl;
		}
		out << "];" << endl;

		// show throughputPackets matrix
		vector < vector < double > > thr_mtx = getAverageThroughputMtx();

		out << endl << "average_throughtput = [" << endl;
		for (unsigned int y = 0; y < thr_mtx.size(); y++) {
			out << "   ";
			for (unsigned int x = 0; x < thr_mtx[y].size(); x++)
			out << setw(12) << thr_mtx[y][x];
			out << endl;
		}
		out << "];" << endl;
		
		// show receivedPackets matrix
		vector < vector < double > > rec_mtx = getReceivedPacketsMtx();

		out << endl << "received_packets = [" << endl;
		for (unsigned int y = 0; y < rec_mtx.size(); y++) {
			out << "   ";
			for (unsigned int x = 0; x < rec_mtx[y].size(); x++)
			out << setw(12) << rec_mtx[y][x];
			out << endl;
		}
		out << "];" << endl;

		// *************
		
		// show MaxDelay matrix
		vector < vector < double > > md_mtx = getMaxDelayMtx();

		out << endl << "max_delay = [" << endl;
		for (unsigned int y = 0; y < md_mtx.size(); y++) {
			out << "   ";
			for (unsigned int x = 0; x < md_mtx[y].size(); x++)
			out << setw(6) << md_mtx[y][x];
			out << endl;
		}
		out << "];" << endl;

		// show RoutedFlits matrix
		vector < vector < unsigned long > > rf_mtx = getRoutedFlitsMtx();

		out << endl << "routed_flits = [" << endl;
		for (unsigned int y = 0; y < rf_mtx.size(); y++) {
			out << "   ";
			for (unsigned int x = 0; x < rf_mtx[y].size(); x++)
			out << setw(10) << rf_mtx[y][x];
			out << endl;
		}
		out << "];" << endl;

		showPowerBreakDown(out);
		showPowerManagerStats(out);
		
		
        
        out << endl;
    }

    int total_cycles = GlobalParams::simulation_time - GlobalParams::stats_warm_up_time;
	cout << "After total_cycle" << endl;
    out << "% Total received packets: " << getReceivedPackets() << endl;
	cout << "After getReceivedPackets()" << endl;

    out << "% Total received flits: " << getReceivedFlits() << endl;
	cout << "After getReceivedFlits()" << endl;

    out << "% Received/Ideal flits Ratio: " << getReceivedFlits()
	/ (GlobalParams::packet_injection_rate * (GlobalParams::min_packet_size +
		GlobalParams::max_packet_size)/2 * total_cycles * GlobalParams::mesh_dim_y * GlobalParams::mesh_dim_x) << endl;
	cout << "After Received/Ideal flits Ratio" << endl;

    //out << "% Average wireless utilization: " << getWirelessPackets()/(double)getReceivedPackets() << endl;
	//cout << "After Average wireless utilization" << endl;
	
    out << "% Global average delay (cycles): " << getAverageDelay() << endl;
	cout << "After Global average delay (cycles)" << endl;

    out << "% Max delay (cycles): " << getMaxDelay() << endl;
	cout << "After getMaxDelay()" << endl;

    out << "% Network throughput (flits/cycle): " << getAggregatedThroughput() << endl;
	cout << "After getAggregatedThroughput()" << endl;

    out << "% Average IP throughput (flits/cycle/IP): " << getThroughput() << endl;
	cout << "After getThroughput()" << endl;

    out << "% Total energy (J): " << getTotalPower() << endl;
	cout << "After getTotalPower()" << endl;

    out << "% \tDynamic energy (J): " << getDynamicPower() << endl;
	cout << "After getDynamicPower()" << endl;

    out << "% \tStatic energy (J): " << getStaticPower() << endl;
	cout << "After getStaticPower()" << endl;
	// added by xuanwei
	//showTop10DynamicPower(out);
	showTop10StaticPower(out);
	// end of added by xuanwei


    if (GlobalParams::show_buffer_stats)
      showBufferStats(out);

}


void GlobalStats::updatePowerBreakDown(map<string,double> &dst,PowerBreakdown* src)
{
    for (int i=0;i!=src->size;i++)
    {
	dst[src->breakdown[i].label]+=src->breakdown[i].value;
    }
}

void GlobalStats::showPowerManagerStats(std::ostream & out)
{
    std::streamsize p = out.precision();
    int total_cycles = sc_time_stamp().to_double() / GlobalParams::clock_period_ps - GlobalParams::reset_time;

    out.precision(4);

    out << "powermanager_stats_tx = [" << endl;
    out << "%\tFraction of: TX Transceiver off (TTXoff), AntennaBufferTX off (ABTXoff) " << endl;
    out << "%\tHUB\tTTXoff\tABTXoff\t" << endl;

    for (map<int, HubConfig>::iterator it = GlobalParams::hub_configuration.begin();
            it != GlobalParams::hub_configuration.end();
            ++it)
    {
	int hub_id = it->first;

	map<int,Hub*>::const_iterator i = noc->hub.find(hub_id);
	Hub * h = i->second;

	out << "\t" << hub_id << "\t" << std::fixed << (double)h->total_ttxoff_cycles/total_cycles << "\t";

	int s = 0;
	for (map<int,int>::iterator i = h->abtxoff_cycles.begin(); i!=h->abtxoff_cycles.end();i++) s+=i->second;

	out << (double)s/h->abtxoff_cycles.size()/total_cycles << endl;
    }

    out << "];" << endl;



    out << "powermanager_stats_rx = [" << endl;
    out << "%\tFraction of: RX Transceiver off (TRXoff), AntennaBufferRX off (ABRXoff), BufferToTile off (BTToff) " << endl;
    out << "%\tHUB\tTRXoff\tABRXoff\tBTToff\t" << endl;



    for (map<int, HubConfig>::iterator it = GlobalParams::hub_configuration.begin();
            it != GlobalParams::hub_configuration.end();
            ++it)
    {
	string bttoff_str;

	out.precision(4);

	int hub_id = it->first;

	map<int,Hub*>::const_iterator i = noc->hub.find(hub_id);
	Hub * h = i->second;

	out << "\t" << hub_id << "\t" << std::fixed << (double)h->total_sleep_cycles/total_cycles << "\t";

	int s = 0;
	for (map<int,int>::iterator i = h->buffer_rx_sleep_cycles.begin();
		i!=h->buffer_rx_sleep_cycles.end();i++)
	    s+=i->second;

	out << (double)s/h->buffer_rx_sleep_cycles.size()/total_cycles << "\t";

	s = 0;
	for (map<int,int>::iterator i = h->buffer_to_tile_poweroff_cycles.begin();
		i!=h->buffer_to_tile_poweroff_cycles.end();i++)
	{
	    double bttoff_fraction = i->second/(double)total_cycles;
	    s+=i->second;
	    if (bttoff_fraction<0.25)
		bttoff_str+=" ";
	    else if (bttoff_fraction<0.5)
		    bttoff_str+=".";
	    else if (bttoff_fraction<0.75)
		    bttoff_str+="o";
	    else if (bttoff_fraction<0.90)
		    bttoff_str+="O";
	    else 
		bttoff_str+="0";
	    

	}
	out << (double)s/h->buffer_to_tile_poweroff_cycles.size()/total_cycles << "\t" << bttoff_str << endl;
    }

    out << "];" << endl;

    out.unsetf(std::ios::fixed);

    out.precision(p);

}
		// ****************
		// Added by XuanWei
		// ****************
void GlobalStats::showTop10DynamicPower(std::ostream& out) {
    std::vector<double> total(NO_BREAKDOWN_ENTRIES_D, 0.0);
    std::vector<std::string> labels(NO_BREAKDOWN_ENTRIES_D);

    for (int i = 0; i < GlobalParams::mesh_dim_y ; ++i) {
    for (int j = 0; j <  GlobalParams::mesh_dim_x ; ++j) {
        PowerBreakdown* breakdown_ptr = nullptr;
	   if(GlobalParams::architecture == ARCH_HYBRID){
            if (!noc->t_Hybrid[i][j] || !noc->t_Hybrid[i][j]->r) continue;
            breakdown_ptr = noc->t_Hybrid[i][j]->r->power.getDynamicPowerBreakDown();
        } 
        else if (GlobalParams::architecture == ARCH_MESH) {
            if (!noc->t[i][j] || !noc->t[i][j]->r) continue;
            breakdown_ptr = noc->t[i][j]->r->power.getDynamicPowerBreakDown();
        } 
	   else {
            if (!noc->t_ONoC[i][j] || !noc->t_ONoC[i][j]->r) continue;
            breakdown_ptr = noc->t_ONoC[i][j]->r->power.getDynamicPowerBreakDown();
        }
        if (!breakdown_ptr) continue;
        for (int k = 0; k < NO_BREAKDOWN_ENTRIES_D; ++k) {
            total[k] += breakdown_ptr->breakdown[k].value;
            labels[k] = breakdown_ptr->breakdown[k].label;
        }
    }
}

    std::vector<std::pair<double, std::string>> breakdown;
    for (int k = 0; k < NO_BREAKDOWN_ENTRIES_D; ++k) {
        breakdown.push_back({total[k], labels[k]});
    }
    std::sort(breakdown.rbegin(), breakdown.rend());

    out << "% Top 10 Dynamic Energy Consumption:" << std::endl;
    for (int i = 0; i < 10 && i < (int)breakdown.size(); ++i) {
        out <<"% "<< i+1 << ". " << breakdown[i].second << ": " << breakdown[i].first << " J" << std::endl;
    }
}


void GlobalStats::showTop10StaticPower(std::ostream& out) {
	//cout << "In showTop10StaticPower()" << endl;
    std::vector<double> total(NO_BREAKDOWN_ENTRIES_D, 0.0);
    std::vector<std::string> labels(NO_BREAKDOWN_ENTRIES_D);
	//cout << "In showTop10StaticPower()" << endl;
    for (int i = 0; i < GlobalParams::mesh_dim_y ; ++i) {
    for (int j = 0; j <  GlobalParams::mesh_dim_x ; ++j) {
        PowerBreakdown* breakdown_ptr = nullptr;

	   if (GlobalParams::architecture == ARCH_HYBRID) {
           if (!noc->t_Hybrid[i][j] || !noc->t_Hybrid[i][j]->r) continue;
           breakdown_ptr = noc->t_Hybrid[i][j]->r->power.getStaticPowerBreakDown();
       }

        else if (GlobalParams::architecture == ARCH_MESH) {
            if (!noc->t[i][j] || !noc->t[i][j]->r) continue;
            breakdown_ptr = noc->t[i][j]->r->power.getStaticPowerBreakDown();
        } else {
            if (!noc->t_ONoC[i][j] || !noc->t_ONoC[i][j]->r) continue;
            breakdown_ptr = noc->t_ONoC[i][j]->r->power.getStaticPowerBreakDown();
        }
        if (!breakdown_ptr) continue;
        for (int k = 0; k < NO_BREAKDOWN_ENTRIES_D; ++k) {
			cout<< "breakdown_ptr->breakdown[" << k << "].label: " << breakdown_ptr->breakdown[k].label << ", value: " << breakdown_ptr->breakdown[k].value << endl;
            	total[k] += breakdown_ptr->breakdown[k].value;
            	labels[k] = breakdown_ptr->breakdown[k].label;
        }
    }
}

    std::vector<std::pair<double, std::string>> breakdown;
    for (int k = 0; k < NO_BREAKDOWN_ENTRIES_D; ++k) {
        breakdown.push_back({total[k], labels[k]});
    }
    std::sort(breakdown.rbegin(), breakdown.rend());

    out << "% Top 10 Static Energy Consumption:" << std::endl;
    for (int i = 0; i < 10 && i < (int)breakdown.size(); ++i) {
        out <<"% "<< i+1 << ". " << breakdown[i].second << ": " << breakdown[i].first << " J" << std::endl;
    }
}
		// ****************
		// Ended by XuanWei
		// ****************

void GlobalStats::showPowerBreakDown(std::ostream & out)
{
    map<string,double> power_dynamic;
    map<string,double> power_static;

    for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
	for (int x = 0; x < GlobalParams::mesh_dim_x; x++)
	{
	    updatePowerBreakDown(power_dynamic, noc->t[x][y]->r->power.getDynamicPowerBreakDown());

	    updatePowerBreakDown(power_static, noc->t[x][y]->r->power.getStaticPowerBreakDown());
	}

    for (map<int, HubConfig>::iterator it = GlobalParams::hub_configuration.begin();
            it != GlobalParams::hub_configuration.end();
            ++it)
    {
	int hub_id = it->first;

	map<int,Hub*>::const_iterator i = noc->hub.find(hub_id);
	Hub * h = i->second;

	    updatePowerBreakDown(power_dynamic, 
		h->power.getDynamicPowerBreakDown());

	    updatePowerBreakDown(power_static, 
		h->power.getStaticPowerBreakDown());
    }

    printMap("power_dynamic",power_dynamic,out);
    printMap("power_static",power_static,out);

}



void GlobalStats::showBufferStats(std::ostream & out)
{
  out << "Router id\tBuffer N\t\tBuffer E\t\tBuffer S\t\tBuffer W\t\tBuffer L" << endl;
  out << "         \tMean\tMax\tMean\tMax\tMean\tMax\tMean\tMax\tMean\tMax" << endl;
  for (int y = 0; y < GlobalParams::mesh_dim_y; y++)
    for (int x = 0; x < GlobalParams::mesh_dim_x; x++)
      {
	out << noc->t[x][y]->r->local_id;
	noc->t[x][y]->r->ShowBuffersStats(out);
	out << endl;
      }
}


//! WRONoC statistics --------------------------------------------------------------------
void GlobalStats::showWRONoCStats(std::ostream & out)
{
	RouterOpticalPower tmp;
	// tmp.printLaserTimeUnsorted(out);
	tmp.calculateWavelengthIntervals();
	// tmp.printWavelengthIntervals(out);
	tmp.calculateWavelengthUsageTime();
	tmp.calculateLaserIntervals();
	// tmp.printOpticalTxTime(out);
	// tmp.printOpticalRxTime(out);
	tmp.calculateOpticalTxIntervals();
	tmp.calculateOpticalRxIntervals();
	
	tmp.printIntervals(out);

	out << "\n\tSimulation time = " << GlobalParams::simulation_time * 1e3 / GlobalParams::clock_period_ps << " cycles." << endl;
	out << "% Total energy (J): " << getTotalEnergy_WRONoC() << endl;
	
	out << "% \t Laser energy (J): " << getLaserEnergy()+getLaserLossEnergy() << endl;
	out << "% \t\t Laser loss energy (J): " << getLaserLossEnergy() << endl;
	out << "% \t MRR energy (J): " << getMRREnergy() << endl; 
	out << "% \t EOE (J): " << getEOEEnergy() << endl;
	out << "% \t\t Tx energy (J): " << getTxEnergy() << endl;
	out << "% \t\t Rx energy (J): " << getRxEnergy() << endl;
	// out << "% \t\t Arbitration and Flow Control energy (J): " << getArbitrationFlowControlEnergy() << endl;

}

double GlobalStats::getTotalEnergy_WRONoC()
{
	return getLaserEnergy() + getEOEEnergy() + getMRREnergy();
}

double GlobalStats::getLaserEnergy()
{
	RouterOpticalPower tmp;
	return tmp.getLaserEnergy() + tmp.getLaserLossEnergy();
}
double GlobalStats::getLaserLossEnergy()
{
	RouterOpticalPower tmp;
	return tmp.getLaserLossEnergy();
}

double GlobalStats::getTxEnergy()
{
	RouterOpticalPower tmp;
	return tmp.getTxEnergy();
}
double GlobalStats::getRxEnergy()
{
	RouterOpticalPower tmp;
	return tmp.getRxEnergy();
}
// double GlobalStats::getArbitrationFlowControlEnergy()
// {
// 	RouterOpticalPower tmp;
// 	return tmp.getArbitrationFlowControlEnergy();
// }
double GlobalStats::getEOEEnergy()
{
	RouterOpticalPower tmp;
	return tmp.getEOEEnergy();
}
double GlobalStats::getMRREnergy()
{
	RouterOpticalPower tmp;
	return tmp.getMRREnergy();
}
// ! -------------------------------------------------------------------------------------