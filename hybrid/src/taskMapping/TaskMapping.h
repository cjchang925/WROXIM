/*
 * File added by LGGM.
 *
 * This file contains the declaration of the task mapping simulation engine
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

#ifndef __NOXIM_TASK_MAPPING__
#define __NOXIM_TASK_MAPPING__

#include <stdint.h>
#include <time.h>

#include <iomanip>
#include <map>
#include <queue>
#include <vector>

#include "../DataStructs.h"
#include "TaskMappingLogs.h"
#include "TaskMappingStats.h"

#define TM_NOT_APP -1
#define TM_NOT_ROOT -1
#define TM_NOT_TASK -1
#define TM_NOT_PE -1
#define TM_START_PBLOCK -1
#define TM_START_MAPPING -1

#define TM_APPID_MIN 0
#define TM_TASKID_MIN 0
#define TM_PEID_MIN 0
#define TM_TIMERESTART_MIN 0
#define TM_EXECTIME_MIN 0
#define TM_PAYLOAD_MIN 2 // min: header and tail flits
#define TM_MAPSTART_MIN 0

#define TM_MFILE_SEP ','
#define TM_MFILE_STRAPP "app:"
#define TM_MFILE_STRTASK "task:"
#define TM_MFILE_STRMAP "map:"
#define TM_MFILE_STRSIM "sim:"

// Timing and Delays (to be included in the mapping file)
#define TM_PE_SCHEDULER_TICK 100
#define TM_PE_SCHEDULER_DELAY 10
#define TM_PE_SCHEDULER_TXSTART 1
#define TM_PE_SCHEDULER_MAPBEFORE 10
#define TM_PE_SCHEDULER_MAPPINGGAP 12
#define TM_PE_SCHEDULER_TICK_MIN 20
#define TM_PE_SCHEDULER_DELAY_MIN 1
#define TM_PE_SCHEDULER_MAPBEFORE_MIN 0
#define TM_PE_SCHEDULER_MAPPINGGAP_MIN 10

typedef int32_t int_cycles;

enum tm_mfile_app_fsm {
  FSMAPP_APPID,
  FSMAPP_MINRESTART,
  FSMAPP_IGNORE,
};
enum tm_mfile_task_fsm {
  FSMTASK_TASKID,
  FSMTASK_PBLOCK_ET,
  FSMTASK_PBLOCK_ST,
  FSMTASK_PBLOCK_PL,
  FSMTASK_PATH
};
enum tm_mfile_map_fsm { FSMMAP_START, FSMMAP_STOP, FSMMAP_TASKID, FSMMAP_PEID };
enum tm_mfile_sim_fsm {
  FSMSIM_SCHEDULER_TICK,
  FSMSIM_SCHEDULER_DELAY,
  FSMSIM_SCHEDULER_MAPBEFORE,
  FSMSIM_SCHEDULER_MAPPINGGAP,
  FSMSIM_SCHEDULER_IGNORE
};

enum tm_task_execstates {
  TM_TASK_EXECSTATE_UNMAPPED = 0,
  TM_TASK_EXECSTATE_BEINGMAPPED,
  TM_TASK_EXECSTATE_WAITINGRX,
  TM_TASK_EXECSTATE_SLEEPING,
  TM_TASK_EXECSTATE_READY,
  TM_TASK_EXECSTATE_SWITCHINGCONTEXT,
  TM_TASK_EXECSTATE_RUNNING
};

using namespace std;

/************************************************
 * Classes to store the information of each app *
 ************************************************/
class tm_task;
class tm_appmapping {
 private:
  int_cycles _start; // time when the root task should start
  int_cycles _stop;  // time when the root task should stop

  // Statistics
  queue<int_cycles> _stat_startx;
  bool _stat_taskstarted;

 public:
  map<int, int> task_on_pe; // Task on which PE? Task id, pe id

  // Statistics
  tm_appexec_statistics<int_cycles> stat_exectime;
  tm_appexec_statistics<int_cycles> stat_commtime;
  tm_appexec_statistics<int_cycles> stat_end2end_latency;
  tm_appexec_statistics<int_cycles> stat_hopcount;
  tm_appexec_statistics<int_cycles> stat_txstart_delay;
  int stat_flits_ignored;
  int stat_flist_to_samepe;

  // Constructor
  tm_appmapping() {
    _start = 0;
    _stop = 0;
    task_on_pe.clear();

    _stat_startx = queue<int_cycles>();
    _stat_taskstarted = false;

    stat_flits_ignored = 0;
    stat_flist_to_samepe = 0;
  }

  inline int_cycles getStartTime(void) { return _start; }
  inline void setStartTime(int_cycles start) { this->_start = start; }
  inline int_cycles getStopTime(void) { return _stop; }
  inline void setStopTime(int_cycles stop) { this->_stop = stop; }

  void statRootExecStarted(void);
  inline void statRootExecFinished(void) { _stat_taskstarted = false; }
  void statLeafExecFinished(tm_task *t_leaf);
};

class tm_app {
 private:
  int _id;          // the app id is also stored as a key in the map object
  int _root_taskid; // id of the root task
  int _last_taskid; // id of the last task added
  int_cycles
      _min_restart; // minimum time between the last pblock and the first one
  int _leaf_taskid; // id of the leaf task

  // Mapping
  int _map_currently_running;
  bool _currently_mapped;
  int_cycles _start; // time when the root task should start
  int_cycles _stop;  // time when the root task should stop

 public:
  map<int, tm_task> task_graph; // KEY: task id, T: tm_task struct

  // Mapping
  vector<tm_appmapping> app_mappings;

  // Structure constructor
  tm_app() {
    _id = TM_NOT_APP;
    _root_taskid = TM_NOT_TASK;
    _last_taskid = TM_NOT_TASK;
    _min_restart = 0;
    _leaf_taskid = TM_NOT_TASK;
    task_graph.clear();

    _map_currently_running = TM_START_MAPPING;
    _currently_mapped = false;
    _start = 0;
    _stop = 0;
    app_mappings.clear();
  }

  inline int getId(void) { return _id; }
  inline void setId(int id) { this->_id = id; }
  inline int getRootId(void) { return _root_taskid; }
  inline void setRootId(int root_taskid) { this->_root_taskid = root_taskid; }
  // added by Xuan
  inline int getLastId(void) { return _last_taskid; }
  inline void setLastId(int last_taskid) { this->_last_taskid = last_taskid; }

  inline int_cycles getMinRestart(void) { return _min_restart; }
  inline void setMinRestart(int_cycles min_restart) {
    this->_min_restart = min_restart;
  }
  inline int getLeafId(void) { return _leaf_taskid; }
  inline void setLeafId(int leaf_taskid) { this->_leaf_taskid = leaf_taskid; }

  inline int getMapCurrentlyRunning(void) { return _map_currently_running; }
  inline void setMapCurrentlyRunning(int map_currently_running) {
    this->_map_currently_running = map_currently_running;
  }
  inline bool getCurrentlyMapped(void) { return _currently_mapped; }
  inline void setCurrentlyMapped(bool currently_mapped) {
    this->_currently_mapped = currently_mapped;
  }
  inline int_cycles getStartTime(void) { return _start; }
  inline void setStartTime(int_cycles start) { this->_start = start; }
  inline int_cycles getStopTime(void) { return _stop; }
  inline void setStopTime(int_cycles stop) { this->_stop = stop; }

  void appBeingMapped(int_cycles start, int_cycles stop);
  void appBeingUnmapped();
  inline tm_appexec_statistics<int_cycles> *getCurrentStatsExecTime(void) {
    return (&app_mappings[_map_currently_running].stat_exectime);
  }
  inline tm_appexec_statistics<int> *getCurrentStatsCommTime(void) {
    return (&app_mappings[_map_currently_running].stat_commtime);
  }
  inline tm_appexec_statistics<int> *getCurrentStatsEnd2EndLatency(void) {
    return (&app_mappings[_map_currently_running].stat_end2end_latency);
  }
  inline tm_appexec_statistics<int> *getCurrentStatsHopCount(void) {
    return (&app_mappings[_map_currently_running].stat_hopcount);
  }
  inline tm_appmapping *getCurrentMapping(void) {
    return (&app_mappings[_map_currently_running]);
  }
  inline int getCurrentMappingInt(void) { return _map_currently_running; }
  void printSubsequentCommTimePerBranch(
      int imap, tm_task *t, string initss = "",
      tm_appexec_statistics<int> initstat = tm_appexec_statistics<int>());
  void printStats(tm_appexec_statistics<int_cycles> &stat_exectime,
                  tm_appexec_statistics<int_cycles> &stat_commtime,
                  tm_appexec_statistics<int_cycles> &stat_txstart_delay,
                  tm_appexec_statistics<int_cycles> &stat_end2end_latency,
                  tm_appexec_statistics<int_cycles> &stat_hopcount,
                  int &stat_flits_ignored, int &stat_flits_to_samepe);
  bool alreadyStopped();
};

/*************************************************
 * Classes to store the information of each task *
 *************************************************/
class tm_pblock_task {
 private:
  int_cycles _exectime; // time in cycles for this block after which the payload
                        // is sent to the subsequent task
  int _staskid; // task ID of the subsequent task, it could be -1 (No Task
                // Associated)
  int _payload; // packets to be sent to the subsequent task, it could be 0 (No
                // payload)
  int _path;    // add by xuemei for path aware mapping

 public:
  // Statistics
  int_cycles _stat_headflitsent;
  map<int, tm_appexec_statistics<int> >
      total_stat_commtime;                      // mapping ID, statistics
  tm_appexec_statistics<int> stat_flit_latency; // mapping ID, statistics
  tm_appexec_statistics<int> stat_flit_hops;    // mapping ID, statistics

  tm_pblock_task() {
    _exectime = 0;
    _staskid = TM_NOT_TASK;
    _payload = 0;
    _path = 0;
    _stat_headflitsent = 0;
  }

  inline int_cycles getExecTime(void) { return _exectime; }
  inline void setExecTime(int_cycles exectime) { this->_exectime = exectime; }
  inline void setPATH(int path) { this->_path = path; }
  inline int getSTaskId(void) { return _staskid; }
  inline int getPath(void) { return _path; }
  inline void setSTaskId(int staskid) { this->_staskid = staskid; }
  inline int getPayLoad(void) { return _payload; }
  inline void setPayLoad(int payload) { this->_payload = payload; }
};

class tm_antecedent_task {
 private:
  int _pb;      // index of the pblock in the antecedent task
  tm_task *_at; // Pointer to the antecedent task

  int _num_rx; // Number of packets received without being processed

 public:
  tm_antecedent_task() {
    _pb = 0;
    _at = NULL;
    _num_rx = 0;
  }

  inline int getPbId(void) { return _pb; }
  inline void setPbId(int pb) { this->_pb = pb; }
  inline tm_task *getATask(void) { return _at; }
  inline void setATask(tm_task *at) { this->_at = at; }
  inline int getNumRx(void) { return _num_rx; }
  inline void incNumRx(void) { _num_rx++; }
  inline void decNumRx(void) { _num_rx--; }
  inline void resetNumRx(void) { _num_rx = 0; }
};

class tm_task {
 private:
  int _id;          // the task id is also stored as a key in the map object
  int _pblock_path; // added by xuanWei for path aware mapping

  tm_app *_ptr_parentapp; // pointer to the parent application
  int _peid_container;    // Container PE id

  int _currentPBlock;        // block being currently executed
  int_cycles _exectime_left; // execution time left in the execution of the
                             // current processing block
  tm_task_execstates
      _execution_state; // indicates the current execution state of the task

 public:
  vector<tm_pblock_task> pblocks_tasks; // vector of processing blocks of each
                                        // task (related to subsequent tasks)
  vector<tm_antecedent_task>
      antecedent_tasks; // vector of antecedent tasks (pointer to
                        // tm_antecedent_task struct)

  tm_task() {
    _id = TM_NOT_TASK;
    pblocks_tasks.clear();
    antecedent_tasks.clear();

    _ptr_parentapp = NULL;
    _peid_container = TM_NOT_PE;

    _currentPBlock = TM_START_PBLOCK;
    _exectime_left = 0;
    _execution_state = TM_TASK_EXECSTATE_UNMAPPED;
    _pblock_path = 0;
  }

  inline void setPath(int path) { this->_pblock_path = path; }
  inline int getPath(void) { return _pblock_path; }

  inline int getId(void) { return _id; }
  inline void setId(int id) { this->_id = id; }
  inline tm_app *getApp(void) { return _ptr_parentapp; }
  inline void setApp(tm_app *ptr_parentapp) {
    this->_ptr_parentapp = ptr_parentapp;
  }
  inline int getPeId(void) { return _peid_container; }
  inline void setPeId(int peid_container) {
    this->_peid_container = peid_container;
  }
  inline int getCurrentPb(void) { return _currentPBlock; }
  inline void setCurrentPb(int currentPBlock) {
    this->_currentPBlock = currentPBlock;
  }
  inline void incCurrentPb(void) { this->_currentPBlock++; }
  inline void resetCurrentPb(void) { this->_currentPBlock = 0; }
  inline int_cycles getExectimeLeft(void) { return _exectime_left; }
  inline void setExectimeLeft(int_cycles exectime_left) {
    this->_exectime_left = exectime_left;
  }
  inline tm_task_execstates getExecState(void) { return _execution_state; }
  inline void setExecState(tm_task_execstates execution_state) {
    this->_execution_state = execution_state;
  }

  inline bool isInBeingMappedState() {
    return (_execution_state == TM_TASK_EXECSTATE_BEINGMAPPED);
  }
  void toBeingMappedState(int peid_container, int_cycles start,
                          int_cycles stop);
  inline bool isInWaitingRxState() {
    return (_execution_state == TM_TASK_EXECSTATE_WAITINGRX);
  }
  void toWaitingRxState();
  inline bool isInSleepingState() {
    return (_execution_state == TM_TASK_EXECSTATE_SLEEPING);
  }
  void toSleepingState();
  inline bool isInReadyState() {
    return (_execution_state == TM_TASK_EXECSTATE_READY);
  }
  void toReadyState();
  void toReadyStateDueToContextSwitching();
  inline bool isInContextSwitchingState() {
    return (_execution_state == TM_TASK_EXECSTATE_SWITCHINGCONTEXT);
  };
  void toContextSwitchingState();
  inline bool isInRunningState() {
    return (_execution_state == TM_TASK_EXECSTATE_RUNNING);
  }
  void toRunningState();
  void toStopAndUnmapState();
  void taskExecutionRunning();
  void taskExecutionCompleted();
  tm_appexec_statistics<int> getTotalLatencyUptoThis(int imap);

  inline bool isRootTask() { return (_id == _ptr_parentapp->getRootId()); }
  inline bool isLeafTask() { return (_id == _ptr_parentapp->getLeafId()); }
};

/**************************************************
 * Structures to store the information of each PE *
 **************************************************/
class tm_mappingonpe {
 private:
  int_cycles _stop; // time when the unmapping must be performed

 public:
  vector<tm_task *> tasks_on_pe; // Task in the PE

  tm_mappingonpe() {
    _stop = 0;
    tasks_on_pe.clear();
  }

  inline int_cycles getStopTime(void) { return _stop; }
  inline void setStopTime(int_cycles stop) { this->_stop = stop; }
};

/*
 * Structure to store the PE information
 */
class tm_pe {
 private:
  int _id; // This id is store as a KEY of the object

 public:
  map<int_cycles, tm_mappingonpe>
      mappingsonpe; // KEY: time event (start), T: structure tm_mappingonpe
  map<int_cycles, tm_mappingonpe>::iterator currentmapping; // iterator

  // Statistics
  tm_peexec_statistics<int_cycles> stats;

  tm_pe() {
    _id = TM_NOT_PE;
    mappingsonpe.clear();
    currentmapping = mappingsonpe.end();
  }

  inline int getId(void) { return _id; }
  inline void setId(int id) { this->_id = id; }
  void printStats(void);
};

// General class for this module: TaskMapping
class TaskMapping {
 private:
  map<int, tm_app> tm_apps; // KEY: App id, T: tm_app struct
  map<int, tm_pe> tm_pes;   // KEY: PE id, T: tm_pe struct
  string objectName;

  int_cycles scheduler_tick, scheduler_delay, scheduler_tx_delay;
  int_cycles scheduler_mapbefore, scheduler_mappinggap;
  friend class GraphParser;

  void addApp(int appid, int_cycles min_restart);
  void addTask(int appid, int taskid);
  void addPBlockToTask(int appid, int taskid, int_cycles exectime, int staskid,
                       int payload, int path);
  void addMap(int appid, int_cycles start, int_cycles stop);
  void addTaskToMap(int appid, int taskid, int peid);
  void deleteCurrentMapping(void);
  void settleRootAndLeafTasks(void);
  void settleAntecedentTasks(void);
  bool checkConsistency(void);
  void performMappingOnPEs(void);
  void showAppInfo(tm_app *app);
  void showTasksPerAppInfo(tm_task *t);
  void showMappingsPerAppInfo(tm_app *app);
  // void showAllAppsAndTasksInfo(void);
  void showMappedTasksPerPEInfo(tm_pe *pe);
  void showAllMappedTasksOnPEsInfo(void);
  void showAppStatistics(void);
  void showPEStatistics(void);
  void showPESummaryStatistics(void);

 public:
  ofstream taskmappinglog_file;
  void showAllAppsAndTasksInfo(void);

  TaskMapping(string objectName) {
    tm_apps.clear();
    tm_pes.clear();

    this->objectName = objectName;

    scheduler_tick = TM_PE_SCHEDULER_TICK;
    scheduler_delay = TM_PE_SCHEDULER_DELAY;
    scheduler_tx_delay = TM_PE_SCHEDULER_TXSTART;
    scheduler_mapbefore = TM_PE_SCHEDULER_MAPBEFORE;
    scheduler_mappinggap = TM_PE_SCHEDULER_MAPPINGGAP;

    if (strcmp(GlobalParams::taskmappinglog_filename.c_str(), TM_NOT_FILE)) {
      taskmappinglog_file.open(GlobalParams::taskmappinglog_filename.c_str(),
                               ofstream::out);
      if (!taskmappinglog_file.is_open()) {
        cerr << "The TaskMapping Log file "
             << GlobalParams::taskmappinglog_filename
             << " could not be created!" << endl;
        exit(0);
      } else {
        time_t rawtime;
        struct tm *timeinfo;

        cout << "Task Mapping Log file is: \""
             << GlobalParams::taskmappinglog_filename << "\"" << endl;
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        taskmappinglog_file << "Task Mapping Simulation File created on: "
                            << asctime(timeinfo);
      }
    }
  }

  ~TaskMapping() {
    if (taskmappinglog_file.is_open()) {
      taskmappinglog_file.close();
    }
  }

  bool createTaskGraphsFromFile(const char *fname);
  tm_app *getApp(int appid);
  tm_task *getTask(int appid, int taskid);
  tm_pe *getPE(int peid);
  int_cycles getSchedulerTick();
  int_cycles getSchedulerDelay();
  int_cycles getSchedulerTxDelay();
  int_cycles getSchedulerMapBefore();
  int_cycles getSchedulerMappingGAP();

  void showTaskMappingConfiguration(void);
  void showTaskMappingStatistics(void);

  inline string name() { return objectName; }
};

extern TaskMapping *tmInstance;

#endif // __NOXIM_TASK_MAPPING__