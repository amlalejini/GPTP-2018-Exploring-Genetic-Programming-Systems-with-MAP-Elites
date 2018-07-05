"""
qsub_diag.py

This script can be used as a tool to diagnose dist_qsub runs:
- What's finished?
- What's not finished?
- What's not finished but also no longer managed by dist_qsub?

Additionally, this script can be used to patch holes in dist_qsub runs. 


NOTES:
- Contents of [dest_dir]/qsub_files/
    - [RUN_NAME]_[RUN_RANGE].qsub_done
        - Submission file
        - NOTE - If you want to recover jobs with this file, make sure to flip 'export CPR=0' to 'export CPR=1'.
    - [RUN_NAME]_[RUN_RANGE].qsub_done_arrayjobs.txt
        - Status document
        - Jobs dist_qsub thinks are done. These might be jobs that are legitimately done, 
          jobs that crashed, etc. Regardless, they are jobs dist_qsub is done handling.
    - [RUN_NAME]_[RUN_RANGE].qsub_successor_jobs.txt
        - JobIDs

Goals:
- For checking if a run is finished:
    - Specify line content to look for in run.log
    - Specify python module that has a CheckRunDone(run_path) call


"""

import argparse, os, copy

# def DefaultCheckRunDone(run_fpath):
#     return False

def DefaultCheckRunDone(run_list_fpath, dstr):
    run_log = None
    with open(os.path.join(run_list_fpath, "run.log"), "r") as fp:
        run_log = fp.read().split("\n")
    for line in run_log:
        line = line.strip()
        if dstr in line: return True
    return False

def ModuleCheckRunDone(run_list_fpath, mstr):
    exec("from {} import CheckRunDone".format(mstr))
    return CheckRunDone(run_list_fpath)

def main():
    parser = argparse.ArgumentParser(description="dist_qsub diagnostics tool")
    parser.add_argument("run_list", type=str, help="Run list for runs we're diagnosing.")
    parser.add_argument("-done_str", type=str, help="The presence of this string in a line of a run's 'run.log' file indicates that the run has finished.")
    parser.add_argument("-done_py", type=str, help="Python module that contains a 'CheckRunDone(run_fpath)' function that returns true/false depending on if the specified run is finished. This argument takes precedence over the 'done_str' argument. NOTE: This module must be located where this script is executed.")

    args = parser.parse_args()

    run_list_fpath = args.run_list

    # Build CheckRunDone function: either load function from specified python module or use specified done_str argument.
    if (args.done_py != None):
        module_name = args.done_py.strip(".py")
        CheckRunDone = lambda r : ModuleCheckRunDone(r, module_name)
    elif (args.done_str != None):
        done_str = args.done_str
        CheckRunDone = lambda r : DefaultCheckRunDone(r, done_str)
    else:
        exit("No method for determining whether or not a run is finished was provided (-done_str, or -done_py). Exiting...")

    # Check that run list exists. 
    if (not os.path.isfile(run_list_fpath)):
        exit("Failed to find provided run_list file ({})! Exiting...".format(run_list_fpath))
    
    # Open run list, extract relevant information. 
    run_list_lines = None
    with open(run_list_fpath, "r") as fp:
        run_list_lines = fp.read().split("\n")

    # Extract destination directory and a list of runs we should expect to see in that directory. 
    rl_runs = []
    rl_arrays = []
    rl_settings = {}
    for line in run_list_lines:
        # First, cleanup line
        if line.find("#") > -1: line = line[:line.find("#")]
        line = line.strip().lstrip()
        # If line is empty post-cleanup, skip it. 
        if len(line) == 0 or line[0] == "#": continue 
        # Two possibilities for lines: parameter definition or run[set] definition.
        if line[:3] == "set":
            bits = line.split(" ")        
            rl_settings[bits[1]] = bits[2]
        else:
            bits = line.split(" ")
            runs_range = list(map(int,bits[0].split("..")))
            runs_name = bits[1]
            rl_runs += ["{}_{}".format(runs_name, i) for i in range(runs_range[0], runs_range[1]+1)]
            rl_arrays.append("{}_{}".format(runs_name, bits[0]))
    
    run_settings_out = "\n".join(["{} = {}".format(key, rl_settings[key]) for key in rl_settings])
    expected_runs_out = "\n".join([run for run in rl_runs])

    print("Run list settings: \n{}".format(run_settings_out))
    print("Expected runs: \n{}".format(expected_runs_out))

    # We need the destination directory.
    if (not "dest_dir" in rl_settings): exit("No 'dest_dir' setting specified in run_list file. Exiting...")
    dest_dir = rl_settings["dest_dir"]
    if (not os.path.isdir(dest_dir)): exit("Could not find specified dest_dir ({}). Exiting...".format(dest_dir))
    
    # Now, let's grab the qsub files. 
    qsubs_dir = os.path.join(dest_dir, "qsub_files")
    if (not os.path.isdir(qsubs_dir)): exit("Could not find 'qsub_files' directory. Exiting...")

    # Find the runs that dist_qsub is finished tracking.
    finished_tracking_runs = []
    for array in rl_arrays:
        qsub_done_arrayjobs_fpath = os.path.join(qsubs_dir, array + ".qsub_done_arrayjobs.txt")
        if (not os.path.isfile(qsub_done_arrayjobs_fpath)): exit("Could not find '{}'. Exiting...".format(qsub_done_arrayjobs_fpath))
        
        runs_name = "_".join(array.split("_")[:-1])
        runs_range = list(map(int,array.split("_")[-1].split("..")))

        # Load list of runs in this array that dist_qsub claims are done.
        done_arrayjobs_content = None
        with open(qsub_done_arrayjobs_fpath, "r") as fp:
            done_arrayjobs_content = fp.read().strip().split("\n")

        finished_tracking_runs += ["{}_{}".format(runs_name, int(i.strip()) + runs_range[0]) for i in done_arrayjobs_content]

    print("Runs that dist_qsub is finished tracking:\n{}".format(str(finished_tracking_runs)))
    
    # Classify runs:
    missing_runs = []       # - Not found (no run directory found in destination). NOTE: This is bad.
    finished_dropped = []   # - Done (CheckRunDone returns true) and found in 'done_arrayjobs' file. NOTE: This is where done jobs *should* get classified!
    finished_tracked = []   # - Done (CheckRunDone returns true) but not found in 'done_arrayjobs' file. NOTE: This is bad.
    unfinished_dropped = [] # - Not done & not tracked (by dist_qsub). NOTE: This is bad. 
    unfinished_tracked = [] # - Not done & tracked (by dist_qsub). NOTE: This is where unfinished jobs *should* get classified.
    
    for run in rl_runs:
        # Is run where we expect it?
        run_path = os.path.join(dest_dir, run)
        if (not os.path.isdir(run_path)):
            missing_runs.append(run)
            continue 
        # We can find the run, so is it finished?
        if (CheckRunDone(run_path)):
            if (run in finished_tracking_runs):
                finished_dropped.append(run)
            else:
                finished_tracked.append(run)
            continue
        
        # The run isn't *actually* finished and it's not missing. 
        # - The run must either still be running (managed by dist_qsub)
        # - Or, the run must have been dropped before finishing. 
        if (run in finished_tracking_runs):
            unfinished_dropped.append(run)
        else:
            unfinished_tracked.append(run)

    # Output diagnostics. 
    # - missing_runs
    # - finished_dropped
    # - finished_tracked
    # - unfinished_dropped
    # - unfinished_tracked
    accounted_runs = sum(list(map(len, [missing_runs, finished_dropped, finished_tracked, unfinished_dropped, unfinished_tracked])))
    print("Total runs: {}\nTotal runs accounted for: {}".format(len(rl_runs), accounted_runs))
    
    with open("MISSING.diag", "w") as fp:
        fp.write("\n".join(missing_runs))
    
    with open("FINISHED.diag", "w") as fp:
        fp.write("\n".join(finished_dropped))
    
    if (len(finished_tracked)):
        with open("FINISHED_TRACKED.diag", "w") as fp:
            fp.write("\n".join(finished_tracked))

    with open("UNFINISHED_DROPPED.diag", "w") as fp:
        fp.write("\n".join(unfinished_dropped))
    
    with open("UNFINISHED_TRACKED.diag", "w") as fp:
        fp.write("\n".join(unfinished_tracked))
        
if __name__ == "__main__":
    main()