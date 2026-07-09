import os
import re
import pandas as pd

# Load the log file content
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
file_path = os.path.join(SCRIPT_DIR, '..', 'tm_log', 'test.log')
with open(file_path, 'r') as file:
    log_content = file.readlines()

task_max_pb = {
    1: 3,
    2: 1,
    3: 2,
    4: 1,
    5: 1,
    6: 1,
    7: 1
}
# Define regex patterns for different log types
patterns = {
    'running': r'\((\d+)\) @(\d+) pe(\d+),app\d+,tsk(\d+) -> in running state\.',
    'complete': r'\((\d+)\) @(\d+) pe(\d+),app\d+,tsk(\d+) -> completes its pbs and releases the pe\.',
    'pb_complete': r'\((\d+)\) @(\d+) pe(\d+),app\d+,tsk(\d+) -> pb(\d+) completes its processing\.',
    'context_switch': r'\((\d+)\) @(\d+) pe(\d+),app\d+,tsk(\d+) -> in ready state due to context switching\.',
    'transfer_start': r'\((\d+)\) @(\d+) pe(\d+),app\d+,tsk(\d+) -> payload of pb\d+ being transferred to WNoC\.',
    'transfer_end_same_pe': r'\((\d+)\) @(\d+) pe(\d+),app\d+,tsk(\d+) -> payload of pb\d+ being transferred to task in the same pe\.',
    'transfer_end_wnoc': r'\((\d+)\) @(\d+) pe(\d+),app\d+,tsk\d+ -> receives payload of pb\d+ \(task(\d+)\) from WNoC\.'
}

laser_interval_section_re = re.compile(r"^Laser Intervals:\s+")
optical_tx_section_re = re.compile(r"^Optical TX Intervals by Chiplet ID:\s+")
optical_rx_section_re = re.compile(r"^Optical RX Intervals by Chiplet ID:\s+")
interval_re = re.compile(
    r"Interval:\s+\[(\d+),\s+(\d+)\),\s+Active\s+Wavelengths:\s+\{([\d,\s]+)\},\s+Count:\s+(\d+),\s+Duration:\s+(\d+)"
)
# Parse log content into structured data
parsed_data = []
for line in log_content:
    for log_type, pattern in patterns.items():
        match = re.match(pattern, line)
        if match:
            if(log_type == 'pb_complete' and int(match.group(5)) == task_max_pb[int(match.group(4))]-1):
                parsed_data.append({
                    'log_type': 'complete',
                    'cycle': int(match.group(2)),
                    'pe_id': int(match.group(3)),
                    'task_id': int(match.group(4))
                })
            else:
                parsed_data.append({
                    'log_type': log_type,
                    'cycle': int(match.group(2)),
                    'pe_id': int(match.group(3)),
                    'task_id': int(match.group(4))
                })


# Create a DataFrame from the parsed data
df = pd.DataFrame(parsed_data)

# Group and analyze by PE
grouped_data = []
for pe_id, group in df.groupby('pe_id'):
    tasks = group[group['log_type'].isin(['running', 'complete', 'context_switch'])]
    transfers = group[group['log_type'].isin(['transfer_start',  'transfer_end_wnoc'])]

    grouped_data.append({
        'pe_id': pe_id,
        'task_events': tasks.to_dict(orient='records'),
        'transfer_events': transfers.to_dict(orient='records')
    })
    # print(grouped_data[-1])

# Convert the grouped data to a DataFrame
grouped_df = pd.DataFrame(grouped_data)
print(grouped_df)
# import ace_tools as tools; tools.display_dataframe_to_user(name="Grouped PE Task and Transfer Data", dataframe=grouped_df)

import matplotlib.pyplot as plt
import matplotlib.patches as mpatches

# Prepare Gantt chart data
# Prepare interval data for Gantt chart
intervals = []
for _, row in grouped_df.iterrows():
    pe_id = row['pe_id']
    task_rows = row['task_events']

    # Ensure task rows are sorted by cycle to process events sequentially
    task_rows = sorted(task_rows, key=lambda x: x['cycle'])

    for event in task_rows:
        log_type_tmp = ''
        if event['log_type'] == 'running':
            start_cycle = event['cycle']
            task_id = event['task_id']
            end_cycle = None
            switch_cycle = None

            # Search for the next context_switch or complete event for the same task_id
            for next_event in task_rows:
                if next_event['task_id'] == task_id:
                    if next_event['log_type'] == 'context_switch' and switch_cycle is None and next_event['cycle'] > start_cycle:
                        switch_cycle = next_event['cycle']
                        log_type_tmp = 'context_switch'
                        break
                    if next_event['log_type'] == 'complete' and end_cycle is None and next_event['cycle'] > start_cycle:
                        end_cycle = next_event['cycle']
                        log_type_tmp = 'complete'
                        break

            # Add intervals for running -> context_switch if it exists
            if switch_cycle:
                intervals.append({'pe_id': pe_id, 'task_id': task_id, 'start': start_cycle, 'end': switch_cycle, 'log_type': log_type_tmp})

            # Add intervals for running -> complete if it exists
            if end_cycle:
                intervals.append({'pe_id': pe_id, 'task_id': task_id, 'start': start_cycle, 'end': end_cycle, 'log_type': log_type_tmp})

# Create DataFrame for intervals
interval_df = pd.DataFrame(intervals)

# Update to include communication (comm) events in the Gantt chart

# Prepare interval data for communication events
comm_intervals_fixed = []
transfer_events = df[df['log_type'].isin(['transfer_start', 'transfer_end_wnoc'])]

# Loop through all transfer_start events and match with the corresponding transfer_end_wnoc
for _, start_event in transfer_events[transfer_events['log_type'] == 'transfer_start'].iterrows():
    start_cycle = start_event['cycle']
    task_id = start_event['task_id']
    pe_id = start_event['pe_id']

    # Look for the matching transfer_end_wnoc event with the same task_id but a different pe_id
    matching_events = transfer_events[
        (transfer_events['log_type'] == 'transfer_end_wnoc') &
        (transfer_events['task_id'] == task_id) &
        (transfer_events['cycle'] > start_cycle)
    ]

    if not matching_events.empty:
        # Select the event with the closest cycle to start_cycle
        closest_event = matching_events.loc[matching_events['cycle'].idxmin()]

        # Add the closest event to comm_intervals_fixed
        comm_intervals_fixed.append({
            'start_pe_id': pe_id,
            'end_pe_id': closest_event['pe_id'],
            'task_id': task_id,
            'start': start_cycle,
            'end': closest_event['cycle']
        })

# Create DataFrame for communication intervals
comm_df_fixed = pd.DataFrame(comm_intervals_fixed)

# Create DataFrame for communication intervals
comm_df = pd.DataFrame(comm_df_fixed)
# comm_df.loc[4] = [12, 1, 3, 1350, 1369]
print("comm_df: \n", comm_df)


laser_intervals = []
tx_intervals = []
rx_intervals = []
with open(file_path, 'r') as log_file:
    for line in log_file:
        # Identify section start
        if laser_interval_section_re.match(line):
            is_laser_section = True
            is_optical_tx_section = False
            is_optical_rx_section = False
            continue
        elif optical_tx_section_re.match(line):
            is_laser_section = False
            is_optical_tx_section = True
            is_optical_rx_section = False
            continue
        elif optical_rx_section_re.match(line):
            is_laser_section = False
            is_optical_tx_section = False
            is_optical_rx_section = True
            continue

        # Parse intervals based on the current section
        interval_match = interval_re.search(line)
        if interval_match:
            start, end = map(int, interval_match.groups()[0:2])
            
            if is_laser_section:
                laser_intervals.append({'start': start, 'end': end})
            elif is_optical_tx_section:
                tx_intervals.append({'start': start/40, 'end': end/40})
            elif is_optical_rx_section:
                rx_intervals.append({'start': start/40, 'end': end/40})
print("laser_intervals:\n", laser_intervals)
print("rx_interval:\n", rx_intervals)
print("tx_intervals:\n", tx_intervals)


laser_row = -0.5
tx_row = -1.5
rx_row = -2.5

# Extend y-axis ticks and labels
original_pe_ids = grouped_df['pe_id'].tolist()
additional_rows = []
additional_labels = []

# 添加额外的行和标签（仅在数据不为空时）
if laser_intervals:
    additional_rows.append(laser_row)
    additional_labels.append("Laser (ON)")

if tx_intervals:
    additional_rows.append(tx_row)
    additional_labels.append("TX Time")

if rx_intervals:
    additional_rows.append(rx_row)
    additional_labels.append("RX Time")

all_yticks = original_pe_ids + additional_rows
all_yticklabels = [f"PE {pe_id}" for pe_id in original_pe_ids] + additional_labels

# Plot combined interval-based Gantt chart
fig, ax = plt.subplots(figsize=(12, 8))
task_colors = ['skyblue', 'lightgreen', 'salmon', 'purple', 'lavender', 'lightyellow']
task_color_map = {}
comm_color = 'orange'
task_occurrences = {}
# Plot task intervals
interval_df.sort_values(by=['pe_id', 'start'], inplace=True)
print("task_df: \n", interval_df)
last_pe_id = -1
counter = 0
task_completed = set()
for _, row in interval_df.iterrows():
    task_id = row['task_id']
    
    if task_id not in task_occurrences:
        task_occurrences[task_id] = 0
    elif task_id in task_completed:
        task_occurrences[task_id] += 1
    
    if last_pe_id == row['pe_id']:
        if task_id not in task_color_map:
            task_color_map[task_id] = task_colors[counter]
            color = task_colors[counter]
        else:
            color = task_color_map[task_id]
        counter = counter + 1
    else:
        last_pe_id = row['pe_id']
        if task_id not in task_color_map:
            color_index = 0
            task_color_map[task_id] = task_colors[color_index]
        color = task_colors[0]
        counter = 1
        
    # 繪製 task execution 的區間線
    ax.plot(
        [row['start'], row['end']],
        [row['pe_id'], row['pe_id']],
        color=color,
        linewidth=4,
        # label=f"Task Execution (Run {task_occurrences[task_id]})" if f"Run {task_occurrences[task_id]}" not in ax.get_legend_handles_labels()[1] else None
        label=f"Task Execution (Run {task_occurrences[task_id]})" if f"Run {task_occurrences[task_id]}" not in ax.get_legend_handles_labels()[1] else None
    )
    # 標註任務 ID
    ax.text(
        (row['start'] + row['end']) / 2,
        row['pe_id'] - 0.3,
        # f"T{task_id} (Run {task_occurrences[task_id]})",
        f"T{task_id}",
        ha='center',
        fontsize=8,
        color='black'
    )
    if row['log_type'] == 'complete':
        task_completed.add(task_id)
print("task_color_map:\n", task_color_map)
# Plot communication intervals
comm_offset_base = 0.2  # Base offset for communication lines
offset_increment = 0.2  # Incremental offset for each unique end_pe_id
comm_df_fixed.sort_values(by='start_pe_id', inplace=True)

last_start_pe = -1
counter = 1
offset = 0
for _, row in comm_df_fixed.iterrows():
    # Calculate offset for end_pe_id differentiation
    if last_start_pe == row['start_pe_id']:
        offset = comm_offset_base + counter * offset_increment
        counter = counter + 1
    else:
        offset = comm_offset_base
        counter = 1
    last_start_pe = row['start_pe_id']
    ax.plot(
        [row['start'], row['end']],
        [row['start_pe_id'] + offset, row['start_pe_id'] + offset],
        color=comm_color,
        linewidth=4,
        # linestyle='dashed',
        label="Communication" if "Communication" not in ax.get_legend_handles_labels()[1] else None
    )
    ax.text(
        (row['start'] + row['end']) / 2,
        row['start_pe_id'] + offset + 0.1,
        f"{row['start_pe_id']} -> {row['end_pe_id']}",
        ha='center',
        fontsize=8,
        color='black'
    )

if laser_intervals:
    for interval in laser_intervals:
        ax.plot(
            [interval['start'], interval['end']],
            [laser_row, laser_row],
            color='red',
            linewidth=4,
            label="Laser (ON)" if "Laser (ON)" not in ax.get_legend_handles_labels()[1] else None
        )
if tx_intervals:
    for interval in tx_intervals:
        ax.plot(
            [interval['start'], interval['end']],
            [tx_row, tx_row],
            color='green',
            linewidth=4,
            label="TX Time" if "TX Time" not in ax.get_legend_handles_labels()[1] else None
        )
if rx_intervals:
    for interval in rx_intervals:
        ax.plot(
            [interval['start'], interval['end']],
            [rx_row, rx_row],
            color='blue',
            linewidth=4,
            label="RX Time" if "RX Time" not in ax.get_legend_handles_labels()[1] else None
        )


# Set labels and title
ax.set_yticks(all_yticks)
ax.set_yticklabels(all_yticklabels)
ax.set_xlabel("Cycle")
ax.set_ylabel("Optical Comm. / PE ID")
ax.set_title("Gantt Chart for Tasks and Communication")

# Add legend
handles = []
labels = []
unique_values = set(task_color_map.values())
for color in unique_values:
    handles.append(plt.Line2D([0], [0], color=color, linewidth=4, label='Task execution time'))
    labels.append('Task execution time (Task id)')
handles.append(plt.Line2D([0], [0], color='orange', linewidth=4, label='Communication (PE_src->PE_dst)'))
labels.append('Communication (PE_src->PE_dst)')
if laser_intervals:
    handles.append(plt.Line2D([0], [0], color='red', linewidth=4, label='Laser (ON)'))
    labels.append('Laser (ON)')
if tx_intervals:
    handles.append(plt.Line2D([0], [0], color='green', linewidth=4, label='TX Time'))
    labels.append('TX Time')
if tx_intervals:
    handles.append(plt.Line2D([0], [0], color='blue', linewidth=4, label='RX Time'))
    labels.append('RX Time')

# labels = ['Task execution time','Communication time','Laser (ON)', 'TX Time', 'RX Time']
ax.legend(handles=handles, labels=labels, loc='best')

# Adjust and save the chart
plt.tight_layout()
output_path_combined = os.path.join(SCRIPT_DIR, 'gantt_chart', 'gantt_chart.png')
fig.savefig(output_path_combined, format='png')
# plt.show()


