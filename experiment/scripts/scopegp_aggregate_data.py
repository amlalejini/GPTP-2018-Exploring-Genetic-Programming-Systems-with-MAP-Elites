"""
scopegp_aggregate_data.py

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
    parser = argparse.ArgumentParser(description="data aggregation script")
    parser.add_argument("data_directory", type=str, help="Target experiment data directory.")
    parser.add_argument("-T", "--trait_snapshots", action="store_true", help="COMMAND: Aggregate trait snapshots (traits.dat files).")
    parser.add_argument("-u", "--update", type=int, help="What update should aggregate? By default, aggregate *all* available updates.")
    parser.add_argument("-dump_dir", type=str, help="Directory to dump aggregated data. Default = './aggregated_data/")
    
    # Extract command line arguments.
    args = parser.parse_args()
    data_directory = args.data_directory
    dump_dir = args.dump_dir if (args.dump_dir != None) else aggregator_dump
    target_update = args.update
    single_update = target_update != None
    
    # Validate input.
    if (not os.path.isdir(data_directory)): exit("Failed to find specified data_directory ({}). Exiting...".format(data_directory))
    # If dump doesn't exist yet, make it!
    if (not os.path.isdir(dump_dir)):
        mkdir_p(dump_dir)
    
    # Aggregate population snapshots.
    if (args.trait_snapshots):
        print("Aggregating ScopeGP trait snapshots!")
        agg_info = ["run_id","agent_id","selection_method","problem",
                    "update","instruction_entropy","scope_count","fitness",
                    "scope_count_bin","inst_ent_bin"]
        agg_content = ",".join(agg_info) + "\n"

        # Data columns to pull from data files:
        # id,fitness,tag_sim_thresh,inst_cnt,inst_entropy,func_cnt,func_used,func_entered,func_entered_entropy,InstructionEntropy__bin,FunctionsUsed__bin
        # Get a list of all runs. 
        runs = [d for d in os.listdir(data_directory) if "SEL_" in d and "PROB_" in d and os.path.isdir(os.path.join(data_directory, d))]
        runs.sort()
        for run in runs:
            # Extract run information.
            run_dir = os.path.join(data_directory, run)
            output_dir = run_dir
            run_id = run.split("_")[-1]
            # run_params = "_".join(run.split("_")[:-1])
            print("============ Run: "+run)
            run_info = {pair.split("_")[0]:pair.split("_")[1] for pair in run.split("__")}
            print(run_info)
            
            # Validate that the snapshot file exists.
            trait_snapshot_fpath = os.path.join(output_dir, "traits.dat")
            if (not os.path.isfile(trait_snapshot_fpath)):
                print("Could not find trait snapshot file: {}\n  Skipping.".format(trait_snapshot_fpath))
                continue    
            file_content = None
            with open(trait_snapshot_fpath, "r") as fp: file_content = fp.readlines()
            header = file_content[0].split(",")
            header_lu = {header[i].strip():i for i in range(0, len(header))}
            file_content = file_content[1:]
            for line in file_content:
                line = line.strip().split(",")
                info = {}
                try:
                    info["run_id"] = run_id
                    info["agent_id"] = line[header_lu["id"]]
                    info["selection_method"] = run_info["SEL"]
                    info["problem"] = run_info["PROB"]
                    info["update"] = line[header_lu["update"]]
                    info["fitness"] = line[header_lu["fitness"]]
                    info["instruction_entropy"] = line[header_lu["instruction_entropy"]]
                    info["scope_count"] = line[header_lu["scope_count"]]
                    info["scope_count_bin"] = line[header_lu["scope_count_bin"]]
                    info["inst_ent_bin"] = line[header_lu["inst_ent_bin"]]
                except:
                    print("Failed to properly trait data from line: {} \n  Skipping.".format(line))
                    continue
                agg_content += ",".join([info[thing] for thing in agg_info]) + "\n"
        fname = "scopegp_trait_{}_data.csv".format(target_update) if (single_update) else "scopegp_trait_data.csv"
        with open(os.path.join(dump_dir, fname), "w") as fp:
            fp.write(agg_content)



          

if __name__ == "__main__":
    main()