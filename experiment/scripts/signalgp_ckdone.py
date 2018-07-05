"""
Module for use with qsub diagnostics script. 
Defines a function that checks if a given run is done. 
"""

import os

def CheckRunDone(run_fpath):
    fit_fpath = os.path.join(run_fpath, "output", "fitness.csv")
    file_content = None
    with open(fit_fpath, "r") as fp:
        file_content = fp.read().strip().split("\n")
    header = file_content[0].split(",")
    header_lu = {header[i].strip():i for i in range(0, len(header))}
    for line in file_content:
        line = line.split(",")
        if line[header_lu["update"]] == "50000":
            return True
    return False
        