#ifndef MAPE_GP_WORLD_H
#define MAPE_GP_WORLD_H

#include <iostream>
#include <functional>
#include <string>
#include <fstream>
#include <sys/stat.h>


// Empirical includes
#include "base/Ptr.h"
#include "base/vector.h"

#include "control/Signal.h"

#include "Evolve/World.h"

#include "hardware/EventDrivenGP.h"
#include "hardware/InstLib.h"
#include "hardware/EventLib.h"
#include "hardware/signalgp_utils.h"

#include "tools/BitVector.h"
#include "tools/Random.h"
#include "tools/random_utils.h"
#include "tools/math.h"
#include "tools/string_utils.h"
#include "tools/stats.h"

// Experiment-specific includes
#include "MapElitesGP_Config.h"
#include "MapElitesGP_Org.h"
#include "MapElitesGP_World.h"

#include "PhenotypeCache.h"

// Major TODOS: 
// - [ ] Add data tracking (snapshots, fitness, dominant, etc)
// - [ ] Add logic 9 problem 
// - [ ] Add testcase problems
// - [ ] Testing/debugging

class MapElitesGPWorld : public emp::World<MapElitesGPOrg> {
public:
  static constexpr double MIN_POSSIBLE_SCORE = -32767;

  enum class WORLD_MODE { EA=0, MAPE=1 };
  enum class PROBLEM_TYPE { CHG_ENV=0, TESTCASES=1 };
  enum class SELECTION_METHOD { TOURNAMENT=0 };
  enum class POP_INIT_METHOD { RANDOM=0, ANCESTOR=1 };
  enum class EVAL_TRIAL_AGG_METHOD { MIN=0, MAX=1, AVG=2 }; 
  enum class CHGENV_TAG_GEN_METHOD { RANDOM=0, LOAD=1 }; 
  enum class ENV_CHG_METHOD { SHUFFLE=0, CYCLE=1, RAND=2 };
  struct OrgPhenotype;

  using org_t = MapElitesGPOrg; 
  using phenotype_t = OrgPhenotype;
  using hardware_t = typename org_t::hardware_t;
  using program_t = typename org_t::program_t;
  using genome_t = typename org_t::genome_t;
  using inst_lib_t = typename org_t::inst_lib_t;
  using event_lib_t = typename org_t::event_lib_t;
  using inst_t = typename hardware_t::inst_t;
  using event_t = typename hardware_t::event_t;
  using state_t = typename hardware_t::State;
  using tag_t = typename hardware_t::affinity_t;
  using trait_id_t = typename org_t::HW_TRAIT_ID;

  using mut_fun_t = std::function<size_t(org_t &, emp::Random &)>;
  using score_fun_t = std::function<double(org_t &, phenotype_t &)>;

  struct OrgPhenotype {
    // Generic
    double score;  
    std::unordered_set<size_t> functions_used_set;
    emp::vector<size_t> function_entries;           ///< All functions entered (allows repeats), in the order they were entered.

    // For changing environment problem
    double env_match_score;

    // For logic 9 problem
    // TODO
    // For testcases problem
    // TODO

    void Reset() {
      score = 0;
      env_match_score = 0;
      function_entries.clear();
      functions_used_set.clear();
    }
  };
  
protected:
  // Localized configurable parameters
  // == General Group ==
  size_t RUN_MODE;
  int RANDOM_SEED;
  size_t POP_SIZE;
  size_t GENERATIONS;
  size_t POP_INIT_METHOD;
  std::string ANCESTOR_FPATH;
  // == Evaluation group ==
  size_t EVAL_TRIAL_CNT;
  size_t EVAL_TRIAL_AGG_METHOD;
  size_t EVAL_TIME;
  // == Selection group ==
  size_t SELECTION_METHOD;
  size_t ELITE_CNT;
  size_t TOURNAMENT_SIZE;
  // == MAPE group ==
  bool USE_MAPE_AXIS__INST_ENTROPY;
  bool USE_MAPE_AXIS__INST_CNT;
  bool USE_MAPE_AXIS__FUNC_USED;
  bool USE_MAPE_AXIS__FUNC_CNT;
  bool USE_MAPE_AXIS__FUNC_ENTERED;
  bool USE_MAPE_AXIS__FUNC_ENTERED_ENTROPY;
  size_t MAPE_AXIS_SIZE__INST_ENTROPY;
  size_t MAPE_AXIS_SIZE__FUNC_ENTERED_ENTROPY;
  // == Problem group ==
  size_t PROBLEM_TYPE;
  std::string TESTCASES_FPATH;
  // == Changing environment group ==
  size_t ENV_TAG_GEN_METHOD;
  std::string ENV_TAG_FPATH;
  size_t ENV_STATE_CNT;
  bool ENV_CHG_SIG;
  bool ENV_DISTRACTION_SIGS;
  size_t ENV_DISTRACTION_SIG_CNT;
  size_t ENV_CHG_METHOD;
  double ENV_CHG_PROB;
  size_t ENV_CHG_RATE;
  bool ENV_SENSORS;
  // == Program constraints group ==
  size_t PROG_MIN_FUNC_CNT;
  size_t PROG_MAX_FUNC_CNT;
  size_t PROG_MIN_FUNC_LEN;
  size_t PROG_MAX_FUNC_LEN;
  size_t PROG_MAX_TOTAL_LEN;
  int PROG_MIN_ARG_VAL;
  int PROG_MAX_ARG_VAL;
  // == Mutation group ==
  double ARG_SUB__PER_ARG;
  double INST_SUB__PER_INST;
  double INST_INS__PER_INST;
  double INST_DEL__PER_INST;
  double SLIP__PER_FUNC;
  double FUNC_DUP__PER_FUNC;
  double FUNC_DEL__PER_FUNC;
  double TAG_BIT_FLIP__PER_BIT;
  bool EVOLVE_HW_TAG_SIM_THRESH;
  // == Hardware group ==
  size_t HW_MAX_THREAD_CNT;
  size_t HW_MAX_CALL_DEPTH;
  double HW_MIN_TAG_SIMILARITY_THRESH;
  // == Data tracking group ==
  std::string DATA_DIRECTORY;
  size_t STATISTICS_INTERVAL;
  size_t SNAPSHOT_INTERVAL;

  emp::SignalGPMutator<org_t::TAG_WIDTH> mutator;
  emp::vector<mut_fun_t> mut_funs;

  inst_lib_t inst_lib;
  event_lib_t event_lib;

  emp::Ptr<hardware_t> eval_hw;

  PhenotypeCache<OrgPhenotype> phen_cache;  // NOTE: cache is not necessarily accurate for everyone in pop during MAPE
  score_fun_t calc_score;
  std::function<double(org_t &)> agg_scores;

  std::function<int(org_t &)> inst_cnt_fun;
  std::function<double(org_t &)> inst_ent_fun;
  std::function<double(org_t &)> func_used_fun;
  std::function<double(org_t &)> func_entered_cnt_fun;
  std::function<double(org_t &)> func_entered_ent_fun;
  std::function<int(org_t &)> func_cnt_fun;

  size_t eval_time;
  size_t trial_id;

  double max_inst_entropy; // TODO: calculate after all instructions have been added to instruction set
  double max_func_entered_entropy; 

  double best_score;
  size_t dominant_id; 

  // == Problem-specific world info ==
  /// World info relevant to changing environment problem. 
  struct ChgEnvProblemInfo {
      emp::vector<tag_t> env_state_tags;        ///< Tags associated with each environment state.
      emp::vector<tag_t> distraction_sig_tags;  ///< Tags associated with distraction signals.
      emp::vector<size_t> env_shuffler;         ///< Used for keeping track of shuffled environment cycling.
      size_t env_shuffle_id;
      size_t env_state;
      
      void ResetEnv(emp::Random & rnd) { 
        emp::Shuffle(rnd, env_shuffler);
        env_shuffle_id=0;
        env_state=(size_t)-1; 
      }
  } chgenv_info;

  // Run signals
  emp::Signal<void(void)> do_begin_run_sig; 
  emp::Signal<void(void)> do_pop_init_sig;
  emp::Signal<void(void)> do_evaluation_sig;    ///< Specific to run mode. Setup by RUN_MODE/WORLD_MODE setup. 
  emp::Signal<void(void)> do_selection_sig;     ///< Specific to run mode. Setup by RUN_MODE/WORLD_MODE setup.
  emp::Signal<void(void)> do_world_update_sig;  ///< Generic. Setup during general Setup.  

  // TODO: when caching phenotypes, make cache max_env size + 1; use last position for MAP-elites eval
  //  - Use GetSize() for max capacity

  // Data-tracking signals
  emp::Signal<void(void)> do_pop_snapshot_sig;

  // Fitness evaluation signals
  emp::Signal<void(org_t &)> begin_org_eval_sig;  ///< Triggered at beginning of agent evaluation (might be multiple trials)
  emp::Signal<void(org_t &)> end_org_eval_sig;    ///< Triggered at beginning of agent evaluation (might be multiple trials)

  emp::Signal<void(org_t &)> begin_org_trial_sig; ///< Triggered at the beginning of an agent trial.
  emp::Signal<void(org_t &)> do_org_trial_sig;    ///< Triggered at the beginning of an agent trial.
  emp::Signal<void(org_t &)> end_org_trial_sig;   ///< Triggered at the beginning of an agent trial.

  emp::Signal<void(org_t &)> do_org_advance_sig;  ///< When triggered, advance SignalGP evaluation hardware
  emp::Signal<void(void)> do_env_advance_sig;         ///< When triggered, advance the environment by one step

  // === Configuration functions ===
  void Init_Configs(MapElitesGPConfig & config);
  void Init_Problem();
  void Init_Mutator();
  void Init_Hardware();
  void Init_WorldMode();

  void SetupProblem_ChgEnv();
  void SetupProblem_Testcases();

  void SetupWorldMode_EA();
  void SetupWorldMode_MAPE();

  // === Population initialization (things that put stuff into the population) ===
  void InitPop_Random();
  void InitPop_Ancestor();

  // === Changing environment utility functions ===
  void SaveChgEnvTags();
  void LoadChgEnvTags();

  // === Data collection/tracking functions ===
  /// Snapshot all programs for current update.
  void Snapshot_Programs();
  
  /// Snapshot population statistics for current update.
  void Snapshot_PopulationStats();
  
  /// Snapshot dominant program performance over many trials (only makes sense in context of EA run). 
  void Snapshot_Dominant();

  /// Snapshot map from MAP-elites (only makes sense in context of MAP-Elites run). 
  void Snapshot_MAP();

  /// Add a data file to track dominant program. Will track at same interval as fitness file. (only makes sense in context of EA run).
  emp::DataFile & AddDominantFile(const std::string & fpath);


  // === Eval hardware utility functions ===
  void ResetEvalHW() {
    eval_hw->ResetHardware();
    // TODO: add signal for onreset hardware
    eval_hw->SetTrait(trait_id_t::ORG_ID, -1);
    eval_hw->SetTrait(trait_id_t::PROBLEM_OUTPUT, -1);
    eval_hw->SetTrait(trait_id_t::ORG_STATE, -1);
  }

  // === Evaluation functions ===
  /// Evaluate given agent.
  void Evaluate(org_t & org) {
    begin_org_eval_sig.Trigger(org);  //? Can I keep trial ID local? 
    for (trial_id = 0; trial_id < EVAL_TRIAL_CNT; ++trial_id) {
      begin_org_trial_sig.Trigger(org);
      do_org_trial_sig.Trigger(org);
      end_org_trial_sig.Trigger(org);
    }
    end_org_eval_sig.Trigger(org);
  }

  /// Used to poke the world as I develop it. 
  void Test() {
    // TODO: run environment for a bit, check changing
    // for (eval_time = 0; eval_time < 100; ++eval_time) {

    // }
  }

public:
  MapElitesGPWorld() : emp::World<org_t>() { ; }
  MapElitesGPWorld(emp::Random & rnd) : emp::World<org_t>(rnd) { ; }
  ~MapElitesGPWorld() {
    eval_hw.Delete(); // Clean up evaluation hardware. 
  }

  // === Configuration/setup functions ===
  /// Configure the experiment
  void Setup(MapElitesGPConfig & config);

  // === Run functions ===
  void Run();
  void RunStep();

  // === Evolution functions ===

};

// World function implementations!

// === Configuration/Setup functions! ===
void MapElitesGPWorld::Setup(MapElitesGPConfig & config) {
  Reset();              // Reset the world
  SetCache();           // We'll be caching fitness scores
  Init_Configs(config); // Initialize configs

  // Setup 
  do_begin_run_sig.AddAction([this]() {
    // Calculate ranges for phenotypic traits. 
    max_inst_entropy = -1 * emp::Log2(1.0/((double)inst_lib.GetSize())); // Instruction set must be locked in by this point. 
    max_func_entered_entropy = -1 * emp::Log2(1.0/((double)PROG_MAX_FUNC_CNT));
  });

  // Configure world update signal. 
  do_world_update_sig.AddAction([this]() {
    std::cout << "Update: " << GetUpdate() << " Max score: " << best_score << std::endl;
    // do_pop_snapshot_sig.Trigger();
    if (update % SNAPSHOT_INTERVAL == 0) do_pop_snapshot_sig.Trigger();
    Update(); 
  });

  // Generic evaluation signal actions. 
  // - At beginning of agent evaluation. 
  begin_org_eval_sig.AddAction([this](org_t & org) {
    eval_hw->SetProgram(org.GetProgram());
  });
  
  // Setup evaluation trial signals
  // - Begin trial
  begin_org_trial_sig.AddAction([this](org_t & org) {
    // Reset hardware.
    ResetEvalHW(); 
    // Reset phenotype
    phen_cache.Get(org.GetPos(), trial_id).Reset();
    // Set org ID in hardware.
    eval_hw->SetTrait(trait_id_t::ORG_ID, org.GetPos());
  });
  // - Do trial
  do_org_trial_sig.AddAction([this](org_t & org) {
    for (eval_time = 0; eval_time < EVAL_TIME; ++eval_time) {
      // 1) Advance environment.
      do_env_advance_sig.Trigger();
      // 2) Advance agent.
      do_org_advance_sig.Trigger(org);
    }
  });
  // - End trial
  end_org_trial_sig.AddAction([this](org_t & org) {
    const size_t id = org.GetPos();
    phenotype_t & phen = phen_cache.Get(id, trial_id); 
    phen.score = calc_score(org, phen);
  });

  // Setup organism advance signal. 
  do_org_advance_sig.AddAction([this](org_t & org) {
    eval_hw->SingleProcess();
  });

  // Setup descriptive functions used by MAP-Elites (and data tracking, etc). 
  // - Based on organism genome
  inst_cnt_fun = [](org_t & org) { return org.GetInstCnt(); };
  inst_ent_fun = [](org_t & org) { return org.GetInstEntropy(); };
  func_cnt_fun = [](org_t & org) { return org.GetFunctionCnt(); };
  // - Based on program execution 
  func_used_fun = [this](org_t & org) {
    double total = 0;
    for (size_t t = 0; t < EVAL_TRIAL_CNT; ++t) {
      total += phen_cache.Get(org.GetPos(), t).functions_used_set.size();
    }
    return total / EVAL_TRIAL_CNT;
  };

  func_entered_cnt_fun = [this](org_t & org) {
    double total = 0;
    for (size_t t = 0; t < EVAL_TRIAL_CNT; ++t) {
      total += phen_cache.Get(org.GetPos(), t).function_entries.size();
    }
    return total / EVAL_TRIAL_CNT;
  };

  func_entered_ent_fun = [this](org_t & org) {
    double total = 0; 
    for (size_t t = 0; t < EVAL_TRIAL_CNT; ++t) {
      total += emp::ShannonEntropy(phen_cache.Get(org.GetPos(), t).function_entries);
    }
    return total / EVAL_TRIAL_CNT;
  };

  // TODO: check placement stuff w/MAPE

  OnPlacement([this](size_t pos) {
    org_t & org = GetOrg(pos);
    org.SetPos(pos);
  });
  
  Init_Mutator();       // Configure SignalGPMutator, mutation function.
  Init_Hardware();      // Configure SignalGP hardware. 
  Init_Problem();       // Configure problem.
  Init_WorldMode();      // Configure run (MAP-Eltes vs. EA, etc)
  
  #ifndef EMSCRIPTEN
  // Make a data directory. 
  mkdir(DATA_DIRECTORY.c_str(), ACCESSPERMS);
  if (DATA_DIRECTORY.back() != '/') DATA_DIRECTORY += '/';

  // Setup generic snapshots. 
  do_pop_snapshot_sig.AddAction([this]() { this->Snapshot_Programs(); });
  do_pop_snapshot_sig.AddAction([this]() { this->Snapshot_PopulationStats(); });
  
  // Setup fitness tracking. 
  SetupFitnessFile(DATA_DIRECTORY + "fitness.csv").SetTimingRepeat(STATISTICS_INTERVAL);
  
  #endif

  // - Setup data tracking (but only in native mode)
  //    - Pop snapshot
  //    - Fitness file
  //    - Dominant file
  // - Setup selection
  //   - MAPE vs. EA
  //      - Setup phen cache size

  // Setup the fitness function
  switch (EVAL_TRIAL_AGG_METHOD) {
    case (size_t)EVAL_TRIAL_AGG_METHOD::MIN: {
      agg_scores = [this](org_t & org) {
        double score = phen_cache.Get(org.GetPos(), 0).score;
        for (size_t tID = 1; tID < EVAL_TRIAL_CNT; ++tID) {
          double other_score = phen_cache.Get(org.GetPos(), tID).score;
          if (other_score < score) score = other_score;
        }
        return score;
      };
      break;
    }
    case (size_t)EVAL_TRIAL_AGG_METHOD::MAX: {
      agg_scores = [this](org_t & org) {
        double score = phen_cache.Get(org.GetPos(), 0).score;
        for (size_t tID = 1; tID < EVAL_TRIAL_CNT; ++tID) {
          double other_score = phen_cache.Get(org.GetPos(), tID).score;
          if (other_score > score) score = other_score;
        }
        return score;
      };
      break;
    }
    case (size_t)EVAL_TRIAL_AGG_METHOD::AVG: {
      agg_scores = [this](org_t & org) {
        double agg_score = phen_cache.Get(org.GetPos(), 0).score;
        for (size_t tID = 1; tID < EVAL_TRIAL_CNT; ++tID) {
          agg_score += phen_cache.Get(org.GetPos(), tID).score;
        }
        return agg_score / EVAL_TRIAL_CNT;

      };
      break;
    }
    default: {
      std::cout << "Unrecognized EVAL_TRIAL_AGG_METHOD (" << EVAL_TRIAL_AGG_METHOD << "). Exiting..." << std::endl;
      exit(-1);
    }
  }

  // TODO: depending on problem, spawn core!

  // Initialize the population
  switch (POP_INIT_METHOD) {
    case (size_t)POP_INIT_METHOD::RANDOM: {
      do_pop_init_sig.AddAction([this]() {
        InitPop_Random();
      });
      break;
    }
    case (size_t)POP_INIT_METHOD::ANCESTOR: {
      do_pop_init_sig.AddAction([this]() {
        InitPop_Ancestor();
      });
      break;
    }
    default: {
      std::cout << "Unrecognized POP_INIT_METHOD (" << POP_INIT_METHOD << "). Exiting..." << std::endl;
      exit(-1);
    }
  }
}

void MapElitesGPWorld::Init_Configs(MapElitesGPConfig & config) {
  RUN_MODE = config.RUN_MODE();
  RANDOM_SEED = config.RANDOM_SEED();
  POP_SIZE = config.POP_SIZE();
  GENERATIONS = config.GENERATIONS();
  POP_INIT_METHOD = config.POP_INIT_METHOD();
  ANCESTOR_FPATH = config.ANCESTOR_FPATH();

  EVAL_TRIAL_CNT = config.EVAL_TRIAL_CNT();
  EVAL_TRIAL_AGG_METHOD = config.EVAL_TRIAL_AGG_METHOD();
  EVAL_TIME = config.EVAL_TIME();

  SELECTION_METHOD = config.SELECTION_METHOD();
  ELITE_CNT = config.ELITE_CNT();
  TOURNAMENT_SIZE = config.TOURNAMENT_SIZE();

  USE_MAPE_AXIS__INST_ENTROPY = config.USE_MAPE_AXIS__INST_ENTROPY();
  USE_MAPE_AXIS__INST_CNT = config.USE_MAPE_AXIS__INST_CNT();
  USE_MAPE_AXIS__FUNC_USED = config.USE_MAPE_AXIS__FUNC_USED();
  USE_MAPE_AXIS__FUNC_CNT = config.USE_MAPE_AXIS__FUNC_CNT();
  USE_MAPE_AXIS__FUNC_ENTERED = config.USE_MAPE_AXIS__FUNC_ENTERED();
  USE_MAPE_AXIS__FUNC_ENTERED_ENTROPY = config.USE_MAPE_AXIS__FUNC_ENTERED_ENTROPY();
  MAPE_AXIS_SIZE__INST_ENTROPY = config.MAPE_AXIS_SIZE__INST_ENTROPY();
  MAPE_AXIS_SIZE__FUNC_ENTERED_ENTROPY = config.MAPE_AXIS_SIZE__FUNC_ENTERED_ENTROPY();

  PROBLEM_TYPE = config.PROBLEM_TYPE();
  TESTCASES_FPATH = config.TESTCASES_FPATH();

  ENV_TAG_GEN_METHOD = config.ENV_TAG_GEN_METHOD();
  ENV_TAG_FPATH = config.ENV_TAG_FPATH();
  ENV_STATE_CNT = config.ENV_STATE_CNT();
  ENV_CHG_SIG = config.ENV_CHG_SIG();
  ENV_DISTRACTION_SIGS = config.ENV_DISTRACTION_SIGS();
  ENV_DISTRACTION_SIG_CNT = config.ENV_DISTRACTION_SIG_CNT();
  ENV_CHG_METHOD = config.ENV_CHG_METHOD();
  ENV_CHG_PROB = config.ENV_CHG_PROB();
  ENV_CHG_RATE = config.ENV_CHG_RATE();
  ENV_SENSORS = config.ENV_SENSORS();

  PROG_MIN_FUNC_CNT = config.PROG_MIN_FUNC_CNT();
  PROG_MAX_FUNC_CNT = config.PROG_MAX_FUNC_CNT();
  PROG_MIN_FUNC_LEN = config.PROG_MIN_FUNC_LEN();
  PROG_MAX_FUNC_LEN = config.PROG_MAX_FUNC_LEN();
  PROG_MAX_TOTAL_LEN = config.PROG_MAX_TOTAL_LEN();
  PROG_MIN_ARG_VAL = config.PROG_MIN_ARG_VAL();
  PROG_MAX_ARG_VAL = config.PROG_MAX_ARG_VAL();

  ARG_SUB__PER_ARG = config.ARG_SUB__PER_ARG();
  INST_SUB__PER_INST = config.INST_SUB__PER_INST();
  INST_INS__PER_INST = config.INST_INS__PER_INST();
  INST_DEL__PER_INST = config.INST_DEL__PER_INST();
  SLIP__PER_FUNC = config.SLIP__PER_FUNC();
  FUNC_DUP__PER_FUNC = config.FUNC_DUP__PER_FUNC();
  FUNC_DEL__PER_FUNC = config.FUNC_DEL__PER_FUNC();
  TAG_BIT_FLIP__PER_BIT = config.TAG_BIT_FLIP__PER_BIT();
  EVOLVE_HW_TAG_SIM_THRESH = config.EVOLVE_HW_TAG_SIM_THRESH();

  HW_MAX_THREAD_CNT = config.HW_MAX_THREAD_CNT();
  HW_MAX_CALL_DEPTH = config.HW_MAX_CALL_DEPTH();
  HW_MIN_TAG_SIMILARITY_THRESH = config.HW_MIN_TAG_SIMILARITY_THRESH();

  DATA_DIRECTORY = config.DATA_DIRECTORY();
  STATISTICS_INTERVAL = config.STATISTICS_INTERVAL();
  SNAPSHOT_INTERVAL = config.SNAPSHOT_INTERVAL();

  // Verify any config constraints
  if (EVAL_TRIAL_CNT < 1) {
    std::cout << "Cannot run experiment with EVAL_TRIAL_CNT < 1. Exiting..." << std::endl;
    exit(-1);
  }

  if (ENV_STATE_CNT == (size_t)-1) {
    std::cout << "ENV_STATE_CNT exceeds maximum allowed! Exiting..." << std::endl;
    exit(-1);
  }
}

/// Initialize world mutator.
void MapElitesGPWorld::Init_Mutator() {
  // We'll use the default mutator set. 
  // Configure program constraints.
  mutator.SetProgMinFuncCnt(PROG_MIN_FUNC_CNT);
  mutator.SetProgMaxFuncCnt(PROG_MAX_FUNC_CNT);
  mutator.SetProgMinFuncLen(PROG_MIN_FUNC_LEN);
  mutator.SetProgMaxFuncLen(PROG_MAX_FUNC_LEN);
  mutator.SetProgMaxTotalLen(PROG_MAX_TOTAL_LEN);
  mutator.SetProgMinArgVal(PROG_MIN_ARG_VAL);
  mutator.SetProgMaxArgVal(PROG_MAX_ARG_VAL);
  // Configure mutation rates. 
  mutator.ARG_SUB__PER_ARG(ARG_SUB__PER_ARG);
  mutator.INST_SUB__PER_INST(INST_SUB__PER_INST);
  mutator.INST_INS__PER_INST(INST_INS__PER_INST);
  mutator.INST_DEL__PER_INST(INST_DEL__PER_INST);
  mutator.SLIP__PER_FUNC(SLIP__PER_FUNC);
  mutator.FUNC_DUP__PER_FUNC(FUNC_DUP__PER_FUNC);
  mutator.FUNC_DEL__PER_FUNC(FUNC_DEL__PER_FUNC);
  mutator.TAG_BIT_FLIP__PER_BIT(TAG_BIT_FLIP__PER_BIT);
  // Hook up mutator to world's MutateFun
  mut_funs.push_back([this](org_t & org, emp::Random & r) {
                        return mutator.ApplyMutations(org.GetProgram(), r);
                      });

  SetMutFun([this](org_t & org, emp::Random & r) {
    org.ResetGenomeInfo();
    size_t mut_cnt = 0; 
    for (size_t f = 0; f < mut_funs.size(); ++f) {
      mut_cnt += mut_funs[f](org, r);
    }
    return mut_cnt;
  });
  // TODO: get rid of elite select version of this function for MAPE
}

void MapElitesGPWorld::Init_Hardware() {
  // Add base set of instructions to instruction library. 
  // Problem-specific instructions will be added when that problem is configured.
  inst_lib.AddInst("Inc", hardware_t::Inst_Inc, 1, "Increment value in local memory Arg1");
  inst_lib.AddInst("Dec", hardware_t::Inst_Dec, 1, "Decrement value in local memory Arg1");
  inst_lib.AddInst("Not", hardware_t::Inst_Not, 1, "Logically toggle value in local memory Arg1");
  inst_lib.AddInst("Add", hardware_t::Inst_Add, 3, "Local memory: Arg3 = Arg1 + Arg2");
  inst_lib.AddInst("Sub", hardware_t::Inst_Sub, 3, "Local memory: Arg3 = Arg1 - Arg2");
  inst_lib.AddInst("Mult", hardware_t::Inst_Mult, 3, "Local memory: Arg3 = Arg1 * Arg2");
  inst_lib.AddInst("Div", hardware_t::Inst_Div, 3, "Local memory: Arg3 = Arg1 / Arg2");
  inst_lib.AddInst("Mod", hardware_t::Inst_Mod, 3, "Local memory: Arg3 = Arg1 % Arg2");
  inst_lib.AddInst("TestEqu", hardware_t::Inst_TestEqu, 3, "Local memory: Arg3 = (Arg1 == Arg2)");
  inst_lib.AddInst("TestNEqu", hardware_t::Inst_TestNEqu, 3, "Local memory: Arg3 = (Arg1 != Arg2)");
  inst_lib.AddInst("TestLess", hardware_t::Inst_TestLess, 3, "Local memory: Arg3 = (Arg1 < Arg2)");
  inst_lib.AddInst("If", hardware_t::Inst_If, 1, "Local memory: If Arg1 != 0, proceed; else, skip block.", emp::ScopeType::BASIC, 0, {"block_def"});
  inst_lib.AddInst("While", hardware_t::Inst_While, 1, "Local memory: If Arg1 != 0, loop; else, skip block.", emp::ScopeType::BASIC, 0, {"block_def"});
  inst_lib.AddInst("Countdown", hardware_t::Inst_Countdown, 1, "Local memory: Countdown Arg1 to zero.", emp::ScopeType::BASIC, 0, {"block_def"});
  inst_lib.AddInst("Close", hardware_t::Inst_Close, 0, "Close current block if there is a block to close.", emp::ScopeType::BASIC, 0, {"block_close"});
  inst_lib.AddInst("Break", hardware_t::Inst_Break, 0, "Break out of current block.");
  inst_lib.AddInst("Call", hardware_t::Inst_Call, 0, "Call function that best matches call affinity.", emp::ScopeType::BASIC, 0, {"affinity"});
  inst_lib.AddInst("Return", hardware_t::Inst_Return, 0, "Return from current function if possible.");
  inst_lib.AddInst("SetMem", hardware_t::Inst_SetMem, 2, "Local memory: Arg1 = numerical value of Arg2");
  inst_lib.AddInst("CopyMem", hardware_t::Inst_CopyMem, 2, "Local memory: Arg1 = Arg2");
  inst_lib.AddInst("SwapMem", hardware_t::Inst_SwapMem, 2, "Local memory: Swap values of Arg1 and Arg2.");
  inst_lib.AddInst("Input", hardware_t::Inst_Input, 2, "Input memory Arg1 => Local memory Arg2.");
  inst_lib.AddInst("Output", hardware_t::Inst_Output, 2, "Local memory Arg1 => Output memory Arg2.");
  inst_lib.AddInst("Commit", hardware_t::Inst_Commit, 2, "Local memory Arg1 => Shared memory Arg2.");
  inst_lib.AddInst("Pull", hardware_t::Inst_Pull, 2, "Shared memory Arg1 => Shared memory Arg2.");
  inst_lib.AddInst("Nop", hardware_t::Inst_Nop, 0, "No operation.");
  inst_lib.AddInst("Fork", hardware_t::Inst_Fork, 0, "Fork a new thread. Local memory contents of callee are loaded into forked thread's input memory.");
  inst_lib.AddInst("Terminate", hardware_t::Inst_Terminate, 0, "Kill current thread.");
  // Configure the evaluation hardware.
  eval_hw = emp::NewPtr<hardware_t>(&inst_lib, &event_lib, random_ptr);
  eval_hw->SetMinBindThresh(HW_MIN_TAG_SIMILARITY_THRESH);
  eval_hw->SetMaxCores(HW_MAX_THREAD_CNT);
  eval_hw->SetMaxCallDepth(HW_MAX_CALL_DEPTH);

  // Collect function call information. 
  eval_hw->OnBeforeFuncCall([this](hardware_t & hw, size_t fID) {
    const size_t org_id =(size_t)hw.GetTrait(trait_id_t::ORG_ID);
    phenotype_t & phen = phen_cache.Get(org_id, trial_id);
    phen.functions_used_set.emplace(fID);
    phen.function_entries.emplace_back(fID);
  });

  eval_hw->OnBeforeCoreSpawn([this](hardware_t & hw, size_t fID) {
    const size_t org_id = (size_t)hw.GetTrait(trait_id_t::ORG_ID);
    phenotype_t & phen = phen_cache.Get(org_id, trial_id);
    phen.functions_used_set.emplace(fID);
    phen.function_entries.emplace_back(fID);
  });

}

/// Initialize selected problem. 
void MapElitesGPWorld::Init_Problem() {
  switch (PROBLEM_TYPE) {
    case (size_t)PROBLEM_TYPE::CHG_ENV: {
      SetupProblem_ChgEnv();
      break;
    }
    case (size_t)PROBLEM_TYPE::TESTCASES: {
      SetupProblem_Testcases();
      break;     
    }
    default: {
      std::cout << "Unrecognized problem type (" << PROBLEM_TYPE << "). Exiting..." << std::endl;
      exit(-1);
    }
  }
}

void MapElitesGPWorld::Init_WorldMode() {
  switch (RUN_MODE) {
    case (size_t)WORLD_MODE::EA: {
      SetupWorldMode_EA();
      break;
    }
    case (size_t)WORLD_MODE::MAPE: {
      SetupWorldMode_MAPE();
      break;
    }
    default: {
      std::cout << "Unrecognized run mode (" << RUN_MODE << "). Exiting..." << std::endl;
      exit(-1);
    }
  }
}

void MapElitesGPWorld::SetupProblem_ChgEnv() {
  // In the changing environment, problem..
  // Setup environment state tags. 
  switch (ENV_TAG_GEN_METHOD) {
    case (size_t)CHGENV_TAG_GEN_METHOD::RANDOM: {
      chgenv_info.env_state_tags = emp::GenRandSignalGPTags<org_t::TAG_WIDTH>(*random_ptr, ENV_STATE_CNT, true);
      if (ENV_DISTRACTION_SIGS) chgenv_info.distraction_sig_tags = emp::GenRandSignalGPTags<org_t::TAG_WIDTH>(*random_ptr, ENV_DISTRACTION_SIG_CNT, true, chgenv_info.env_state_tags);
      SaveChgEnvTags();
      break;
    }
    case (size_t)CHGENV_TAG_GEN_METHOD::LOAD: {
      LoadChgEnvTags();
      break;
    }
    default: {
      std::cout << "Unrecognized ENV_TAG_GEN_METHOD (" << ENV_TAG_GEN_METHOD << "). Exiting..." << std::endl;
      exit(-1);
    }
  }

  // Print environment tags
  std::cout << "Environment tags (" << chgenv_info.env_state_tags.size() << "): " << std::endl;
  for (size_t i = 0; i < chgenv_info.env_state_tags.size(); ++i) {
    std::cout << i << ":";
    chgenv_info.env_state_tags[i].Print();
    std::cout << std::endl;
  }
  std::cout << "Distraction signal tags (" << chgenv_info.distraction_sig_tags.size() << "): " << std::endl;
  for (size_t i = 0; i < chgenv_info.distraction_sig_tags.size(); ++i) {
    std::cout << i << ":";
    chgenv_info.distraction_sig_tags[i].Print();
    std::cout << std::endl;
  }
  
  // Populate the environment shuffler (used when changing the environment).
  for (size_t i = 0; i < chgenv_info.env_state_tags.size(); ++i) chgenv_info.env_shuffler.emplace_back(i);
  chgenv_info.env_shuffle_id = 0;

  // Setup env advance signal action.
  // - Setup environment state changing
  switch (ENV_CHG_METHOD) {
    case (size_t)ENV_CHG_METHOD::SHUFFLE: {
      do_env_advance_sig.AddAction([this]() {
        ChgEnvProblemInfo & env = chgenv_info;
        if (env.env_state == (size_t)-1 || random_ptr->P(ENV_CHG_PROB)) {
          // Trigger change!
          // What state should we switch to?
          env.env_state = env.env_shuffler[env.env_shuffle_id]; 
          env.env_shuffle_id += 1;
          // If shuffle id exceeds env states, reset to 0 and shuffle!
          if (env.env_shuffle_id >= ENV_STATE_CNT) {
            env.env_shuffle_id = 0;
            emp::Shuffle(*random_ptr, env.env_shuffler);
          }
          // 2) Trigger environment state event.
          eval_hw->TriggerEvent("EnvSignal", env.env_state_tags[env.env_state]);
        }
      });
      break;
    }
    case (size_t)ENV_CHG_METHOD::CYCLE: {
      do_env_advance_sig.AddAction([this]() {
        ChgEnvProblemInfo & env = chgenv_info;
        if (env.env_state == (size_t)-1 || ((eval_time % ENV_CHG_RATE) == 0)) {
          // Trigger change!
          // What state should we switch to?
          env.env_state = env.env_shuffler[env.env_shuffle_id]; 
          env.env_shuffle_id += 1;
          // If shuffle id exceeds env states, reset to 0 and shuffle!
          if (env.env_shuffle_id >= ENV_STATE_CNT) {
            env.env_shuffle_id = 0;
            emp::Shuffle(*random_ptr, env.env_shuffler);
          }
          // 2) Trigger environment state event.
          eval_hw->TriggerEvent("EnvSignal", env.env_state_tags[env.env_state]);
        }
      });
      break;
    }
    case (size_t)ENV_CHG_METHOD::RAND: {
      do_env_advance_sig.AddAction([this]() {
        ChgEnvProblemInfo & env = chgenv_info;
        if (env.env_state == (size_t)-1 || random_ptr->P(ENV_CHG_PROB)) {
          // Trigger change!
          // What state should we switch to?
          env.env_state = random_ptr->GetUInt(ENV_STATE_CNT);
          // 2) Trigger environment state event.
          eval_hw->TriggerEvent("EnvSignal", env.env_state_tags[env.env_state]);
        }
      });
      break;
    }
    default: {
      std::cout << "Unrecognized ENV_CHG_METHOD (" << ENV_CHG_METHOD << "). Exiting..." << std::endl;
      exit(-1);
    }
  }

  // - Setup distraction signals
  if (ENV_DISTRACTION_SIGS) {
    do_env_advance_sig.AddAction([this]() {
      if (random_ptr->P(ENV_DISTRACTION_SIG_CNT)) {
        const size_t id = random_ptr->GetUInt(chgenv_info.distraction_sig_tags.size());
        eval_hw->TriggerEvent("EnvSignal", chgenv_info.distraction_sig_tags[id]);
      }
    });
  }

  calc_score = [](org_t & org, phenotype_t & phen) {
    return phen.env_match_score;
  };

  // Reset the environment at the begining of a trial
  begin_org_trial_sig.AddAction([this](org_t & org) {
    chgenv_info.ResetEnv(*random_ptr);
  });

  do_org_advance_sig.AddAction([this](org_t & org) {
    const size_t org_id = org.GetPos();
    if ((size_t)eval_hw->GetTrait(org_t::ORG_STATE) == chgenv_info.env_state) {
      phen_cache.Get(org_id, trial_id).env_match_score += 1;
    }
  });

  // Setup instructions/events specific to changing environment problem.

  // Add 1 set state instruction for every possible environment state.
  for (size_t i = 0; i < ENV_STATE_CNT; ++i) {
    inst_lib.AddInst("SetState-" + emp::to_string(i),
      [i](hardware_t & hw, const inst_t & inst) {
        hw.SetTrait(trait_id_t::ORG_STATE, i);
     }, 0, "Set internal state to " + emp::to_string(i));
  }

  // Add events!
  if (ENV_CHG_SIG) {
    // Use event-driven events.
    event_lib.AddEvent("EnvSignal", [](hardware_t & hw, const event_t & event) { hw.SpawnCore(event.affinity, hw.GetMinBindThresh(), event.msg); }, "Event handler for ");
    event_lib.RegisterDispatchFun("EnvSignal", [](hardware_t & hw, const event_t & event) { hw.QueueEvent(event); } );
  } else {
    // Use nop events.
    event_lib.AddEvent("EnvSignal", [](hardware_t &, const event_t &) { ; }, "");
    event_lib.RegisterDispatchFun("EnvSignal", [](hardware_t &, const event_t &) { ; });
  }

  // Add sensors!
  if (ENV_SENSORS) {
    // Add sensors to instruction set.
    for (int i = 0; i < ENV_STATE_CNT; ++i) {
      inst_lib.AddInst("SenseState-" + emp::to_string(i),
        [this, i](hardware_t & hw, const inst_t & inst) {
          state_t & state = hw.GetCurState();
          state.SetLocal(inst.args[0], this->chgenv_info.env_state==i);
        }, 1, "Sense if current environment state is " + emp::to_string(i));
    }
  }
  
}

void MapElitesGPWorld::SetupProblem_Testcases() {
  // TODO
}

void MapElitesGPWorld::SetupWorldMode_EA() {
  
  SetPopStruct_Mixed(true);
  SetAutoMutate([this](size_t pos){ return pos > ELITE_CNT; }); // Mutations will occur before deciding where a new organism is placed. 
  
  std::cout << "Configuring world mode: standard evolutionary algorithm" << std::endl;
  // do_evaluation_sig
  do_evaluation_sig.AddAction([this]() {
    // Evaluate e'rybody! 
    for (size_t id = 0; id < GetSize(); ++id) {
      org_t & org = GetOrg(id);
      org.SetPos(id);
      Evaluate(org);
      double fitness = CalcFitnessOrg(org);
      if (fitness > best_score || id == 0) { best_score = fitness; dominant_id = id; }
    }
  });

  // do_selection_sig
  switch (SELECTION_METHOD) {
    case (size_t)SELECTION_METHOD::TOURNAMENT: {
      do_selection_sig.AddAction([this]() {
        if (ELITE_CNT) emp::EliteSelect(*this, ELITE_CNT, 1);
        emp::TournamentSelect(*this, TOURNAMENT_SIZE, POP_SIZE-ELITE_CNT);
      });
      break;
    }
    default: {
      std::cout << "Unrecognized SELECTION_METHOD (" << SELECTION_METHOD << "). Exiting..." << std::endl;
      exit(-1);
    }
  }

  SetFitFun([this](org_t & org) {
    return agg_scores(org);
  });

  // EA-specific data tracking
  #ifndef EMSCRIPTEN
  // Setup dominant snapshotting.
  do_pop_snapshot_sig.AddAction([this]() { Snapshot_Dominant(); }); 

  // Add dominant file. 
  // AddDominantFile(DATA_DIRECTORY + "dominant.csv").SetTimingRepeat(STATISTICS_INTERVAL);
  #endif

    // One of last things to do before run: resize phenotype cache
  do_begin_run_sig.AddAction([this]() {
    std::cout << "Resizing the phenotype cache(" << POP_SIZE << ")!" << std::endl;
    phen_cache.Resize(POP_SIZE, EVAL_TRIAL_CNT); // Add one position as temp position for MAP-elites
  });
  
}

void MapElitesGPWorld::SetupWorldMode_MAPE() {
  std::cout << "Configuring world mode: MAPE" << std::endl;

  SetAutoMutate();

  // do_evaluation_sig
  do_evaluation_sig.AddAction([this]() {
    best_score = MIN_POSSIBLE_SCORE;
  });

  // do_selection_sig
  do_selection_sig.AddAction([this]() {
    emp::RandomSelect(*this, POP_SIZE);
  });
  
  OnBeforePlacement([this](org_t & org, size_t pos) {
    org.SetPos(GetSize()); // Indicate that this organism has not been placed yet (useful for MAPE). 
  });

  // Setup fitness function
  SetFitFun([this](org_t & org) {
    // const size_t id = GetSize();
    // org.SetPos(id);
    // TODO: confirm organism position!
    // Evaluate!
    Evaluate(org);
    // Grab score
    const double score = agg_scores(org);
    if (score > best_score) { best_score = score; }
    return score;
    
  });

  // MAPE-specific data tracking
  #ifndef EMSCRIPTEN
  do_pop_snapshot_sig.AddAction([this]() { Snapshot_MAP(); }); 
  #endif

  // Setup traits
  do_begin_run_sig.AddAction([this](){
    emp::vector<size_t> trait_bin_sizes;
    if (USE_MAPE_AXIS__INST_ENTROPY) {
      std::cout << "Configuring instruction entropy axis" << std::endl;
      AddPhenotype("InstructionEntropy", inst_ent_fun, 0.0, max_inst_entropy + 0.1); 
      trait_bin_sizes.emplace_back(MAPE_AXIS_SIZE__INST_ENTROPY);
    }
    if (USE_MAPE_AXIS__INST_CNT) {
      std::cout << "Configuring instruction count axis" << std::endl;
      AddPhenotype("InstructionCnt", inst_cnt_fun, 0, (int)PROG_MAX_TOTAL_LEN);
      trait_bin_sizes.emplace_back(PROG_MAX_TOTAL_LEN+1);
    }
    if (USE_MAPE_AXIS__FUNC_USED) {
      std::cout << "Configuring functions used axis" << std::endl;
      AddPhenotype("FunctionsUsed", func_used_fun, 0, PROG_MAX_FUNC_CNT+1);
      trait_bin_sizes.emplace_back(PROG_MAX_FUNC_CNT+1);
    }
    if (USE_MAPE_AXIS__FUNC_CNT) {
      std::cout << "Configuring function count axis" << std::endl;
      AddPhenotype("FunctionCnt", func_cnt_fun, 0, (int)PROG_MAX_FUNC_CNT+1);
      trait_bin_sizes.emplace_back(PROG_MAX_FUNC_CNT+1);
    }
    if (USE_MAPE_AXIS__FUNC_ENTERED) {
      std::cout << "Configuring functions entered axis" << std::endl;
      AddPhenotype("FunctionsEntered", func_entered_cnt_fun, 0, (EVAL_TIME * HW_MAX_THREAD_CNT)+1);
      trait_bin_sizes.emplace_back((EVAL_TIME * HW_MAX_THREAD_CNT)+1);

    }
    if (USE_MAPE_AXIS__FUNC_ENTERED_ENTROPY) {
      std::cout << "Configuring functions entered entropy axis" << std::endl;
      AddPhenotype("FunctionsEnteredEntropy", func_entered_ent_fun, 0.0, max_func_entered_entropy);
      trait_bin_sizes.emplace_back(MAPE_AXIS_SIZE__FUNC_ENTERED_ENTROPY);
    }
    emp::SetMapElites(*this, trait_bin_sizes);
  });

  // One of last things to do before run: resize phenotype cache
  do_begin_run_sig.AddAction([this]() {
    std::cout << "Resizing the phenotype cache (" << GetSize() + 1 << ")!" << std::endl;
    phen_cache.Resize(GetSize() + 1, EVAL_TRIAL_CNT); // Add one position as temp position for MAP-elites
  });

  do_world_update_sig.AddAction([this]() {
    ClearCache();
  });

}

// === Run functions ===
void MapElitesGPWorld::Run() {
  switch(RUN_MODE) {
    case (size_t)WORLD_MODE::EA: 
      // EA-mode does the same thing as MAPE-mode during a run. (we leave the break out and drop into MAPE case)
    case (size_t)WORLD_MODE::MAPE: {
      do_begin_run_sig.Trigger();
      do_pop_init_sig.Trigger();
      for (size_t u = 0; u <= GENERATIONS; ++u) {
        RunStep();
      }
      break;
    }
    default: {
      std::cout << "Unrecognized run mode (" << RUN_MODE << "). Exiting..." << std::endl;
      exit(-1);
    }
  }
}

void MapElitesGPWorld::RunStep() {
  // could move these onto OnUpdate signal
  do_evaluation_sig.Trigger();
  do_selection_sig.Trigger();
  do_world_update_sig.Trigger();
}

// === Changing environment utility functions
void MapElitesGPWorld::SaveChgEnvTags() {
  // Save out environment states.
  std::ofstream envtags_ofstream(ENV_TAG_FPATH);
  envtags_ofstream << "tag_id,tag_type,tag\n";
  for (size_t i = 0; i < chgenv_info.env_state_tags.size(); ++i) {
    envtags_ofstream << i << ",env,"; chgenv_info.env_state_tags[i].Print(envtags_ofstream); envtags_ofstream << "\n";
  }
  for (size_t i = 0; i < chgenv_info.distraction_sig_tags.size(); ++i) {
    envtags_ofstream << i << ",dist,"; chgenv_info.distraction_sig_tags[i].Print(envtags_ofstream); envtags_ofstream << "\n";
  }
  envtags_ofstream.close();
}

void MapElitesGPWorld::LoadChgEnvTags() {
  chgenv_info.env_state_tags.resize(ENV_STATE_CNT, tag_t());
  chgenv_info.distraction_sig_tags.resize(ENV_DISTRACTION_SIG_CNT, tag_t());

  std::ifstream tag_fstream(ENV_TAG_FPATH);
  if (!tag_fstream.is_open()) {
    std::cout << "Failed to open " << ENV_TAG_FPATH << ". Exiting..." << std::endl;
    exit(-1);
  }

  std::string cur_line;
  emp::vector<std::string> line_components;

  const size_t tag_id_pos = 0;
  const size_t tag_type_pos = 1;
  const size_t tag_pos = 2;

  std::getline(tag_fstream, cur_line); // Consume header.

  while (!tag_fstream.eof()) {
    std::getline(tag_fstream, cur_line);
    emp::remove_whitespace(cur_line);
    
    if (cur_line == emp::empty_string()) continue;
    emp::slice(cur_line, line_components, ',');

    size_t tag_id = (size_t)std::stoi(line_components[tag_id_pos]);
    std::string tag_type = line_components[tag_type_pos];

    if (tag_type == "env") {
      // Load environment state tag!
      if (tag_id > chgenv_info.env_state_tags.size()) {
        std::cout << "WARNING: tag ID exceeds environment states!" << std::endl;
        continue;
      }
      for (size_t i = 0; i < line_components[tag_pos].size(); ++i) {
        if (i >= org_t::TAG_WIDTH) break;
        if (line_components[tag_pos][i] == '1') chgenv_info.env_state_tags[tag_id].Set(chgenv_info.env_state_tags[tag_id].GetSize() - i - 1, true);
      }
    } else if (tag_type == "dist") {
      // Load distraction signal tag!
      if (tag_id > chgenv_info.distraction_sig_tags.size()) {
        std::cout << "WARNING: tag ID exceeds distraction signals!" << std::endl;
        continue;
      }
      for (size_t i = 0; i < line_components[tag_pos].size(); ++i) {
        if (i >= org_t::TAG_WIDTH) break;
        if (line_components[tag_pos][i] == '1') chgenv_info.distraction_sig_tags[tag_id].Set(chgenv_info.distraction_sig_tags[tag_id].GetSize() - i - 1, true);
      }
    } else {
      std::cout << "Unrecognized tag type: " << tag_type << "; continuing..." << std::endl;
    }
  }
  tag_fstream.close();
}

// === Functions that initialize the population
void MapElitesGPWorld::InitPop_Random() {
  // NOTE: If particular attribute is evolvable, randomize it!
  std::cout << "Randomly initializing population!" << std::endl;
  for (size_t i = 0; i < POP_SIZE; ++i) {
    program_t prog(emp::GenRandSignalGPProgram(*random_ptr, inst_lib, 
                                               PROG_MIN_FUNC_CNT, PROG_MAX_FUNC_CNT,
                                               PROG_MIN_FUNC_LEN, PROG_MAX_FUNC_LEN,
                                               PROG_MIN_ARG_VAL, PROG_MAX_ARG_VAL));
    const double sim_thresh = (EVOLVE_HW_TAG_SIM_THRESH) ? random_ptr->GetDouble(0, 1.0) : HW_MIN_TAG_SIMILARITY_THRESH;
    genome_t ancestor_genome(prog, sim_thresh);
    Inject(ancestor_genome, 1.0);  
  }
  std::cout << "Done randomly initializing population!" << std::endl;
}

// WARNING (to future self; 'sup future self): currently no support for loading in custom similarity threshold. 
void MapElitesGPWorld::InitPop_Ancestor() {
  std::cout << "Initializing population from ancestor file (" << ANCESTOR_FPATH << ")!" << std::endl;
  // Configure the ancestor program.
  program_t ancestor_prog(&inst_lib);
  std::ifstream ancestor_fstream(ANCESTOR_FPATH);
  if (!ancestor_fstream.is_open()) {
    std::cout << "Failed to open ancestor program file(" << ANCESTOR_FPATH << "). Exiting..." << std::endl;
    exit(-1);
  }
  ancestor_prog.Load(ancestor_fstream);
  std::cout << " --- Ancestor program: ---" << std::endl;
  ancestor_prog.PrintProgramFull();
  std::cout << " -------------------------" << std::endl;
  genome_t ancestor_genome(ancestor_prog, HW_MIN_TAG_SIMILARITY_THRESH);
  Inject(ancestor_genome, POP_SIZE);    // Inject population!
}

// === Functions to track/record data ===
/// Snapshot all programs for current update.
void MapElitesGPWorld::Snapshot_Programs() {
  std::string snapshot_dir = DATA_DIRECTORY + "pop_" + emp::to_string(GetUpdate());
  mkdir(snapshot_dir.c_str(), ACCESSPERMS);
  // For each program in the population, dump the full program description in a single file.
  std::ofstream prog_ofstream(snapshot_dir + "/pop_" + emp::to_string(GetUpdate()) + ".pop");
  for (size_t i = 0; i < GetSize(); ++i) {
    if (!IsOccupied(i)) continue;
    prog_ofstream << "==={id:" << i << ",fitness:" << CalcFitnessID(i) << ",sim_thresh:" << GetOrg(i).GetTagSimilarityThreshold() << "}===\n";
    org_t & org = GetOrg(i);
    org.GetProgram().PrintProgramFull(prog_ofstream);
  }
  prog_ofstream.close();
}

/// Snapshot population statistics for current update.
void MapElitesGPWorld::Snapshot_PopulationStats() {
  // TODO: this
}

/// Snapshot dominant program performance over many trials (only makes sense in context of EA run). 
void MapElitesGPWorld::Snapshot_Dominant() {
  // TODO: this
}

/// Snapshot map from MAP-elites (only makes sense in context of MAP-Elites run). 
void MapElitesGPWorld::Snapshot_MAP(void) {
  // TODO: this
}

/// Add a data file to track dominant program. Will track at same interval as fitness file. (only makes sense in context of EA run).
emp::DataFile & MapElitesGPWorld::AddDominantFile(const std::string & fpath) {
  // TODO: this
}


#endif