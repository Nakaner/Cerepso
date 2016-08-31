#! /usr/bin/env python3

import sys

pgimporter_running = False
second = 0
current_memory_usage = 0
logfile = open(sys.argv[1], "r")
csvfile = sys.stdout 
for line in logfile:
    if pgimporter_running == False:
        if line.find("=== start ===") >= 0:
            pgimporter_running = True
    elif line.find("grep") >= 0:
        continue
    elif line.startswith("====="):
        csvfile.write(str(second) + "\t" + str(current_memory_usage) + "\n")
        current_memory_usage = 0
        second += 1
    elif line.find("=== stop ===") >= 0:
        pgimporter_running = False
        logfile.close()
        csvfile.close()
        exit(0)
    else:
        line_elements = line.split(" ")
        try:
            if len(line_elements) > 1:
                current_memory_usage += int(line_elements[1])
        except ValueError:
            continue
logfile.close()
csvfile.close()
        