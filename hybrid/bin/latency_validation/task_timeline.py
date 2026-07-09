import matplotlib.pyplot as plt

# 你的資料
tasks = [
    {"start": 1191, "end": 1242, "label": "(1, 2)", "pe": "(0, 1)"},
    {"start": 1241, "end": 1262, "label": "(1, 3)", "pe": "(0, 2)"},
    {"start": 1413, "end": 1532, "label": "(2, 4)", "pe": "(1, 3)"},
    {"start": 1413, "end": 1534, "label": "(3, 4)", "pe": "(2, 3)"},
]

fig, ax = plt.subplots(figsize=(8, 4))

# 畫出 timeline
for i, task in enumerate(tasks):
    ax.barh(
        y=i,
        width=task["end"] - task["start"],
        left=task["start"],
        height=0.4,
        align="center",
        label=f"{task['label']} {task['pe']}"
    )
    ax.text(task["start"], i, f"{task['label']} {task['pe']}", va="center", ha="left", fontsize=8)

# 美化
ax.set_xlabel("Time")
ax.set_ylabel("Tasks")
ax.set_yticks(range(len(tasks)))
ax.set_yticklabels([t["label"] for t in tasks])
ax.set_title("Task Timeline")
ax.grid(True, axis="x", linestyle="--", alpha=0.6)

plt.tight_layout()
plt.show()
