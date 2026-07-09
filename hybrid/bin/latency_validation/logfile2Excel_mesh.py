import re
import pandas as pd
from collections import defaultdict
import os
import sys

# === Step 1: Parse mapping file ===
def parse_task_to_pe_mapping(mapping_file):
    path = os.path.dirname(os.path.abspath(__file__))

    with open(os.path.join(path, "..", "..", "tm_examples", mapping_file), "r") as f:
        for line in f:
            if line.startswith("map:"):
                parts = line.strip().split(",")[2:]  # 忽略開頭兩個數字
                mapping = {int(parts[i]): int(parts[i+1]) for i in range(0, len(parts), 2)}
                return mapping
    return {}

# === Step 2: Parse log file ===
def parse_log_file(log_file):
    pe_to_pe_data = defaultdict(dict)

    path = os.path.dirname(os.path.abspath(__file__))
    with open(os.path.join(path, "..", "..", "tm_log", f"{log_file}.log"), "r") as file:
        for line in file:
            match_pe_start = re.search(r"tsk(\d+)->tsk(\d+),from pe to pe latency -> start transmitting head_flit at ([\d\.]+)", line)
            match_pe_end = re.search(r"tsk(\d+)->tsk(\d+),from pe to pe latency -> end receiving tail_flit at ([\d\.]+)", line)
            match_full = re.search(
                r"tsk(\d+)->tsk(\d+),\s*Buffer\s*->\s*FULL\s+cycles:\s*([\d\.]+)",
                line, flags=re.IGNORECASE
            )

            if match_pe_start:
                src, dst, start = match_pe_start.groups()
                key = (int(src), int(dst))
                if 'start_time' not in pe_to_pe_data[key]:
                    pe_to_pe_data[key]['src_task_id'] = int(src)
                    pe_to_pe_data[key]['dst_task_id'] = int(dst)
                    pe_to_pe_data[key]['start_time'] = float(start)

            elif match_pe_end:
                src, dst, end = match_pe_end.groups()
                key = (int(src), int(dst))
                if 'end_time' not in pe_to_pe_data[key]:
                    pe_to_pe_data[key]['end_time'] = float(end)
                    pe_to_pe_data[key]['duration'] = float(end) - pe_to_pe_data[key]['start_time']

            if match_full:
                src, dst, cycles = match_full.groups()
                key = (int(src), int(dst))
                if 'src_task_id' not in pe_to_pe_data[key]:
                    pe_to_pe_data[key]['src_task_id'] = int(src)
                    pe_to_pe_data[key]['dst_task_id'] = int(dst)
                if 'full_cycles' not in pe_to_pe_data[key]:
                    pe_to_pe_data[key]['full_cycles'] = float(cycles)

    return pe_to_pe_data

# === Step 3: 加入 mapping 並輸出 Excel ===
def process_and_export(pe_data, mapping):
    path = os.path.dirname(os.path.abspath(__file__))

    df_pe = pd.DataFrame(pe_data.values()).sort_values(
        by=["src_task_id", "dst_task_id"]
    )
    df_pe["(src_task_id, dst_task_id)"] = df_pe.apply(
        lambda row: f"({int(row['src_task_id'])}, {int(row['dst_task_id'])})", axis=1)
    df_pe["(src_pe_id, dst_pe_id)"] = df_pe.apply(
        lambda row: f"({mapping.get(row['src_task_id'], -1)}, {mapping.get(row['dst_task_id'], -1)})", axis=1)
    df_pe = df_pe.drop(columns=["src_task_id", "dst_task_id"])

    output_name = file_name.split('/')[-1]
    with pd.ExcelWriter(path + f"/latency_excel/Simulation_latency_{output_name}_statistics.xlsx") as writer:
        df_pe.to_excel(writer, sheet_name="pe_to_pe_latency", index=False)

# === Main ===
if __name__ == "__main__":
    path = os.path.dirname(os.path.abspath(__file__))
    param = sys.argv[1:]

    file_name = "01-oneapp-fourtasks1_mesh"
    map_file_name = "01-oneapp-fourtasks1.map"
    mapping = parse_task_to_pe_mapping(map_file_name)
    print(mapping)
    pe_data = parse_log_file(file_name)
    print(pe_data)
    process_and_export(pe_data, mapping)