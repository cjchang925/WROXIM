import pandas as pd
import re
import sys
import os
# 原始任務資料
# data = [
#     (0, 102, 2, 100),
#     (0, 91, 7, 97),
#     (0, 109, 11, 95),
#     (2, 98, 3, 91),
#     (3, 99, 4, 102),
#     (4, 94, 5, 105),
#     (5, 99, 6, 102),
#     (6, 103, 1, 101),
#     (8, 102, 9, 109),
#     (9, 95, 10, 100),
#     (9, 102, 15, 109),
#     (11, 102, 12, 96),
#     (12, 105, 13, 105),
#     (13, 94, 1, 105),
#     (10, 108, 14, 95),
#     (14, 108, 15, 96),
#     (15, 110, 16, 98),
#     (16, 98, 17, 106),
#     (17, 109, 1, 93),
#     (7, 91, 18, 97),
#     (7, 108, 23, 96),
#     (7, 109, 28, 93),
#     (18, 105, 19, 105),
#     (18, 105, 30, 105),
#     (19, 107, 20, 97),
#     (20, 102, 21, 100),
#     (20, 108, 31, 96),
#     (21, 102, 22, 109),
#     (22, 103, 8, 92),
#     (23, 103, 24, 96),
#     (24, 98, 25, 91),
#     (24, 100, 27, 92),
#     (24, 105, 30, 105),
#     (25, 109, 26, 95),
#     (26, 95, 27, 100),
#     (27, 95, 8, 100),
#     (27, 107, 22, 92),
#     (28, 105, 29, 95),
#     (29, 91, 30, 107),
#     (30, 98, 31, 91),
#     (31, 108, 8, 98),
#     (1, 108, -1, 0),
# ]


path = sys.path[0]
print(path)

#map_filename = "experiment_tg5"
map_filename = "contention_demo_large"

with open(os.path.join(path, "..", "..", "tm_examples", f"{map_filename}.map"), "r") as file:
    lines = file.readlines()


data = []
task_to_pe = {}

for line in lines:
    if line.startswith('map:'):
        nums = list(map(int, re.findall(r'-?\d+', line)))
        task_pe_pairs = nums[2:]  # 跳過前兩個數值
        task_to_pe = {task_pe_pairs[i]: task_pe_pairs[i + 1] for i in range(0, len(task_pe_pairs), 2)}
        break
print(task_to_pe)
for line in lines:
    if line.startswith('task:'):
        nums = list(map(int, re.findall(r'-?\d+', line)))
        task = nums[0]
        for i in range(1, len(nums), 4):
            if i + 2 < len(nums):
                exe = nums[i]
                sub_task = nums[i+1]
                payload = nums[i+2]
                if (task in task_to_pe) and (sub_task in task_to_pe) and (task_to_pe[task] == task_to_pe[sub_task]) or sub_task == -1:
                    continue
                data.append((task, exe, sub_task, payload))

# 顯示結果
df = pd.DataFrame(data, columns=["task", "exe", "sub_task", "payload"])
print(df)
# exit()


optical_cycle_period = 0.025  # ns
PE_cycle_period = 1 #ns
R_oeeo = 1/optical_cycle_period
W_payload = 1

E_O_LATENCY = W_payload/R_oeeo
O_E_LATENCY = W_payload/R_oeeo
flit_size = 34  # flit size in bits
# 延遲參數
OEEO_latency = E_O_LATENCY + O_E_LATENCY

D_FLIT = 2*PE_cycle_period + optical_cycle_period*2  + OEEO_latency*flit_size
D_ACK = optical_cycle_period*2 + OEEO_latency

D_Buf_Buf = optical_cycle_period*2  + OEEO_latency*flit_size
D_PE_Buf = 1 * PE_cycle_period
D_Buf_PE = 1 * PE_cycle_period
print("E_O_LATENCY:", E_O_LATENCY, "ns")
print("O_E_LATENCY:", O_E_LATENCY, "ns")
print("D_FLIT:", D_FLIT, "ns")
print("D_ACK:", D_ACK, "ns")

print("D_Buf_Buf:", D_Buf_Buf, "ns")
print("D_PE_Buf:", D_PE_Buf, "ns")
print("D_Buf_PE:", D_Buf_PE, "ns")


# 定義通訊延遲公式
def compute_latency(n_flits):
    return 0.0 if n_flits == 0 else D_PE_Buf + (D_Buf_Buf + (n_flits - 1)*(D_Buf_Buf + D_ACK)) + D_PE_Buf
def compute_oeeo_latency(n_flits):
    return 0.0 if n_flits == 0 else (D_Buf_Buf + (n_flits - 1)*(D_Buf_Buf + D_ACK))  * 1000
# 計算通訊延遲與總時間
df["D_total_ns"] = df["payload"].apply(compute_latency)
df["D_oeeo_ps"] = df["payload"].apply(compute_oeeo_latency)

# 顯示結果
df = df.sort_values(by=["task", "sub_task"]).reset_index(drop=True)
df["(task, sub_task)"] = df.apply(
    lambda row: f"({int(row['task'])}, {int(row['sub_task'])})", axis=1)
df = df.drop(columns=["task", "sub_task"])
print(df)
df.to_excel(path + f"/latency_excel/Ideal_latency_{map_filename}.xlsx", index=False)