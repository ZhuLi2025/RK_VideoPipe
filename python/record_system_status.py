import psutil
import pandas as pd
import time
from datetime import datetime
import signal
import sys
import os
from tqdm import tqdm

class SystemRecorder:
    def __init__(self, interval=5, duration=3600, output_file="system_status.xlsx"):
        self.interval = interval
        self.duration = duration
        self.output_file = output_file
        self.records = []
        self.start_time = None
        self.running = True
        self.cpu_count = psutil.cpu_count(logical=True)

        # 注册信号（Ctrl+C 或 kill）
        signal.signal(signal.SIGINT, self.handle_exit)
        signal.signal(signal.SIGTERM, self.handle_exit)

    def handle_exit(self, signum, frame):
        """捕获中断信号并保存"""
        print(f"\n 检测到中断信号 ({signum})，正在保存当前数据...")
        self.running = False
        self.save_data()

    def collect_once(self):
        """采样一次系统状态"""
        ts = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

        # 获取 CPU 占用率（总 + 每核）
        cpu_total = psutil.cpu_percent(interval=None)
        cpu_per_core = psutil.cpu_percent(interval=None, percpu=True)

        mem = psutil.virtual_memory()
        swap = psutil.swap_memory()
        disk = psutil.disk_usage('/')
        load1, load5, load15 = psutil.getloadavg()

        # 构造记录字典
        record = {
            "timestamp": ts,
            "cpu_total_percent": cpu_total,
            "mem_used_MB": mem.used / 1024 / 1024,
            "mem_total_MB": mem.total / 1024 / 1024,
            "mem_percent": mem.percent,
            "swap_percent": swap.percent,
            "disk_used_GB": disk.used / 1024 / 1024 / 1024,
            "disk_total_GB": disk.total / 1024 / 1024 / 1024,
            "disk_percent": disk.percent,
            "load_1min": load1,
            "load_5min": load5,
            "load_15min": load15,
        }

        # 动态加入每个核心占用率
        for i, usage in enumerate(cpu_per_core):
            record[f"cpu_core_{i}_percent"] = usage

        self.records.append(record)

    def save_data(self):
        """保存采样结果至 Excel"""
        if not self.records:
            print("⚠️ 无采样数据，无需保存。")
            return
        df = pd.DataFrame(self.records)
        tmp_file = self.output_file
        df.to_excel(tmp_file, index=False)
        print(f"✅ 数据已保存至 {os.path.abspath(tmp_file)}")

    def run(self):
        """开始采集"""
        self.start_time = time.time()
        total_samples = self.duration // self.interval
        print(f"开始记录系统状态（间隔 {self.interval}s，计划持续 {self.duration/60:.1f} 分钟）...")
        print(f"检测到 {self.cpu_count} 个 CPU 核心，将分别记录占用率。")

        try:
            for i in tqdm(range(int(total_samples))):
                if not self.running:
                    break
                self.collect_once()
                elapsed = time.time() - self.start_time
                next_sample = (i + 1) * self.interval
                time.sleep(max(0, next_sample - elapsed))
        except KeyboardInterrupt:
            print("\n⚠️ 手动中断，正在保存当前数据...")
        except Exception as e:
            print(f"\n❌ 发生异常: {e}，正在保存数据...")
        finally:
            self.save_data()

if __name__ == "__main__":
    recorder = SystemRecorder(interval=5, duration=300)
    recorder.run()
