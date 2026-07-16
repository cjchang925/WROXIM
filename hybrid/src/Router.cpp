/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2015 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file contains the implementation of the router
 */

#include "Router.h"

#include "./taskMapping/TaskMapping.h"
#include "./taskMapping/TaskMappingLogs.h"
void Router::rxProcess()
{
    if (reset.read()) {
		// Clear outputs and indexes of receiving protocol
		for (int i = 0; i < DIRECTIONS + 2; i++) {
			ack_rx[i].write(0);
			current_level_rx[i] = 0;
		}
		routed_flits = 0;
		local_drained = 0;
    } else {
	// For each channel decide if a new flit can be accepted
	//
	// This process simply sees a flow of incoming flits. All arbitration
	// and wormhole related issues are addressed in the txProcess()

		for (int i = 0; i < DIRECTIONS + 2; i++) {
			// To accept a new flit, the following conditions must match:
			//
			// 1) there is an incoming request
			// 2) there is a free slot in the input buffer of direction i

			if ((req_rx[i].read() == 1 - current_level_rx[i])
			&& !buffer[i].IsFull()) {
				Flit received_flit = flit_rx[i].read();
				
				cout << "In Router_ENoC::rxProcess_PE(), received_flit.src_id = " << received_flit.src_id << ", req_rx[0].read() = " << req_rx[0].read() << endl;
						// *************
						// Added by LGGM
						// *************
						// Perhaps the authors forgot this sentence
				if (local_id != received_flit.src_id)
					received_flit.hop_no++;

				// Store the incoming flit in the circular buffer
				buffer[i].Push(received_flit);

				power.bufferRouterPush();

				// Negate the old value for Alternating Bit Protocol (ABP)
				current_level_rx[i] = 1 - current_level_rx[i];
				//cout<< "current_level_rx[" << i << "] = " << current_level_rx[i] <<"  received_flit = "<< received_flit << endl;

				// if a new flit is injected from local PE
				if (received_flit.src_id == local_id)
				power.networkInterface();
			}
			else if(buffer[i].IsFull()){
				cout<<"Router "<<local_id<<" buffer full at buffer "<<i<<endl;
			}
			
			ack_rx[i].write(current_level_rx[i]);
		}
    }
}



void Router::txProcess()
{
  if (reset.read()) 
    {
      // Clear outputs and indexes of transmitting protocol
      for (int i = 0; i < DIRECTIONS + 2; i++) 
	{
	  req_tx[i].write(0);
	  current_level_tx[i] = 0;
	  rtable_cnt[i] = 0;
	  cur_path[i] = 0;
	}
    } 
  else 
    {
      // 1st phase: Reservation
      for (int j = 0; j < DIRECTIONS + 2; j++) 
	{
	  int i = (start_from_port + j) % (DIRECTIONS + 2);
		

	  // buffer[i].deadlockCheck();

	  if (!buffer[i].IsEmpty()) 
	    {

			Flit flit = buffer[i].Front();
			power.bufferRouterFront();

			if (flit.flit_type == FLIT_TYPE_HEAD) 
			{
				// prepare data for routing
				RouteData route_data;
				route_data.current_id = local_id;
				route_data.src_id = flit.src_id;
				route_data.dst_id = flit.dst_id;
				route_data.dir_in = i;
				route_data.path = flit.path; // added by xuanWei for path aware mapping
				cout<< "In Router::txProcess(), flit.path = " << flit.path << endl;
			
				int o = route(route_data);
				
				rtable_cnt[o] ++;
				cout <<"router_id = "<< local_id << " rtable_cnt[" << o << "] = " << rtable_cnt[o] << endl;
			
				#if defined(DEFINE_TM_SIMEXECLOG) || defined(DEFINE_TM_SIMSTATSLOG)
					TM_SIMWRITELOG("router id = "<<local_id<<" current flit = "<<flit<<"->"<< " Router path = " << o << endl);
				#endif 
				//o = 5; //force to HUB for testing
				LOG << " checking reservation availability of direction " << o << " for flit " << flit<<" path ="<< flit.path << endl;

				if (reservation_table.isAvailable(o)) 
				{
					LOG << " reserving direction " << o << " for flit " << flit <<"path ="<< flit.path << endl;
					reservation_table.reserve(i, o);
					
					cout << " reservation_table.rtable[" << o << "] = " << i << endl;
				}
				else
				{
					LOG << " cannot reserve direction " << o << " for flit " << flit << endl;
					if(cur_path[o] == 0 ||(cur_path[o]!= 0 && flit.path == 0))
					{		
						rtable_cnt[o] --;
						cout <<"router_id = "<< local_id << " rtable_cnt[" << o << "] = " << rtable_cnt[o] << endl;
					}
				}
			}
	    }
	}
      start_from_port++;


      // 2nd phase: Forwarding
      for (int i = 0; i < DIRECTIONS + 2; i++) 
      {
	  if (!buffer[i].IsEmpty()) 
	  { 
	      // power contribution already computed in 1st phase
	      Flit flit = buffer[i].Front();
	      int o = reservation_table.getOutputPort(i);
		 cout<<"router"<<local_id<<" flit = " << flit << "  o =" << o<<endl;
		//if(i == DIRECTION_HUB)
		 	//cout<<o<<endl;

		
	      if (o != NOT_RESERVED) 
	      {
			//cout << "current_level_tx[o] = " << current_level_tx[o] << ", ack_tx[o].read() = " << ack_tx[o].read() << endl;
			if (current_level_tx[o] == ack_tx[o].read()) 
				{
					//if (GlobalParams::verbose_mode > VERBOSE_OFF) 
					LOG << "Input[" << i << "] forward to Output[" << o << "], flit: " << flit << endl;
					cur_path[o] = flit.path; // added by xuanWei for path aware mapping 
					flit_tx[o].write(flit);
					if (o == DIRECTION_HUB)
					{
					power.r2hLink();
					LOG << "Forwarding to ONOC " << flit << endl;
					}
					else
					{
					power.r2rLink();
					}

					power.crossBar();

					current_level_tx[o] = 1 - current_level_tx[o];
					req_tx[o].write(current_level_tx[o]);
					buffer[i].Pop();

					power.bufferRouterPop();

					// if flit has been consumed
					if (flit.dst_id == local_id)
					power.networkInterface();

					if ((flit.flit_type == FLIT_TYPE_TAIL ) ){
						rtable_cnt[o] --;
						cout<<"router_id = "<< local_id << " rtable_cnt[" << o << "] = " << rtable_cnt[o] << endl;
					}
					//
					if ((flit.flit_type == FLIT_TYPE_TAIL && rtable_cnt[o] == 0) && (o == 4 && GlobalParams::architecture== ARCH_HYBRID))
					//if ((flit.flit_type == FLIT_TYPE_TAIL ))
					reservation_table.release(o);
					
					//if (((flit.flit_type == FLIT_TYPE_TAIL )&& o != 4) || GlobalParams::architecture!= ARCH_HYBRID)
					if((flit.flit_type == FLIT_TYPE_TAIL )&&( o != 4||GlobalParams::architecture!= ARCH_HYBRID))
						reservation_table.release(o);

							// Update stats
					if (o == DIRECTION_LOCAL) 
					{
					LOG << "Consumed flit " << flit << endl;
					stats.receivedFlit(sc_time_stamp().to_double() / GlobalParams::clock_period_ps, flit);
					if (GlobalParams::
						max_volume_to_be_drained) 
					{
						if (drained_volume >= GlobalParams:: max_volume_to_be_drained)
						sc_stop();
						else 
						{
						drained_volume++;
						local_drained++;
						}
					}
					} 
					else if (i != DIRECTION_LOCAL) 
					{
					// Increment routed flits counter
					routed_flits++;
					}
				}
			else
			{
				LOG << " cannot forward Input[" << i << "] forward to Output[" << o << "], flit: " << flit << endl;
				if (flit.flit_type == FLIT_TYPE_HEAD){
					rtable_cnt[o] --;
				  	cout<<"router_id = "<< local_id << " rtable_cnt[" << o << "] = " << rtable_cnt[o] << endl;
					reservation_table.release(o);
					
				 	
				  
				}
			}
	      }
	  }
      }
    }				// else reset read
}

NoP_data Router::getCurrentNoPData()
{
    NoP_data NoP_data;

    for (int j = 0; j < DIRECTIONS; j++) {
	try {
		NoP_data.channel_status_neighbor[j].free_slots = free_slots_neighbor[j].read();
		NoP_data.channel_status_neighbor[j].available = (reservation_table.isAvailable(j));
	}
	catch (int e)
	{
	    if (e!=NOT_VALID) assert(false);
	    // Nothing to do if an NOT_VALID direction is caught
	};
    }

    NoP_data.sender_id = local_id;

    return NoP_data;
}

void Router::perCycleUpdate()
{
    if (reset.read()) {
	for (int i = 0; i < DIRECTIONS + 1; i++)
	    free_slots[i].write(buffer[i].GetMaxBufferSize());
    } else {
        selectionStrategy->perCycleUpdate(this);
		//cout<< "In Router::perCycleUpdate(), power leakage calculation starts." << endl;
	power.leakageRouter();
	for (int i = 0; i < DIRECTIONS + 1; i++)
	{
	    power.leakageBufferRouter();
	    power.leakageLinkRouter2Router();
	}

	power.leakageLinkRouter2Hub();
    }
}

vector < int > Router::routingFunction(const RouteData & route_data)
{
    if (GlobalParams::use_winoc)
    {
        if (hasRadioHub(local_id) &&
                hasRadioHub(route_data.dst_id) &&
                !sameRadioHub(local_id,route_data.dst_id)
           )
        {
            LOG << "Setting direction HUB to reach destination node " << route_data.dst_id << endl;

            vector<int> dirv;
            dirv.push_back(DIRECTION_HUB);
            return dirv;
        }
    }
    LOG << "Wired routing for dst = " << route_data.dst_id << endl;

    return routingAlgorithm->route(this, route_data);
}

int Router::route(const RouteData & route_data)
{

    if (route_data.dst_id == local_id)
	return DIRECTION_LOCAL;


	if(GlobalParams::architecture==ARCH_MESH) {
		power.routing();
     	vector < int >candidate_channels = routingFunction(route_data);
		power.selection();
	   	return selectionFunction(candidate_channels, route_data);
	}
	else if(GlobalParams::architecture==ARCH_LAMBDA){
		return 5; //directly to ONOC
	} else{
    //cout << "In Router::route(), route_data.path = " << route_data.path << endl;
		if(route_data.path == 0)
		{	
			power.routing();
			vector < int >candidate_channels = routingFunction(route_data);
			power.selection();
			return selectionFunction(candidate_channels, route_data);	
		}
		else return 5;
	}

     

	


    // return selectionFunction(candidate_channels, route_data);
    // return 5;
}

void Router::NoP_report() const
{
    NoP_data NoP_tmp;
	LOG << "NoP report: " << endl;

    for (int i = 0; i < DIRECTIONS; i++) {
	NoP_tmp = NoP_data_in[i].read();
	if (NoP_tmp.sender_id != NOT_VALID)
	    cout << NoP_tmp;
    }
}

//---------------------------------------------------------------------------

int Router::NoPScore(const NoP_data & nop_data,
			  const vector < int >&nop_channels) const
{
    int score = 0;

    for (unsigned int i = 0; i < nop_channels.size(); i++) {
	int available;

	if (nop_data.channel_status_neighbor[nop_channels[i]].available)
	    available = 1;
	else
	    available = 0;

	int free_slots =
	    nop_data.channel_status_neighbor[nop_channels[i]].free_slots;

	score += available * free_slots;
    }

    return score;
}

int Router::selectionFunction(const vector < int >&directions,
				   const RouteData & route_data)
{
    // not so elegant but fast escape ;)
    if (directions.size() == 1)
	return directions[0];

    return selectionStrategy->apply(this, directions, route_data);
}

void Router::configure(const int _id,
			    const double _warm_up_time,
			    const unsigned int _max_buffer_size,
			    GlobalRoutingTable & grt)
{
    local_id = _id;
    stats.configure(_id, _warm_up_time);

    start_from_port = DIRECTION_LOCAL;

    if (grt.isValid())
	routing_table.configure(grt, _id);

    for (int i = 0; i < DIRECTIONS + 2; i++)
	buffer[i].SetMaxBufferSize(_max_buffer_size);

    int row = _id / GlobalParams::mesh_dim_x;
    int col = _id % GlobalParams::mesh_dim_x;
    if (row == 0)
      buffer[DIRECTION_NORTH].Disable();
    if (row == GlobalParams::mesh_dim_y-1)
      buffer[DIRECTION_SOUTH].Disable();
    if (col == 0)
      buffer[DIRECTION_WEST].Disable();
    if (col == GlobalParams::mesh_dim_x-1)
      buffer[DIRECTION_EAST].Disable();

}

unsigned long Router::getRoutedFlits()
{
    return routed_flits;
}

unsigned int Router::getFlitsCount()
{
    unsigned count = 0;

    for (int i = 0; i < DIRECTIONS + 2; i++)
	count += buffer[i].Size();

    return count;
}


int Router::reflexDirection(int direction) const
{
    if (direction == DIRECTION_NORTH)
	return DIRECTION_SOUTH;
    if (direction == DIRECTION_EAST)
	return DIRECTION_WEST;
    if (direction == DIRECTION_WEST)
	return DIRECTION_EAST;
    if (direction == DIRECTION_SOUTH)
	return DIRECTION_NORTH;

    // you shouldn't be here
    assert(false);
    return NOT_VALID;
}

int Router::getNeighborId(int _id, int direction) const
{
    Coord my_coord = id2Coord(_id);

    switch (direction) {
    case DIRECTION_NORTH:
	if (my_coord.y == 0)
	    return NOT_VALID;
	my_coord.y--;
	break;
    case DIRECTION_SOUTH:
	if (my_coord.y == GlobalParams::mesh_dim_y - 1)
	    return NOT_VALID;
	my_coord.y++;
	break;
    case DIRECTION_EAST:
	if (my_coord.x == GlobalParams::mesh_dim_x - 1)
	    return NOT_VALID;
	my_coord.x++;
	break;
    case DIRECTION_WEST:
	if (my_coord.x == 0)
	    return NOT_VALID;
	my_coord.x--;
	break;
    default:
	LOG << "Direction not valid : " << direction;
	assert(false);
    }

    int neighbor_id = coord2Id(my_coord);

    return neighbor_id;
}

bool Router::inCongestion()
{
    for (int i = 0; i < DIRECTIONS; i++) {

	if (free_slots_neighbor[i]==NOT_VALID) continue;

	int flits = GlobalParams::buffer_depth - free_slots_neighbor[i];
	if (flits > (int) (GlobalParams::buffer_depth * GlobalParams::dyad_threshold))
	    return true;
    }

    return false;
}

void Router::ShowBuffersStats(std::ostream & out)
{
  for (int i=0; i<DIRECTIONS+2; i++)
    buffer[i].ShowStats(out);
}


//! -------------------------------------------------- WRONoC --------------------------------------------------
//! -------------------------------------------------- WRONoC --------------------------------------------------
//! -------------------------------------------------- WRONoC --------------------------------------------------
//! -------------------------------------------------- WRONoC --------------------------------------------------
//! -------------------------------------------------- WRONoC --------------------------------------------------
void Router_ONoC::rxProcess_PE()
{
    if (reset.read()) {
		// Clear outputs and indexes of receiving protocol
		
		ack_rx_local.write(0);

		current_level_rx[0] = 0;
		
		routed_flits = 0;
		local_drained = 0;

    } else {
		Flit received_flit = flit_rx[0].read();
		if ((req_rx[0].read() == 1 - current_level_rx[0]) && (tx_buffer[received_flit.dst_id].Size() < tx_buffer[received_flit.dst_id].GetMaxBufferSize()-1)) {
		
			cout << "In Router_ONoC::rxProcess_PE(), received_flit.src_id = " << received_flit.src_id << ", req_rx[0].read() = " << req_rx[0].read() << endl;

			// *************
			// Added by LGGM
			// *************
			// Perhaps the authors forgot this sentence
			if (local_id != received_flit.src_id)
				received_flit.hop_no++;

			// Store the incoming flit in the circular buffer
			if(received_flit.dst_id == local_id){
				string t = "\nIn Router_ONoC::rxProcess_PE(), Error condition -> received_flit.dst_id == local_id.";
				TM_ASSERT(false, t);
				assert(false);
			}
			else{
				tx_buffer[received_flit.dst_id].Push(received_flit);
				//cout << "In Router_ONoC::rxProcess_PE(), local_id = "<<local_id<<", Push src_task = "<<static_cast<tm_task*>(received_flit.src_task)->getId()<<", dst_task = "<<static_cast<tm_task*>(received_flit.dst_task)->getId()<<", flit :" << received_flit << " into tx_buffer" << endl;
				tx_buffer[received_flit.dst_id].Print("tx_buffer");
				//cout << "In Router_ONoC::rxProcess_PE(), local_id = "<<local_id<<", tx_buffer[" << received_flit.dst_id << "].Size() = " << tx_buffer[received_flit.dst_id].Size() << endl;

				

			}

			power.bufferRouterPush();
			
			// Negate the old value for Alternating Bit Protocol (ABP)
			current_level_rx[0] = 1 - current_level_rx[0];
			//cout << "In Router_ONoC::rxProcess_PE(), received flit, current_level_rx[0] = " << current_level_rx[0] << endl;

			// if a new flit is injected from local PE
			if (received_flit.src_id == local_id)
				power.networkInterface();
			
			cout << "In Router_ONoC::rxProcess_PE(), flowControl_signal["<<received_flit.dst_id<<"] = " << flowControl_signal[received_flit.dst_id] << endl;
			
			
			
		}
		ack_rx_local.write(current_level_rx[0]);
    }
}
void Router_ONoC::rxProcess_centralRouter()
{
    if (reset.read()) {
		// Clear outputs and indexes of receiving protocol
		
		for (int i = 1; i < GlobalParams::mesh_dim_x*GlobalParams::mesh_dim_y + 1; i++) {
			ACK t;
			t.value = 0;
			t.src_id = local_id;
			t.dst_id = local_id;
	    	ack_rx[i].write(t);
	    	current_level_rx[i] = 0;
		}
		
		for (int i = 0; i < GlobalParams::mesh_dim_x*GlobalParams::mesh_dim_y ; i++) {
			flowControl_signal[i] = true;
			flowControl_table[i] = true;
		}
		
		routed_flits = 0;
		local_drained = 0;

		for (int i = 0; i < GlobalParams::mesh_dim_x*GlobalParams::mesh_dim_y ; i++) {
			buffer_full_counter[i] = 0.0;
		}
		

    } else if(GlobalParams::network_protocol == ALTERNETING_BIT_PROTOCOL) {
		for (int i = 1; i < GlobalParams::mesh_dim_x*GlobalParams::mesh_dim_y + 1; i++) {
			Flit received_flit = flit_rx[i].read();
			if ((req_rx[i].read() == 1 - current_level_rx[i]) && ((received_flit.src_id != local_id) && (received_flit.src_id + 1 == i)) && (!rx_buffer[received_flit.src_id].IsFull())) {
				
				cout << "In Router_ONoC::rxProcess_centralRouter(), received_flit.src_id = " << received_flit.src_id << ", req_rx[i].read() = " << req_rx[i].read() << endl;

				// *************
				// Added by LGGM
				// *************
				// Perhaps the authors forgot this sentence
				if (local_id != received_flit.src_id)
					received_flit.hop_no++;

				// Store the incoming flit in the circular rx_buffer
				if(received_flit.dst_id == local_id){
					rx_buffer[received_flit.src_id].Push(received_flit);
					cout << "In Router_ONoC::rxProcess_centralRouter(), Push flit :" << received_flit << " into rx_buffer["<<received_flit.src_id<<"]" << endl;
					
					rx_buffer[received_flit.src_id].Print("rx_buffer");
					cout << "In Router_ONoC::rxProcess_centralRouter(), rx_buffer[received_flit.src_id].Size() = " << rx_buffer[received_flit.src_id].Size() << endl;

					power.bufferRouterPush();

					// laser power
					if (received_flit.flit_type == FLIT_TYPE_TAIL){
						int src_mid = opticalPower.getMasterId(received_flit.src_id);
						int dst_mid = opticalPower.getMasterId(received_flit.dst_id);
						PathInfo tmpPathInfo = opticalPower.getPathInfo(src_mid, dst_mid);
						opticalPower.addNewLaserTime(tmpPathInfo.Wavelength_type, src_mid, dst_mid, -1, (sc_time_stamp().to_double() / GlobalParams::clock_period_ps));
						// addNewLaserTime(int w_type, int src_id, int dst_id, int start_time, int end_time);
						cout << "\nopticalPower.addNewLaserTime: local_id = " << local_id << ", received_flit.src_id = " << received_flit.src_id << ", received_flit.dst_id = " << received_flit.dst_id << ", end_time = " << sc_time_stamp().to_double() / GlobalParams::clock_period_ps << endl;
					}
				}
				else{
					string t = "\nIn Router_ONoC::rxProcess_centralRouter(), Error condition -> received_flit.dst_id != local_id.";
					TM_ASSERT(false, t);
					assert(false);

				}

				// Negate the old value for Alternating Bit Protocol (ABP)
				current_level_rx[i] = 1 - current_level_rx[i];
				if(received_flit.flit_type == FLIT_TYPE_HEAD){	//ACK Laser on
					int src_mid = opticalPower.getMasterId(local_id);
					int dst_mid = opticalPower.getMasterId(i-1);
					PathInfo tmpPathInfo = opticalPower.getPathInfo(src_mid, dst_mid);
					opticalPower.addNewLaserTime(tmpPathInfo.Wavelength_type, src_mid, dst_mid, (sc_time_stamp().to_double() / GlobalParams::clock_period_ps), -1);
					cout << "\nopticalPower.addNewLaserTime: local_id = " << local_id << ", i-1 = " << i-1 << ", start_time = " << sc_time_stamp().to_double() / GlobalParams::clock_period_ps << endl;
				}
				else if(received_flit.flit_type == FLIT_TYPE_TAIL){	//ACK Laser OFF
					int src_mid = opticalPower.getMasterId(local_id);
					int dst_mid = opticalPower.getMasterId(i-1);
					PathInfo tmpPathInfo = opticalPower.getPathInfo(src_mid, dst_mid);
					opticalPower.addNewLaserTime(tmpPathInfo.Wavelength_type, src_mid, dst_mid, -1, (sc_time_stamp().to_double() / GlobalParams::clock_period_ps));
					cout << "\nopticalPower.addNewLaserTime: local_id = " << local_id << ", i-1 = " << i-1 << ", end_time = " << sc_time_stamp().to_double() / GlobalParams::clock_period_ps << endl;

					//print buffer contention duration
					TM_SIMBUFFERSTATUSLOG(34, static_cast<tm_task*>(received_flit.src_task)->getApp()->getId(), static_cast<tm_task*>(received_flit.src_task)->getId(), static_cast<tm_task*>(received_flit.dst_task)->getId(), " FULL cycles: " << fixed << setprecision(2) << (buffer_full_counter[received_flit.src_id] / GlobalParams::clock_period_optical_ps));
					buffer_full_counter[received_flit.src_id] = 0.0;
				}

				if(received_flit.flit_type == FLIT_TYPE_TAIL){
					TM_SIMOPTICALEND2ENDLATENCYLOG(34, static_cast<tm_task*>(received_flit.src_task)->getApp()->getId(), static_cast<tm_task*>(received_flit.src_task)->getId(), static_cast<tm_task*>(received_flit.dst_task)->getId(), "end receiving tail_flit at " << fixed << setprecision(2) << (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps));
				}

				// if a new flit is injected from local PE
				if (received_flit.src_id == local_id)
					power.networkInterface();
			}
			else if((req_rx[i].read() == 1 - current_level_rx[i]) && rx_buffer[received_flit.src_id].IsFull()){
				
				buffer_full_counter[received_flit.src_id] += 1;
			}
			ACK t;
			t.value = current_level_rx[i];
			t.src_id = local_id;
			t.dst_id = i-1;
			ack_rx[i].write(t);
		}
    } else if(GlobalParams::network_protocol == ON_OFF_FLOWCONTROL) {
		for (int i = 1; i < GlobalParams::mesh_dim_x*GlobalParams::mesh_dim_y + 1; i++) {
			Flit received_flit = flit_rx[i].read();
			if ((req_rx[i].read() == 1 - current_level_rx[i]) && ((received_flit.src_id != local_id) && (received_flit.src_id + 1 == i)) && (!rx_buffer[received_flit.src_id].IsFull() || received_flit.flit_type == FLIT_TYPE_FLOWCONTROL)) {
				cout << "In Router_ONoC::rxProcess_centralRouter(), received_flit.src_id = " << received_flit.src_id << ", req_rx[i].read() = " << req_rx[i].read() << endl;

				// *************
				// Added by LGGM
				// *************
				// Perhaps the authors forgot this sentence
				if (local_id != received_flit.src_id)
					received_flit.hop_no++;

				// Store the incoming flit in the circular rx_buffer
				if(received_flit.dst_id == local_id){
					if(received_flit.flit_type == FLIT_TYPE_FLOWCONTROL){
						flowControl_signal[received_flit.src_id] = received_flit.flow_control;
						cout << "In Router_ONoC::rxProcess_centralRouter(), local_id = "<<local_id<<", received flowControl :" << received_flit <<  endl;
						
						// laser power
						// laser off for flow control 
						int src_mid = opticalPower.getMasterId(received_flit.src_id);
						int dst_mid = opticalPower.getMasterId(received_flit.dst_id);
						PathInfo tmpPathInfo = opticalPower.getPathInfo(src_mid, dst_mid);
						opticalPower.addNewLaserTime(tmpPathInfo.Wavelength_type, src_mid, dst_mid, -1, (sc_time_stamp().to_double() / GlobalParams::clock_period_ps));
						// addNewLaserTime(int w_type, int src_id, int dst_id, int start_time, int end_time);
						cout << "\nopticalPower.addNewLaserTime: local_id = " << local_id << ", received_flit.src_id = " << received_flit.src_id << ", received_flit.dst_id = " << received_flit.dst_id << ", end_time = " << sc_time_stamp().to_double() / GlobalParams::clock_period_ps << endl;
						if(received_flit.flow_control == false){	// laser off
							
							int src_mid = opticalPower.getMasterId(local_id);
							int dst_mid = opticalPower.getMasterId(received_flit.src_id);
							PathInfo tmpPathInfo = opticalPower.getPathInfo(src_mid, dst_mid);
							opticalPower.addNewLaserTime(tmpPathInfo.Wavelength_type, src_mid, dst_mid, -1, (sc_time_stamp().to_double() / GlobalParams::clock_period_ps));
							// addNewLaserTime(int w_type, int src_id, int dst_id, int start_time, int end_time);
							cout << "\nopticalPower.addNewLaserTime: local_id = " << local_id << ", received_flit.src_id = " << received_flit.src_id << ", received_flit.dst_id = " << received_flit.dst_id << ", end_time = " << sc_time_stamp().to_double() / GlobalParams::clock_period_ps << endl;
						}	
						else if(!tx_buffer[received_flit.src_id].IsEmpty()){	// laser on
							int src_mid = opticalPower.getMasterId(local_id);
							int dst_mid = opticalPower.getMasterId(received_flit.src_id);
							PathInfo tmpPathInfo = opticalPower.getPathInfo(src_mid, dst_mid);
							opticalPower.addNewLaserTime(tmpPathInfo.Wavelength_type, src_mid, dst_mid, (sc_time_stamp().to_double() / GlobalParams::clock_period_ps), -1);
							// addNewLaserTime(int w_type, int src_id, int dst_id, int start_time, int end_time);
							cout << "\nopticalPower.addNewLaserTime: local_id = "<<local_id<<", received_flit.src_id = " << received_flit.src_id << ", received_flit.dst_id = " << received_flit.dst_id << ", start_time = " << sc_time_stamp().to_double() / GlobalParams::clock_period_ps << endl;
						}
					}
					else {
						rx_buffer[received_flit.src_id].Push(received_flit);
						cout << "In Router_ONoC::rxProcess_centralRouter(), Push flit :" << received_flit << " into rx_buffer["<<received_flit.src_id<<"]" << endl;
						
						rx_buffer[received_flit.src_id].Print("rx_buffer");
						cout << "In Router_ONoC::rxProcess_centralRouter(), rx_buffer[received_flit.src_id].Size() = " << rx_buffer[received_flit.src_id].Size() << endl;

						power.bufferRouterPush();

						// laser power
						if (received_flit.flit_type == FLIT_TYPE_TAIL){
							int src_mid = opticalPower.getMasterId(received_flit.src_id);
							int dst_mid = opticalPower.getMasterId(received_flit.dst_id);
							PathInfo tmpPathInfo = opticalPower.getPathInfo(src_mid, dst_mid);
							opticalPower.addNewLaserTime(tmpPathInfo.Wavelength_type, src_mid, dst_mid, -1, (sc_time_stamp().to_double() / GlobalParams::clock_period_ps));
							// addNewLaserTime(int w_type, int src_id, int dst_id, int start_time, int end_time);
							cout << "In Router_ONoC::rxProcess_centralRouter(), addNewLaserTime, local_id = "<<local_id<<", received tail flit = " << received_flit << ", Wavelength_type = " << tmpPathInfo.Wavelength_type << endl;
						}
						if(flowControl_table[received_flit.src_id] == true && (rx_buffer[received_flit.src_id].getFlowControlOFF() >= rx_buffer[received_flit.src_id].GetMaxBufferSize() - rx_buffer[received_flit.src_id].Size())){
							Flit flit;
							double ccycle = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
							flit.src_id = local_id;
							flit.dst_id = received_flit.src_id;
							flit.timestamp = ccycle;
							flit.sequence_no = 0;
							flit.sequence_length = 1;
							flit.hop_no = 0;
							flit.src_task = NULL;
							flit.dst_task = NULL;
							flit.pblock_id = NULL;
							flit.flow_control = false;
							flit.flit_type = FLIT_TYPE_FLOWCONTROL;

							tx_buffer[received_flit.src_id].InsertFlowControl(flit);
							flowControl_table[received_flit.src_id] = false;
							power.bufferRouterPush();
							cout << "\nIn Router_ONoC::rxProcess_centralRouter(), local_id = "<<local_id<<" : insert flowControl=false, rx_buffer["<<received_flit.src_id<<"].Size() = " << rx_buffer[received_flit.src_id].Size() << ", rx_buffer["<<received_flit.src_id<<"].getFlowControlOFF() = " << rx_buffer[received_flit.src_id].getFlowControlOFF() << endl;
							cout << "Insert flowControl_flit(" << flit << ") to tx_buffer["<< received_flit.src_id << "], size = " << tx_buffer[received_flit.src_id].Size() << endl;
							tx_buffer[received_flit.src_id].Print("tx_buffer");

						}
						
						if(received_flit.flit_type == FLIT_TYPE_TAIL){
							TM_SIMOPTICALEND2ENDLATENCYLOG(34, static_cast<tm_task*>(received_flit.src_task)->getApp()->getId(), static_cast<tm_task*>(received_flit.src_task)->getId(), static_cast<tm_task*>(received_flit.dst_task)->getId(), "end receiving tail_flit at " << fixed << setprecision(2) << (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps));
						}
					}
				}
				else{
					string t = "\nIn Router_ONoC::rxProcess_centralRouter(), Error condition -> received_flit.dst_id != local_id.";
					TM_ASSERT(false, t);
					assert(false);

				}

				// Negate the old value for Alternating Bit Protocol (ABP)
				current_level_rx[i] = 1 - current_level_rx[i];

				// if a new flit is injected from local PE
				if (received_flit.src_id == local_id)
					power.networkInterface();
			}
			
		}
	}
}


void Router_ONoC::txProcess_centralRouter()	
{
  	if (reset.read()) 
    {
		// Clear outputs and indexes of transmitting protocol
		for (int i = 0; i < DIRECTIONS_ONOC; i++) {

			req_tx[i].write(0);
		}
		for (int i=1;i<GlobalParams::mesh_dim_x*GlobalParams::mesh_dim_y + 1;i++){
			current_level_tx[i] = 0;
		}
		for (int i=0;i<GlobalParams::mesh_dim_x*GlobalParams::mesh_dim_y;i++){
			next_flit_state[i] = 0;
		}
    } 
  	else if(GlobalParams::network_protocol == ALTERNETING_BIT_PROTOCOL)
	{	
      // 1st phase: Reservation

		int o;
		// buffer[i].deadlockCheck();
		for (int i = 1; i < GlobalParams::mesh_dim_x*GlobalParams::mesh_dim_y + 1; i++) {

			if (!tx_buffer[i-1].IsEmpty()) 
			{

				Flit flit = tx_buffer[i-1].Front();
				power.bufferRouterFront();

				
				
				if (flit.dst_id == local_id)
					o = DIRECTION_LOCAL_ONOC;
				else
					o = DIRECTION_CENTRAL_ROUTER;

				//! ----------------------------
				// Flit flit = buffer[j].Front();
				// cout << "In Router_ONoC::txProcess_centralRouter(), router local_id = " << local_id << ", current_level_tx["<<i<<"] = " << current_level_tx[i] << ", ack_tx["<<i<<"].read() = " << ack_tx[i].read().value << endl;

				if (current_level_tx[i] == ack_tx[i].read().value) //current_level_tx[o] == ack_tx[o].read()
				{
					cout << "\nIn txProcess_centralRouter(), router local_id = " << local_id << endl;
					//if (GlobalParams::verbose_mode > VERBOSE_OFF) 
					// LOG << "Input[" << i << "] forward to Output[" << o << "], flit: " << flit << endl;
					if(o == DIRECTION_LOCAL_ONOC){
						string t = "\nIn Router_ONoC::txProcess_centralRouter(), Error condition -> o == DIRECTION_LOCAL_ONOC.";
						TM_ASSERT(false, t);
						assert(false);
					}
					else
						cout << "Flit:"<< flit <<" forward to direction: " << "CENTRAL_ROUTER (Serializer)" << endl; 

					flit_tx[i].write(flit);
					
					// power.r2rLink();	//

					// power.crossBar();	//

					if(o == DIRECTION_LOCAL_ONOC){
						string t = "\nIn Router_ONoC::txProcess_centralRouter(), Error condition -> o == DIRECTION_LOCAL_ONOC.";
						TM_ASSERT(false, t);
						assert(false);
					}
					else {
						current_level_tx[i] = 1 - current_level_tx[i];
						req_tx[i].write(current_level_tx[i]);
						cout << "In Router_ONoC::txProcess_centralRouter(), to central_router req_tx = " << current_level_tx[i] << endl;
					}

					if(flit.flit_type == FLIT_TYPE_HEAD){
						TM_SIMOPTICALEND2ENDLATENCYLOG(34, static_cast<tm_task*>(flit.src_task)->getApp()->getId(), static_cast<tm_task*>(flit.src_task)->getId(), static_cast<tm_task*>(flit.dst_task)->getId(), "start transmitting head_flit at " << fixed << setprecision(2) << (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps));
					}
					
					tx_buffer[i-1].Pop();

					power.bufferRouterPop();
					

					// laser power
					if (flit.flit_type == FLIT_TYPE_HEAD){
						int src_mid = opticalPower.getMasterId(flit.src_id);
						int dst_mid = opticalPower.getMasterId(flit.dst_id);
						PathInfo tmpPathInfo = opticalPower.getPathInfo(src_mid, dst_mid);
						opticalPower.addNewLaserTime(tmpPathInfo.Wavelength_type, src_mid, dst_mid, (sc_time_stamp().to_double() / GlobalParams::clock_period_ps), -1);
						// addNewLaserTime(int w_type, int src_id, int dst_id, int start_time, int end_time);
						cout << "\nopticalPower.addNewLaserTime: local_id = "<<local_id<<", received_flit.src_id = " << flit.src_id << ", received_flit.dst_id = " << flit.dst_id << ", start_time = " << sc_time_stamp().to_double() / GlobalParams::clock_period_ps << endl;
					}

					if (i != DIRECTION_LOCAL_ONOC) 
					{
						// Increment routed flits counter
						routed_flits++;
					}
				}
			}
		}
    }
	else if(GlobalParams::network_protocol == ON_OFF_FLOWCONTROL)
	{
		int o;
		for (int i = 1; i < GlobalParams::mesh_dim_x*GlobalParams::mesh_dim_y + 1; i++){

			if (!tx_buffer[i-1].IsEmpty() && next_flit_state[i-1] == 0) 
			{
				Flit flit = tx_buffer[i-1].Front();
				power.bufferRouterFront();

				// cout << "\nIn txProcess_centralRouter(), router local_id = " << local_id << ", flowControl_signal["<<i-1<<"] = " << flowControl_signal[i-1] << ", flit = " << flit << endl;

				if(flowControl_signal[i-1] || flit.flit_type == FLIT_TYPE_FLOWCONTROL){
					next_flit_state[i-1] = 1;

					if (flit.dst_id == local_id){
						o = DIRECTION_LOCAL_ONOC;
						string t = "In Router_ONoC::txProcess_centralRouter(), Error condition->route_data.dst_id == local_id.";
						TM_ASSERT(false, t);
						assert(false);
					}
					else
						o = DIRECTION_CENTRAL_ROUTER;

					//! ----------------------------
					// Flit flit = buffer[j].Front();
					// cout << "\nIn txProcess_centralRouter(), router local_id = " << local_id << endl;
					
					cout << "Flit:"<< flit <<" forward to direction: " << "CENTRAL_ROUTER (Serializer)" << endl; 

					flit_tx[i].write(flit);
					tx_buffer[i-1].Pop();
					power.bufferRouterPop();
					
					cout << "In txProcess_centralRouter(), to central_router(Serializer),before current_level_tx[i] = 1 - current_level_tx[i]; req_tx and current_level_tx["<< i <<"] = " << current_level_tx[i] << ", o = " << o << endl;
					current_level_tx[i] = 1 - current_level_tx[i];
					req_tx[i].write(current_level_tx[i]);
					cout << "In txProcess_centralRouter(), to central_router(Serializer) req_tx and current_level_tx["<< i <<"] = " << current_level_tx[i] << endl;
					if(flit.flit_type == FLIT_TYPE_HEAD){
						cout << "In txProcess_centralRouter(), flit.flit_type == FLIT_TYPE_HEAD, flit = " << flit << endl;
						TM_SIMOPTICALEND2ENDLATENCYLOG(34, static_cast<tm_task*>(flit.src_task)->getApp()->getId(), static_cast<tm_task*>(flit.src_task)->getId(), static_cast<tm_task*>(flit.dst_task)->getId(), "start transmitting head_flit at " << fixed << setprecision(2) << (sc_time_stamp().to_double() / GlobalParams::clock_period_optical_ps));
					}
					else if(flit.flit_type == FLIT_TYPE_FLOWCONTROL){
						if(flit.flow_control == true){
							TM_SIMFLOWCONTROLONLOG(35, local_id, flit.dst_id, flit);
						}
						else {
							TM_SIMFLOWCONTROLOFFLOG(35, local_id, flit.dst_id, flit);
						}
					}

					// laser power
					if (flit.flit_type == FLIT_TYPE_HEAD || flit.flit_type == FLIT_TYPE_FLOWCONTROL){
						int src_mid = opticalPower.getMasterId(flit.src_id);
						int dst_mid = opticalPower.getMasterId(flit.dst_id);
						PathInfo tmpPathInfo = opticalPower.getPathInfo(src_mid, dst_mid);
						opticalPower.addNewLaserTime(tmpPathInfo.Wavelength_type, src_mid, dst_mid, (sc_time_stamp().to_double() / GlobalParams::clock_period_ps), -1);
						// addNewLaserTime(int w_type, int src_id, int dst_id, int start_time, int end_time);
						cout << "\nopticalPower.addNewLaserTime: local_id = "<<local_id<<", received_flit.src_id = " << flit.src_id << ", received_flit.dst_id = " << flit.dst_id << ", start_time = " << sc_time_stamp().to_double() / GlobalParams::clock_period_ps << endl;
					}
				}
				
			}
			else if(next_flit_state[i-1] == 1 && ~read_enable[i-1].read()){
				next_flit_state[i-1] = 2;
			}
			else if(next_flit_state[i-1] == 2 && read_enable[i-1].read()){
				next_flit_state[i-1] = 0;
			}
			
		}
	}
}


void Router_ONoC::txProcess_PE()	//reservation output port to local pe
{
  	if (reset.read()) 
    {
		// Clear outputs and indexes of transmitting protocol
		for (int i = 0; i < GlobalParams::mesh_dim_x*GlobalParams::mesh_dim_y + 1; i++) {

			req_tx[i].write(0);
		}
		for (int i=0;i<GlobalParams::mesh_dim_x*GlobalParams::mesh_dim_y + 1;i++){
			current_level_tx[i] = 0;
		}
		start_from_port = 0;
    } 
  	else 
	{	
      // 1st phase: Reservation

		int o;

		// buffer[i].deadlockCheck();
		for (int offset = 0; offset < GlobalParams::mesh_dim_x*GlobalParams::mesh_dim_y + 1; offset++) {
			// cout << "In Router_ONoC::txProcess_PE(), print buffer: " << endl;
			// cout << "i = " << i ;
			// rx_buffer[i].Print("rx_buffer");
			int i = (start_from_port + offset) % (GlobalParams::mesh_dim_x*GlobalParams::mesh_dim_y + 1); 
			if (i == 0) i = 1;

			if (!rx_buffer[i-1].IsEmpty()) 
			{
				Flit flit = rx_buffer[i-1].Front();
				power.bufferRouterFront();


				cout << "flit.dst_id = " << flit.dst_id << ", local_id = " << local_id << endl;
				if (flit.dst_id == local_id)
					o = DIRECTION_LOCAL_ONOC;
				else
					o = DIRECTION_CENTRAL_ROUTER;

				//! ----------------------------
				// Flit flit = buffer[j].Front();
				//cout << " o = "<< o << endl;

				//cout << "In Router_ONoC::txProcess_PE(), router local_id = " << local_id << ", current_level_tx[o] = "<<current_level_tx[o]<<", ack_tx_local = " << ack_tx_local << ", flit = " << flit << endl;
				//cout << "current_level_tx[o] = " << current_level_tx[o] << ", ack_tx[o].read() = " << ack_tx_local.read() << endl;

				if (current_level_tx[o] == ack_tx_local.read()) //current_level_tx[o] == ack_tx[o].read()
				{
					//if (GlobalParams::verbose_mode > VERBOSE_OFF) 
					// LOG << "Input[" << i << "] forward to Output[" << o << "], flit: " << flit << endl;
					cout << "in"<< endl;
					if(o == DIRECTION_LOCAL_ONOC)
						cout << "Flit:"<< flit <<" forward to direction: " << "LOCAL" << endl; 
					else{
						cout << "Flit:"<< flit <<" forward to direction: " << "CENTRAL_ROUTER" << endl; 
						cout << "rx_buffer["<<i-1<<"].size() = " << rx_buffer[i-1].Size() << endl;
						rx_buffer[i-1].Print("rx_buffer");
						string t = "\nIn Router_ONoC::txProcess_PE(), Error condition -> o != DIRECTION_LOCAL_ONOC.";
						TM_ASSERT(false, t);
						assert(false);
					}

					flit_tx[o].write(flit);
					// req_tx[o].write(1);
					
					// power.r2rLink();	//

					// power.crossBar();	//

					if(o == DIRECTION_LOCAL_ONOC){
						current_level_tx[o] = 1 - current_level_tx[o];
						req_tx[o].write(current_level_tx[o]);
					}
					// else {
					// 	current_level_tx[o+flit.dst_id] = 1 - current_level_tx[o+flit.dst_id];
					// 	req_tx[o].write(current_level_tx[o+flit.dst_id]);
					// 	cout << "In Router_ONoC::txProcess_PE(), to central_router req_tx = " << current_level_tx[o+flit.dst_id] << endl;
					// }
					

					rx_buffer[i-1].Pop();

					power.bufferRouterPop();
					
					cout << "In Router_ONoC::txProcess_PE(), local_id =  " << local_id << ", flowControl_table["<<i-1<<"] = "<<flowControl_table[i-1]<<", after pop rx_buffer["<<i-1<<"]:" << endl;
					
					rx_buffer[i-1].Print("rx_buffer");
					

					// cout << "In Router_ONoC::txProcess_PE(), router local_id = " << local_id << endl;
					// cout << "req_tx[o].write: " << current_level_tx[o] << endl << endl;

					// if flit has been consumed
					if (flit.dst_id == local_id)
						power.networkInterface();

					// if (flit.flit_type == FLIT_TYPE_TAIL){
					// // 	reservation_table.release(o);
					// 	current_level_tx[o] = 0;
					// }

							// Update stats
					if (o == DIRECTION_LOCAL_ONOC) 
					{
						LOG << "Consumed flit " << flit << endl;
						stats.receivedFlit(sc_time_stamp().to_double() / GlobalParams::clock_period_ps, flit);
						if (GlobalParams::max_volume_to_be_drained) 
						{
							if (drained_volume >= GlobalParams:: max_volume_to_be_drained)
								sc_stop();
							else 
							{
								drained_volume++;
								local_drained++;
							}
						}
					} 

					if(GlobalParams::network_protocol == ON_OFF_FLOWCONTROL && flowControl_table[i-1] == false && (rx_buffer[i-1].GetMaxBufferSize() - rx_buffer[i-1].Size() >= rx_buffer[i-1].getFlowControlON())){
						Flit flit;
						double ccycle = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
						flit.src_id = local_id;
						flit.dst_id = (i-1);
						flit.timestamp = ccycle;
						flit.sequence_no = 0;
						flit.sequence_length = 1;
						flit.hop_no = 0;
						flit.src_task = NULL;
						flit.dst_task = NULL;
						flit.pblock_id = NULL;
						flit.flow_control = true;
						flit.flit_type = FLIT_TYPE_FLOWCONTROL;
	
						tx_buffer[i-1].InsertFlowControl(flit);
						flowControl_table[i-1] = true;
	
						power.bufferRouterPush();
	
						cout << "\nIn Router_ONoC::txProcess_PE(), local_id = " << local_id << " : insert flowControl=true, tx_buffer["<<i-1<<"].Size() = " << tx_buffer[i-1].Size() << endl;
						cout << "Insert flowControl_flit(" << flit << ") to tx_buffer["<< i-1 << "]" << endl;
						cout << "rx_buffer[i-1].GetMaxBufferSize() = " << rx_buffer[i-1].GetMaxBufferSize() << ", rx_buffer[i-1].Size() = " << rx_buffer[i-1].Size() << ", rx_buffer[i-1].getFlowControlON() = " << rx_buffer[i-1].getFlowControlON() << endl;
						cout << "rx_buffer["<<i-1<<"] ->";
						rx_buffer[i-1].Print("rx_buffer");
						cout << "tx_buffer["<<i-1<<"] ->";
						tx_buffer[i-1].Print("tx_buffer");
					}

					start_from_port = (i  + 1 ) % (GlobalParams::mesh_dim_x*GlobalParams::mesh_dim_y+1);
					//cout << "In Router_ONoC::txProcess_PE(), local_id =  " << local_id << ", i = " << i << ", start_from_port = " << start_from_port << ", sending flit: " << flit <<"  req_tx[o].read() ="<<req_tx[o].read() << endl;
					break;
				}
				

				

				
			}
		}
    }				// else reset read
}

void Router_ONoC::perCycleUpdate()
{
    // if (reset.read()) {
	// for (int i = 0; i < DIRECTIONS + 1; i++)
	//     free_slots[i].write(buffer[i].GetMaxBufferSize());
    // } else {
    //     selectionStrategy->perCycleUpdate(this);
	if(!reset.read()){
		power.leakageRouter();
		for (int i = 0; i < GlobalParams::mesh_dim_x*GlobalParams::mesh_dim_y*2; i++)
		{
			power.leakageBufferRouter();
			// power.leakageLinkRouter2Router();
		}
	}
// 	power.leakageLinkRouter2Hub();
//     }
}

void Router_ONoC::configure(const int _id,
			    const double _warm_up_time,
			    const unsigned int _max_buffer_size,
			    GlobalRoutingTable & grt)
{
    local_id = _id;
    stats.configure(_id, _warm_up_time);

    start_from_port = 0;

    if (grt.isValid())
	routing_table.configure(grt, _id);

	for (int i = 0; i < GlobalParams::mesh_dim_x*GlobalParams::mesh_dim_y; i++) {

		rx_buffer[i].SetMaxBufferSize(_max_buffer_size);
		tx_buffer[i].SetMaxBufferSize(_max_buffer_size);

		int row = _id / GlobalParams::mesh_dim_x;
		int col = _id % GlobalParams::mesh_dim_x;
		// if (row == 0)
		// buffer[i].Disable();
		// if (row == GlobalParams::mesh_dim_y-1)
		// buffer[i].Disable();
		// if (col == 0)
		// buffer[i].Disable();
		// if (col == GlobalParams::mesh_dim_x-1)
		// buffer[i].Disable();
	}

}

unsigned long Router_ONoC::getRoutedFlits()
{
    return routed_flits;
}

unsigned int Router_ONoC::getFlitsCount()
{
    unsigned count = 0;

	for (int i = 0; i < GlobalParams::mesh_dim_x*GlobalParams::mesh_dim_y+1; i++) {
		count += rx_buffer[i].Size();
		count += tx_buffer[i].Size();
	}
    return count;
}


int Router_ONoC::reflexDirection(int direction) const
{
    if (direction == DIRECTION_NORTH)
	return DIRECTION_SOUTH;
    if (direction == DIRECTION_EAST)
	return DIRECTION_WEST;
    if (direction == DIRECTION_WEST)
	return DIRECTION_EAST;
    if (direction == DIRECTION_SOUTH)
	return DIRECTION_NORTH;

    // you shouldn't be here
    assert(false);
    return NOT_VALID;
}

int Router_ONoC::getNeighborId(int _id, int direction) const
{
    Coord my_coord = id2Coord(_id);

    switch (direction) {
    case DIRECTION_NORTH:
	if (my_coord.y == 0)
	    return NOT_VALID;
	my_coord.y--;
	break;
    case DIRECTION_SOUTH:
	if (my_coord.y == GlobalParams::mesh_dim_y - 1)
	    return NOT_VALID;
	my_coord.y++;
	break;
    case DIRECTION_EAST:
	if (my_coord.x == GlobalParams::mesh_dim_x - 1)
	    return NOT_VALID;
	my_coord.x++;
	break;
    case DIRECTION_WEST:
	if (my_coord.x == 0)
	    return NOT_VALID;
	my_coord.x--;
	break;
    default:
	LOG << "Direction not valid : " << direction;
	assert(false);
    }

    int neighbor_id = coord2Id(my_coord);

    return neighbor_id;
}


void Router_ONoC::ShowBuffersStats(std::ostream & out)
{
	for (int i = 0; i < GlobalParams::mesh_dim_x*GlobalParams::mesh_dim_y; i++) {
	    rx_buffer[i].ShowStats(out);
	    tx_buffer[i].ShowStats(out);
	}
}
