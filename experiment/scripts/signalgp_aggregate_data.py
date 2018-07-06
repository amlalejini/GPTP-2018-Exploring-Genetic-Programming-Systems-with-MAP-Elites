"""
signalgp_aggregate_data.py

This script aggregates experiment data collected on the HPCC into more concise .csv files, 
which will be used by our data analysis/data visualization scripts. 

"""


import argparse, os, copy, errno

aggregator_dump = "./aggregated_data"


def mkdir_p(path):
    """
    This is functionally equivalent to the mkdir -p [fname] bash command
    """
    try:
        os.makedirs(path)
    except OSError as exc: # Python >2.5
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else: raise

def main():
    pass

if __name__ == "__main__":
    main()