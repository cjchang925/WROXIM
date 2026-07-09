/*
 * File added by LGGM.
 *
 * This file contains the declaration for the Statistics of the task mapping
 * simulation engine
 * 
 * Task Mapping Module: core of the Task Mapping Engine
 * (C) 2016 by the University of Antioquia, Colombia
 *
 * Noxim - the NoC Simulator
 * 
 * (C) 2005-2010 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 */

#ifndef __NOXIM_TASK_MAPPING_STATS__
#define __NOXIM_TASK_MAPPING_STATS__

#include "../DataStructs.h"

/*******************************
 * Classes to store statistics *
 *******************************/ 
template <class T> 
class tm_peexec_statistics {
private:
  T         _stat_cswitching_acum;
  T         _stat_tskexec_acum;
  T         _stat_idle_acum;
  int       _stat_flits_tx;  
  int       _stat_flits_rx;  
  
  T         _stat_cycles_total;
  double    _stat_cswitching_energy_d;
  double    _stat_tskexec_energy_d;
  double    _stat_idle_energy_d;
  double    _stat_total_energy_d;
  
  void _reset() {
    _stat_cswitching_acum = 0;
    _stat_tskexec_acum = 0;
    _stat_idle_acum = 0;
    _stat_flits_tx = 0;
    _stat_flits_rx = 0;
    
    _stat_cycles_total = 0;
    _stat_cswitching_energy_d = 0;
    _stat_tskexec_energy_d = 0;
    _stat_idle_energy_d = 0;
    _stat_total_energy_d = 0;
  }
  
public:
  tm_peexec_statistics() {
    _reset();
  }
  tm_peexec_statistics<T> operator+(const tm_peexec_statistics<T>& other) {
    tm_peexec_statistics<T> temp;
    temp._stat_cswitching_acum = _stat_cswitching_acum + other._stat_cswitching_acum;
    temp._stat_tskexec_acum = _stat_tskexec_acum + other._stat_tskexec_acum;
    temp._stat_idle_acum = _stat_idle_acum + other._stat_idle_acum;
    temp._stat_flits_tx = _stat_flits_tx + other._stat_flits_tx;
    temp._stat_flits_rx = _stat_flits_rx + other._stat_flits_rx;
    
    temp._stat_cycles_total = _stat_cycles_total + other._stat_cycles_total;
    temp._stat_cswitching_energy_d = _stat_cswitching_energy_d + other._stat_cswitching_energy_d;
    temp._stat_tskexec_energy_d = _stat_tskexec_energy_d + other._stat_tskexec_energy_d;
    temp._stat_idle_energy_d = _stat_idle_energy_d + other._stat_idle_energy_d;
    temp._stat_total_energy_d = _stat_total_energy_d + other._stat_total_energy_d;
    
    return temp;
  }
  
  inline void statAddCSwitchingCycles(T cycles) {
    this->_stat_cswitching_acum += cycles;
  }
  inline T statGetCSwitchingCycles(void) {
    return _stat_cswitching_acum;
  }
  inline void statAddTskExecCycles(T cycles) {
    this->_stat_tskexec_acum += cycles;
  }
  inline T statGetTskExecCycles(void) {
    return _stat_tskexec_acum;
  }
  inline void statAddIdleCycles(T cycles) {
    this->_stat_idle_acum += cycles;
  }
  inline T statGetIdleCycles(void) {
    return _stat_idle_acum;
  }
  inline void statIncFlitsTx(void) {
    this->_stat_flits_tx++;
  }
  inline T statGetFlitsTx(void) {
    return _stat_flits_tx;
  }
  inline void statIncFlitsRx(void) {
    this->_stat_flits_rx++;
  }
  inline T statGetFlitsRx(void) {
    return _stat_flits_rx;
  }
  inline void statComputePeTotalCyclesAndEnergy(void) {
    this->_stat_cycles_total = _stat_cswitching_acum + 
                               _stat_tskexec_acum + 
                               _stat_idle_acum;
    
    _stat_cswitching_energy_d = GlobalParams::power_configuration.pePowerConfig.running_mode * (_stat_cswitching_acum);
    _stat_tskexec_energy_d = GlobalParams::power_configuration.pePowerConfig.running_mode * (_stat_tskexec_acum);
    _stat_idle_energy_d = GlobalParams::power_configuration.pePowerConfig.idle_mode * (_stat_idle_acum);
    _stat_total_energy_d = _stat_cswitching_energy_d + _stat_tskexec_energy_d + _stat_idle_energy_d;
  }
  inline T statGetPeTotalCycles(void) {
    return _stat_cycles_total;
  }
  inline double statGetCSwitchingEnergy_d(void) {
    return this->_stat_cswitching_energy_d;
  }
  inline double statGetTskExecEnergy_d(void) {
    return this->_stat_tskexec_energy_d;
  }
  inline double statGetIdleEnergy_d(void) {
    return this->_stat_idle_energy_d;
  }
  inline double statGetTotalEnergy_d(void) {
    return this->_stat_total_energy_d;
  }
  void reset(void) {
    _reset();
  }
};

template <class T> 
class tm_appexec_statistics {
private:
  int       _stat_exec_number;
  double    _stat_avg;
  T         _stat_last;
  T         _stat_min;
  T         _stat_max;

  void _reset(void) {
    _stat_exec_number = 0;
    _stat_avg = 0;
    _stat_last = 0;
    _stat_min = 0;
    _stat_max = 0;
  }
public:  
  tm_appexec_statistics() {
    _reset();
  }
  tm_appexec_statistics<T> operator+(const tm_appexec_statistics<T>& other) {
    tm_appexec_statistics<T> temp;
    temp._stat_exec_number = _stat_exec_number + other._stat_exec_number;
    temp._stat_avg = _stat_avg + other._stat_avg;
    temp._stat_last = _stat_last + other._stat_last;
    temp._stat_min = _stat_min + other._stat_min;
    temp._stat_max = _stat_max + other._stat_max;  
    return temp;
  }
  
  inline int statGetExecNumber(void) { 
    return (_stat_exec_number);
  }
  inline void statSetExecNumber(T _stat_exec_number) {
    this->_stat_exec_number = _stat_exec_number;
  }
  inline double statGetAverage(void) {
    return (_stat_avg);
  }
  inline void statSetAverage(T _stat_avg) {
    this->_stat_avg = _stat_avg;
  }
  inline T statGetLast(void) {
    return (_stat_last);
  }
  inline void statSetLast(T _stat_last) {
    this->_stat_last = _stat_last;
  }
  inline T statGetMin(void) {
    return (_stat_min);
  }
  inline void statSetMin(T _stat_min) {
    this->_stat_min = _stat_min;
  }
  inline T statGetMax(void) {
    return (_stat_max);
  }
  inline void statSetMax(T _stat_max) {
    this->_stat_max = _stat_max;
  }
  void reset(void) {
    _reset();
  }
  void statAddNewValue(T stat_newvalue) {
    _stat_last = stat_newvalue;
    _stat_exec_number++;
    _stat_avg = ((_stat_avg * (_stat_exec_number-1)) + stat_newvalue) / (_stat_exec_number);
    if (stat_newvalue < _stat_min || _stat_exec_number == 1)
      _stat_min = stat_newvalue;
    if (stat_newvalue > _stat_max)
      _stat_max = stat_newvalue;
  }
  void statAddNewValue(tm_appexec_statistics<T>& other) {
    if ((_stat_exec_number + other._stat_exec_number) == 0) {
      return;
    }
    else if (other._stat_exec_number == 0) {
      return;
    }
    else {
      _stat_last = other._stat_last;
      _stat_avg = ((_stat_avg * _stat_exec_number) + (other._stat_avg * other._stat_exec_number)) / (_stat_exec_number + other._stat_exec_number);
      if (other._stat_min < _stat_min || _stat_exec_number == 0)
        _stat_min = other._stat_min;
      if (other._stat_max > _stat_max)
        _stat_max = other._stat_max;
      _stat_exec_number += other._stat_exec_number;
    }
  }
};

#endif // __NOXIM_TASK_MAPPING_STATS__