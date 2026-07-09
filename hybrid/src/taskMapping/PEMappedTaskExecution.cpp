/*
 * File added by LGGM.
 *
 * This file contains the functions of the task mapping simulation engine
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

#include <iostream>
#include <string>
#include "PEMappedTaskExecution.h"

using namespace std;

/***************************************************************************************
 * PEMappedTaskExecution Class *
 ***************************************************************************************/ 

/*
 * Private functions: functions to support the task execution states
 */

PEMappedTaskExecution::PEMappedTaskExecution(string objectName) { 
  resetTaskStates();
  this->objectName = objectName;
}

void PEMappedTaskExecution::resetTaskStates(void) {
  waitingrx_tasks.clear();
  sleeping_tasks.clear();
  ready_tasks.clear();
  running_task = NULL;

  cout << "In resetTaskStates(), clear the waitingrx_tasks." << endl;
  
  waitingpb_tx = queue < tm_tx_block >();
  
  nextTimeToMapTasks = GlobalParams::reset_time + GlobalParams::simulation_time + 1;
  nextTimeToUnmapTasks = nextTimeToMapTasks;
  nextTimeToWakeUpTasks = nextTimeToMapTasks;
  nextTimeToCheckRunningTasks = nextTimeToMapTasks;
  lastTimeRunningTasksWereChecked = nextTimeToMapTasks;
  lastTimePeWentIdle = -1;
  
  schedulerTicksLeft = 0;
}

bool PEMappedTaskExecution::firstTaskPBlock(tm_task * t) {
  // Check if task has Processing Blocks
  if (t->pblocks_tasks.size() > 0) {
    t->resetCurrentPb();
    t->setExectimeLeft(t->pblocks_tasks[t->getCurrentPb()].getExecTime());
    return true;
  }
  
  return false;
}

bool PEMappedTaskExecution::gotoNextPBlockExecution(void) {
  if (running_task->getCurrentPb() < (int)(running_task->pblocks_tasks.size()-1)) {
    running_task->incCurrentPb();
    running_task->setExectimeLeft(running_task->pblocks_tasks[running_task->getCurrentPb()].getExecTime());
    return true;
  }
  else {  // Task has completed all its pbs 
    running_task->taskExecutionCompleted();
    
    if (running_task->isRootTask() && running_task->getApp()->getMinRestart() == 0) {
      running_task->resetCurrentPb();
      running_task->setExectimeLeft(running_task->pblocks_tasks[running_task->getCurrentPb()].getExecTime());
      running_task->taskExecutionRunning();
      return true;
    }  
  }

  return false;
}

bool PEMappedTaskExecution::isSleepingTaskReady(tm_task * t) {
  double ccycle = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
  
  if (ccycle >= t->getApp()->getStartTime()) {
    (void)firstTaskPBlock(t);
    return true;
  }
  else
    return false;
}

bool PEMappedTaskExecution::isWaitingRxTaskReady(tm_task * t) {
  // check all the antecedent tasks
  cout << "In isWaitingRxTaskReady()." << endl;
  for (vector< tm_antecedent_task >::iterator it_pb_ant=t->antecedent_tasks.begin(); it_pb_ant!=t->antecedent_tasks.end(); ++it_pb_ant) {
    TM_ASSERT(it_pb_ant->getNumRx() >= 0, "Number of packets received without being processed is negative!");
    if (it_pb_ant->getNumRx() == 0) {
      return false;
    }
  }

  TM_SIMEXECLOG(TM_SIMEXECLOG_RECEIVESALLANT, 
                pe->getId(), t->getApp()->getId(), t->getId(), "receives all antecedent payloads.");
  for (vector< tm_antecedent_task >::iterator it_pb_ant=t->antecedent_tasks.begin(); it_pb_ant!=t->antecedent_tasks.end(); ++it_pb_ant) {
    it_pb_ant->decNumRx();		// This task continues when all the antecedent tasks have sent their data
  }

  if (!firstTaskPBlock(t)) {		// If the task has pblocks to execute, put it in ready task list
    return false;
  }
  
  return true;
}

void PEMappedTaskExecution::taskToIdle() {
  bool checkSleepingTasks = false;
  double ccycle = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;

  if (running_task->isRootTask()) {
    TM_ASSERT(running_task->getApp()->getMinRestart() > 0, 
              TM_STRAPPTASK(running_task) << " with min_restart equal to zero but went to idle!");
    running_task->getApp()->setStartTime(ccycle + running_task->getApp()->getMinRestart());
    running_task->toSleepingState();
    sleeping_tasks.push_back(running_task);
    checkSleepingTasks = true;
  }
  else {
    if (isWaitingRxTaskReady(running_task)) {
      running_task->toReadyState();
      ready_tasks.push_back(running_task);
    }
    else {
      running_task->toWaitingRxState();
      waitingrx_tasks.push_back(running_task);
    }
    
  }
  running_task = NULL;
  computeNextTimeToCheckRunningTasks(true);	// reset time

  // Check if time to wake up tasks must be computed
  if (checkSleepingTasks)
    computeNextTimeToWakeUpTasks();	// Compute time to wake up tasks (maybe this task is ready)
  
  // Check if there is a ready task to be executed
  tasksInReadyState();
}

bool PEMappedTaskExecution::preparePayloadForTransmission(void) {
  tm_tx_block tx_b;	// Transmission Block
  double ccycle = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;

  if (running_task->pblocks_tasks[running_task->getCurrentPb()].getSTaskId() != TM_NOT_TASK) {
    //TM_ASSERT(running_task->pblocks_tasks[running_task->getCurrentPb()].getPayLoad() > 0, "PBlock with subsequent task and payload equal to zero!");
    if(running_task->pblocks_tasks[running_task->getCurrentPb()].getPayLoad() == 0)
    {
      return false;
    }
    tx_b.setSrcTask(running_task);
    tx_b.setDstTask(tmInstance->getTask(running_task->getApp()->getId(), running_task->pblocks_tasks[running_task->getCurrentPb()].getSTaskId()));
    tx_b.setPbId(running_task->getCurrentPb());
    tx_b.setPath(running_task->pblocks_tasks[running_task->getCurrentPb()].getPath()); // added by xuanWei for path aware mapping
    cout << "In preparePayloadForTransmission(), set path = " << tx_b.getPath() <<"set pbid ="<< tx_b.getPbId() << endl;
    tx_b.setStartTxCycle(ccycle + tmInstance->getSchedulerTxDelay());	// Starts a x cycles later
    waitingpb_tx.push(tx_b);

    
    return true;
  }  
  
  return false;
}

void PEMappedTaskExecution::computeNextTimeToMapTasks(bool reset) {
  if (reset)
    pe->currentmapping = pe->mappingsonpe.begin();
  else
    pe->currentmapping++;
  
  if (pe->currentmapping != pe->mappingsonpe.end())
    nextTimeToMapTasks = pe->currentmapping->first - tmInstance->getSchedulerMapBefore();
  else
    nextTimeToMapTasks = GlobalParams::reset_time + GlobalParams::simulation_time + 1;
}

int_cycles PEMappedTaskExecution::_computeNextTimeToUnmapTask(tm_task * t, int_cycles _nextTimeToUnmap) {
  int_cycles nextEvent;

  if (!t->getApp()->alreadyStopped()) {
    nextEvent = t->getApp()->getStopTime();
    if (nextEvent < _nextTimeToUnmap) {
      return (nextEvent);
    }
  }
  
  return _nextTimeToUnmap;
}

void PEMappedTaskExecution::computeNextTimeToUnmapTasks(void) {
  nextTimeToUnmapTasks = GlobalParams::reset_time + GlobalParams::simulation_time + 1;
  
  // Waiting Rx Tasks
  for (list< tm_task * >::iterator it=waitingrx_tasks.begin(); it!=waitingrx_tasks.end(); ++it) {
    nextTimeToUnmapTasks = _computeNextTimeToUnmapTask((*it), nextTimeToUnmapTasks);
  }
  // Sleeping Tasks
  for (list< tm_task * >::iterator it=sleeping_tasks.begin(); it!=sleeping_tasks.end(); ++it) {
    nextTimeToUnmapTasks = _computeNextTimeToUnmapTask((*it), nextTimeToUnmapTasks);
  }
  // Ready Tasks
  for (list< tm_task * >::iterator it=ready_tasks.begin(); it!=ready_tasks.end(); ++it) {
    nextTimeToUnmapTasks = _computeNextTimeToUnmapTask((*it), nextTimeToUnmapTasks);
  }
  // Running Task
  if (running_task) {
    nextTimeToUnmapTasks = _computeNextTimeToUnmapTask((running_task), nextTimeToUnmapTasks);
  }
}

void PEMappedTaskExecution::computeNextTimeToWakeUpTasks(void) {
  tm_task * t;
  double ccycle = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
  double nextEvent;
  
  nextTimeToWakeUpTasks = GlobalParams::reset_time + GlobalParams::simulation_time + 1;
  
  if (sleeping_tasks.size() > 0) {
    for (list< tm_task * >::iterator it=sleeping_tasks.begin(); it!=sleeping_tasks.end(); it++) {
      t = (*it);
      TM_ASSERT(t->isRootTask(), "Non root task in sleeping_tasks vector!");
      
      nextEvent = t->getApp()->getStartTime();
      
      if (ccycle >= (nextEvent-1)) {
	nextTimeToWakeUpTasks = ccycle + 1;		// Task needs to be awakened next cycle
	break;
      } 
      else if (nextEvent < nextTimeToWakeUpTasks) {
	nextTimeToWakeUpTasks = nextEvent;
      }
    } // End for
  } // End if sleeping size
}

void PEMappedTaskExecution::computeNextTimeToCheckRunningTasks(bool reset, int_cycles cycles) {
  double ccycle = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
  int_cycles nextEvent;
  
  if (reset) {
    nextTimeToCheckRunningTasks = GlobalParams::reset_time + GlobalParams::simulation_time + 1;
    lastTimeRunningTasksWereChecked = nextTimeToCheckRunningTasks;
  }
  else {
    if (cycles >= schedulerTicksLeft)
      nextEvent = ccycle + schedulerTicksLeft;
    else
      nextEvent = ccycle + cycles;
    
    nextTimeToCheckRunningTasks = nextEvent;
    lastTimeRunningTasksWereChecked = ccycle;
  }
}

bool PEMappedTaskExecution::switchTaskContext() {
  // Perform context switching only of there is at least one task in ready list
  if (!ready_tasks.empty()) {
    if (running_task != NULL) {
      running_task->toReadyStateDueToContextSwitching();
      ready_tasks.push_back(running_task);
      running_task = NULL;
    }
    
    running_task = ready_tasks.front();
    ready_tasks.pop_front();	// Just remove the task from the ready list
    running_task->toContextSwitchingState();
    schedulerTicksLeft = tmInstance->getSchedulerDelay() + tmInstance->getSchedulerTick();
    computeNextTimeToCheckRunningTasks(false, tmInstance->getSchedulerDelay());
    
    // Statistics - PE Idle Cycles - It must be also done when tasks are unmapped
    double ccycle = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
    if (lastTimePeWentIdle != -1) {
      pe->stats.statAddIdleCycles(ccycle - lastTimePeWentIdle);
      lastTimePeWentIdle = -1;
    }
    
    return true;
  }
  else {
    if (running_task != NULL) {
      schedulerTicksLeft = tmInstance->getSchedulerTick();
    }
    else {
      computeNextTimeToCheckRunningTasks(true);
      
      // Statistics - Pe Idle Cycles
      double ccycle = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
      if (lastTimePeWentIdle == -1) {
        if (sleeping_tasks.size() > 0 || waitingrx_tasks.size() > 0) {
          lastTimePeWentIdle = ccycle;
        }
      }
    }
  }
  
  return false;
}

/*
 * Private functions: functions to control the task execution states
 */
void PEMappedTaskExecution::tasksInSleepingState() {
  tm_task * t;
  bool taskReadyFound = false;
  for (list< tm_task * >::iterator it=sleeping_tasks.begin(); it!=sleeping_tasks.end(); ) {
    t = (*it);
    TM_ASSERT(t->isRootTask(), "Non root task in sleeping_tasks vector!");
    // cout << "In PEMappedTaskExecution::tasksInSleepingState()" << endl;
    // cout << "sleeping_tasks.size() = " << sleeping_tasks.size() << endl;
    // cout << "Task id = " << t->getId() << endl;
    // cout << "isSleepingTaskReady(t) = " << isSleepingTaskReady(t) << endl;
    if (isSleepingTaskReady(t)) {
      t->toReadyState();
      ready_tasks.push_back(t);
      it = sleeping_tasks.erase(it);
      taskReadyFound = true;
    }
    else {
      it++;
    }
  }
  
  if (taskReadyFound) {
    // Compute new time for waking up tasks
    computeNextTimeToWakeUpTasks();
    // Check if there is a task to be put in running state
    tasksInReadyState();
  }
}

void PEMappedTaskExecution::tasksInWaitingRxState() {
    tm_task * t;
    bool taskReady, taskReadyFound = false;
    cout << "In tasksInWaitingRxState():" << endl;
    cout << "waitingrx_tasks.size() = " << waitingrx_tasks.size() << endl;
    for (list< tm_task * >::iterator it=waitingrx_tasks.begin(); it!=waitingrx_tasks.end(); ) {
        t = (*it);
        TM_ASSERT(!t->isRootTask(), "Root task in waiting_rx_tasks vector!");
        // cout << "In tasksInWaitingRxState():" << endl;
        // cout << "waitingrx_tasks.size() = " << waitingrx_tasks.size() << endl;
        cout << "Task id = " << t->getId() << endl;
        taskReady = isWaitingRxTaskReady(t);
        if (taskReady) {
            t->toReadyState();
            ready_tasks.push_back(t);
            it = waitingrx_tasks.erase(it);
            taskReadyFound = true;
            cout << "In tasksInWaitingRxState(), erase a waitingrx_tasks." << endl;
        }
        else {
            it++;
        } // end taskReady
    } // end for
        
    if (taskReadyFound) {
        // Check if there is a task to be put in running state
        tasksInReadyState();
    }
}

void PEMappedTaskExecution::tasksInReadyState() {
  if (running_task == NULL) {		// If there is no a task currently running 
    switchTaskContext();
  }
}

void PEMappedTaskExecution::tasksInRunningState() {
  double ccycle = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
  
  TM_ASSERT(running_task != NULL, "tasksInRunningState function called without a task in execution!");

  // Execution: pblock, context switching, transmission
  int_cycles cyclesSinceLastEvent = (ccycle - lastTimeRunningTasksWereChecked);
  TM_ASSERT(cyclesSinceLastEvent <= schedulerTicksLeft, "Context Switching was performed in a time greater than the Scheduler Ticks Left!");
  schedulerTicksLeft -= cyclesSinceLastEvent;

  if (running_task->isInContextSwitchingState()) {
    running_task->toRunningState();
    TM_ASSERT(cyclesSinceLastEvent == tmInstance->getSchedulerDelay(), "Context Switching took longer than expected!");
    TM_ASSERT(schedulerTicksLeft >= TM_PE_SCHEDULER_TICK_MIN, "Scheduler Tick is less than " << TM_PE_SCHEDULER_TICK_MIN << "!");
    #ifdef DEFINE_TM_SIMEXECLOG      
      if (running_task->getExectimeLeft() == running_task->pblocks_tasks[running_task->getCurrentPb()].getExecTime()) {
        TM_SIMEXECLOG(TM_SIMEXECLOG_STARTSEXEC, pe->getId(), running_task->getApp()->getId(), running_task->getId(),
                      "starts its execution with pb" << running_task->getCurrentPb() << ".");
      }
      else {
        TM_SIMEXECLOG(TM_SIMEXECLOG_CONTINUESEXECAFTERCONEXTSW,pe->getId(), running_task->getApp()->getId(), running_task->getId(),
                      "continues its execution with pb" << running_task->getCurrentPb() << ".");
      }  
    #endif   

    // Statistics - Context Switching Cycles
    pe->stats.statAddCSwitchingCycles(cyclesSinceLastEvent);
  }
  else {
    TM_ASSERT(running_task->isInRunningState(), "Task is not in running state!");
    running_task->setExectimeLeft(running_task->getExectimeLeft() - cyclesSinceLastEvent);
    TM_ASSERT(running_task->getExectimeLeft() >= 0, "Execution Time Left must be greater than or equal to zero: app" << running_task->getApp()->getId() << ", task" << running_task->getId() << "!");

    // Statistics - Task Execution Cycles
    pe->stats.statAddTskExecCycles(cyclesSinceLastEvent);

    // Context Switching needed?
    if ( schedulerTicksLeft == 0 ) {
      // Context Switching required
      if (switchTaskContext())
        return;
    }
  }

  // Process all the Payloads with exectime equal to zero
  while (running_task->getExectimeLeft() == 0) {
    TM_SIMEXECLOG(TM_SIMEXECLOG_COMPLETESPB, pe->getId(), running_task->getApp()->getId(), running_task->getId(),
                  "pb" << running_task->getCurrentPb() << " completes its processing.");

    if (preparePayloadForTransmission()) {
      TM_SIMEXECLOG(TM_SIMEXECLOG_PAYLOADREADYTOTRANSF, pe->getId(), running_task->getApp()->getId(), running_task->getId(),
                    "payload of pb" << running_task->getCurrentPb() << " is ready to be transferred.");
    }

    if (!gotoNextPBlockExecution()) {
      TM_SIMEXECLOG(TM_SIMEXECLOG_COMPLETESALLPBS, pe->getId(), running_task->getApp()->getId(), running_task->getId(),
                    "completes its pbs and releases the pe.");
      taskToIdle();
      return;
    }
    else {
      TM_SIMEXECLOG(TM_SIMEXECLOG_CONTINUESEXEC, pe->getId(), running_task->getApp()->getId(), running_task->getId(), 
                    "continues execution with pb" << running_task->getCurrentPb() << ".");
    }
  }

  // This function will be called again once the counter is zero (peProcess)
  computeNextTimeToCheckRunningTasks(false, running_task->getExectimeLeft());
}

void PEMappedTaskExecution::tasksToBeMapped() {
  tm_task * t;

  for (vector< tm_task * >::iterator it=pe->currentmapping->second.tasks_on_pe.begin(); it!=pe->currentmapping->second.tasks_on_pe.end(); ++it) {
    t = (*it);
    
    t->toBeingMappedState(pe->getId(), pe->currentmapping->first, pe->currentmapping->second.getStopTime());
    
    if (t->isRootTask()) {
      if (isSleepingTaskReady(t)) {
        t->toReadyState();
        ready_tasks.push_back(t);
      }
      else {
        t->toSleepingState();
        sleeping_tasks.push_back(t);
      }
    }
    else {
      t->toWaitingRxState();
      waitingrx_tasks.push_back(t);
      cout << "In tasksToBeMapped():" << endl;
      cout << "t->getId() = " << t->getId() << endl;
      cout << "t->getPeId() = " << t->getPeId() << endl;
      //cout << "t->getPATH() = " << t->getPath() << endl;
      cout << "waitingrx_tasks.size() = " << waitingrx_tasks.size() << endl;
    }
  }

  // Compute times to wake up, perform context switching and unmap tasks
  computeNextTimeToUnmapTasks();
  computeNextTimeToWakeUpTasks();

  // Compute the time to map other tasks
  computeNextTimeToMapTasks(false);
  
  // Check if there is a ready task to be executed
  tasksInReadyState();  
}

void PEMappedTaskExecution::tasksToBeUnmapped() {
  tm_task * t;
  bool waitingRxTaskStopped = false;
  bool sleepingTaskStopped = false;
  bool readyTaskStopped = false;
  bool runningTaskStopped = false;

  // Waiting Rx Tasks
  for (list< tm_task * >::iterator it=waitingrx_tasks.begin(); it!=waitingrx_tasks.end(); ) {
    t = (*it);
    // Should the task be stopped?
    if (t->getApp()->alreadyStopped()) {
      t->toStopAndUnmapState();
      it = waitingrx_tasks.erase(it);		// Just remove the task from the waiting rx list
      waitingRxTaskStopped = true;
      cout << "In tasksToBeUnmapped(), erase a waitingrx_tasks." << endl;
    }
    else {
      it++;
    } // end alreadyStopped
  } // end for

  // Sleeping Tasks
  for (list< tm_task * >::iterator it=sleeping_tasks.begin(); it!=sleeping_tasks.end(); ) {
    t = (*it);
    // Should the task be stopped?
    if (t->getApp()->alreadyStopped()) {
      t->toStopAndUnmapState();
      it = sleeping_tasks.erase(it);		// Just remove the task from the sleeping list
      sleepingTaskStopped = true;
    }
    else {
      it++;
    } // end alreadyStopped
  } // end for

  // Ready Tasks
  for (list< tm_task * >::iterator it=ready_tasks.begin(); it!=ready_tasks.end(); ) {
    t = (*it);
    // Should the task be stopped?
    if (t->getApp()->alreadyStopped()) {
      t->toStopAndUnmapState();
      it = ready_tasks.erase(it);		// Just remove the task from the ready list
      readyTaskStopped = true;
    }
    else {
      it++;
    } // end alreadyStopped
  } // end for
  
  // Running Task
  if (running_task) {
    if (running_task->getApp()->alreadyStopped()) {
      // Statistics (tasksInRunningState won't collect it because the task is being stopped)
      double ccycle = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
      int_cycles cyclesSinceLastEvent = (ccycle - lastTimeRunningTasksWereChecked);
      if (running_task->isInContextSwitchingState()) {
        pe->stats.statAddCSwitchingCycles(cyclesSinceLastEvent);
      }
      else {
        pe->stats.statAddTskExecCycles(cyclesSinceLastEvent);
      }
  
      // Stop and unmap task
      running_task->toStopAndUnmapState();
      running_task = NULL;
      runningTaskStopped = true;
    }
  }

  // Compute new time for waking up tasks
  if (sleepingTaskStopped)
    computeNextTimeToWakeUpTasks();
  
  // Check if there is a task to be put in running state
  if (runningTaskStopped)
    tasksInReadyState();
  
  TM_ASSERT(waitingRxTaskStopped ||
	    sleepingTaskStopped ||
	    readyTaskStopped ||
	    runningTaskStopped,
	    "There were no found any task to stop and unmap!");
  
  // Compute the time to stop and unmap other tasks
  computeNextTimeToUnmapTasks();
  
  // Statistics - Idle Cycles
  if (lastTimePeWentIdle != -1) {
    if (ready_tasks.size() == 0 && 
        sleeping_tasks.size() == 0 && 
        waitingrx_tasks.size() == 0 &&
        running_task == NULL) {
        double ccycle = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
        pe->stats.statAddIdleCycles(ccycle - lastTimePeWentIdle);
        lastTimePeWentIdle = -1;
    }
  }
  
  // Statistics for this pe
  TM_SIMPEMETRICSLOG(TM_SIMSTATSLOG_STATPECYCLESANDENERGY, 
                     pe->getId(), "acum_cycles_performed",
                        "contextswitching=" << pe->stats.statGetCSwitchingCycles() << ","
                     << "taskexecution=" << pe->stats.statGetTskExecCycles() << ","
                     << "pe_idle=" << pe->stats.statGetIdleCycles());
}

/*
 * Public functions: functions to be called at reset time
 */

void PEMappedTaskExecution::taskStatesInitialization(tm_pe * pe) {
  resetTaskStates();
  
  if (pe) {
    this->pe = pe;
    
    TM_ASSERT(pe->mappingsonpe.size() > 0, "PE" << pe->getId() << " does not have mappings!");
    
    computeNextTimeToMapTasks(true);    // First block
  }  
}

/*
 * Public functions: functions to be called from peProcess
 */
void PEMappedTaskExecution::timeToMapTasks(double ccycle) {
  TM_ASSERT(ccycle <= nextTimeToMapTasks, "Time to map tasks already passed!");
  if (ccycle == nextTimeToMapTasks)
    tasksToBeMapped();
}

void PEMappedTaskExecution::timeToUnmapTasks(double ccycle) {
  TM_ASSERT(ccycle <= nextTimeToUnmapTasks, "Time to unmap tasks already passed!");
  if (ccycle == nextTimeToUnmapTasks)
    tasksToBeUnmapped();
}

void PEMappedTaskExecution::timeToWakeUpTasks(double ccycle) {
  TM_ASSERT(ccycle <= nextTimeToWakeUpTasks, "Time to wake up tasks already passed!");
  if (ccycle == nextTimeToWakeUpTasks)
    tasksInSleepingState();
}

void PEMappedTaskExecution::timeToCheckRunningTasks(double ccycle) {
  TM_ASSERT(ccycle <= nextTimeToCheckRunningTasks, "Time to check running tasks already passed!");
  if (ccycle == nextTimeToCheckRunningTasks)
    tasksInRunningState();
}

/*
 * Public functions: functions to be called from peTxProcess and peRxProcess
 */

bool PEMappedTaskExecution::canShot(queue < Packet > & qp) {
  tm_tx_block * tx_b;		// Transmission Block
  tm_task * t_src, *t_dst;
  double ccycle = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
  bool retValue = false;
  
  while (!waitingpb_tx.empty()) {
    tx_b = &waitingpb_tx.front();
    if (ccycle >= tx_b->getStartTxCycle()) {
      Packet p;
      
      t_src = ((tm_tx_block *)tx_b)->getSrcTask();
      t_dst = ((tm_tx_block *)tx_b)->getDstTask();
      
      p.src_id = t_src->getPeId();	// source pe
      p.dst_id = t_dst->getPeId();	// destiny pe
      p.timestamp = ccycle;             // Packet ready to be transferred
      p.size = p.flit_left = t_src->pblocks_tasks[tx_b->getPbId()].getPayLoad();
      p.src_task = t_src;
      p.dst_task = t_dst;
      p.pblock_id = tx_b->getPbId();
      p.path = tx_b->getPath();

      qp.push(p);
      waitingpb_tx.pop();
      
#ifdef DEFINE_TM_SIMEXECLOG
      if (p.src_id == p.dst_id) {
	TM_SIMEXECLOG(TM_SIMEXECLOG_PLBEINGTRANSTOSAMEPE, 
                      t_src->getPeId(), t_src->getApp()->getId(), t_src->getId(),
		      "payload of pb" << p.pblock_id <<
		      " being transferred to task in the same pe.");
      }
      else if(GlobalParams::architecture == ARCH_MESH){
	TM_SIMEXECLOG(TM_SIMEXECLOG_PLBEINGTRANSTOWNOC,
                      t_src->getPeId(), t_src->getApp()->getId(), t_src->getId(),
		      "payload of pb" << p.pblock_id <<
		      " being transferred to WNoC.");
      }
      else {
        TM_SIMEXECLOG(TM_SIMEXECLOG_PLBEINGTRANSTOWNOC,
                      t_src->getPeId(), t_src->getApp()->getId(), t_src->getId(),
		      "payload of pb" << p.pblock_id <<
		      " being transferred to local router.");
      }
#endif      
      retValue = true;
    }
    else
      break;
  }
  
  return retValue;
}

Flit PEMappedTaskExecution::_tm_nextFlit(Packet & p)
{
  Flit flit;
  double ccycle = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;

  flit.src_id = p.src_id;
  flit.dst_id = p.dst_id;
  flit.timestamp = ccycle;
  flit.sequence_no = p.size - p.flit_left;
  flit.sequence_length = p.size;
  flit.hop_no = 0;

  // These are mandatory to compute metrics
  flit.src_task = p.src_task;
  flit.dst_task = p.dst_task;
  flit.pblock_id = p.pblock_id;
  flit.path = p.path;  //added by xuanWei for path aware mapping
cout<< "In _tm_nextFlit(), set path = " << flit.path <<"set pbid ="<< flit.pblock_id << endl;
  if (p.size == p.flit_left) {
    tm_task *t_src;
    int appCurrentMapping;
    
    t_src = (tm_task *)flit.src_task;
    appCurrentMapping = t_src->getApp()->getCurrentMappingInt();

    // FLIT Statistics
    if (flit.src_id != flit.dst_id) {
      t_src->getApp()->app_mappings[appCurrentMapping].stat_txstart_delay.statAddNewValue(ccycle - p.timestamp);
    }
    
    flit.flit_type = FLIT_TYPE_HEAD;
  }
  else if (p.flit_left == 1)
    flit.flit_type = FLIT_TYPE_TAIL;
  else
    flit.flit_type = FLIT_TYPE_BODY;

  TM_ASSERT(p.flit_left > 0, "Number of unexpected FLITs!");
  p.flit_left--;
  
  return flit;
}

Flit PEMappedTaskExecution::tm_nextFlit_samepe(Packet & p) {
  Flit flit;

  flit = _tm_nextFlit(p);

  return flit;
}

Flit PEMappedTaskExecution::tm_nextFlit(queue < Packet > & packet_queue)
{
  Flit flit;
  Packet & p = packet_queue.front();

  flit = _tm_nextFlit(p);

  // PE Statistics - Flit Transmitted
  pe->stats.statIncFlitsTx();
  
  if (p.flit_left == 0)
      packet_queue.pop();

  return flit;
}

void PEMappedTaskExecution::processReceivedFlit(Flit flit) {
  tm_task *t_src, *t_dst;
  tm_pblock_task *t_st;
  tm_antecedent_task *t_at;
  int src_pblock_id;
  int appCurrentMapping;
  double ccycle = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;

  TM_ASSERT(flit.src_task && flit.dst_task, "Flit received is wrong. No source and destination Tasks received!");
  
  t_src = (tm_task *)flit.src_task;
  t_dst = (tm_task *)flit.dst_task;
  cout << "In processReceivedFlit(), t_src->getId() = " << t_src->getId() << endl;
  cout << "In processReceivedFlit(), t_dst->getId() = " << t_dst->getId() << endl;
  src_pblock_id = flit.pblock_id;
  t_st = &t_src->pblocks_tasks[src_pblock_id];
  appCurrentMapping = t_src->getApp()->getCurrentMappingInt();

  TM_ASSERT(t_dst->getApp()->getCurrentMappingInt() >= 0,
            "Current Mapping is -1 when task " << t_dst->getId() << " received data!");

  // FLIT Statistics
  if (flit.src_id != flit.dst_id) {
    t_st->stat_flit_latency.statAddNewValue(ccycle - flit.timestamp);
    t_st->stat_flit_hops.statAddNewValue(flit.hop_no);
    if (flit.flit_type == FLIT_TYPE_HEAD) {
      t_st->_stat_headflitsent = flit.timestamp;
    } 
    // PE Statistics - Flit Received
    pe->stats.statIncFlitsRx();
  }
  else {
    t_src->getApp()->app_mappings[appCurrentMapping].stat_flist_to_samepe++;
  }
  
  if (flit.flit_type == FLIT_TYPE_TAIL) {
    if (!t_dst->getApp()->alreadyStopped()) {
      bool dst_pblock_found = false;
      for (vector< tm_antecedent_task >::iterator it_at = t_dst->antecedent_tasks.begin(); it_at != t_dst->antecedent_tasks.end(); it_at++) {
        t_at = &(*it_at);
        if (t_at->getATask() == t_src && t_at->getPbId() == src_pblock_id) {
          dst_pblock_found = true;
          t_at->incNumRx();		// The packet has been received

#ifdef DEFINE_TM_SIMEXECLOG
          if (flit.src_id == flit.dst_id) {
            TM_SIMEXECLOG(TM_SIMEXECLOG_RECEIVESPLFROMSAMEPE,
                          pe->getId(), t_dst->getApp()->getId(), t_dst->getId(),
                          "receives payload of pb" << src_pblock_id << " (task" << t_src->getId() << ") from same PE.");
          }
          else {
            TM_SIMEXECLOG(TM_SIMEXECLOG_RECEIVESPLFROMWNOC,
                          pe->getId(), t_dst->getApp()->getId(), t_dst->getId(),
                          "receives payload of pb" << src_pblock_id << " (task" << t_src->getId() << ") from WNoC.");
          }
#endif      

          // Check the tasks in Waiting Tx to move the ready ones to ready list
          if (t_dst->isInWaitingRxState()){
            cout << "In processReceivedFlit(), t_dst isInWaitingRxState, " << "t_dst->getPeId() = " << t_dst->getPeId() << endl;
            cout << "In processReceivedFlit(), waitingrx_tasks.size() = " << waitingrx_tasks.size() << endl;
            cout << "pe->getId() = " << pe->getId() << endl;
            cout << endl;
            tasksInWaitingRxState();
          }

          if (flit.src_id != flit.dst_id) {
            TM_SIMEND2ENDLATENCYLOG(TM_SIMSTATSLOG_STATEND2ENDLATENCY, 
                          t_dst->getApp()->getId(), t_src->getId(), t_dst->getId(), 
                             "flits=" << t_st->stat_flit_latency.statGetExecNumber()
                          << ",min=" << t_st->stat_flit_latency.statGetMin()
                          << ",avg=" << t_st->stat_flit_latency.statGetAverage()
                          << ",max=" << t_st->stat_flit_latency.statGetMax());
            TM_SIMEND2ENDHOPSLOG(TM_SIMSTATSLOG_STATEND2ENDHOPCOUNT, 
                          t_dst->getApp()->getId(), t_src->getId(), t_dst->getId(), 
                             "flits=" << t_st->stat_flit_hops.statGetExecNumber()
                          << ",min=" << t_st->stat_flit_hops.statGetMin()
                          << ",avg=" << t_st->stat_flit_hops.statGetAverage()
                          << ",max=" << t_st->stat_flit_hops.statGetMax());

            // TOTAL Statistics
            int_cycles start;
            TM_ASSERT((start = t_st->_stat_headflitsent) > 0, "Unexpected behavior in the Flits!");
            t_st->total_stat_commtime[appCurrentMapping].statAddNewValue(ccycle - start);

            cout << "DEBUG ccycle: " << ccycle << ", start: " << start << endl;
            

            t_src->getApp()->app_mappings[appCurrentMapping].stat_end2end_latency.statAddNewValue(t_st->stat_flit_latency);
            t_src->getApp()->app_mappings[appCurrentMapping].stat_hopcount.statAddNewValue(t_st->stat_flit_hops);
          }
          break;
        }
      } // End for
      if (!dst_pblock_found) {
        TM_ASSERT(false, "There is a packet received at PE " << 
                        t_dst->getPeId() << " that was not taken by any pb!");
      }  
    }
    // Task already stopped
    else {
      t_src->getApp()->app_mappings[appCurrentMapping].stat_flits_ignored += flit.sequence_length;
      TM_SIMEXECLOG(TM_SIMEXECLOG_DESTNOTREACHEABLE,
                    pe->getId(), t_dst->getApp()->getId(), t_dst->getId(),
                    "Destination task already stopped. Payload ignored!");
    }
    // Clean statistics
    t_st->stat_flit_latency.reset();
    t_st->stat_flit_hops.reset();
    t_st->_stat_headflitsent = 0;
  } // FLIT_TYPE_TAIL
}

int PEMappedTaskExecution::getPeId(){
  return pe->getId();
}
