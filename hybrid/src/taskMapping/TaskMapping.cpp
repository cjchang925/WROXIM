/*
 * File added by LGGM.
 *
 * This file contains the functions of the task mapping simulation engine
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

#include "TaskMapping.h"

#include <time.h>

#include <cstdlib>
#include <sstream>
#include <string>

#include "../GlobalParams.h"

using namespace std;

/***************************************************************************************
 * tm_appmapping Class Functions
 ***************************************************************************************/

void tm_appmapping::statRootExecStarted(void) {
  double ccycle = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;

  if (_stat_taskstarted == false) {
    _stat_startx.push(ccycle);
    _stat_taskstarted = true;
  }
}
void tm_appmapping::statLeafExecFinished(tm_task *t_leaf) {
  int_cycles finish =
      (int_cycles)(sc_time_stamp().to_double() / GlobalParams::clock_period_ps);
  int_cycles start, exectime;

  TM_ASSERT(!_stat_startx.empty(),
            "App has finished one execution but does not have starting time!");
  start = _stat_startx.front();
  _stat_startx.pop();
  exectime = finish - start;

  stat_exectime.statAddNewValue(exectime);
  stat_commtime =
      t_leaf->getTotalLatencyUptoThis(t_leaf->getApp()->getCurrentMappingInt());
}

/***************************************************************************************
 * tm_app Class Functions
 ***************************************************************************************/

void tm_app::appBeingMapped(int_cycles start, int_cycles stop) {
  if (!_currently_mapped) {
    this->_start = start;
    this->_stop = stop;
    _map_currently_running++;
    _currently_mapped = true;
  }
}

void tm_app::appBeingUnmapped() { _currently_mapped = false; }

bool tm_app::alreadyStopped() {
  double ccycle = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;

  return (ccycle >= _stop);
}

void tm_app::printSubsequentCommTimePerBranch(
    int imap, tm_task *t, string initss, tm_appexec_statistics<int> initstat) {
  ostringstream ssfromthis;
  tm_appexec_statistics<int> statfromthis;
  tm_task *t_st;
  static int currentBranch = 0;

  if (initss.empty()) {
    currentBranch = 0;
  }

  for (unsigned int ind_pb = 0; ind_pb < t->pblocks_tasks.size(); ++ind_pb) {
    ssfromthis.str("");
    statfromthis = tm_appexec_statistics<int>();

    t_st = &task_graph[t->pblocks_tasks[ind_pb].getSTaskId()];
    ssfromthis
        << t->getId() << "-" << t_st->getId() << "="
        << t->pblocks_tasks[ind_pb]
               .total_stat_commtime[imap]
               .statGetExecNumber()
        << ","
        << t->pblocks_tasks[ind_pb].total_stat_commtime[imap].statGetMin()
        << ","
        << (int)t->pblocks_tasks[ind_pb]
               .total_stat_commtime[imap]
               .statGetAverage()
        << ","
        << t->pblocks_tasks[ind_pb].total_stat_commtime[imap].statGetMax();
    statfromthis = t->pblocks_tasks[ind_pb].total_stat_commtime[imap];

    if (!t_st->isLeafTask()) {
      ssfromthis << " -> ";
      printSubsequentCommTimePerBranch(imap, t_st, initss + ssfromthis.str(),
                                       initstat + statfromthis);
    } else {
      tm_appexec_statistics<int> totalstat = initstat + statfromthis;
      ssfromthis << " -->> T(min,avg,max)="
                 // << totalstat.statGetExecNumber() << ","  // real payloads
                 // transmitted in the Branch from root to leaf
                 << totalstat.statGetMin() << ","
                 << (int)totalstat.statGetAverage() << ","
                 << totalstat.statGetMax();
      TM_SIMSTATSLOG("  br" << currentBranch << ": " << initss
                            << ssfromthis.str() << " (cycles)" << endl);
      currentBranch++;
    }
  }
}

void tm_app::printStats(tm_appexec_statistics<int_cycles> &stat_exectime,
                        tm_appexec_statistics<int_cycles> &stat_commtime,
                        tm_appexec_statistics<int_cycles> &stat_txstart_delay,
                        tm_appexec_statistics<int_cycles> &stat_end2end_latency,
                        tm_appexec_statistics<int_cycles> &stat_hopcount,
                        int &stat_flits_ignored, int &stat_flits_to_samepe) {
  for (unsigned int imap = 0; imap < app_mappings.size(); imap++) {
    stat_exectime.statAddNewValue(app_mappings[imap].stat_exectime);
    TM_SIMSTATSLOG(
        "*** App" << _id << " - Mapping" << imap << " ***" << endl
                  << "Execution Time (CPU + Comm.) (executions,min,avg,max)="
                  << app_mappings[imap].stat_exectime.statGetExecNumber() << ","
                  << app_mappings[imap].stat_exectime.statGetMin() << ","
                  << (int)app_mappings[imap].stat_exectime.statGetAverage()
                  << "," << app_mappings[imap].stat_exectime.statGetMax()
                  << " (cycles)" << endl);
    TM_SIMSTATSLOG("Communication Time per Branch (payloads,min,avg,max)"
                   << endl);
    printSubsequentCommTimePerBranch(imap, &task_graph[_root_taskid]);
    stat_commtime.statAddNewValue(app_mappings[imap].stat_commtime);
    TM_SIMSTATSLOG(
        "Communication Time Critical Branch (min,avg,max)="
        // << app_mappings[imap].stat_commtime.statGetExecNumber() << "," //
        // real payloads transmitted in the Branch from root to leaf
        << app_mappings[imap].stat_commtime.statGetMin() << ","
        << (int)app_mappings[imap].stat_commtime.statGetAverage() << ","
        << app_mappings[imap].stat_commtime.statGetMax() << " (cycles)"
        << endl);
    stat_txstart_delay.statAddNewValue(app_mappings[imap].stat_txstart_delay);
    TM_SIMSTATSLOG(
        "Transmission Start Delay (head_flits_tx,min,avg,max)="
        << app_mappings[imap].stat_txstart_delay.statGetExecNumber()
        << "," // real payloads transmitted in the Branch from root to leaf
        << app_mappings[imap].stat_txstart_delay.statGetMin() << ","
        << (int)app_mappings[imap].stat_txstart_delay.statGetAverage() << ","
        << app_mappings[imap].stat_txstart_delay.statGetMax() << " (cycles)"
        << endl);
    stat_flits_to_samepe += app_mappings[imap].stat_flist_to_samepe;
    TM_SIMSTATSLOG("FLITs Transmitted to Same PE="
                   << app_mappings[imap].stat_flist_to_samepe << endl);
    stat_flits_ignored += app_mappings[imap].stat_flits_ignored;
    TM_SIMSTATSLOG("FLITs Ignored due to Application Unmapping="
                   << app_mappings[imap].stat_flits_ignored << endl);
    stat_end2end_latency.statAddNewValue(
        app_mappings[imap].stat_end2end_latency);
    TM_SIMSTATSLOG(
        "FLIT End2End Latency (flits,min,avg,max)="
        << app_mappings[imap].stat_end2end_latency.statGetExecNumber() << ","
        << app_mappings[imap].stat_end2end_latency.statGetMin() << ","
        << app_mappings[imap].stat_end2end_latency.statGetAverage() << ","
        << app_mappings[imap].stat_end2end_latency.statGetMax() << " (cycles)"
        << endl);
    stat_hopcount.statAddNewValue(app_mappings[imap].stat_hopcount);
    TM_SIMSTATSLOG("FLIT End2End HopCount (flits,min,avg,max)="
                   << app_mappings[imap].stat_hopcount.statGetExecNumber()
                   << "," << app_mappings[imap].stat_hopcount.statGetMin()
                   << "," << app_mappings[imap].stat_hopcount.statGetAverage()
                   << "," << app_mappings[imap].stat_hopcount.statGetMax()
                   << endl);
    TM_SIMSTATSLOG("FLIT Communication Cost (flits x average hopcount)="
                   << app_mappings[imap].stat_hopcount.statGetExecNumber() *
                          app_mappings[imap].stat_hopcount.statGetAverage()
                   << endl);
    TM_SIMSTATSLOG(endl);
  }
}

/***************************************************************************************
 * tm_task Class Functions
 ***************************************************************************************/

void tm_task::toBeingMappedState(int peid_container, int_cycles start,
                                 int_cycles stop) {
  _ptr_parentapp->appBeingMapped(start, stop);
  _peid_container = peid_container;
  _execution_state = TM_TASK_EXECSTATE_BEINGMAPPED;

  TM_SIMEXECLOG(TM_SIMEXECLOG_BEINGMAPPEDSTATE, _peid_container,
                _ptr_parentapp->getId(), _id,
                "being mapped to pe" << peid_container << ".");
}

void tm_task::toWaitingRxState() {
  _execution_state = TM_TASK_EXECSTATE_WAITINGRX;
  TM_SIMEXECLOG(TM_SIMEXECLOG_WAITINGRXSTATE, _peid_container,
                _ptr_parentapp->getId(), _id, "in waiting rx state.");
}

void tm_task::toSleepingState() {
  _execution_state = TM_TASK_EXECSTATE_SLEEPING;
  TM_SIMEXECLOG(TM_SIMEXECLOG_SLEEPINGSTATE, _peid_container,
                _ptr_parentapp->getId(), _id, "in sleeping state.");
}

void tm_task::toReadyState() {
  _execution_state = TM_TASK_EXECSTATE_READY;
  TM_SIMEXECLOG(TM_SIMEXECLOG_READYSTATE, _peid_container,
                _ptr_parentapp->getId(), _id, "in ready state.");
}

void tm_task::toReadyStateDueToContextSwitching() {
  _execution_state = TM_TASK_EXECSTATE_READY;
  TM_SIMEXECLOG(TM_SIMEXECLOG_READYSTATEDUETOCONTEXTSWITCHING, _peid_container,
                _ptr_parentapp->getId(), _id,
                "in ready state due to context switching.");
}

void tm_task::toContextSwitchingState() {
  _execution_state = TM_TASK_EXECSTATE_SWITCHINGCONTEXT;
  TM_SIMEXECLOG(TM_SIMEXECLOG_CONTEXTSWITCHINGSTATE, _peid_container,
                _ptr_parentapp->getId(), _id, "in context switching state.");
}

void tm_task::toRunningState() {
  taskExecutionRunning();
  _execution_state = TM_TASK_EXECSTATE_RUNNING;
  TM_SIMEXECLOG(TM_SIMEXECLOG_RUNNINGSTATE, _peid_container,
                _ptr_parentapp->getId(), _id, "in running state.");
}

void tm_task::toStopAndUnmapState() {
  double ccycle = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;

  _ptr_parentapp->appBeingUnmapped();

  _currentPBlock = TM_START_PBLOCK;
  this->_exectime_left = 0;
  for (unsigned int i = 0; i < antecedent_tasks.size(); i++) {
    antecedent_tasks[i].resetNumRx();
  }
  _execution_state = TM_TASK_EXECSTATE_UNMAPPED;
  if (isRootTask()) {
    TM_SIMEXECLOG(TM_SIMEXECLOG_STOPSANDUNMAPPED, _peid_container,
                  _ptr_parentapp->getId(), _id,
                  "stops and is unmapped at @" << ccycle << " (expected at @"
                                               << _ptr_parentapp->getStopTime()
                                               << ").");
    _ptr_parentapp->setStopTime(ccycle);
  } else {
    TM_SIMEXECLOG(TM_SIMEXECLOG_STOPSANDUNMAPPED, _peid_container,
                  _ptr_parentapp->getId(), _id,
                  "stops and is unmapped at @"
                      << ccycle << " (root stopped at @"
                      << _ptr_parentapp->getStopTime() << ").");
  }
  _peid_container = TM_NOT_PE;
}

void tm_task::taskExecutionRunning() {
  // Statistics
  if (isRootTask()) {
    _ptr_parentapp->getCurrentMapping()->statRootExecStarted();
  }
}

void tm_task::taskExecutionCompleted() {
  // Statistics
  if (isRootTask()) {
    _ptr_parentapp->getCurrentMapping()->statRootExecFinished();
  } else if (isLeafTask()) {
    _ptr_parentapp->getCurrentMapping()->statLeafExecFinished(this);

    TM_SIMAPPMETRICSLOG(TM_SIMSTATSLOG_STATAPPEXECTIME, _ptr_parentapp->getId(),
                        "exec_finished",
                        "exectime=" << _ptr_parentapp->getCurrentStatsExecTime()
                                           ->statGetLast());
  }
}

tm_appexec_statistics<int> tm_task::getTotalLatencyUptoThis(int imap) {
  tm_pblock_task *t_st;
  tm_appexec_statistics<int> _stat;

  for (vector<tm_antecedent_task>::iterator it_at = antecedent_tasks.begin();
       it_at != antecedent_tasks.end(); it_at++) {
    tm_appexec_statistics<int> _stat_branch;
    t_st = &(*it_at).getATask()->pblocks_tasks[(*it_at).getPbId()];

    _stat_branch = t_st->total_stat_commtime[imap] +
                   (*it_at).getATask()->getTotalLatencyUptoThis(imap);

    if (_stat.statGetMin() < _stat_branch.statGetMin()) {
      _stat = _stat_branch;
    }
  }

  return (_stat);
}

/***************************************************************************************
 * tm_pe Class Functions
 ***************************************************************************************/

void tm_pe::printStats(void) {
  int_cycles start = 0, stop = 0, acum_expected = 0;

  for (map<int_cycles, tm_mappingonpe>::iterator it = mappingsonpe.begin();
       it != mappingsonpe.end(); ++it) {
    if ((it->first + tmInstance->getSchedulerMapBefore()) > stop) {
      acum_expected += (stop - start);
      start = (it->first - tmInstance->getSchedulerMapBefore());
      stop = it->second.getStopTime();
    } else if (it->second.getStopTime() > stop) {
      stop = it->second.getStopTime();
    }
  }
  acum_expected += (stop - start);

  // Compute total cycles and energy consumption
  stats.statComputePeTotalCyclesAndEnergy();

  TM_SIMSTATSLOG("*** pe: " << _id << " ***" << endl);
  TM_SIMSTATSLOG(
      "Total FLITs Transmitted="
      << stats.statGetFlitsTx() << endl
      << "Total FLITs Received=" << stats.statGetFlitsRx() << endl
      << "Total Context Switching Cycles=" << stats.statGetCSwitchingCycles()
      << endl
      << "Total Task Execution Cycles=" << stats.statGetTskExecCycles() << endl
      << "Total PE Idle Cycles=" << stats.statGetIdleCycles() << endl
      << "Total Mapped Task Cycles=" << stats.statGetPeTotalCycles() << endl
      << "Total Mapped Task Cycles (expected)=" << acum_expected << endl
      << "Total Energy (J)=" << stats.statGetTotalEnergy_d() << endl
      << "      Context Switching Energy (J)="
      << stats.statGetCSwitchingEnergy_d() << endl
      << "      Task Execution Energy (J)=" << stats.statGetTskExecEnergy_d()
      << endl
      << "      Idle Energy (J)=" << stats.statGetIdleEnergy_d() << endl);
  TM_SIMSTATSLOG(endl);
}

/***************************************************************************************
 * TaskMapping Class Functions
 ***************************************************************************************/

TaskMapping *tmInstance = NULL;

/*
 * Private functions
 */

void TaskMapping::addApp(int appid, int_cycles min_restart) {
  tm_app app;

  TM_ASSERT(tm_apps.count(appid) == 0, "App" << appid << " already added!");

  app.setId(appid);
  app.setMinRestart(min_restart);

  // App will be added without any task. To do so, use the addTask function
  tm_apps[appid] = app;
}

void TaskMapping::addTask(int appid, int taskid) {
  tm_app *app;
  tm_task t;

  TM_ASSERT(tm_apps.count(appid) > 0,
            "App" << appid << " has not been added yet!");
  app = &tm_apps[appid];
  TM_ASSERT(app->task_graph.count(taskid) == 0,
            "Task" << taskid << " already added!");

  t.setId(taskid);
  t.setApp(app);
  t.setCurrentPb(TM_START_PBLOCK);

  // Task will be added without any processing block. To add pbs, use the
  // addPBlockToTask function
  app->task_graph[taskid] = t;
}

void TaskMapping::addPBlockToTask(int appid, int taskid, int_cycles exectime,
                                  int staskid, int payload, int path) {
  tm_app *app;
  tm_task *t;
  tm_pblock_task p;

  TM_ASSERT(tm_apps.count(appid) > 0,
            "App" << appid << " has not been added yet!");
  app = &tm_apps[appid];
  TM_ASSERT(app->task_graph.count(taskid) > 0,
            "Task" << taskid << " has not been added yet!");
  t = &app->task_graph[taskid];

  p.setExecTime(exectime);
  p.setPayLoad(payload);
  p.setSTaskId(staskid);
  p.setPATH(path); //  path value
  if (payload > 0) {
    p.setSTaskId(staskid);
  } else if (staskid != TM_NOT_TASK) {
    p.setSTaskId(TM_NOT_TASK);
    TM_WARNING("PBlock "
               << (t->pblocks_tasks.size() + 1) << " of task " << taskid
               << " in app" << appid
               << " has a payload equal to zero. Subsequent task is ignored!");
  }

  t->pblocks_tasks.push_back(p);
}

void TaskMapping::addMap(int appid, int_cycles start, int_cycles stop) {
  tm_app *app;
  tm_appmapping app_mapping;

  TM_ASSERT(tm_apps.count(appid) > 0,
            "App" << appid << " has not been added yet!");
  app = &tm_apps[appid];

  app_mapping.setStartTime(start);
  app_mapping.setStopTime(stop);

  // Mapping will be added without association of tasks to PEs. To do so, use
  // the function addTaskToMap
  app->app_mappings.push_back(app_mapping);
}

void TaskMapping::addTaskToMap(int appid, int taskid, int peid) {
  tm_app *app;
  tm_appmapping *app_mapping;
  int numberOfPEs = GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y;

  TM_ASSERT(tm_apps.count(appid) > 0,
            "App" << appid << " has not been added yet!");
  app = &tm_apps[appid];
  TM_ASSERT(app->task_graph.count(taskid) > 0,
            "Task" << taskid << " has not been added yet!");
  TM_ASSERT(app->app_mappings.size() > 0,
            "There are not mapped task in app" << appid << "!");
  app_mapping = &app->app_mappings.back();
  TM_ASSERT(app_mapping->task_on_pe.count(taskid) == 0,
            "Task" << taskid << " was already mapped in app" << appid << "!");
  TM_ASSERT(peid < numberOfPEs,
            "The PE id (" << peid
                          << ") is greater than or equal to the number of PEs ("
                          << numberOfPEs << ") available!" << endl
                          << "Task" << taskid << " of app" << appid
                          << " cannot be mapped!");
  app_mapping->task_on_pe[taskid] = peid;
}

void TaskMapping::deleteCurrentMapping(void) {
  tm_apps.clear();
  tm_pes.clear();
}

void TaskMapping::settleRootAndLeafTasks(void) {
  tm_task *t, *lt;

  for (map<int, tm_app>::iterator it_app = tm_apps.begin();
       it_app != tm_apps.end(); ++it_app) {
    TM_ASSERT(
        it_app->second.task_graph.size(),
        "There are no tasks added yet to app" << it_app->second.getId() << "!");

    // Root Task: should be the first task
    it_app->second.setRootId(it_app->second.task_graph.begin()->second.getId());
    lt = NULL;

    for (map<int, tm_task>::iterator it_task =
             it_app->second.task_graph.begin();
         it_task != it_app->second.task_graph.end(); ++it_task) {
      bool stNotFound = true;
      t = &it_task->second;

      for (unsigned int ind_pb = 0; ind_pb < t->pblocks_tasks.size();
           ++ind_pb) {
        if (t->pblocks_tasks[ind_pb].getSTaskId() != TM_NOT_TASK) {
          stNotFound = false;
          break;
        }
      }

      if (stNotFound == true) {
        TM_ASSERT(lt == NULL, "There are multiple leaf tasks!");
        lt = t;
      }
    }

    // Leaf Task
    TM_ASSERT(lt, "There were no found leaf tasks in app"
                      << it_app->second.getId() << "!");
    it_app->second.setLeafId(lt->getId());
  }
}

void TaskMapping::settleAntecedentTasks(void) {
  tm_task *t, *st;
  tm_antecedent_task at;
  int staskid;

  for (map<int, tm_app>::iterator it_app = tm_apps.begin();
       it_app != tm_apps.end(); ++it_app) {
    for (map<int, tm_task>::iterator it_task =
             it_app->second.task_graph.begin();
         it_task != it_app->second.task_graph.end(); ++it_task) {
      t = &it_task->second;
      for (unsigned int ind_pb = 0; ind_pb < t->pblocks_tasks.size();
           ++ind_pb) {
        staskid = t->pblocks_tasks[ind_pb].getSTaskId();
        if (staskid != TM_NOT_TASK) {
          TM_ASSERT(it_app->second.task_graph.count(staskid) > 0,
                    "Subsequent Task " << staskid << " does not exist!");
          st = &it_app->second.task_graph[staskid];
          at.setPbId(ind_pb);
          at.setATask(t);
          st->antecedent_tasks.push_back(at);
        }
      }
    }
  }
}

bool TaskMapping::checkConsistency(void) {
  tm_app *app;
  tm_task *t;

  // Check if there were found apps
  if (tm_apps.size() == 0) {
    cout << "There were no found any applications in the task mapping file."
         << endl;
    return false;
  }

  // Check the tasks of each app
  for (map<int, tm_app>::iterator it_app = tm_apps.begin();
       it_app != tm_apps.end(); ++it_app) {
    app = &it_app->second;

    // Check if the app has at least one mapping
    if (app->app_mappings.size() == 0) {
      cout << "The app" << app->getId() << " must have at least one mapping."
           << endl;
      return false;
    } else { // Check if the start time of each mapping meets the requirements
      if (getSchedulerMapBefore() >= getSchedulerMappingGAP()) {
        cout << "Scheduler Map Before cycles must be less than Scheduler "
                "Mapping GAP cycles."
             << endl;
        return false;
      }

      for (unsigned int imap = 0; imap < app->app_mappings.size(); imap++) {
        if ((app->app_mappings[imap].getStartTime() - getSchedulerMapBefore()) <
            GlobalParams::reset_time) {
          cout << "The mapping time ("
               << (app->app_mappings[imap].getStartTime() -
                   getSchedulerMapBefore())
               << ") in mapping" << imap << " of app" << app->getId()
               << " is less than the reset time (" << GlobalParams::reset_time
               << ")!";
          return false;
        }

        if (imap > 0) {
          if (app->app_mappings[imap].getStartTime() <
              (app->app_mappings[imap - 1].getStopTime() +
               getSchedulerMappingGAP())) {
            cout << "Start time (" << app->app_mappings[imap].getStartTime()
                 << ") of Mapping" << imap
                 << " is not greater than stop time of Mapping" << (imap - 1)
                 << " + Mapping GAP ("
                 << (app->app_mappings[imap - 1].getStopTime() +
                     getSchedulerMappingGAP())
                 << ")." << endl;
            return false;
          }
        }
      }
    }

    // Check if there were found tasks per each app
    if (app->task_graph.size() == 0) {
      cout << "There were no found any tasks for app" << app->getId() << endl;
      return false;
    }

    // Check if the root task exists
    if (app->task_graph.count(app->getRootId()) == 0) {
      cout << "Root task" << app->getRootId() << " of app" << app->getId()
           << " does not exist!" << endl;
      return false;
    }

    // Check if the leaf task exists
    if (app->task_graph.count(app->getLeafId()) == 0) {
      cout << "leaf task" << app->getLeafId() << " of app" << app->getId()
           << " does not exist!" << endl;
      return false;
    }

    // Check if the root task has antecedent tasks
    if (app->task_graph[app->getRootId()].antecedent_tasks.size() > 0) {
      cout << "Root task" << app->getRootId() << " of app" << app->getId()
           << " has antecedent tasks!" << endl;
      return false;
    }

    // Check several issues in the tasks of the app
    for (map<int, tm_task>::iterator it_task = app->task_graph.begin();
         it_task != app->task_graph.end(); ++it_task) {
      t = &it_task->second;

      // Check if the task exists in all the mappings
      //     for (unsigned int imap=0; imap < app->app_mappings.size(); imap++)
      //     {
      // if ( app->app_mappings[imap].task_on_pe.count(t->getId()) == 0 ) {
      //   cout << "The task" << t->getId() << " of app" << app->getId() << "
      //   does not exist in mapping" << imap << "." << endl; return false;
      // }
      //     }

      // Check if the task has processing blocks
      if (t->pblocks_tasks.size() == 0) {
        cout << "Task" << t->getId() << " of app" << app->getId()
             << " has no processing blocks!" << endl;
        return false;
      }

      // Check for each non root task if they have antecedent tasks
      if (app->getRootId() != t->getId() && t->antecedent_tasks.size() == 0) {
        cout << "The non root task " << t->getId() << " of app " << app->getId()
             << " has no antecedent tasks!" << endl;
        return false;
      }

      // Check for the leaf task if it has more than one pblocks
      if (app->getLeafId() == t->getId() && t->pblocks_tasks.size() > 1) {
        cout << "The leaf task " << t->getId() << " of app " << app->getId()
             << " has more than one pblocks!" << endl;
        return false;
      }

      // Check if all the processing blocks have subsequent task (except leaf
      // task)
      if (!t->isLeafTask()) {
        bool st_nofound = false;
        for (vector<tm_pblock_task>::iterator it_pb = t->pblocks_tasks.begin();
             it_pb != t->pblocks_tasks.end(); it_pb++) {
          if ((*it_pb).getSTaskId() == TM_NOT_TASK) {
            st_nofound = true;
            break;
          }
        }
        if (st_nofound) {
          cout << "Task" << t->getId() << " of app " << app->getId()
               << " has pbs with no subsequent task!" << endl;
          return false;
        }
      } else {
        bool st_found = false;
        for (vector<tm_pblock_task>::iterator it_pb = t->pblocks_tasks.begin();
             it_pb != t->pblocks_tasks.end(); it_pb++) {
          if ((*it_pb).getSTaskId() != TM_NOT_TASK) {
            st_found = true;
            break;
          }
        }
        if (st_found) {
          cout << "Leaf task" << t->getId() << " of app " << app->getId()
               << " has pbs with subsequent task!" << endl;
          return false;
        }
      }

      // Check if this task is a subsequent task of its subsequent tasks
      // (dependency loop) Check if the execution time of the task is zero
      tm_task *t_st;
      int_cycles execution_time_len = 0;
      for (vector<tm_pblock_task>::iterator it_pb = t->pblocks_tasks.begin();
           it_pb != t->pblocks_tasks.end(); it_pb++) {
        execution_time_len += (*it_pb).getExecTime();

        if ((*it_pb).getSTaskId() != TM_NOT_TASK) {
          TM_ASSERT(app->task_graph.count((*it_pb).getSTaskId()) > 0,
                    "Subsequent task " << (*it_pb).getSTaskId()
                                       << " does not exist in the task graph!");
          // First check there is no a local loop
          if ((*it_pb).getSTaskId() == t->getId()) {
            cout << "Task " << t->getId() << " has a local dependency loop!"
                 << endl;
            return false;
          }
          t_st = &app->task_graph[(*it_pb).getSTaskId()];
          for (vector<tm_pblock_task>::iterator it_pb_st =
                   t_st->pblocks_tasks.begin();
               it_pb_st != t_st->pblocks_tasks.end(); it_pb_st++) {
            if ((*it_pb_st).getSTaskId() == t->getId()) {
              cout << "Task " << t->getId() << " and task " << t_st->getId()
                   << " have a dependency loop!" << endl;
              return false;
            }
          }
        }
      }
      // Comment this out becuase we will have virtual leaf node which has 0
      // execution time.
      //     if (execution_time_len == 0) {
      // cout << "Task " << t->getId() << " of app " << app->getId() << " has an
      // execution time of 0 cycles, which is not allowed!" << endl; return
      // false;
      //     }
    }
  }

  return true;
}

void TaskMapping::performMappingOnPEs(void) {
  tm_app *app;
  int_cycles start, stop;
  int peid, taskid;

  // Check the mapping of each app
  for (map<int, tm_app>::iterator it_app = tm_apps.begin();
       it_app != tm_apps.end(); ++it_app) {
    app = &it_app->second;

    // Take the tasks from each mapping and map it to the corresponding PE
    for (unsigned int imap = 0; imap < app->app_mappings.size(); imap++) {
      start = app->app_mappings[imap].getStartTime();
      stop = app->app_mappings[imap].getStopTime();
      for (map<int, int>::iterator it =
               app->app_mappings[imap].task_on_pe.begin();
           it != app->app_mappings[imap].task_on_pe.end(); it++) {
        taskid = it->first;
        peid = it->second;

        TM_ASSERT(app->task_graph.count(taskid) > 0,
                  "Task" << taskid << " of app" << app->getId()
                         << " does not exist, so cannot be mapped!");
        tm_pes[peid].setId(peid);
        if (tm_pes[peid].mappingsonpe.count(start) == 0) {
          tm_mappingonpe mappingonpe;
          mappingonpe.setStopTime(stop);
          mappingonpe.tasks_on_pe.push_back(&app->task_graph[taskid]);
          tm_pes[peid].mappingsonpe[start] = mappingonpe;
        } else {
          tm_pes[peid].mappingsonpe[start].tasks_on_pe.push_back(
              &app->task_graph[taskid]);
        }
      }
    }
  }
}

void TaskMapping::showAppInfo(tm_app *app) {
  TM_SIMCONFIGLOG("app: " << app->getId() << ", root_task: " << app->getRootId()
                          << ", min_restart: " << app->getMinRestart()
                          << ", leaf_task: " << app->getLeafId() << endl);
}

void TaskMapping::showTasksPerAppInfo(tm_task *t) {
  TM_SIMCONFIGLOG("  task: " << t->getId());
  if (t->pblocks_tasks.size() > 0) {
    for (unsigned int i = 0; i < t->pblocks_tasks.size(); i++) {
      TM_SIMCONFIGLOG(", pb" << i << " ( "
                             << "et: " << t->pblocks_tasks[i].getExecTime()
                             << ", st: " << t->pblocks_tasks[i].getSTaskId()
                             << ", pl: " << t->pblocks_tasks[i].getPayLoad()
                             << ", path: " << t->pblocks_tasks[i].getPath()
                             << " )");
    }
  }
  if (t->antecedent_tasks.size() > 0) {
    TM_SIMCONFIGLOG(", ant: " << (t->antecedent_tasks[0].getATask())->getId());
    for (unsigned int i = 1; i < t->antecedent_tasks.size(); i++) {
      TM_SIMCONFIGLOG(", " << (t->antecedent_tasks[i].getATask())->getId());
    }
  }
  TM_SIMCONFIGLOG(endl);
}

void TaskMapping::showMappingsPerAppInfo(tm_app *app) {
  TM_SIMCONFIGLOG(endl);
  for (unsigned int imap = 0; imap < app->app_mappings.size(); imap++) {
    TM_SIMCONFIGLOG("  mapping: "
                    << imap
                    << ", start: " << app->app_mappings[imap].getStartTime()
                    << ", stop: " << app->app_mappings[imap].getStopTime());
    for (map<int, int>::iterator it_tonpe =
             app->app_mappings[imap].task_on_pe.begin();
         it_tonpe != app->app_mappings[imap].task_on_pe.end(); it_tonpe++) {
      TM_SIMCONFIGLOG(", task" << it_tonpe->first << "(pe" << it_tonpe->second
                               << ")");
    }
    TM_SIMCONFIGLOG(endl);
  }
  TM_SIMCONFIGLOG(endl);
}

void TaskMapping::showAllAppsAndTasksInfo(void) {
  for (map<int, tm_app>::iterator it = tm_apps.begin(); it != tm_apps.end();
       ++it) {
    showAppInfo(&it->second);
    for (map<int, tm_task>::iterator it2 = it->second.task_graph.begin();
         it2 != it->second.task_graph.end(); ++it2) {
      showTasksPerAppInfo(&it2->second);
    }
    showMappingsPerAppInfo(&it->second);
  }
}

void TaskMapping::showMappedTasksPerPEInfo(tm_pe *pe) {
  TM_SIMCONFIGLOG("pe: " << pe->getId() << endl);
  for (map<int_cycles, tm_mappingonpe>::iterator it = pe->mappingsonpe.begin();
       it != pe->mappingsonpe.end(); ++it) {
    TM_SIMCONFIGLOG("  start-stop: " << it->first << "-"
                                     << it->second.getStopTime());
    for (unsigned int it_task = 0; it_task < it->second.tasks_on_pe.size();
         it_task++) {
      TM_SIMCONFIGLOG(", app"
                      << it->second.tasks_on_pe[it_task]->getApp()->getId()
                      << ",task" << it->second.tasks_on_pe[it_task]->getId());
    }
    TM_SIMCONFIGLOG(endl);
  }
}

void TaskMapping::showAllMappedTasksOnPEsInfo(void) {
  for (map<int, tm_pe>::iterator it = tm_pes.begin(); it != tm_pes.end();
       ++it) {
    showMappedTasksPerPEInfo(&it->second);
  }
}

void TaskMapping::showAppStatistics(void) {
  tm_appexec_statistics<int_cycles> stat_exectime, stat_commtime,
      stat_txstart_delay, stat_end2end_latency, stat_hopcount;
  int stat_flits_ignored = 0;
  int stat_flits_to_samepe = 0;

  for (map<int, tm_app>::iterator it = tm_apps.begin(); it != tm_apps.end();
       ++it) {
    it->second.printStats(stat_exectime, stat_commtime, stat_txstart_delay,
                          stat_end2end_latency, stat_hopcount,
                          stat_flits_ignored, stat_flits_to_samepe);
  }

  TM_SIMSTATSLOG("*** App overall ***" << endl);
  TM_SIMSTATSLOG("Execution Time (CPU + Comm.) (executions,min,avg,max)="
                 << stat_exectime.statGetExecNumber() << ","
                 << stat_exectime.statGetMin() << ","
                 << (int)stat_exectime.statGetAverage() << ","
                 << stat_exectime.statGetMax() << " (cycles)" << endl);
  TM_SIMSTATSLOG("Communication Time Critical Branch (min,avg,max)="
                 << stat_commtime.statGetMin() << ","
                 << (int)stat_commtime.statGetAverage() << ","
                 << stat_commtime.statGetMax() << " (cycles)" << endl);
  TM_SIMSTATSLOG(
      "Transmission Start Delay (head_flits_tx,min,avg,max)="
      << stat_txstart_delay.statGetExecNumber()
      << "," // real payloads transmitted in the Branch from root to leaf
      << stat_txstart_delay.statGetMin() << ","
      << (int)stat_txstart_delay.statGetAverage() << ","
      << stat_txstart_delay.statGetMax() << " (cycles)" << endl);
  TM_SIMSTATSLOG("FLITs Transmitted to Same PE=" << stat_flits_to_samepe
                                                 << endl);
  TM_SIMSTATSLOG("FLITs Ignored due to Application Unmapping="
                 << stat_flits_ignored << endl);
  TM_SIMSTATSLOG("FLIT End2End Latency (flits,min,avg,max)="
                 << stat_end2end_latency.statGetExecNumber() << ","
                 << stat_end2end_latency.statGetMin() << ","
                 << stat_end2end_latency.statGetAverage() << ","
                 << stat_end2end_latency.statGetMax() << " (cycles)" << endl);
  TM_SIMSTATSLOG("FLIT End2End HopCount (flits,min,avg,max)="
                 << stat_hopcount.statGetExecNumber() << ","
                 << stat_hopcount.statGetMin() << ","
                 << stat_hopcount.statGetAverage() << ","
                 << stat_hopcount.statGetMax() << endl);
  TM_SIMSTATSLOG("FLIT Communication Cost (flits x average hopcount)="
                 << stat_hopcount.statGetExecNumber() *
                        stat_hopcount.statGetAverage()
                 << endl);
  TM_SIMSTATSLOG(endl << endl;);
}

void TaskMapping::showPEStatistics(void) {
  tm_peexec_statistics<int_cycles> stats_acum;

  for (map<int, tm_pe>::iterator it = tm_pes.begin(); it != tm_pes.end();
       ++it) {
    it->second.printStats();

    stats_acum = stats_acum + it->second.stats;
  }

  TM_SIMSTATSLOG("*** pe overall ***" << endl);
  TM_SIMSTATSLOG(
      "Total FLITs Transmitted="
      << stats_acum.statGetFlitsTx() << endl
      << "Total FLITs Received=" << stats_acum.statGetFlitsRx() << endl
      << "Total Context Switching Cycles="
      << stats_acum.statGetCSwitchingCycles() << endl
      << "Total Task Execution Cycles=" << stats_acum.statGetTskExecCycles()
      << endl
      << "Total PE Idle Cycles=" << stats_acum.statGetIdleCycles() << endl
      << "Total Mapped Task Cycles=" << stats_acum.statGetPeTotalCycles()
      << endl
      << "Total Energy (J)=" << stats_acum.statGetTotalEnergy_d() << endl
      << "      Context Switching Energy (J)="
      << stats_acum.statGetCSwitchingEnergy_d() << endl
      << "      Task Execution Energy (J)="
      << stats_acum.statGetTskExecEnergy_d() << endl
      << "      Idle Energy (J)=" << stats_acum.statGetIdleEnergy_d() << endl);
  TM_SIMSTATSLOG(endl);
}

/*
 * Public functions
 */

bool TaskMapping::createTaskGraphsFromFile(const char *fname) {
  ifstream infile(fname);
  const string strapp(TM_MFILE_STRAPP);
  const string strtask(TM_MFILE_STRTASK);
  const string strmap(TM_MFILE_STRMAP);
  const string strsim(TM_MFILE_STRSIM);
  char *nextChar;
  long int valueRead;
  int appid = TM_NOT_APP;

  TM_SIMCONFIGLOG(endl
                  << "************************" << endl
                  << "TaskMapping File Reading" << endl
                  << "************************" << endl
                  << endl);

  if (infile.is_open()) {
    deleteCurrentMapping();

    // Read every line of the file
    while (infile) {
      string line;

      if (!getline(infile, line))
        break;

      // Determine if the line corresponds to an app, a task or a pe
      if (line.length() >= strapp.length() &&
          line.compare(0, strapp.length(), strapp) == 0) {
        // App
        line = line.substr(strapp.length(), line.length() - strapp.length());
        TM_SIMCONFIGLOG("App: " << line << endl);

        istringstream segline(line);
        tm_mfile_app_fsm app_fsm = FSMAPP_APPID;
        int_cycles min_restart = 0;

        while (segline) {
          string seg;

          if (!getline(segline, seg, TM_MFILE_SEP))
            break;

          valueRead = strtol(seg.c_str(), &nextChar, 10);
          TM_ASSERT(*nextChar == 0, "Characters in the file are wrong!");

          cout << "In TaskMapping::createTaskGraphsFromFile(), segline = "
               << seg << endl;

          switch (app_fsm) {
          case FSMAPP_APPID:
            appid = valueRead;
            TM_ASSERT(appid >= TM_APPID_MIN,
                      "App identifier ("
                          << appid << ") must be greater than or equal to "
                          << TM_APPID_MIN << "!");
            app_fsm = FSMAPP_MINRESTART;
            break;
          case FSMAPP_MINRESTART:
            min_restart = valueRead;
            TM_ASSERT(min_restart >= TM_TIMERESTART_MIN,
                      "Restart time (" << min_restart
                                       << ") must be greater or equal to "
                                       << TM_TIMERESTART_MIN << "!");
            addApp(appid, min_restart);
            app_fsm = FSMAPP_IGNORE;
            break;
          default:
            break;
          }
        }
      } // End if App
      else if (line.length() >= strtask.length() &&
               line.compare(0, strtask.length(), strtask) == 0) {
        // Task
        line = line.substr(strtask.length(), line.length() - strtask.length());
        TM_SIMCONFIGLOG("Task: " << line << endl);

        istringstream segline(line);
        tm_mfile_task_fsm task_fsm = FSMTASK_TASKID;
        int id = TM_NOT_TASK, staskid = TM_NOT_TASK, payload = 0;
        int_cycles exectime = 0;
        int path = 0;

        while (segline) {
          string seg;

          if (!getline(segline, seg, TM_MFILE_SEP))
            break;

          valueRead = strtol(seg.c_str(), &nextChar, 10);
          TM_ASSERT(*nextChar == 0, "Characters in the file are wrong!");

          cout << "In TaskMapping::createTaskGraphsFromFile(), segline = "
               << seg << endl;

          if (appid == TM_NOT_APP)
            break;

          switch (task_fsm) {
          case FSMTASK_TASKID:
            id = valueRead;
            TM_ASSERT(id >= TM_TASKID_MIN,
                      "Task identifier ("
                          << id << ") must be greater than or equal to "
                          << TM_TASKID_MIN << "!");
            addTask(appid, id);
            task_fsm = FSMTASK_PBLOCK_ET;
            break;
          case FSMTASK_PBLOCK_ET:
            exectime = valueRead;
            TM_ASSERT(exectime >= TM_EXECTIME_MIN,
                      "Execution Time (" << exectime << ") cannot be less than "
                                         << TM_EXECTIME_MIN << "!");
            task_fsm = FSMTASK_PBLOCK_ST;
            break;
          case FSMTASK_PBLOCK_ST:
            staskid = valueRead;
            // added by Xuan
            if (valueRead == TM_NOT_TASK) {
              tm_apps[appid].setLastId(id);
              cout << "Set Last Task ID of App " << appid << " to " << id
                   << endl;
            }
            TM_ASSERT(staskid >= TM_NOT_TASK, "Subsequent Task ID ("
                                                  << staskid
                                                  << ") shouldn't be less than "
                                                  << TM_NOT_TASK << "!");
            task_fsm = FSMTASK_PBLOCK_PL;
            break;
          case FSMTASK_PBLOCK_PL:
            if (staskid != TM_NOT_TASK) {
              payload = valueRead;
              TM_ASSERT(payload >= TM_PAYLOAD_MIN,
                        "Payload (" << payload << ") cannot be less than "
                                    << TM_PAYLOAD_MIN << "!");
            } else
              payload = 0; // If there is no subsequent task, payload is 0
            // task_fsm = FSMTASK_PATH;
            task_fsm = FSMTASK_PATH; // Directly go to PATH
            break;
          case FSMTASK_PATH:
            if (staskid != TM_NOT_TASK) {
              path = valueRead;
              // TM_ASSERT(path >= TM_PATH_MIN, "Path (" << path << ") cannot be
              // less than " << TM_PATH_MIN << "!");
            }
            addPBlockToTask(appid, id, exectime, staskid, payload, path);
            cout << "Added PBlock to Task: appid=" << appid << ", taskid=" << id
                 << ", exectime=" << exectime << ", staskid=" << staskid
                 << ", payload=" << payload << ", path=" << path << endl;
            task_fsm = FSMTASK_PBLOCK_ET;

            break;
          }
        }
      } // End if Task
      else if (line.length() >= strmap.length() &&
               line.compare(0, strmap.length(), strmap) == 0) {
        // MAP
        line = line.substr(strmap.length(), line.length() - strmap.length());
        TM_SIMCONFIGLOG("MAP: " << line << endl);
        istringstream segline(line);
        tm_mfile_map_fsm map_fsm = FSMMAP_START;
        int taskid = TM_NOT_TASK, peid = TM_NOT_PE;
        int_cycles start = 0, stop = 0;
        int nmapping = tm_apps[appid].app_mappings.size();

        while (segline) {
          string seg;

          if (!getline(segline, seg, TM_MFILE_SEP))
            break;

          valueRead = strtol(seg.c_str(), &nextChar, 10);
          TM_ASSERT(*nextChar == 0, "Characters in the file are wrong!");

          cout << "In TaskMapping::createTaskGraphsFromFile(), segline = "
               << seg << endl;

          switch (map_fsm) {
          case FSMMAP_START:
            start = valueRead;
            TM_ASSERT(start >= TM_MAPSTART_MIN,
                      "Start Time (" << start << ") cannot be less than "
                                     << TM_MAPSTART_MIN << " in app" << appid
                                     << ", mapping" << nmapping << "!");
            map_fsm = FSMMAP_STOP;
            break;
          case FSMMAP_STOP:
            stop = valueRead;
            TM_ASSERT(stop > start, "Stop Time ("
                                        << stop
                                        << ") must be greater than Start Time ("
                                        << start << ") in app" << appid
                                        << " mapping" << nmapping << "!");
            addMap(appid, start, stop);
            map_fsm = FSMMAP_TASKID;
            break;
          case FSMMAP_TASKID:
            taskid = valueRead;
            TM_ASSERT(taskid >= TM_TASKID_MIN,
                      "Task identifier ("
                          << taskid << ") must be greater than or equal to "
                          << TM_TASKID_MIN << " in app" << appid << ", mapping"
                          << nmapping - 1 << "!");
            map_fsm = FSMMAP_PEID;
            break;
          case FSMMAP_PEID:
            peid = valueRead;
            TM_ASSERT(peid >= TM_PEID_MIN,
                      "PE identifier ("
                          << peid << ") must be greater than or equal to "
                          << TM_PEID_MIN << " in app" << appid << ", mapping"
                          << nmapping - 1 << "!");
            addTaskToMap(appid, taskid, peid);
            map_fsm = FSMMAP_TASKID;
            break;
          }
        }
      } // End if MAP
      else if (line.length() >= strsim.length() &&
               line.compare(0, strsim.length(), strsim) == 0) {
        // SIM
        line = line.substr(strsim.length(), line.length() - strsim.length());
        TM_SIMCONFIGLOG("SIM: " << line << endl);

        istringstream segline(line);
        tm_mfile_sim_fsm sim_fsm = FSMSIM_SCHEDULER_TICK;
        int sch_tick = TM_PE_SCHEDULER_TICK, sch_delay = TM_PE_SCHEDULER_DELAY,
            sch_mapbefore = TM_PE_SCHEDULER_MAPBEFORE,
            sch_mappinggap = TM_PE_SCHEDULER_MAPPINGGAP;

        while (segline) {
          string seg;

          if (!getline(segline, seg, TM_MFILE_SEP))
            break;

          valueRead = strtol(seg.c_str(), &nextChar, 10);
          TM_ASSERT(*nextChar == 0, "Characters in the file are wrong!");

          cout << "In TaskMapping::createTaskGraphsFromFile(), segline = "
               << seg << endl;

          switch (sim_fsm) {
          case FSMSIM_SCHEDULER_TICK:
            sch_tick = valueRead;
            TM_ASSERT(sch_tick >= TM_PE_SCHEDULER_TICK_MIN,
                      "Scheduler Tick ("
                          << sch_tick << ") must be greater than or equal to "
                          << TM_PE_SCHEDULER_TICK_MIN << "!");
            sim_fsm = FSMSIM_SCHEDULER_DELAY;
            break;
          case FSMSIM_SCHEDULER_DELAY:
            sch_delay = valueRead;
            TM_ASSERT(sch_delay >= TM_PE_SCHEDULER_DELAY_MIN,
                      "Scheduler Delay ("
                          << sch_delay << ") must be greater than or equal to "
                          << TM_PE_SCHEDULER_DELAY_MIN << "!");
            sim_fsm = FSMSIM_SCHEDULER_MAPBEFORE;
            break;
          case FSMSIM_SCHEDULER_MAPBEFORE:
            sch_mapbefore = valueRead;
            TM_ASSERT(sch_mapbefore >= TM_PE_SCHEDULER_MAPBEFORE_MIN,
                      "Scheduler Map Before cycles ("
                          << sch_mapbefore
                          << ") must be greater than or equal to "
                          << TM_PE_SCHEDULER_MAPBEFORE_MIN << "!");
            sim_fsm = FSMSIM_SCHEDULER_MAPPINGGAP;
            break;
          case FSMSIM_SCHEDULER_MAPPINGGAP:
            sch_mappinggap = valueRead;
            TM_ASSERT(sch_mappinggap >= TM_PE_SCHEDULER_MAPPINGGAP_MIN,
                      "Scheduler Mapping Gap cycles ("
                          << sch_mappinggap << ") must be greater than "
                          << TM_PE_SCHEDULER_MAPPINGGAP_MIN << "!");
            scheduler_tick = sch_tick;
            scheduler_delay = sch_delay;
            scheduler_mapbefore = sch_mapbefore;
            scheduler_mappinggap = sch_mappinggap;
            sim_fsm = FSMSIM_SCHEDULER_IGNORE;
            break;
          default:
            break;
          }
        }
      } // End if SIM
    }

    TM_ASSERT(infile.eof(), "The task mapping file was not read entirely!");

    // Settle Root and Leaf Tasks
    settleRootAndLeafTasks();

    // Settle Antecedent Tasks
    settleAntecedentTasks();

    // Check Consistency
    bool cc = checkConsistency();
    TM_ASSERT(cc, "Check consistency failed!");

    // Perform the task mapping on PEs
    if (cc) {
      performMappingOnPEs();
    }

    return cc;
  } else {
    TM_ASSERT(infile.is_open(), "Error opening mapping file.");
    return false;
  }

  return false;
}

tm_app *TaskMapping::getApp(int appid) {
  tm_app *app = NULL;

  TM_ASSERT(tm_apps.count(appid) > 0, "App has not been added yet!");
  app = &tm_apps[appid];

  return app;
}

tm_task *TaskMapping::getTask(int appid, int taskid) {
  tm_app *app = NULL;
  tm_task *t = NULL;

  TM_ASSERT(tm_apps.count(appid) > 0, "App has not been added yet!");
  app = &tm_apps[appid];
  TM_ASSERT(app->task_graph.count(taskid) > 0, "Task has not been added yet!");
  t = &app->task_graph[taskid];

  return t;
}

tm_pe *TaskMapping::getPE(int peid) {
  tm_pe *pe = NULL;

  if (tm_pes.count(peid) > 0) {
    pe = &tm_pes[peid];
    cout << "In TaskMapping::getPE(), setting pe = " << peid << endl;
  }

  return pe;
}

int_cycles TaskMapping::getSchedulerTick() { return scheduler_tick; }

int_cycles TaskMapping::getSchedulerDelay() { return scheduler_delay; }

int_cycles TaskMapping::getSchedulerTxDelay() { return scheduler_tx_delay; }

int_cycles TaskMapping::getSchedulerMapBefore() { return scheduler_mapbefore; }

int_cycles TaskMapping::getSchedulerMappingGAP() {
  return scheduler_mappinggap;
}

void TaskMapping::showTaskMappingConfiguration(void) {
  TM_SIMCONFIGLOG(endl
                  << "**************************" << endl
                  << "Task Mapping Configuration" << endl
                  << "**************************" << endl
                  << endl);

  showAllAppsAndTasksInfo();
  showAllMappedTasksOnPEsInfo();
}

void TaskMapping::showTaskMappingStatistics(void) {
  TM_SIMSTATSLOG(endl
                 << "***********************" << endl
                 << "Task Mapping Statistics" << endl
                 << "***********************" << endl
                 << endl);

  showAppStatistics();
  showPEStatistics();
}

/*

int sc_main(int argc, char* argv[]) {
  TaskMapping taskMap;

  srand(time(NULL));

  cout << endl << "\t\tNoxim - the NoC Simulator" << endl;
  cout << "\t\t(C) University of Catania" << endl << endl;

  taskMap.createTaskGraphFromFile("TaskMappingExample.txt");

  taskMap.showAllAppsAndTasksInfo();

  return 0;
}

*/