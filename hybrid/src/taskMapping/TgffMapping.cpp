/*
 * File added by ACAG
 *
 * TGFF Random Graph Parser for Task Mapping Module
 * (C) 2016 by the University of California Irvine

 * Task Mapping Module:
 * (C) 2016 by the University of Antioquia, Colombia
 *
 * Noxim - the NoC Simulator
 * 
 * (C) 2005-2010 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 */

#include <string>
#include <sstream>
#include <cstdlib>
#include <fstream>
#include "TgffMapping.h"

using namespace std;
extern TaskMapping *tmInstance;
/***************************************************************************************
 * Graph Parser Member Functions 
 ***************************************************************************************/
bool GraphParser::Parse(const char *fname)
{
    ifstream infile(fname);
    FSM_PARSER_DATA_TABLE_STATE state = FSM_PARSER_DATA_TABLE_IDLE;

    cout << endl;
    cout << "************************" << endl;
    cout << "Parsing TGFF Graph" << endl;
    cout << "************************" << endl;
    cout << endl;

    // Create appliation first
    int appId = 1;
    int startCycle = 1100, endCycle = 8000;
    tmInstance->addApp(appId, 20000);


    if (infile.is_open())
    {
        // Read every line of the file
        while (infile)
        {
            string line;

            if (!getline(infile, line))
                break;

            string keyWord = GetFirstString(line);
            if (keyWord.compare("TASK") == 0)
            {
                // Task Block
                ParseTaskBlock(appId, line);
            }
            else if (keyWord.compare("ARC") == 0)
            {
                // ARC Block
                ParseArcBlock(appId, line);
            }
            else if (keyWord.compare("@COMMUN") == 0 || state != FSM_PARSER_DATA_TABLE_IDLE)
            {
                // Execute time and communication volume table
                switch (state)
                {
                case FSM_PARSER_DATA_TABLE_IDLE:
                    state = FSM_PARSER_DATA_TABLE_HEADER;
                    break;
                case FSM_PARSER_DATA_TABLE_HEADER:
                    if (line.find("type") != string::npos)
                    {
                        state = FSM_PARSER_DATA_TABLE_DATA;
                    }
                    break;
                case FSM_PARSER_DATA_TABLE_DATA:
                    if (!keyWord.empty() && isdigit(keyWord[0]))
                        ParseDataBlock(line);
                    break;
                default:
                    break;
                }
            }
        } // End while

        TM_ASSERT(infile.eof(), "The task mapping file was not read entirely!");

        BuildTaskGraphMapping(appId, startCycle, endCycle);

        return true;
    }
    else
    {
        TM_ASSERT(infile.is_open(), "Error opening graph file.");
        return false;
    }

    return false;
}

string GraphParser::GetFirstString(string s)
{
    string ret;
    if (s.empty())
        return ret;

    int start = 0, end = 0, n = s.length();
    while (start < n && (s[start] == ' ' || s[start] == '\t'))
        ++start;
    if (start == n)
        return ret;
    for (end = start; end < n && s[end] != ' '; ++end)
        ;
    return s.substr(start, end - start);
}

/*  State Machine Functions */
void GraphParser::ParseTaskBlock(int appId, string line)
{
    string buf;
    stringstream ss(line);
    vector<string> tokens;
    int taskId = 0, taskType = 0;
    while (ss >> buf)
    {
        tokens.push_back(buf);
    }

    TM_ASSERT(tokens.size() == 4, "Error parsing Task block.");
    taskId = ExtractValue(tokens[1]);
    taskType = atoi(tokens[tokens.size() - 1].c_str());
    // DEBUG
   // cout << taskId << " " << taskType << " " << endl
    //     << endl;

    // Add task into App
    tmInstance->addTask(appId, taskId);
    taskTable[taskId] = taskType;
}

void GraphParser::ParseArcBlock(int appId, string line)
{
    string buf;
    stringstream ss(line);
    vector<string> tokens;
    while (ss >> buf)
    {
        tokens.push_back(buf);
    }

    TM_ASSERT(tokens.size() == 8, "Error parsing Task block.");
    int src = ExtractValue(tokens[3]);
    int dst = ExtractValue(tokens[5]);
    int typeId = atoi(tokens[7].c_str());
    //cout << src << " " << dst << " " << typeId << " " << endl;
    arcTable[make_pair(src, dst)] = typeId;
}

void GraphParser::ParseDataBlock(string line)
{
    string buf;
    stringstream ss(line);
    vector<string> tokens;

    while (ss >> buf)
    {
        tokens.push_back(buf);
    }

    TM_ASSERT(tokens.size() == 3, "Error parsing Data block.");
    int typeId = atoi(tokens[0].c_str());

    // Update data table
    execTimeTable[typeId] = atoi(tokens[1].c_str());
    commVolumTable[typeId] = atoi(tokens[2].c_str());
    // DEBUG
    //cout << typeId << " " << execTimeTable[typeId] << " " << commVolumTable[typeId] << " " << endl;
}

void GraphParser::BuildTaskGraphMapping(int appId, int_cycles start, int_cycles end)
{
    // Todo: if there is only one leaf task, this can be simplifed.
    // First Set execute time for each task
    for (map<int, int>::iterator it = taskTable.begin(); it != taskTable.end(); ++it)
    {
        int taskId = it->first;
        int type = it->second;

        TM_ASSERT(execTimeTable.count(taskId) > 0, " Cannot find src task in execute time table.");

        tmInstance->addPBlockToTask(appId, taskId, execTimeTable[type], TM_NOT_TASK, 0, 0);
    }

    // Then add PB for each task
    for (map<pair<int, int>, int>::iterator it = arcTable.begin(); it != arcTable.end(); ++it)
    {
        int src = it->first.first;
        int dst = it->first.second;
        TM_ASSERT(taskTable.count(src) > 0, " Cannot find src task.");
        int type = it->second;

        TM_ASSERT(execTimeTable.count(type) > 0, " Cannot find execute time type .");
        TM_ASSERT(commVolumTable.count(type) > 0, " Cannot find communication volume type .");

        tm_task *srcTask = &tmInstance->tm_apps[appId].task_graph[src];
        if (srcTask->pblocks_tasks.size() == 1 && srcTask->pblocks_tasks[0].getSTaskId() == TM_NOT_TASK)
        {
            // Overriding first PB
            tm_pblock_task &pb = srcTask->pblocks_tasks[0];
            //p.setExecTime(execTimeTable[type]);
            pb.setPayLoad(commVolumTable[type]);
            pb.setSTaskId(dst);
            //tmInstance->addPBlockToTask(appId, src, 0, dst, commVolumTable[type]);
        }
        else
        {
            tmInstance->addPBlockToTask(appId, src, 0, dst, commVolumTable[type], 0);
        }
    }

    /* Merge multiple leaf nodes into one leaf   */
    int n = taskTable.size();
    vector<int> indegree(n, 0);
    vector<int> outdegree(n, 0);
    for (map<pair<int, int>, int>::iterator it = arcTable.begin(); it != arcTable.end(); ++it)
    {
        indegree[it->first.second]++;
        outdegree[it->first.first]++;

    }
    int numRoot = 0;
    for(size_t i=0; i<indegree.size(); ++i)
    {
        if(indegree[i]==0)
        {
            ++numRoot;
            cout<<endl<<i<<" ";
        }
    }

    cout<<endl;

    TM_ASSERT(numRoot==1, "Error: Multiple Root Nodes");
    vector<int> leafs;
    for(size_t i=0; i<outdegree.size(); ++i)
    {
        if(outdegree[i]==0)
            leafs.push_back(i);
    }

    if(leafs.size() > 1)
    {
        int newTask = n;
        // Add a new virtual leaf node
        tmInstance->addTask(appId, newTask);
        tmInstance->addPBlockToTask(appId, newTask, 0, TM_NOT_TASK, 0, 0);

        for(size_t i = 0; i < leafs.size(); ++i)
        {
            // Redirect leaf node to pointing to new virtual leaf node
            tm_task *srcTask = &tmInstance->tm_apps[appId].task_graph[leafs[i]];
            // Overriding PBs
            tm_pblock_task &pb = srcTask->pblocks_tasks[0];
            pb.setPayLoad(0);
            pb.setSTaskId(newTask);
        }
    }

    //Settle Root and Leaf Tasks
    tmInstance->settleRootAndLeafTasks();
    // Settle Antecedent Tasks
    tmInstance->settleAntecedentTasks();

    //Add mapping
    tmInstance->addMap(appId, start, end);
    // Performance task mapping on PEs
    GenerateRandomTaskMapping(appId);
    
    // Check Consistency
    bool isValid = tmInstance->checkConsistency();
    TM_ASSERT(isValid, "Check consistency failed!");
    if(isValid)
    {              
        tmInstance->performMappingOnPEs();
    }
}

int GraphParser::ExtractValue(string s)
{
    TM_ASSERT(s.empty() == false, "Error parsing block.");

    size_t pos = s.find('_');
    TM_ASSERT(pos != string::npos, "Error cannot find _");

    return atoi(s.substr(pos + 1).c_str());
}

void GraphParser::PrintParsedTables(void)
{
    // Print Task table
    cout << endl;
    cout << "************************" << endl;
    cout << "Print Task Table" << endl;
    cout << "************************" << endl;
    cout << endl;
    for (map<int, int>::iterator it = taskTable.begin(); it != taskTable.end(); ++it)
    {
        cout << "Task Id: " << it->first << " Task Type: " << it->second << endl;
    }

    // Print ARC table
    cout << endl;
    cout << "************************" << endl;
    cout << "Print ARC Table" << endl;
    cout << "************************" << endl;
    cout << endl;
    for (map<pair<int, int>, int>::iterator it = arcTable.begin(); it != arcTable.end(); ++it)
    {
        pair<int, int> key = it->first;
        cout << "Src Task: " << key.first << " Dst Task: " << key.second << " Type:" << it->second << endl;
    }
    cout << endl;
    cout << "************************" << endl;
    cout << "Print Execute Time Table" << endl;
    cout << "************************" << endl;
    cout << endl;
    for (map<int, int>::iterator it = execTimeTable.begin(); it != execTimeTable.end(); ++it)
    {
        cout << "Type: " << it->first << " Exec Time: " << it->second << endl;
    }

    cout << endl;
    cout << "************************" << endl;
    cout << "Print Communication Volume Table" << endl;
    cout << "************************" << endl;
    cout << endl;
    for (map<int, int>::iterator it = commVolumTable.begin(); it != commVolumTable.end(); ++it)
    {
        cout << "Type: " << it->first << " Comm Vol: " << it->second << endl;
    }
}

/***************************************************************************************
 * Task Mapping Generation
 ***************************************************************************************/
void GraphParser::GenerateRandomTaskMapping(int appId)
{
    srand(time(NULL));
    int dimension = GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y;
    //cout<<"dimension" << dimension<<endl;
    int taskNum = taskTable.size();

    for (int task=0; task<taskNum; ++task)
    {
        int peid = rand() % (dimension);
        //cout<<endl<<"Task: "<<task<<"peid "<<peid<<endl;
        tmInstance->addTaskToMap(appId, task, peid);
    }
}

/*
int sc_main(int argc, char *argv[])
{

    GraphParser parser;

    srand(time(NULL));
    tmInstance = new TaskMapping("TaskMapping");
    cout << endl
         << "\t\tGraph Parser Testing" << endl;

    parser.Parse("graph.tgff");
    //parser.PrintParsedTables();
    tmInstance->showAllAppsAndTasksInfo();

    return 0;
}*/