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
                # 轉換成整數並兩兩一組配對 task_id -> pe_id
                mapping = {int(parts[i]): int(parts[i+1]) for i in range(0, len(parts), 2)}
                return mapping
    return {}

# === Step 2: Parse log file ===
# === Step 2: Parse log file ===
def parse_log_file(log_file):
    pe_to_pe_data = defaultdict(dict)
    buffer_to_buffer_data = defaultdict(dict)

    path = os.path.dirname(os.path.abspath(__file__))
    with open(os.path.join(path, "..", "..", "tm_log", f"{file_name}.log"), "r") as file:
        for line in file:
            # 第一、第二類型：pe to pe latency
            match_pe_start = re.search(r"tsk(\d+)->tsk(\d+),from pe to pe latency -> start transmitting head_flit at ([\d\.]+)", line)
            match_pe_end = re.search(r"tsk(\d+)->tsk(\d+),from pe to pe latency -> end receiving tail_flit at ([\d\.]+)", line)

            # 第三、第四類型：buffer to buffer latency
            match_buf_start = re.search(r"tsk(\d+)->tsk(\d+),optical bufer2buffer_latency -> start transmitting head_flit at ([\d\.]+)", line)
            match_buf_end = re.search(r"tsk(\d+)->tsk(\d+),optical bufer2buffer_latency -> end receiving tail_flit at ([\d\.]+)", line)

            # 新增：Buffer FULL cycles
            # 例：(34) @3942.00 app00,tsk27->tsk08,Buffer ->  FULL cycles: 549.84
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

            elif match_buf_start:
                src, dst, start = match_buf_start.groups()
                key = (int(src), int(dst))
                if 'start_time' not in buffer_to_buffer_data[key]:
                    buffer_to_buffer_data[key]['src_task_id'] = int(src)
                    buffer_to_buffer_data[key]['dst_task_id'] = int(dst)
                    buffer_to_buffer_data[key]['start_time'] = float(start)

            elif match_buf_end:
                src, dst, end = match_buf_end.groups()
                key = (int(src), int(dst))
                if 'end_time' not in buffer_to_buffer_data[key]:
                    buffer_to_buffer_data[key]['end_time'] = float(end)
                    buffer_to_buffer_data[key]['duration'] = float(end) - buffer_to_buffer_data[key]['start_time']

            # 把 FULL cycles 併到 pe_to_pe_data（用同一個 key）
            if match_full:
                src, dst, cycles = match_full.groups()
                key = (int(src), int(dst))
                # 確保基本欄位存在（即使這對任務沒有 latency 記錄也能出現在表中）
                if 'src_task_id' not in pe_to_pe_data[key]:
                    pe_to_pe_data[key]['src_task_id'] = int(src)
                    pe_to_pe_data[key]['dst_task_id'] = int(dst)
                # 只保留第一次出現的 full_cycles
                if 'full_cycles' not in pe_to_pe_data[key]:
                    pe_to_pe_data[key]['full_cycles'] = float(cycles)

    return pe_to_pe_data, buffer_to_buffer_data


# === Step 3: 加入 mapping 並輸出 Excel ===
def process_and_export(pe_data, buffer_data, mapping):
    path = os.path.dirname(os.path.abspath(__file__))
    
    # 處理 pe_to_pe
    df_pe = pd.DataFrame(pe_data.values()).sort_values(
        by=["src_task_id", "dst_task_id"]
    )
    df_pe["(src_task_id, dst_task_id)"] = df_pe.apply(
        lambda row: f"({int(row['src_task_id'])}, {int(row['dst_task_id'])})", axis=1)
    df_pe["(src_pe_id, dst_pe_id)"] = df_pe.apply(
        lambda row: f"({mapping.get(row['src_task_id'], -1)}, {mapping.get(row['dst_task_id'], -1)})", axis=1)
    df_pe = df_pe.drop(columns=["src_task_id", "dst_task_id"])

    # 處理 buffer_to_buffer
    df_buf = pd.DataFrame(buffer_data.values()).sort_values(
        by=["src_task_id", "dst_task_id"]
    )
    df_buf["(src_task_id, dst_task_id)"] = df_buf.apply(
        lambda row: f"({int(row['src_task_id'])}, {int(row['dst_task_id'])})", axis=1)
    df_buf["(src_pe_id, dst_pe_id)"] = df_buf.apply(
        lambda row: f"({mapping.get(row['src_task_id'], -1)}, {mapping.get(row['dst_task_id'], -1)})", axis=1)
    df_buf = df_buf.drop(columns=["src_task_id", "dst_task_id"])
    
    

    # 輸出 Excel
    output_name = file_name.split('/')[-1]
    with pd.ExcelWriter(path + f"/latency_excel/Simulation_latency_{output_name}_Buffer_statistics.xlsx") as writer:
        df_pe.to_excel(writer, sheet_name="pe_to_pe_latency", index=False)
        df_buf.to_excel(writer, sheet_name="buffer_to_buffer_latency", index=False)

# === Main ===
if __name__ == "__main__":
    path = os.path.dirname(os.path.abspath(__file__))
    param = sys.argv[1:]

    #file_name = "experiments/tg5/experiment_tg5_GWOR_b8"
    #map_file_name = "experiments/experiment_tg5.map"
    file_name = "experiment_tg3_lambda"
    map_file_name = "experiment_tg3.map"
    mapping = parse_task_to_pe_mapping(map_file_name)
    print(mapping)
    pe_data, buf_data = parse_log_file(file_name)
    print(pe_data)
    print(buf_data)
    
    process_and_export(pe_data, buf_data, mapping)
