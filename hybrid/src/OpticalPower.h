#ifndef __OPTICALPOWER_H__
#define __OPTICALPOWER_H__

#include <systemc.h>

#include <iomanip>
#include <iostream>
#include <set>

#include "GlobalParams.h"

using namespace std;

struct PathInfo {
  int MRR_type;
  int Wavelength_type;
};
struct laser_time {
  int start_time;
  int end_time;

  laser_time(int start = -1, int end = -1) : start_time(start), end_time(end) {}
};

class OpticalPower {

 public:
  OpticalPower() {};
  void parsePathData();
  PathInfo getPathInfo(int src_id, int dst_id);
  void parsePathLossFile();

  void parseMasterSlaveData();
  int getMasterId(int core_id);

  void addNewLaserTime(int w_type, int src_id, int dst_id, int start_time,
                       int end_time);
  void printLaserTimeUnsorted(std::ostream &out);

  void calculateWavelengthIntervals();
  void printWavelengthIntervals(std::ostream &out);

  void calculateWavelengthUsageTime();
  void calculateLaserIntervals();

  void addNewTxTime(int chiplet_id, int w_type, int start_time, int end_time);
  void addNewRxTime(int chiplet_id, int w_type, int start_time, int end_time);
  void calculateOpticalTxIntervals();
  void calculateOpticalRxIntervals();

  void printOpticalTxTime(std::ostream &out);
  void printOpticalRxTime(std::ostream &out);
  void printIntervals(std::ostream &out);

  // Energy
  double getLaserEnergy();
  double getLaserLossEnergy();
  double getTxEnergy();
  double getRxEnergy();
  // double getArbitrationFlowControlEnergy();
  double getEOEEnergy();
  double getMRREnergy();

 private:
  static map<pair<int, int>, PathInfo> pathData;
  static map<int, pair<int, int>> MasterSlave_id;
  static map<int, double> path_loss;

  static multimap<int, tuple<int, int, vector<laser_time>>>
      laser_time_unsorted; // <w_type, tuple<src_id, dst_id, vector<start_time,
                           // end_time>>>
  static map<int, vector<laser_time>>
      wavelength_intervals;                   // w_type ,laser on/off time
  static map<int, int> wavelength_usage_time; // <w_type, usage_time>
  static vector<tuple<int, int, int, set<int>>>
      laser_intervals; // <start_time, end_time, active_W_count,
                       // set<active_W_type>>

  static multimap<int, tuple<int, vector<laser_time>>>
      optical_tx_time_unsorted; // <chiplet_id, tuple<w_type, vector<start_time,
                                // end_time>>>
  static multimap<int, tuple<int, vector<laser_time>>>
      optical_Rx_time_unsorted; // <chiplet_id, tuple<w_type, vector<start_time,
                                // end_time>>>
  static map<int, vector<tuple<int, int, int, set<int>>>>
      optical_tx_intervals; // <start_time, end_time, active_W_count,
                            // set<active_W_type>>
  static map<int, vector<tuple<int, int, int, set<int>>>>
      optical_Rx_intervals; // <start_time, end_time, active_W_count,
                            // set<active_W_type>>
};

class RouterOpticalPower : public OpticalPower {

 public:
  RouterOpticalPower() {};

 private:
};
class ChipletOpticalPower : public OpticalPower {

 public:
  ChipletOpticalPower() {};

 private:
};

#endif