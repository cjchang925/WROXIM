/*
 * File added by LGGM.
 *
 * This file contains the declaration for the LOGs of the task mapping
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

#ifndef __NOXIM_TASK_MAPPING_LOGS__
#define __NOXIM_TASK_MAPPING_LOGS__

#include <iostream>

#include "../DataStructs.h"

// ***************************
// Messages, Warnings and Info
// ***************************

#define _TM_ASSERT(X,MSG,EXIT)  if ( !(X) ) { \
                                  double ccycle = sc_time_stamp().to_double() / GlobalParams::clock_period_ps; \
                                  cout << endl \
                                       << "******************" << endl \
                                       << "ABNORMAL SITUATION : (cycle=" << ccycle << ") " << MSG << endl \
                                       << "******************" << endl; \
                                  if (EXIT) \
                                     assert((X)); \
                                } 

// if DEFINE_TM_ASSERT is defined, tm_engine stops when anormal situations occur
// #define  DEFINE_TM_ASSERT
#ifdef   DEFINE_TM_ASSERT
 #define TM_ASSERT(X,MSG)       _TM_ASSERT(X,MSG,1); 
#else
 #define TM_ASSERT(X,MSG)       _TM_ASSERT(X,MSG,0); 
#endif

// if DEFINE_TM_WARNING is defined, tm_engine shows warnings when they occur
// #define  DEFINE_TM_WARNING
#ifdef   DEFINE_TM_WARNING
 #define TM_WARNING(MSG)         { \
			            double ccycle = sc_time_stamp().to_double() / GlobalParams::clock_period_ps; \
				    cout << endl \
					<< "*******" << endl \
					<< "WARNING : (" << ccycle << ") " << MSG << endl \
					<< "*******" << endl; \
				 }	
#else
 #define TM_WARNING(MSG)
#endif

#define TM_SIMWRITELOG(MSG) \
        { \
          if (tmInstance->taskmappinglog_file.is_open()) { \
            tmInstance->taskmappinglog_file << MSG; \
          } \
          else { \
            cout << MSG; \
          } \
        } 

// if DEFINE_TM_SIMCONFIGLOG is defined, tm_engine writes simulation configuration logs to the specified log file
#define DEFINE_TM_SIMCONFIGLOG  //Chagne by JengDe
#ifdef DEFINE_TM_SIMCONFIGLOG
 #define TM_SIMCONFIGLOG(MSG) TM_SIMWRITELOG(MSG)
#else
 #define TM_SIMCONFIGLOG(MSG)
#endif

// if DEFINE_TM_SIMEXECLOG is defined, tm_engine writes simulation execution logs to the specified log file
// if TM_SIMEXECLOG_EXTENDED is defined, tm_engine writes extended logs
#define  DEFINE_TM_SIMEXECLOG                           1 //Change by JengDe
#define  TM_SIMEXECLOG_EXTENDED                         1 //Change by JengDe
#define TM_SIMEXECLOG_BEINGMAPPEDSTATE                  0
#define TM_SIMEXECLOG_WAITINGRXSTATE                    1
#define TM_SIMEXECLOG_SLEEPINGSTATE                     2
#define TM_SIMEXECLOG_READYSTATE                        3
#define TM_SIMEXECLOG_READYSTATEDUETOCONTEXTSWITCHING   4
#define TM_SIMEXECLOG_CONTEXTSWITCHINGSTATE             5
#define TM_SIMEXECLOG_RUNNINGSTATE                      6
#define TM_SIMEXECLOG_STOPSANDUNMAPPED                  7
#define TM_SIMEXECLOG_RECEIVESALLANT                    8
#define TM_SIMEXECLOG_STARTSEXEC                        9
#define TM_SIMEXECLOG_CONTINUESEXECAFTERCONEXTSW        10
#define TM_SIMEXECLOG_COMPLETESPB                       11
#define TM_SIMEXECLOG_COMPLETESALLPBS                   12
#define TM_SIMEXECLOG_PAYLOADREADYTOTRANSF              13
#define TM_SIMEXECLOG_CONTINUESEXEC                     14
#define TM_SIMEXECLOG_PLBEINGTRANSTOSAMEPE              15
#define TM_SIMEXECLOG_PLBEINGTRANSTOWNOC                16
#define TM_SIMEXECLOG_PLTOTALLYOUTTOSAMEPE              17
#define TM_SIMEXECLOG_PLTOTALLYOUTTOWNOC                18
#define TM_SIMEXECLOG_RECEIVESPLFROMSAMEPE              19
#define TM_SIMEXECLOG_RECEIVESPLFROMWNOC                20
#define TM_SIMEXECLOG_DESTNOTREACHEABLE                 21

#ifdef   DEFINE_TM_SIMEXECLOG
 #ifdef   TM_SIMEXECLOG_EXTENDED
  #define TM_SIMEXECLOG(CODE,PE,APP,TSK,MSG) \
          { \
            if (tmInstance->taskmappinglog_file.is_open()) { \
              double ccycle = sc_time_stamp().to_double() / GlobalParams::clock_period_ps; \
              tmInstance->taskmappinglog_file << "(" << setfill('0') << setw(2) << CODE << ") " \
                                              << "@" << setfill('0') << setw(6) << ccycle << " " \
                                              << "pe" << setfill('0') << setw(3) << PE << "," \
                                              << "app" << setfill('0') << setw(2) << APP << "," \
                                              << "tsk" << setfill('0') << setw(2) << TSK \
                                              << " -> " << MSG << endl; \
            } \
          } 
 #else
  #define TM_SIMEXECLOG(CODE,PE,APP,TSK,MSG) \
          { \
            if (tmInstance->taskmappinglog_file.is_open()) { \
              double ccycle = sc_time_stamp().to_double() / GlobalParams::clock_period_ps; \
              tmInstance->taskmappinglog_file << "(" << setfill('0') << setw(2) << CODE << ") " \
                                              << "@" << setfill('0') << setw(6) << ccycle << " " \
                                              << "pe" << setfill('0') << setw(3) << PE << "," \
                                              << "app" << setfill('0') << setw(2) << APP << "," \
                                              << "tsk" << setfill('0') << setw(2) << TSK << endl; \
            } \
          } 
 #endif
#else
 #define TM_SIMEXECLOG(CODE,PE,APP,TSK,MSG)
#endif

#define TM_SIMSTATSLOG_STATEND2ENDLATENCY               30
#define TM_SIMSTATSLOG_STATEND2ENDHOPCOUNT              31
#define TM_SIMSTATSLOG_STATAPPEXECTIME                  32
#define TM_SIMSTATSLOG_STATPECYCLESANDENERGY            33

#define TM_SIMPRECISEEND2ENDLATENCYLOG                  34  //Added by JengDe
#define TM_SIMCONTENTIONINFORMATIONLOG                  35  //Added by JengDe

// if DEFINE_TM_SIMSTATSLOG is defined, tm_engine writes simulation statistics logs to the specified log file
#define DEFINE_TM_SIMSTATSLOG 
#ifdef DEFINE_TM_SIMSTATSLOG
 #define TM_SIMSTATSLOG(MSG) TM_SIMWRITELOG(MSG)
 #define TM_SIMPEMETRICSLOG(CODE,PE,MSG1,MSG2) \
         { \
           double ccycle = sc_time_stamp().to_double() / GlobalParams::clock_period_ps; \
           if (tmInstance->taskmappinglog_file.is_open()) { \
             tmInstance->taskmappinglog_file << "(" << setfill('0') << setw(2) << CODE << ") " \
                                             << "@" << setfill('0') << setw(6) << ccycle << " "\
                                             << "pe" << setfill('0') << setw(3) << PE << "," << MSG1 << " -> " \
                                             << MSG2 << endl; \
           } \
           else \
           { \
             cout << "(" << setfill('0') << setw(2) << CODE << ") " \
                  << "@" << setfill('0') << setw(6) << ccycle << " "\
                  << "pe" << setfill('0') << setw(3) << PE << "," << MSG1 << " -> " \
                  << MSG2 << endl; \
           } \
         } 
 #define TM_SIMAPPMETRICSLOG(CODE,APP,MSG1,MSG2) \
         { \
           double ccycle = sc_time_stamp().to_double() / GlobalParams::clock_period_ps; \
           if (tmInstance->taskmappinglog_file.is_open()) { \
             tmInstance->taskmappinglog_file << "(" << setfill('0') << setw(2) << CODE << ") " \
                                             << "@" << setfill('0') << setw(6) << ccycle << " "\
                                             << "app" << setfill('0') << setw(2) << APP << "," << MSG1 << " -> " \
                                             << MSG2 << endl; \
           } \
           else \
           { \
             cout << "(" << setfill('0') << setw(2) << CODE << ") " \
                  << "@" << setfill('0') << setw(6) << ccycle << " "\
                  << "app" << setfill('0') << setw(2) << APP << "," << MSG1 << " -> " \
                  << MSG2 << endl; \
           } \
         } 
 #define TM_SIMEND2ENDLATENCYLOG(CODE,APP,TSK1,TSK2,MSG) \
         { \
           double ccycle = sc_time_stamp().to_double() / GlobalParams::clock_period_ps; \
           if (tmInstance->taskmappinglog_file.is_open()) { \
             tmInstance->taskmappinglog_file << "(" << setfill('0') << setw(2) << CODE << ") " \
                                             << "@" << setfill('0') << setw(6) << ccycle << " " \
                                             << "app" << setfill('0') << setw(2) << APP << "," \
                                             << "tsk" << setfill('0') << setw(2) << TSK1 << "->" \
                                             << "tsk" << setfill('0') << setw(2) << TSK2 << "," \
                                             << "end2end_latency -> " << MSG << endl; \
           } \
           else \
           { \
             cout << "(" << setfill('0') << setw(2) << CODE << ") " \
                  << "@" << setfill('0') << setw(6) << ccycle << " " \
                  << "app" << setfill('0') << setw(2) << APP << "," \
                  << "tsk" << setfill('0') << setw(2) << TSK1 << "->" \
                  << "tsk" << setfill('0') << setw(2) << TSK2 << "," \
                  << "end2end_latency -> " << MSG << endl; \
           } \
         } 
 #define TM_SIMEND2ENDHOPSLOG(CODE,APP,TSK1,TSK2,MSG) \
         { \
           double ccycle = sc_time_stamp().to_double() / GlobalParams::clock_period_ps; \
           if (tmInstance->taskmappinglog_file.is_open()) { \
             tmInstance->taskmappinglog_file << "(" << setfill('0') << setw(2) << CODE << ") " \
                                             << "@" << setfill('0') << setw(6) << ccycle << " " \
                                             << "app" << setfill('0') << setw(2) << APP << "," \
                                             << "tsk" << setfill('0') << setw(2) << TSK1 << "->" \
                                             << "tsk" << setfill('0') << setw(2) << TSK2 << "," \
                                             << "end2end_hopcount -> " << MSG << endl; \
           } \
           else \
           { \
             cout << "(" << setfill('0') << setw(2) << CODE << ") " \
                  << "@" << setfill('0') << setw(6) << ccycle << " " \
                  << "app" << setfill('0') << setw(2) << APP << "," \
                  << "tsk" << setfill('0') << setw(2) << TSK1 << "->" \
                  << "tsk" << setfill('0') << setw(2) << TSK2 << "," \
                  << "end2end_hopcount -> " << MSG << endl; \
           } \
         } 
#else
 #define TM_SIMSTATSLOG(MSG)
 #define TM_SIMPEMETRICSLOG(CODE,PE,MSG1,MSG2)
 #define TM_SIMAPPMETRICSLOG(CODE,APP,MSG1,MSG2)
 #define TM_SIMEND2ENDLATENCYLOG(CODE,APP,TSK1,TSK2,MSG)
 #define TM_SIMEND2ENDHOPSLOG(CODE,APP,TSK1,TSK2,MSG)
#endif

#define TM_STRAPPTASK(T)         "app" << (T)->getApp()->getId() << ",task" << (T)->getId()  

#ifdef TM_SIMPRECISEEND2ENDLATENCYLOG
  #define TM_SIMOPTICALEND2ENDLATENCYLOG(CODE,APP,TSK1,TSK2,MSG) \
          { \
            double ccycle = sc_time_stamp().to_double() / GlobalParams::clock_period_ps; \
            if (tmInstance->taskmappinglog_file.is_open()) { \
              tmInstance->taskmappinglog_file << "(" << setfill('0') << setw(2) << CODE << ") " \
                                              << "@" << setfill('0') << setw(6) << ccycle << " " \
                                              << "app" << setfill('0') << setw(2) << APP << "," \
                                              << "tsk" << setfill('0') << setw(2) << TSK1 << "->" \
                                              << "tsk" << setfill('0') << setw(2) << TSK2 << "," \
                                              << "optical bufer2buffer_latency -> " << MSG << endl; \
            } \
            else \
            { \
              cout << "(" << setfill('0') << setw(2) << CODE << ") " \
                    << "@" << setfill('0') << setw(6) << ccycle << " " \
                    << "app" << setfill('0') << setw(2) << APP << "," \
                    << "tsk" << setfill('0') << setw(2) << TSK1 << "->" \
                    << "tsk" << setfill('0') << setw(2) << TSK2 << "," \
                    << "optical bufer2buffer_latency -> " << MSG << endl; \
            } \
          }
  #define TM_SIMPEEND2ENDLATENCYLOG(CODE,APP,TSK1,TSK2,MSG) \
          { \
            double ccycle = sc_time_stamp().to_double() / GlobalParams::clock_period_ps; \
            if (tmInstance->taskmappinglog_file.is_open()) { \
              tmInstance->taskmappinglog_file << "(" << setfill('0') << setw(2) << CODE << ") " \
                                              << "@" << setfill('0') << setw(6) << ccycle << " " \
                                              << "app" << setfill('0') << setw(2) << APP << "," \
                                              << "tsk" << setfill('0') << setw(2) << TSK1 << "->" \
                                              << "tsk" << setfill('0') << setw(2) << TSK2 << "," \
                                              << "from pe to pe latency -> " << MSG << endl; \
            } \
            else \
            { \
              cout << "(" << setfill('0') << setw(2) << CODE << ") " \
                  << "@" << setfill('0') << setw(6) << ccycle << " " \
                  << "app" << setfill('0') << setw(2) << APP << "," \
                  << "tsk" << setfill('0') << setw(2) << TSK1 << "->" \
                  << "tsk" << setfill('0') << setw(2) << TSK2 << "," \
                  << "from pe to pe latency -> " << MSG << endl; \
            } \
        }
  #define TM_SIMBUFFERSTATUSLOG(CODE,APP,TSK1,TSK2,MSG) \
          { \
            double ccycle = sc_time_stamp().to_double() / GlobalParams::clock_period_ps; \
            if (tmInstance->taskmappinglog_file.is_open()) { \
              tmInstance->taskmappinglog_file << "(" << setfill('0') << setw(2) << CODE << ") " \
                                              << "@" << setfill('0') << setw(6) << ccycle << " " \
                                              << "app" << setfill('0') << setw(2) << APP << "," \
                                              << "tsk" << setfill('0') << setw(2) << TSK1 << "->" \
                                              << "tsk" << setfill('0') << setw(2) << TSK2 << "," \
                                              << "Buffer -> " << MSG << endl; \
            } \
            else \
            { \
              cout << "(" << setfill('0') << setw(2) << CODE << ") " \
                  << "@" << setfill('0') << setw(6) << ccycle << " " \
                  << "app" << setfill('0') << setw(2) << APP << "," \
                  << "tsk" << setfill('0') << setw(2) << TSK1 << "->" \
                  << "tsk" << setfill('0') << setw(2) << TSK2 << "," \
                  << "Buffer -> " << MSG << endl; \
            } \
        }
#else
  #define TM_SIMOPTICALEND2ENDLATENCYLOG(CODE,APP,TSK1,TSK2,MSG);
  #define TM_SIMPEEND2ENDLATENCYLOG(CODE,APP,TSK1,TSK2,MSG);
#endif
#ifdef TM_SIMCONTENTIONINFORMATIONLOG
  #define TM_SIMFLOWCONTROLOFFLOG(CODE,PE1,PE2,MSG) \
          { \
            double ccycle = sc_time_stamp().to_double() / GlobalParams::clock_period_ps; \
            if (tmInstance->taskmappinglog_file.is_open()) { \
              tmInstance->taskmappinglog_file << "(" << setfill('0') << setw(2) << CODE << ") " \
                                              << "@" << setfill('0') << setw(6) << ccycle << " " \
                                              << "pe" << setfill('0') << setw(2) << PE1 << "->" \
                                              << "pe" << setfill('0') << setw(2) << PE2 << ", " \
                                              << "buffer congested, send OFF flow control -> " << MSG << endl; \
            } \
            else \
            { \
              cout << "(" << setfill('0') << setw(2) << CODE << ") " \
                  << "@" << setfill('0') << setw(6) << ccycle << " " \
                  << "pe" << setfill('0') << setw(2) << PE1 << "->" \
                  << "pe" << setfill('0') << setw(2) << PE2 << ", " \
                  << "buffer congested, send OFF flow control -> " << MSG << endl; \
            } \
          }
  #define TM_SIMFLOWCONTROLONLOG(CODE,PE1,PE2,MSG) \
          { \
            double ccycle = sc_time_stamp().to_double() / GlobalParams::clock_period_ps; \
            if (tmInstance->taskmappinglog_file.is_open()) { \
              tmInstance->taskmappinglog_file << "(" << setfill('0') << setw(2) << CODE << ") " \
                                              << "@" << setfill('0') << setw(6) << ccycle << " " \
                                              << "pe" << setfill('0') << setw(2) << PE1 << "->" \
                                              << "pe" << setfill('0') << setw(2) << PE2 << ", " \
                                              << "buffer available, send ON flow control -> " << MSG << endl; \
            } \
            else \
            { \
              cout << "(" << setfill('0') << setw(2) << CODE << ") " \
                  << "@" << setfill('0') << setw(6) << ccycle << " " \
                  << "pe" << setfill('0') << setw(2) << PE1 << "->" \
                  << "pe" << setfill('0') << setw(2) << PE2 << ", " \
                  << "buffer available, send ON flow control -> " << MSG << endl; \
            } \
        }
#else

#endif


#endif // __NOXIM_TASK_MAPPING_LOGS__