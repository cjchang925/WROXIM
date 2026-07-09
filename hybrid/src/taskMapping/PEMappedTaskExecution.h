/*
 * File added by LGGM.
 *
 * This file contains the declarations of the task mapping simulation engine
 * 
 * Processing Element Mapped Task Execution Module
 * (C) 2016 by the University of Antioquia, Colombia
 *
 * Noxim - the NoC Simulator
 * 
 * (C) 2005-2010 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 */

#ifndef __NOXIM_PE_MAPPED_TASK_EXECUTION__
#define __NOXIM_PE_MAPPED_TASK_EXECUTION__

#include <list>
#include <queue>
#include "TaskMapping.h"

class tm_tx_block {
private:    
  tm_task    * _src_task;		// pointer to the source task
  tm_task    * _dst_task;		// pointer to the subsequent task
  int	       _pblock_id;		// index of the PBlock of the source task
  int_cycles   _starttx_cycle;		// delay for the transmission
  int          _pblock_path;                  // added by xuanWei for path aware mapping
public:    
  tm_tx_block() {
    _src_task = NULL;
    _dst_task = NULL;
    _pblock_id = 0;
    _starttx_cycle = 0;
    _pblock_path = 0;
  }
  
  inline tm_task * getSrcTask(void) {
    return _src_task;
  }
  inline void setSrcTask(tm_task * src_task) {
    this->_src_task = src_task;
  }
  inline tm_task * getDstTask(void) {
    return _dst_task;
  }
  inline void setDstTask(tm_task * dst_task) {
    this->_dst_task = dst_task;
  }
  inline int getPbId(void) {
    return _pblock_id;
  }
  inline int getPath(void) {
    return _pblock_path;
  }


  
  inline void setPbId(int pblock_id) {
    this->_pblock_id = pblock_id;
  }
  inline void setPath(int pblock_path) {
    this->_pblock_path = pblock_path;
  }
  inline int_cycles getStartTxCycle(void) {
    return _starttx_cycle;
  }
  inline void setStartTxCycle(int_cycles starttx_cycle) {
    this->_starttx_cycle = starttx_cycle;
  }
};

class PEMappedTaskExecution {
private:  
  string objectName;

  tm_pe * pe;
  
  // Execution states of the tasks running on PE[id]
  list < tm_task * > waitingrx_tasks;	// Tasks waiting for incoming data
  list < tm_task * > sleeping_tasks;	// Root tasks ready to start
  list < tm_task * > ready_tasks;	// Tasks waiting for the CPU
  tm_task * running_task;		// Task running currently
  
  queue < tm_tx_block > waitingpb_tx;	// PBlocks that have payload to be transmitted
  
  // Execution Monitor
  int_cycles	nextTimeToMapTasks;				// In cycles
  int_cycles	nextTimeToUnmapTasks;				// In cycles
  int_cycles	nextTimeToWakeUpTasks;				// In cycles
  int_cycles 	nextTimeToCheckRunningTasks;                    // In cycles
  int_cycles 	lastTimeRunningTasksWereChecked;                // In cycles
  
  // Scheduler Timing
  int_cycles 	schedulerTicksLeft;	// In cycles
  
  // Statistics
  int_cycles 	lastTimePeWentIdle;                             // In cycles
  
  void resetTaskStates(void);
  bool firstTaskPBlock(tm_task * t);
  bool gotoNextPBlockExecution(void);
  bool isSleepingTaskReady(tm_task * t);
  bool isWaitingRxTaskReady(tm_task * t);
  void taskToIdle();
  bool preparePayloadForTransmission(void);
  void computeNextTimeToMapTasks(bool reset);
  int_cycles _computeNextTimeToUnmapTask(tm_task * t, int_cycles _nextTimeToUnmap);
  void computeNextTimeToUnmapTasks(void);
  void computeNextTimeToWakeUpTasks(void);
  void computeNextTimeToCheckRunningTasks(bool reset, int_cycles cycles = 0);
  bool switchTaskContext();

  void tasksInSleepingState();
  void tasksInWaitingRxState();
  void tasksInReadyState();
  void tasksInRunningState();
  void tasksToBeMapped();
  void tasksToBeUnmapped();

  Flit _tm_nextFlit(Packet & p);
  
public:
  PEMappedTaskExecution(string objectName);

  // Functions to be called at reset time
  void taskStatesInitialization(tm_pe * pe);

  // Functions to be called from peProcess
  void timeToMapTasks(double ccycle);
  void timeToUnmapTasks(double ccycle);
  void timeToWakeUpTasks(double ccycle);
  void timeToCheckRunningTasks(double ccycle);

  // Functions to be called from peTxProcess and peRxProcess
  bool canShot(queue < Packet > & qp);
  Flit tm_nextFlit_samepe(Packet & p);
  Flit tm_nextFlit(queue < Packet > & packet_queue);
  void processReceivedFlit(Flit flit);

  int getPeId();

  inline string name() {
    return objectName;
  }
};

#endif // __NOXIM_PE_MAPPED_TASK_EXECUTION__
