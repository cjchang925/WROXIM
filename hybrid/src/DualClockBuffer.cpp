#include "DualClockBuffer.h"

#include "Utils.h"

DualClockBuffer::DualClockBuffer() {
  SetMaxBufferSize(GlobalParams::buffer_depth);
  max_occupancy = 0;
  hold_time = 0.0;
  last_event = 0.0;
  hold_time_sum = 0.0;
  previous_occupancy = 0;
  mean_occupancy = 0.0;
  true_buffer = true;
  full_cycles_counter = 0;
  last_front_flit_seq = NOT_VALID;
}

void DualClockBuffer::Print(const char *prefix) {
  deque<Flit> m = buffer;

  string bstr = "";
  char t[] = "HBTF";

  LOG << prefix << " | ";
  while (!m.empty()) {
    Flit f = m.front();
    m.pop_front();
    if (f.flit_type == FLIT_TYPE_FLOWCONTROL) {
      cout << bstr << t[f.flit_type] << f.sequence_no << "(" << f.src_id << "->"
           << f.dst_id << "," << f.flow_control << ") | ";
    } else
      cout << bstr << t[f.flit_type] << f.sequence_no << "(" << f.src_id << "->"
           << f.dst_id << ") | ";
  }
  cout << endl;
}

void DualClockBuffer::deadlockCheck() {
  if (IsEmpty())
    return;

  Flit f = buffer.front();
  int seq = f.sequence_no;

  if (last_front_flit_seq == seq) {
    full_cycles_counter++;
  } else {
    last_front_flit_seq = seq;
    full_cycles_counter = 0;
  }
}

bool DualClockBuffer::deadlockFree() {
  if (IsEmpty())
    return true;

  Flit f = buffer.front();
  int seq = f.sequence_no;

  if (last_front_flit_seq == seq) {
    full_cycles_counter++;
  } else {
    last_front_flit_seq = seq;
    full_cycles_counter = 0;
  }

  if (full_cycles_counter > 10000)
    return false;

  return true;
}

void DualClockBuffer::Disable() { true_buffer = false; }

void DualClockBuffer::SetMaxBufferSize(const unsigned int bms) {
  assert(bms > 0);
  max_buffer_size = bms;
}

unsigned int DualClockBuffer::GetMaxBufferSize() const {
  return max_buffer_size;
}

bool DualClockBuffer::IsFull() const {
  return buffer.size() == max_buffer_size;
}

bool DualClockBuffer::IsEmpty() const { return buffer.empty(); }

void DualClockBuffer::Drop(const Flit &flit) const { assert(false); }

void DualClockBuffer::Empty() const { assert(false); }

void DualClockBuffer::Push(const Flit &flit) {
  SaveOccupancyAndTime();

  if (IsFull()) {
    Drop(flit);
  } else {
    buffer.push_back(flit); // Push flit to the back of the deque
  }

  UpdateMeanOccupancy();

  if (max_occupancy < buffer.size())
    max_occupancy = buffer.size();
}

Flit DualClockBuffer::Pop() {
  Flit f;

  SaveOccupancyAndTime();

  if (IsEmpty()) {
    Empty();
  } else {
    f = buffer.front();
    buffer.pop_front(); // Pop flit from the front of the deque
  }

  UpdateMeanOccupancy();

  return f;
}

Flit DualClockBuffer::Front() const {
  Flit f;

  if (IsEmpty()) {
    Empty();
  } else {
    f = buffer.front();
  }

  return f;
}

unsigned int DualClockBuffer::Size() const { return buffer.size(); }

unsigned int DualClockBuffer::getFlowControlOFF() const {
  int trt = 2;                           // cycle
  int b_value = GlobalParams::flit_size; // bit/cycle
  int Lf = GlobalParams::flit_size;

  return (trt * b_value / Lf);
}
unsigned int DualClockBuffer::getFlowControlON() const {
  return max_buffer_size - getFlowControlOFF();
}

unsigned int DualClockBuffer::getCurrentFreeSlots() const {
  return (GetMaxBufferSize() - Size());
}

void DualClockBuffer::SaveOccupancyAndTime() {
  previous_occupancy = buffer.size();
  hold_time = (sc_time_stamp().to_double() / GlobalParams::clock_period_ps) -
              last_event;
  last_event = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
}

void DualClockBuffer::UpdateMeanOccupancy() {
  double current_time =
      sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
  if (current_time - GlobalParams::reset_time <
      GlobalParams::stats_warm_up_time)
    return;

  mean_occupancy =
      mean_occupancy * (hold_time_sum / (hold_time_sum + hold_time)) +
      (1.0 / (hold_time_sum + hold_time)) * hold_time * buffer.size();

  hold_time_sum += hold_time;
}

void DualClockBuffer::ShowStats(std::ostream &out) {
  if (true_buffer)
    out << "\t" << mean_occupancy << "\t" << max_occupancy;
  else
    out << "\t\t";
}

// Insert credit with higher priority than data (insert at the front of the
// deque)
void DualClockBuffer::InsertFlowControl(const Flit &flowControl_flit) {
  if (!IsFull()) {
    buffer.push_front(flowControl_flit); // Insert flowControl_flit at the front
  }
}
