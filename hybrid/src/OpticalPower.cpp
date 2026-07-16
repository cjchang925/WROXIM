#include "OpticalPower.h"

using namespace std;

std::multimap<int, tuple<int, int, vector<laser_time>>>
    OpticalPower::laser_time_unsorted;
std::map<int, std::vector<laser_time>> OpticalPower::wavelength_intervals;
std::map<std::pair<int, int>, PathInfo> OpticalPower::pathData;
std::map<int, int> OpticalPower::wavelength_usage_time;
std::vector<tuple<int, int, int, set<int>>> OpticalPower::laser_intervals;
std::map<int, std::pair<int, int>> OpticalPower::MasterSlave_id;
std::multimap<int, tuple<int, vector<laser_time>>>
    OpticalPower::optical_tx_time_unsorted;
std::multimap<int, tuple<int, vector<laser_time>>>
    OpticalPower::optical_Rx_time_unsorted;
std::map<int, vector<tuple<int, int, int, set<int>>>>
    OpticalPower::optical_tx_intervals;
std::map<int, vector<tuple<int, int, int, set<int>>>>
    OpticalPower::optical_Rx_intervals;
std::map<int, double> OpticalPower::path_loss;

#define mW2J(mwatt) ((mwatt * 1.0e-3) * GlobalParams::clock_period_ps * 1.0e-12)
#define mW2J_clk2(mwatt) \
  ((mwatt * 1.0e-3) * GlobalParams::clock_period_optical_ps * 1.0e-12)

void OpticalPower::parsePathData() {
  string file_path =
      "./path_input/" + GlobalParams::architecture +
      to_string(GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y) + ".txt";
  ifstream file(file_path);
  if (!file) {
    cout << "Can't open path_data file: " << file_path << endl;
    assert(false);
  } else {
    string line;
    while (getline(file, line)) {
      istringstream iss(line);
      int src_id, dst_id;
      PathInfo pathInfo = {-1, -1};
      string token;
      cout << "line: " << line << endl;

      if (!(iss >> src_id >> dst_id)) {
        cout << "Parsing src_id and dst_id failed! in line: " << line << endl;
        continue;
      }

      while (iss >> token) {
        // cout << "token: " << token << endl;
        if (token == "D:") {
          if (iss >> token) {
            pathInfo.MRR_type = std::stoi(token);
          } else {
            std::cerr << "Error: Missing value after D:" << std::endl;
            assert(false);
          }
        } else if (token == "W:") {
          if (iss >> token) {
            pathInfo.Wavelength_type = std::stoi(token);
          } else {
            std::cerr << "Error: Missing value after W:" << std::endl;
            assert(false);
          }
        }
      }

      // if (pathInfo.MRR_type == -1 || pathInfo.Wavelength_type == -1) {
      //     cerr << "Parsing D: or W: failed for line: " << line << endl;
      //     continue;
      // }

      pathData[{src_id, dst_id}] = pathInfo;
    }
    file.close();

    // cout << "Parsing path_input result:\n";
    // for (const auto& entry : pathData) {
    //     cout << "<" << entry.first.first << ", " << entry.first.second
    //               << "> -> D: " << entry.second.MRR_type
    //               << ", W: " << entry.second.Wavelength_type << "\n";
    // }
  }
}
void OpticalPower::parseMasterSlaveData() {
  string file_path =
      "./MasterSlave/" + GlobalParams::architecture +
      to_string(GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y) + ".txt";
  ifstream file(file_path);

  if (!file) {
    cerr << "Can't open file: " << file_path << endl;
    assert(false);
  }

  string line;
  while (getline(file, line)) {
    istringstream iss(line);
    int key, master, slave;
    string master_label, slave_label;

    if (!(iss >> key >> master_label >> master >> slave_label >> slave)) {
      cerr << "Parsing failed for line: " << line << endl;
      continue;
    }

    if (master_label != "master:" || slave_label != "slave:") {
      cerr << "Unexpected labels in line: " << line << endl;
      assert(false);
    }

    MasterSlave_id[key] = {master, slave};
  }

  file.close();

  // cout << "Parsed MasterSlave_id data:\n";
  // for (const auto& entry : MasterSlave_id) {
  //     cout << "Key: " << entry.first << ", Master: " << entry.second.first
  //                 << ", Slave: " << entry.second.second << "\n";
  // }
}
void OpticalPower::parsePathLossFile() {
  string filename =
      "./power_input/" + GlobalParams::architecture +
      to_string(GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y) + ".txt";
  std::ifstream infile(filename);
  if (!infile) {
    std::cerr << "Error: Unable to open file " << filename << std::endl;
    return;
  }

  std::string line;
  while (std::getline(infile, line)) {
    std::istringstream iss(line);
    int master_id, slave_id;
    double loss_value;

    // Parse the line
    if (!(iss >> master_id >> slave_id >> loss_value)) {
      std::cout << "Invalid line format: " << line << std::endl;
      continue;
    }

    // Look up the Wavelength_type in pathData
    auto path_key = std::make_pair(master_id, slave_id);
    if (pathData.find(path_key) != pathData.end()) {
      int wavelength_type = pathData[path_key].Wavelength_type;

      // Update the maximum loss value for the Wavelength_type
      if (path_loss.find(wavelength_type) == path_loss.end() ||
          path_loss[wavelength_type] < loss_value) {
        path_loss[wavelength_type] = loss_value;
      }
    }
  }

  infile.close();
}

PathInfo OpticalPower::getPathInfo(int src_id, int dst_id) {
  return pathData[{src_id, dst_id}];
}
int OpticalPower::getMasterId(int core_id) {
  return MasterSlave_id[core_id].first;
}

void OpticalPower::addNewTxTime(int chiplet_id, int w_type, int start_time,
                                int end_time) {
  if (end_time == -1) { // add new start_time
    laser_time new_laser_time{start_time, end_time};
    auto range = optical_tx_time_unsorted.equal_range(chiplet_id);
    for (auto it = range.first; it != range.second; ++it) {
      int tmp_w_type = get<0>(it->second);
      vector<laser_time> &times = get<1>(it->second);
      if (w_type == tmp_w_type) {
        times.push_back(new_laser_time);
        return;
      }
    }
    optical_tx_time_unsorted.emplace(
        chiplet_id, make_tuple(w_type, vector<laser_time>{new_laser_time}));
    return;
  } else if (start_time == -1) { // add new end_time
    auto range = optical_tx_time_unsorted.equal_range(chiplet_id);
    for (auto it = range.first; it != range.second; ++it) {
      int tmp_w_type = get<0>(it->second);
      vector<laser_time> &times = get<1>(it->second);
      if (tmp_w_type == w_type) {
        for (auto &time : times) {
          if (time.end_time == -1) {
            time.end_time = end_time;
            return;
          }
        }
      }
    }
  } else {
    cout << endl
         << "******************" << endl
         << "ABNORMAL SITUATION : (cycle="
         << sc_time_stamp().to_double() / GlobalParams::clock_period_ps
         << ") , in OpticalPower::addNewLaserTime()." << endl
         << "******************" << endl;
    assert(false);
  }
}
void OpticalPower::addNewRxTime(int chiplet_id, int w_type, int start_time,
                                int end_time) {
  if (end_time == -1) { // add new start_time
    laser_time new_laser_time{start_time, end_time};
    auto range = optical_Rx_time_unsorted.equal_range(chiplet_id);
    for (auto it = range.first; it != range.second; ++it) {
      int tmp_w_type = get<0>(it->second);
      vector<laser_time> &times = get<1>(it->second);
      if (w_type == tmp_w_type) {
        times.push_back(new_laser_time);
        return;
      }
    }
    optical_Rx_time_unsorted.emplace(
        chiplet_id, make_tuple(w_type, vector<laser_time>{new_laser_time}));
    return;
  } else if (start_time == -1) { // add new end_time
    auto range = optical_Rx_time_unsorted.equal_range(chiplet_id);
    for (auto it = range.first; it != range.second; ++it) {
      int tmp_w_type = get<0>(it->second);
      vector<laser_time> &times = get<1>(it->second);
      if (tmp_w_type == w_type) {
        for (auto &time : times) {
          if (time.end_time == -1) {
            time.end_time = end_time;
            return;
          }
        }
      }
    }
  } else {
    cout << endl
         << "******************" << endl
         << "ABNORMAL SITUATION : (cycle="
         << sc_time_stamp().to_double() / GlobalParams::clock_period_ps
         << ") , in OpticalPower::addNewLaserTime()." << endl
         << "******************" << endl;
    assert(false);
  }
}

void OpticalPower::addNewLaserTime(int w_type, int src_id, int dst_id,
                                   int start_time, int end_time) {
  if (end_time == -1) { // add new start_time
    laser_time new_laser_time{start_time, end_time};
    auto range = laser_time_unsorted.equal_range(w_type);
    for (auto it = range.first; it != range.second; ++it) {
      int s_id = get<0>(it->second);
      int d_id = get<1>(it->second);
      vector<laser_time> &times = get<2>(it->second);
      if (s_id == src_id && d_id == dst_id) {
        times.push_back(new_laser_time);
        return;
      }
    }
    laser_time_unsorted.emplace(
        w_type, make_tuple(src_id, dst_id, vector<laser_time>{new_laser_time}));
    return;
  } else if (start_time == -1) { // add new end_time
    auto range = laser_time_unsorted.equal_range(w_type);
    for (auto it = range.first; it != range.second; ++it) {
      int s_id = get<0>(it->second);
      int d_id = get<1>(it->second);
      vector<laser_time> &times = get<2>(it->second);
      if (s_id == src_id && d_id == dst_id) {
        for (auto &time : times) {
          if (time.end_time == -1) {
            time.end_time = end_time;
            return;
          }
        }
      }
    }
  } else {
    cout << endl
         << "******************" << endl
         << "ABNORMAL SITUATION : (cycle="
         << sc_time_stamp().to_double() / GlobalParams::clock_period_ps
         << ") , in OpticalPower::addNewLaserTime()." << endl
         << "******************" << endl;
    assert(false);
    // laser_time new_laser_time{start_time, end_time};
    // auto range = laser_time_unsorted.equal_range(w_type);
    // for (auto it = range.first; it != range.second; ++it) {
    //     int s_id = get<0>(it->second);
    //     int d_id = get<1>(it->second);
    //     vector<laser_time>& times = get<2>(it->second);
    //     if (s_id == src_id && d_id == dst_id) {
    //         times.push_back(new_laser_time);
    //         return;
    //     }
    // }
    // laser_time_unsorted.emplace(w_type, make_tuple(src_id, dst_id,
    // vector<laser_time>{new_laser_time}));
  }
}
void OpticalPower::calculateWavelengthIntervals() {
  wavelength_intervals.clear();

  std::map<int, std::vector<laser_time>> combined_intervals;

  for (const auto &entry : laser_time_unsorted) {
    int w_type = entry.first;
    const auto &[src_id, dst_id, times] = entry.second;

    combined_intervals[w_type].insert(combined_intervals[w_type].end(),
                                      times.begin(), times.end());
  }

  for (auto &[w_type, laser_intervals] : combined_intervals) {
    std::sort(laser_intervals.begin(), laser_intervals.end(),
              [](const laser_time &a, const laser_time &b) {
                return a.start_time < b.start_time;
              });

    std::vector<laser_time> merged_intervals;
    for (auto &interval : laser_intervals) {
      if (interval.end_time == -1) {
        interval.end_time =
            GlobalParams::simulation_time + GlobalParams::reset_time;
      }
      if (merged_intervals.empty() ||
          merged_intervals.back().end_time < interval.start_time) {
        merged_intervals.push_back(interval);
      } else {
        merged_intervals.back().end_time =
            std::max(merged_intervals.back().end_time, interval.end_time);
      }
    }

    wavelength_intervals[w_type] = merged_intervals;
  }
}
void OpticalPower::calculateWavelengthUsageTime() {
  for (const auto &[wavelength, laser_intervals] : wavelength_intervals) {
    int total_time = 0;

    for (const auto &interval : laser_intervals) {
      int usage_time = interval.end_time - interval.start_time;
      if (usage_time < 0) {
        std::cerr << "Error: Invalid interval for wavelength " << wavelength
                  << " with start_time = " << interval.start_time
                  << " and end_time = " << interval.end_time << "\n";
        throw std::runtime_error("Invalid interval with negative usage time.");
      }
      total_time += usage_time;
    }
    wavelength_usage_time[wavelength] = total_time;
    cout << "Wavelength " << wavelength
         << " -> total usage time: " << total_time << endl;
  }
}

void OpticalPower::calculateLaserIntervals() {

  vector<tuple<int, int>> events;
  for (const auto &[w_type, laser_intervals] : wavelength_intervals) {
    for (const auto &interval : laser_intervals) {
      events.emplace_back(interval.start_time, w_type);
      events.emplace_back(interval.end_time, -w_type);
    }
  }

  sort(events.begin(), events.end(),
       [](const tuple<int, int> &a, const tuple<int, int> &b) {
         if (get<0>(a) != get<0>(b)) {
           return get<0>(a) < get<0>(b);
         }
         return get<1>(a) > get<1>(b);
       });

  set<int> active_wavelengths;
  int prev_time = -1;

  for (const auto &[time, change] : events) {
    if (prev_time != -1 && !active_wavelengths.empty()) {

      laser_intervals.emplace_back(prev_time, time, active_wavelengths.size(),
                                   active_wavelengths);
    }

    if (change > 0) {
      active_wavelengths.insert(change);
    } else {
      active_wavelengths.erase(-change);
    }

    prev_time = time;
  }

  cout << "\n\n";
  for (const auto &[start, end, count, wavelengths] : laser_intervals) {
    cout << "Interval: [" << start << ", " << end << "], Active Wavelengths: {";
    for (auto it = wavelengths.begin(); it != wavelengths.end(); ++it) {
      cout << *it;
      if (next(it) != wavelengths.end()) {
        cout << ", ";
      }
    }
    cout << "}, Count: " << count << ", Duration: " << (end - start + 1)
         << "\n";
  }
}

void OpticalPower::calculateOpticalTxIntervals() {

  optical_tx_intervals.clear();

  map<int, map<int, vector<laser_time>>> combined_intervals;

  for (const auto &entry : optical_tx_time_unsorted) {
    int chiplet_id = entry.first;
    const auto &data = entry.second;
    int w_type = get<0>(data);
    const auto &times = get<1>(data);

    combined_intervals[chiplet_id][w_type].insert(
        combined_intervals[chiplet_id][w_type].end(), times.begin(),
        times.end());
  }

  for (const auto &chiplet_entry : combined_intervals) {
    int chiplet_id = chiplet_entry.first;
    const auto &w_type_map = chiplet_entry.second;

    vector<tuple<int, int, int, set<int>>> chiplet_intervals;

    vector<tuple<int, int>> events;
    for (const auto &w_type_entry : w_type_map) {
      int w_type = w_type_entry.first;
      const auto &intervals = w_type_entry.second;

      vector<laser_time> merged_intervals = intervals;
      sort(merged_intervals.begin(), merged_intervals.end(),
           [](const laser_time &a, const laser_time &b) {
             return a.start_time < b.start_time;
           });

      vector<laser_time> final_intervals;
      for (auto &interval : merged_intervals) {
        if (interval.end_time == -1) {
          interval.end_time =
              GlobalParams::simulation_time + GlobalParams::reset_time;
        }
        if (final_intervals.empty() ||
            final_intervals.back().end_time < interval.start_time) {
          final_intervals.push_back(interval);
        } else {
          final_intervals.back().end_time =
              max(final_intervals.back().end_time, interval.end_time);
        }
      }

      for (const auto &interval : final_intervals) {
        events.emplace_back(interval.start_time, w_type);
        events.emplace_back(interval.end_time, -w_type);
      }
    }

    sort(events.begin(), events.end(),
         [](const tuple<int, int> &a, const tuple<int, int> &b) {
           if (get<0>(a) != get<0>(b)) {
             return get<0>(a) < get<0>(b);
           }
           return get<1>(a) > get<1>(b);
         });

    set<int> active_wavelengths;
    int prev_time = -1;

    for (const auto &event : events) {
      int time = get<0>(event);
      int change = get<1>(event);

      if (prev_time != -1 && !active_wavelengths.empty()) {

        chiplet_intervals.emplace_back(
            prev_time, time, active_wavelengths.size(), active_wavelengths);
      }

      if (change > 0) {
        active_wavelengths.insert(change);
      } else {
        active_wavelengths.erase(-change);
      }

      prev_time = time;
    }

    optical_tx_intervals[chiplet_id] = chiplet_intervals;
  }

  cout << "\nOptical TX Intervals by Chiplet ID:\n";
  for (const auto &interval_entry : optical_tx_intervals) {
    int chiplet_id = interval_entry.first;
    const auto &intervals = interval_entry.second;

    cout << "Chiplet ID: " << chiplet_id << "\n";
    for (const auto &interval : intervals) {
      int start = get<0>(interval);
      int end = get<1>(interval);
      int count = get<2>(interval);
      const set<int> &wavelengths = get<3>(interval);

      cout << "  Interval: [" << start << ", " << end
           << "], Active Wavelengths: {";
      for (auto it = wavelengths.begin(); it != wavelengths.end(); ++it) {
        cout << *it;
        if (next(it) != wavelengths.end()) {
          cout << ", ";
        }
      }
      cout << "}, Count: " << count << ", Duration: " << (end - start + 1)
           << "\n";
    }
  }
}
void OpticalPower::calculateOpticalRxIntervals() {

  optical_Rx_intervals.clear();

  map<int, map<int, vector<laser_time>>> combined_intervals;

  for (const auto &entry : optical_Rx_time_unsorted) {
    int chiplet_id = entry.first;
    const auto &data = entry.second;
    int w_type = get<0>(data);
    const auto &times = get<1>(data);

    combined_intervals[chiplet_id][w_type].insert(
        combined_intervals[chiplet_id][w_type].end(), times.begin(),
        times.end());
  }

  for (const auto &chiplet_entry : combined_intervals) {
    int chiplet_id = chiplet_entry.first;
    const auto &w_type_map = chiplet_entry.second;

    vector<tuple<int, int, int, set<int>>> chiplet_intervals;

    vector<tuple<int, int>> events;
    for (const auto &w_type_entry : w_type_map) {
      int w_type = w_type_entry.first;
      const auto &intervals = w_type_entry.second;

      vector<laser_time> merged_intervals = intervals;
      sort(merged_intervals.begin(), merged_intervals.end(),
           [](const laser_time &a, const laser_time &b) {
             return a.start_time < b.start_time;
           });

      vector<laser_time> final_intervals;
      for (auto &interval : merged_intervals) {
        if (interval.end_time == -1) {
          interval.end_time =
              GlobalParams::simulation_time + GlobalParams::reset_time;
        }
        if (final_intervals.empty() ||
            final_intervals.back().end_time < interval.start_time) {
          final_intervals.push_back(interval);
        } else {
          final_intervals.back().end_time =
              max(final_intervals.back().end_time, interval.end_time);
        }
      }

      for (const auto &interval : final_intervals) {
        events.emplace_back(interval.start_time, w_type);
        events.emplace_back(interval.end_time, -w_type);
      }
    }

    sort(events.begin(), events.end(),
         [](const tuple<int, int> &a, const tuple<int, int> &b) {
           if (get<0>(a) != get<0>(b)) {
             return get<0>(a) < get<0>(b);
           }
           return get<1>(a) > get<1>(b);
         });

    set<int> active_wavelengths;
    int prev_time = -1;

    for (const auto &event : events) {
      int time = get<0>(event);
      int change = get<1>(event);

      if (prev_time != -1 && !active_wavelengths.empty()) {

        chiplet_intervals.emplace_back(
            prev_time, time, active_wavelengths.size(), active_wavelengths);
      }

      if (change > 0) {
        active_wavelengths.insert(change);
      } else {
        active_wavelengths.erase(-change);
      }

      prev_time = time;
    }

    optical_Rx_intervals[chiplet_id] = chiplet_intervals;
  }

  cout << "\nOptical RX Intervals by Chiplet ID:\n";
  for (const auto &interval_entry : optical_Rx_intervals) {
    int chiplet_id = interval_entry.first;
    const auto &intervals = interval_entry.second;

    cout << "Chiplet ID: " << chiplet_id << "\n";
    for (const auto &interval : intervals) {
      int start = get<0>(interval);
      int end = get<1>(interval);
      int count = get<2>(interval);
      const set<int> &wavelengths = get<3>(interval);

      cout << "  Interval: [" << start << ", " << end
           << "], Active Wavelengths: {";
      for (auto it = wavelengths.begin(); it != wavelengths.end(); ++it) {
        cout << *it;
        if (next(it) != wavelengths.end()) {
          cout << ", ";
        }
      }
      cout << "}, Count: " << count << ", Duration: " << (end - start + 1)
           << "\n";
    }
  }
}

double OpticalPower::getLaserEnergy() {
  double total_laser_energy = 0.0;
  double wavelength_total_count =
      GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y;

  for (const auto &[start, end, count, wavelengths] : laser_intervals) {
    total_laser_energy +=
        mW2J(GlobalParams::optical_power_configuration.laserConfig.active_mode *
             wavelength_total_count * count) *
        (end - start + 1);
  }
  return total_laser_energy;
}
double OpticalPower::getLaserLossEnergy() {
  double total_laserLoss_energy = 0.0;
  double Er = -20; // Receiver Sensitivity
  double Ec = 0.9; // Coupling Efficiency
  double El = 0.2; // Laser Efficiency

  double pathloss_power_ = 0.0;
  pathloss_power_ = pow(10, (path_loss[0] + Er) / 10) / (Ec * El);
  cout << "pathloss_power(): " << pathloss_power_ << endl;
  cout << "pathloss_power: " << mW2J(pathloss_power_) << endl;

  for (const auto &[start, end, count, wavelengths] : laser_intervals) {
    double pathloss_power = 0.0;
    for (const auto &w : wavelengths) {
      pathloss_power += pow(10, (path_loss[w] + Er) / 10) / (Ec * El);
    }

    total_laserLoss_energy += (mW2J(pathloss_power) * (end - start + 1));
  }

  // for (const auto& [start, end, count, wavelengths] : laser_intervals) {
  //     double pathloss_power = 0.0;
  //     for (const auto& w : wavelengths){
  //         cout<< "wavelength: " << w << ", path_loss: " << path_loss[w] <<
  //         endl; pathloss_power += pow(10, (path_loss[w]+Er)/10)/ (Ec*El);
  //     }
  //     cout << "pathloss_power: " << mW2J(pathloss_power) << endl;
  //     total_laserLoss_energy += (mW2J(pathloss_power) * (end - start + 1));

  // }

  // for (const auto& [start, end, count, wavelengths] : laser_intervals) {
  //     double pathloss_power = 0.0;
  //     for (const auto& w : wavelengths){
  //         pathloss_power += pow(10, (path_loss[w]+Er)/10)/ (Ec*El);
  //     }

  //     total_laserLoss_energy += (mW2J(pathloss_power) * (end - start + 1));
  // }

  return total_laserLoss_energy;
}
double OpticalPower::getTxEnergy() {
  double total_tx_energy = 0.0;
  double wavelength_total_count =
      GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y;
  // cout << endl << endl;

  for (const auto &chiplet_entry : optical_tx_intervals) {
    int chiplet_id = chiplet_entry.first;
    const auto &intervals = chiplet_entry.second;

    // cout << "In OpticalPower::getTxEnergy():" << endl;
    for (const auto &interval : intervals) {
      int start = std::get<0>(interval);
      int end = std::get<1>(interval);
      int count = std::get<2>(interval);
      const auto &wavelengths = std::get<3>(interval);

      // cout << "start: " << start << ", end: " << end << ", count: " << count
      // << endl;

      double energy = mW2J_clk2((GlobalParams::optical_power_configuration
                                     .driverConfig.active_mode *
                                 count) +
                                (GlobalParams::optical_power_configuration
                                     .serializerConfig.active_mode *
                                 count) +
                                (GlobalParams::optical_power_configuration
                                     .serializerConfig.idle_mode *
                                 (wavelength_total_count - count))) *
                      (end - start + 1);
      total_tx_energy += energy;
      // cout << "Tx Chiplet ID: " << chiplet_id << ", Interval: [" << start <<
      // ", " << end << "], Count: " << count
      //           << ", Energy: " << energy << " J\n";
    }
  }

  return total_tx_energy;
}
double OpticalPower::getRxEnergy() {
  double total_rx_energy = 0.0;
  double wavelength_total_count =
      GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y;
  // cout << endl << endl;
  for (const auto &chiplet_entry : optical_Rx_intervals) {
    int chiplet_id = chiplet_entry.first;
    const auto &intervals = chiplet_entry.second;

    for (const auto &interval : intervals) {
      int start = get<0>(interval);
      int end = get<1>(interval);
      int count = get<2>(interval);
      const auto &wavelengths = get<3>(interval);

      double energy =
          mW2J_clk2(
              (GlobalParams::optical_power_configuration.tiaConfig.active_mode *
               count) +
              (GlobalParams::optical_power_configuration.rxComparatorConfig
                   .active_mode *
               count) +
              (GlobalParams::optical_power_configuration.rxComparatorConfig
                   .idle_mode *
               (wavelength_total_count - count))) *
          (end - start + 1);
      total_rx_energy += energy;
      // cout << "Rx Chiplet ID: " << chiplet_id << ", Interval: [" << start <<
      // ", " << end << "], Count: " << count
      //           << ", Energy: " << energy << " J\n";
    }
  }

  return total_rx_energy;
}
// double OpticalPower::getArbitrationFlowControlEnergy()
// {
//     double total_ArbitrationFlowControl_energy = 0.0;
//     double wavelength_total_count =
//     GlobalParams::mesh_dim_x*GlobalParams::mesh_dim_y;

//     for (const auto& [start, end, count, wavelengths] : laser_intervals) {
//         total_ArbitrationFlowControl_energy +=
//         mW2J(GlobalParams::optical_power_configuration.arbitrationFlowControlConfig.active_mode
//         * (count/wavelength_total_count) +
//         GlobalParams::optical_power_configuration.arbitrationFlowControlConfig.idle_mode
//         * ((wavelength_total_count-count)/wavelength_total_count)) * (end -
//         start + 1);
//     }
//     return total_ArbitrationFlowControl_energy;
// }
double OpticalPower::getEOEEnergy() { return getTxEnergy() + getRxEnergy(); }

double OpticalPower::getMRREnergy() {
  string file_path =
      "./MRR_info/MRR" +
      to_string(GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y) + ".txt";
  ifstream file(file_path);
  map<string, int> MRR_number;

  if (!file) {
    cerr << "Failed to open file: " << file_path << endl;
    assert(false);
  }

  string line;
  while (getline(file, line)) {
    istringstream iss(line);
    string key;
    char colon;
    int value;

    if (iss >> key >> colon) {

      if (colon != ':') {
        cerr << "key = " << key << ", colon = " << colon << endl;
        cerr << "Invalid format (missing colon) in line: " << line << endl;
        assert(false);
      }

      char next_char;
      if (iss.peek() == ' ') {
        iss.get(next_char);
      }

      if (iss >> value) {
        MRR_number[key] = value;
      } else {
        cerr << "Invalid value format in line: " << line << endl;
        assert(false);
      }
    } else {
      cerr << "Failed to parse line: " << line << endl;
      assert(false);
    }
  }
  file.close();

  double chiplet_MRR_number =
      (GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y * 2) *
      (GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y);
  // cout << "\nMRR_number[" << GlobalParams::architecture << "]" <<
  // MRR_number[GlobalParams::architecture] << endl;
  return mW2J(GlobalParams::optical_power_configuration.mrrConfig.active_mode *
              (MRR_number[GlobalParams::architecture] + chiplet_MRR_number)) *
         (GlobalParams::simulation_time * 1e3 / GlobalParams::clock_period_ps);
}

void OpticalPower::printLaserTimeUnsorted(std::ostream &out) {

  map<int, vector<tuple<int, int, vector<laser_time>>>> grouped_entries;

  for (const auto &entry : laser_time_unsorted) {
    int w_type = entry.first;
    grouped_entries[w_type].push_back(entry.second);
  }

  for (const auto &group : grouped_entries) {
    int w_type = group.first;
    const auto &entries = group.second;

    out << "Wavelength: " << w_type << "\n";

    for (const auto &t : entries) {
      int src_id = get<0>(t);
      int dst_id = get<1>(t);
      const vector<laser_time> &times = get<2>(t);

      out << "\tKey(src_id, dst_id): (" << src_id << ", " << dst_id << ")\n";

      if (times.empty()) {
        out << "\t\tNo laser_time entries.\n";
        continue;
      }

      for (const auto &time : times) {
        if (time.start_time <= 0 ||
            (time.start_time >= time.end_time && time.end_time != -1)) {
          out << endl
              << "******************" << endl
              << "ABNORMAL SITUATION : in OpticalPower::printLaserTime(). "
                 "Laser's start_time <= 0 or Laser's start_time >= end_time"
              << endl
              << "time.start_time: " << time.start_time
              << ", time.end_time: " << time.end_time << endl
              << "******************" << endl;
          assert(false);
        }
        // else if (time.end_time <= 0) {
        //     // out << endl \
        //     //         << "******************" << endl \
        //     //         << "Please check ABNORMAL SITUATION : in
        //     OpticalPower::printLaserTime(). Laser's end_time <= 0" << endl \
        //     //         << "time.start_time: " << time.start_time << ",
        //     time.end_time: " << time.end_time << endl \
        //     //         << "******************" << endl;
        // }
        else {
          out << "\t\tCycle: (" << time.start_time << ", " << time.end_time
              << ")\n";
        }
      }
    }
  }
}

void OpticalPower::printWavelengthIntervals(std::ostream &out) {
  out << "\nLaser On/Off time (cycle):" << endl;
  for (const auto &entry : wavelength_intervals) {
    int w_type = entry.first;
    const auto &laser_intervals = entry.second;

    out << "Wavelength " << w_type << ":\n";
    for (const auto &interval : laser_intervals) {
      out << "\tLaser On: " << interval.start_time
          << ", Off: " << interval.end_time << "\n";
    }
  }
}

void OpticalPower::printOpticalTxTime(std::ostream &out) {
  for (const auto &entry : optical_tx_time_unsorted) {
    int chiplet_id = entry.first;
    const auto &[w_type, times] = entry.second;

    out << "\nOpticalTxTime unsorted: \n";
    out << "Chiplet ID: " << chiplet_id << "\n";
    out << "  Wavelength Type: " << w_type << "\n";
    out << "  Tx Times:\n";

    for (const auto &time : times) {
      out << "    Start Time: " << time.start_time
          << ", End Time: " << time.end_time << "\n";
    }
  }
}

void OpticalPower::printOpticalRxTime(std::ostream &out) {
  for (const auto &entry : optical_Rx_time_unsorted) {
    int chiplet_id = entry.first;
    const auto &[w_type, times] = entry.second;

    out << "\nOpticalRxTime unsorted: \n";
    out << "Chiplet ID: " << chiplet_id << "\n";
    out << "  Wavelength Type: " << w_type << "\n";
    out << "  Rx Times:\n";

    for (const auto &time : times) {
      out << "    Start Time: " << time.start_time
          << ", End Time: " << time.end_time << "\n";
    }
  }
}

void OpticalPower::printIntervals(std::ostream &out) {

  out << "\n\nLaser Intervals: \n";
  for (const auto &[start, end, count, wavelengths] : laser_intervals) {
    out << "Interval: [" << start << ", " << end << "], Active Wavelengths: {";
    for (auto it = wavelengths.begin(); it != wavelengths.end(); ++it) {
      out << *it;
      if (next(it) != wavelengths.end()) {
        out << ", ";
      }
    }
    out << "}, Count: " << count << ", Duration: " << (end - start + 1) << "\n";
  }

  out << "\nOptical TX Intervals by Chiplet ID:\n";
  for (const auto &interval_entry : optical_tx_intervals) {
    int chiplet_id = interval_entry.first;
    const auto &intervals = interval_entry.second;

    out << "Chiplet ID: " << chiplet_id << "\n";
    for (const auto &interval : intervals) {
      int start = get<0>(interval);
      int end = get<1>(interval);
      int count = get<2>(interval);
      const set<int> &wavelengths = get<3>(interval);

      out << "  Interval: [" << start << ", " << end
          << "], Active Wavelengths: {";
      for (auto it = wavelengths.begin(); it != wavelengths.end(); ++it) {
        out << *it;
        if (next(it) != wavelengths.end()) {
          out << ", ";
        }
      }
      out << "}, Count: " << count << ", Duration: " << (end - start + 1)
          << "\n";
    }
  }
  out << "\nOptical RX Intervals by Chiplet ID:\n";
  for (const auto &interval_entry : optical_Rx_intervals) {
    int chiplet_id = interval_entry.first;
    const auto &intervals = interval_entry.second;

    out << "Chiplet ID: " << chiplet_id << "\n";
    for (const auto &interval : intervals) {
      int start = get<0>(interval);
      int end = get<1>(interval);
      int count = get<2>(interval);
      const set<int> &wavelengths = get<3>(interval);

      out << "  Interval: [" << start << ", " << end
          << "], Active Wavelengths: {";
      for (auto it = wavelengths.begin(); it != wavelengths.end(); ++it) {
        out << *it;
        if (next(it) != wavelengths.end()) {
          out << ", ";
        }
      }
      out << "}, Count: " << count << ", Duration: " << (end - start + 1)
          << "\n";
    }
  }
}