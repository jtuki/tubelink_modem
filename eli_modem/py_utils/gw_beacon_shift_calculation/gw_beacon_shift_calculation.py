# -*- encoding: utf-8 -*-

import re

beacon_log_pattern = \
    re.compile(
    """\d\d/\d\d-(\d\d):(\d\d):(\d\d):(\d\d\d)\s(\w+)\.(\d\d\d)\s*:\s*"""
    )

gl_beacons = []

def read_beacon_log_file(filename):
    r"""注意，这个文件里只能存在类似如下格式的日志：
    09/18-16:40:54:694 2.006  : 
    09/18-16:44:13:110 C6.006  : 
    下一行的时间应该大于上一行的时间
    
    update 2015-11-04 - 去掉了后面的 gw beacon，适用于一般的分析。
    """
    global gl_beacons
    with open(filename) as f:
        for line in f:
            matched = beacon_log_pattern.match(line)
            assert matched
            h = int(matched.group(1))
            m = int(matched.group(2))
            s = int(matched.group(3))
            ms = int(matched.group(4))
            mcu_s = int(matched.group(5), 16) # hex format
            mcu_ms = int(matched.group(6))
            
            beacon_time = (h,m,s,ms, mcu_s,mcu_ms)
            
            gl_beacons.append(beacon_time)
    
def calculate_delta(btime1, btime2):
    r"""计算两个 beacon time 的差值。
    参数格式： beacon_time = (h,m,s,ms, mcu_s,mcu_ms)
    """
    h,m,s,ms, mcu_s,mcu_ms = btime1
    pc_time1 = 1000*(3600*h + 60*m + s) + ms
    mcu_time1 = 1000*mcu_s + mcu_ms
    
    h,m,s,ms, mcu_s,mcu_ms = btime2
    pc_time2 = 1000*(3600*h + 60*m + s) + ms
    mcu_time2 = 1000*mcu_s + mcu_ms
    
    
    assert pc_time2 > pc_time1 and mcu_time2 > mcu_time1
    pc_delta_ms = pc_time2 - pc_time1
    mcu_delta_ms = mcu_time2 - mcu_time1
    
    print ("pc_delta_ms:%d; mcu_delta_ms:%d; (pc-mcu)/mcu:%f" 
            % (pc_delta_ms, mcu_delta_ms, (pc_delta_ms - mcu_delta_ms) / (mcu_delta_ms * 1.0)))
            
if __name__ == '__main__':
    read_beacon_log_file('beacon_log_file.txt')
    
    prev = next = None
    for log in gl_beacons:
        if not prev:
            prev = log
            continue
        else:
            next = log
            calculate_delta(prev, next)
            prev = log