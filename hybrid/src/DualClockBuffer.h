#ifndef __NOXIMDUALCLOCKBUFFER_H__
#define __NOXIMDUALCLOCKBUFFER_H__

#include <deque>
#include <cassert>
#include <iostream>
#include "DataStructs.h"
#include "Utils.h"

using namespace std;

class DualClockBuffer {

  public:

    DualClockBuffer();

    virtual ~DualClockBuffer() {}

    void SetMaxBufferSize(const unsigned int bms);	// Set buffer max size (in flits)

    unsigned int GetMaxBufferSize() const;	// Get max buffer size

    unsigned int getCurrentFreeSlots() const;	// free buffer slots

    bool IsFull() const;	// Returns true if buffer is full

    bool IsEmpty() const;	// Returns true if buffer is empty

    virtual void Drop(const Flit & flit) const;	// Called by Push() when buffer is full

    virtual void Empty() const;	// Called by Pop() when buffer is empty

    void Push(const Flit & flit);	// Push a flit. Calls Drop method if buffer is full

    Flit Pop();		// Pop a flit

    Flit Front() const;	// Return a copy of the first flit in the buffer

    unsigned int Size() const;
    unsigned int getFlowControlOFF() const; // Reference: Principles and Practices of Interconnection Networks(ON/OFF Flow Control)
    unsigned int getFlowControlON() const;  // Reference: Principles and Practices of Interconnection Networks(ON/OFF Flow Control)

    void ShowStats(std::ostream & out);

    void Disable();

    void Print(const char*);

    bool deadlockFree();
    void deadlockCheck();

    inline string name() { return "DualClockBuffer"; };

    // Insert a credit with higher priority than data
    void InsertFlowControl(const Flit & flowControl_flit);

  private:

    bool true_buffer;

    int full_cycles_counter;
    int last_front_flit_seq;

    unsigned int max_buffer_size;

    // Use deque for dual-end operations
    deque<Flit> buffer;

    unsigned int max_occupancy;
    double hold_time, last_event, hold_time_sum;
    double mean_occupancy;
    int previous_occupancy;

    void SaveOccupancyAndTime();
    void UpdateMeanOccupancy();
};

#endif
