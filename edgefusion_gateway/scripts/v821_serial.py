#!/usr/bin/env python3
"""
通过串口 /dev/ttyUSB0 操作 V821：杀掉旧 sample_smartIPC_demo，重启，捕获 RTSP 推流地址。
用法（root）: sudo python3 scripts/v821_serial.py
"""
import os, sys, time, select, re, subprocess

SERIAL = "/dev/ttyUSB0"

def open_serial():
    subprocess.run(["stty", "-F", SERIAL, "115200", "cs8", "-cstopb",
                    "-parenb", "raw", "-echo", "-ixon", "-ixoff"], check=True)
    return os.open(SERIAL, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)

def write(fd, s): os.write(fd, s.encode())
def read_for(fd, seconds, log=False):
    buf = b""; end = time.time() + seconds
    while time.time() < end:
        r, _, _ = select.select([fd], [], [], 0.2)
        if r:
            try: chunk = os.read(fd, 4096)
            except OSError: break
            if chunk:
                buf += chunk
                if log:
                    sys.stdout.write(chunk.decode('utf-8','replace')); sys.stdout.flush()
    return buf.decode('utf-8','replace')

def main():
    fd = open_serial()
    try:
        write(fd, "\r\n"); out = read_for(fd, 1.5, log=True)
        if "#" not in out and "$" not in out:
            write(fd, "\x03"); read_for(fd, 1.0, log=True)
            write(fd, "\r\n"); out += read_for(fd, 1.0, log=True)
        if "#" not in out:
            write(fd, "root\r\n"); read_for(fd, 1.0, log=True)

        # 杀掉旧 sample
        write(fd, "killall sample_smartIPC_demo 2>/dev/null\r\n")
        read_for(fd, 2.0, log=True)

        # 重启
        write(fd, "cd /mnt/extsd/\r\n"); read_for(fd, 1.0, log=True)
        write(fd, "./sample_smartIPC_demo -path ./sample_smartIPC_demo.conf\r\n")
        out = read_for(fd, 12.0, log=True)

        urls = sorted(set(re.findall(r'rtsp://[^\s"\'<>]+', out)))
        print("\n\n==== RTSP 推流地址 ====")
        for u in urls: print(" ", u)
        if not urls: print("  未找到 rtsp:// 地址")
        print("==== 程序已在 V821 运行 ====")
    finally:
        os.close(fd)

if __name__ == "__main__":
    main()
