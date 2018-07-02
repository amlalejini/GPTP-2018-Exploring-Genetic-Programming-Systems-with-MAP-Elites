#ifndef MAPE_SIGNALGP_WORLD_H
#define MAPE_SIGNALGP_WORLD_H

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
#include "MapElitesSignalGP_Org.h"

#include "TestcaseSet.h"
#include "TaskSet.h"
#include "PhenotypeCache.h"

// Major TODOS: 
// - [ ] More Testing
// - [ ] Documentation

class MapElitesSignalGPWorld : public emp::World<MapElitesSignalGPOrg> {
public:
  static constexpr double MIN_POSSIBLE_SCORE = -32767;
  static constexpr size_t MAX_LOGIC_TASK_NUM_INPUTS = 2;
  static constexpr uint32_t MIN_LOGIC_TASK_INPUT = 0;
  static constexpr uint32_t MAX_LOGIC_TASK_INPUT = 1000000000;

  enum class WORLD_MODE { EA=0, MAPE=1 };
  enum class PROBLEM_TYPE { CHG_ENV=0, TESTCASES=1, LOGIC=2 };
  enum class SELECTION_METHOD { TOURNAMENT=0 };
  enum class POP_INIT_METHOD { RANDOM=0, ANCESTOR=1 };
  enum class EVAL_TRIAL_AGG_METHOD { MIN=0, MAX=1, AVG=2 }; 
  enum class CHGENV_TAG_GEN_METHOD { RANDOM=0, LOAD=1 }; 
  enum class ENV_CHG_METHOD { SHUFFLE=0, CYCLE=1, RAND=2 };
  struct OrgPhenotype;

  using org_t = MapElitesSignalGPOrg; 
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
  using memory_t = typename hardware_t::memory_t;

  using trait_id_t = typename org_t::HW_TRAIT_ID;

  using mut_fun_t = std::function<size_t(org_t &, emp::Random &)>;
  using score_fun_t = std::function<double(org_t &, phenotype_t &)>;

  using task_io_t = uint32_t;
  using taskset_t = TaskSet<std::array<task_io_t, MAX_LOGIC_TASK_NUM_INPUTS>, task_io_t>;

  struct OrgPhenotype {
    // Generic
    double score;  
    std::unordered_set<size_t> functions_used_set;
    emp::vector<size_t> function_entries;           ///< All functions entered (allows repeats), in the order they were entered.

    // For changing environment problem
    double env_match_score;

    // For testcase problems
    emp::vector<double> testcase_results;

    // For logic problem
    size_t task_cnt;
    size_t time_all_logic_tasks_done;
    size_t unique_logic_tasks_done;
    emp::vector<size_t> logic_tasks_done_by_task;

    void SetTaskCnt(size_t val) {
      task_cnt = val;
      logic_tasks_done_by_task.resize(task_cnt, 0);
    }

    void Reset() {
      score = 0;
      function_entries.clear();
      functions_used_set.clear();

      env_match_score = 0;

      testcase_results.clear();

      time_all_logic_tasks_done = 0;
      unique_logic_tasks_done = 0;
      for (size_t i = 0; i < task_cnt; ++i) logic_tasks_done_by_task[i] = 0;
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
  // == Testcase problem group ==
  size_t NUM_TEST_CASES;
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
  size_t DOM_SNAPSHOT_TRIAL_CNT;
  size_t MAP_SNAPSHOT_TRIAL_CNT;

  emp::SignalGPMutator<org_t::TAG_WIDTH> mutator;
  emp::vector<mut_fun_t> mut_funs;

  inst_lib_t inst_lib;
  event_lib_t event_lib;

  emp::Ptr<hardware_t> eval_hw;

  TestcaseSet<int, double> testcases;
  emp::vector<size_t> testcase_ids;

  taskset_t task_set;
  std::array<task_io_t, MAX_LOGIC_TASK_NUM_INPUTS> task_inputs;
  size_t input_load_id;


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
  double best_score;
  size_t dominant_id; 

  emp::vector<size_t> trait_bin_sizes;
  double max_inst_entropy; 
  double max_func_entered_entropy; 

  struct PhenTraitInfo {
    size_t id;
    std::string name;
    std::string desc;
    PhenTraitInfo(size_t _id, std::string _name, std::string _desc="") : id(_id), name(_name), desc(_desc) { ; }
  };
  emp::vector<PhenTraitInfo> phen_traits;

  /// Bundles statistic about population (used by pop stats snapshot)
  /// - name
  /// - calc
  /// - desc
  struct PopStat {
    using stat_fun_t = std::function<double(void)>;
    std::string name;
    stat_fun_t fun;
    std::string desc;
    PopStat(const std::string & n, const stat_fun_t & f, const std::string & d="") : name(n), fun(f), desc(d) { ; }
  };
  emp::vector<PopStat> pop_snapshot_stats;
  emp::vector<PopStat> dom_file_stats;

  struct PopStatsInfo {
    size_t cur_org_id;
  } pop_snapshot_info;
  
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
  void SetupProblem_Logic();

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

  // === Logic task problem utility functions ===
  /// Reset logic tasks, guaranteeing no solution collisions among the tasks.
  void ResetTasks() {
    input_load_id = 0;
    task_inputs[0] = random_ptr->GetUInt(MIN_LOGIC_TASK_INPUT, MAX_LOGIC_TASK_INPUT);
    task_inputs[1] = random_ptr->GetUInt(MIN_LOGIC_TASK_INPUT, MAX_LOGIC_TASK_INPUT);
    task_set.SetInputs(task_inputs);
    while (task_set.IsCollision()) {
      task_inputs[0] = random_ptr->GetUInt(MIN_LOGIC_TASK_INPUT, MAX_LOGIC_TASK_INPUT);
      task_inputs[1] = random_ptr->GetUInt(MIN_LOGIC_TASK_INPUT, MAX_LOGIC_TASK_INPUT);
      task_set.SetInputs(task_inputs);
    }
  }

  // === Eval hardware utility functions ===
  void ResetEvalHW() {
    eval_hw->ResetHardware();
    // TODO: add signal for onreset hardware
    eval_hw->SetTrait(trait_id_t::ORG_ID, -1);
    eval_hw->SetTrait(trait_id_t::PROBLEM_OUTPUT, -1);
    eval_hw->SetTrait(trait_id_t::ORG_STATE, -1);
    eval_hw->SetTrait(trait_id_t::OUTPUT_SET, 0);
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
  MapElitesSignalGPWorld() : emp::World<org_t>() { ; }
  MapElitesSignalGPWorld(emp::Random & rnd) : emp::World<org_t>(rnd) { ; }
  ~MapElitesSignalGPWorld() {
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
void MapElitesSignalGPWorld::Setup(MapElitesGPConfig & config) {
  Reset();              // Reset the world
  SetCache();           // We'll be caching fitness scores
  Init_Configs(config); // Initialize configs

  // Setup 
  do_begin_run_sig.AddAction([this]() {
    // Calculate ranges for phenotypic traits. 
    max_inst_entropy = -1 * emp::Log2(1.0/((double)inst_lib.GetSize())); // Instruction set must be locked in by this point. 
    max_func_entered_entropy = -1 * emp::Log2(1.0/((double)PROG_MAX_FUNC_CNT));
    
    // Add phenotypic traits. 
    if (USE_MAPE_AXIS__INST_ENTROPY) {
      std::cout << "Configuring instruction entropy axis" << std::endl;
      phen_traits.emplace_back(GetPhenotypes().GetSize(), "InstructionEntropy");
      AddPhenotype("InstructionEntropy", inst_ent_fun, 0.0, max_inst_entropy + 0.1); 
      trait_bin_sizes.emplace_back(MAPE_AXIS_SIZE__INST_ENTROPY);
    }
    if (USE_MAPE_AXIS__INST_CNT) {
      std::cout << "Configuring instruction count axis" << std::endl;
      phen_traits.emplace_back(GetPhenotypes().GetSize(), "InstructionCnt");
      AddPhenotype("InstructionCnt", inst_cnt_fun, 0, (int)PROG_MAX_TOTAL_LEN);
      trait_bin_sizes.emplace_back(PROG_MAX_TOTAL_LEN+1);
    }
    if (USE_MAPE_AXIS__FUNC_USED) {
      std::cout << "Configuring functions used axis" << std::endl;
      phen_traits.emplace_back(GetPhenotypes().GetSize(), "FunctionsUsed");
      AddPhenotype("FunctionsUsed", func_used_fun, 0, PROG_MAX_FUNC_CNT+1);
      trait_bin_sizes.emplace_back(PROG_MAX_FUNC_CNT+1);
    }
    if (USE_MAPE_AXIS__FUNC_CNT) {
      std::cout << "Configuring function count axis" << std::endl;
      phen_traits.emplace_back(GetPhenotypes().GetSize(), "FunctionCnt");
      AddPhenotype("FunctionCnt", func_cnt_fun, 0, (int)PROG_MAX_FUNC_CNT+1);
      trait_bin_sizes.emplace_back(PROG_MAX_FUNC_CNT+1);
    }
    if (USE_MAPE_AXIS__FUNC_ENTERED) {
      std::cout << "Configuring functions entered axis" << std::endl;
      phen_traits.emplace_back(GetPhenotypes().GetSize(), "FunctionsEntered");
      AddPhenotype("FunctionsEntered", func_entered_cnt_fun, 0, (EVAL_TIME * HW_MAX_THREAD_CNT)+1);
      trait_bin_sizes.emplace_back((EVAL_TIME * HW_MAX_THREAD_CNT)+1);

    }
    if (USE_MAPE_AXIS__FUNC_ENTERED_ENTROPY) {
      std::cout << "Configuring functions entered entropy axis" << std::endl;
      phen_traits.emplace_back(GetPhenotypes().GetSize(), "FunctionsEnteredEntropy");
      AddPhenotype("FunctionsEnteredEntropy", func_entered_ent_fun, 0.0, max_func_entered_entropy);
      trait_bin_sizes.emplace_back(MAPE_AXIS_SIZE__FUNC_ENTERED_ENTROPY);
    }

  });

  // Configure world update signal. 
  do_world_update_sig.AddAction([this]() {
    std::cout << "Update: " << GetUpdate() << " Max score: " << best_score << std::endl;
    // do_pop_snapshot_sig.Trigger();
    if (update % SNAPSHOT_INTERVAL == 0) do_pop_snapshot_sig.Trigger();
    Update(); 
    ClearCache();
  });

  // Generic evaluation signal actions. 
  // - At beginning of agent evaluation. 
  begin_org_eval_sig.AddAction([this](org_t & org) {
    eval_hw->SetProgram(org.GetProgram());
    pop_snapshot_info.cur_org_id = org.GetPos();
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

  // Setup population statistics TODO: fill out descriptions
  pop_snapshot_stats.emplace_back("update", [this]() { return GetUpdate(); }, "Current world update (generation).");  
  pop_snapshot_stats.emplace_back("id", [this]() { return pop_snapshot_info.cur_org_id; }, "World ID of organism.");
  pop_snapshot_stats.emplace_back("fitness", [this]() { return agg_scores(GetOrg(pop_snapshot_info.cur_org_id)); }, "Fitness of organism (in context of problem).");
  pop_snapshot_stats.emplace_back("tag_sim_thresh", [this]() { return GetOrg(pop_snapshot_info.cur_org_id).GetTagSimilarityThreshold(); }, "Tag similarity threshold for organism.");
  pop_snapshot_stats.emplace_back("inst_cnt", [this]() { return GetOrg(pop_snapshot_info.cur_org_id).GetInstCnt(); }, "");
  pop_snapshot_stats.emplace_back("inst_entropy", [this]() { return GetOrg(pop_snapshot_info.cur_org_id).GetInstEntropy(); }, "");
  pop_snapshot_stats.emplace_back("func_cnt", [this]() { return func_cnt_fun(GetOrg(pop_snapshot_info.cur_org_id)); }, "");
  pop_snapshot_stats.emplace_back("func_used", [this]() { return func_used_fun(GetOrg(pop_snapshot_info.cur_org_id)); }, "");
  pop_snapshot_stats.emplace_back("func_entered", [this]() { return func_entered_cnt_fun(GetOrg(pop_snapshot_info.cur_org_id)); }, "");
  pop_snapshot_stats.emplace_back("func_entered_entropy", [this]() { return func_entered_ent_fun(GetOrg(pop_snapshot_info.cur_org_id)); }, "");
  
  do_begin_run_sig.AddAction([this]() {
    // for each phenotype, add pop snapshot stats thing
    for (size_t i = 0; i < phen_traits.size(); ++i) {
      pop_snapshot_stats.emplace_back(phen_traits[i].name + "__bin", 
        [this, i]() { 
          return GetPhenotypes()[phen_traits[i].id].EvalBin(GetOrg(pop_snapshot_info.cur_org_id), trait_bin_sizes[phen_traits[i].id]); 
        }, phen_traits[i].desc); 
    }
  });

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

void MapElitesSignalGPWorld::Init_Configs(MapElitesGPConfig & config) {
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

  NUM_TEST_CASES = config.NUM_TEST_CASES();

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
  DOM_SNAPSHOT_TRIAL_CNT = config.DOM_SNAPSHOT_TRIAL_CNT();
  MAP_SNAPSHOT_TRIAL_CNT = config.MAP_SNAPSHOT_TRIAL_CNT();

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
void MapElitesSignalGPWorld::Init_Mutator() {
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

void MapElitesSignalGPWorld::Init_Hardware() {
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
  inst_lib.AddInst("DerefWorking", [](hardware_t & hw, const inst_t & inst) {
    state_t & state = hw.GetCurState();
    state.SetLocal(inst.args[1], state.GetLocal( (int)state.GetLocal(inst.args[0]) ) );
  }, 2, "WM[Arg2] = WM[WM[Arg1]]");
  
  inst_lib.AddInst("DerefInput", [](hardware_t & hw, const inst_t & inst) {
    state_t & state = hw.GetCurState();
    state.SetLocal(inst.args[1], state.GetInput( (int)state.GetLocal(inst.args[0]) ) );
  }, 2, "WM[Arg2] = IN[WM[Arg1]]");

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
void MapElitesSignalGPWorld::Init_Problem() {
  switch (PROBLEM_TYPE) {
    case (size_t)PROBLEM_TYPE::CHG_ENV: {
      SetupProblem_ChgEnv();
      break;
    }
    case (size_t)PROBLEM_TYPE::TESTCASES: {
      SetupProblem_Testcases();
      break;     
    }
    case (size_t)PROBLEM_TYPE::LOGIC: {
      SetupProblem_Logic();
      break;
    }
    default: {
      std::cout << "Unrecognized problem type (" << PROBLEM_TYPE << "). Exiting..." << std::endl;
      exit(-1);
    }
  }
}

void MapElitesSignalGPWorld::Init_WorldMode() {
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

void MapElitesSignalGPWorld::SetupProblem_ChgEnv() {
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

void MapElitesSignalGPWorld::SetupProblem_Testcases() {
  // TODO: fix warnings!
  testcases.LoadTestcases(TESTCASES_FPATH);
  std::cout << "Loaded test cases (" << testcases.GetTestcases().size() << ") from: " << TESTCASES_FPATH << std::endl;
  emp_assert(NUM_TEST_CASES <= testcases.GetTestcases().size());

  for (size_t i = 0; i < testcases.GetTestcases().size(); ++i) testcase_ids.emplace_back(i);

  // Setup fitness stuff
  // do_begin_eval
  //  begin_trial: reset hardware
  //  - TODO: spawn core (main) w/appropriate input
  begin_org_trial_sig.Clear();
  do_org_trial_sig.Clear();

  begin_org_trial_sig.AddAction([this](org_t & org) {
    // Reset phenotype
    phen_cache.Get(org.GetPos(), trial_id).Reset();
  });
  
  do_org_trial_sig.AddAction([this](org_t & org) {
    // TODO: pick random subset of testcases
    emp::Shuffle(*random_ptr, testcase_ids);
    for (size_t t = 0; t < NUM_TEST_CASES; ++t) {
      size_t testcase = testcase_ids[t];
      ResetEvalHW();
      eval_hw->SetTrait(trait_id_t::ORG_ID, org.GetPos());
      // Fill out input memory with testcase info. 
      memory_t input_mem;
      for (size_t i = 0; i < testcases.GetInput(testcase).size(); ++i) {
        input_mem[(int)i] = testcases.GetInput(testcase)[i];
      }
      // Spawn main core!
      eval_hw->SpawnCore(tag_t(), 0.0, input_mem, true);

      // Process!
      for (eval_time = 0; eval_time < EVAL_TIME; ++eval_time) {
        // Advance agent.
        do_org_advance_sig.Trigger(org);
      }

      // Check output
      double output = eval_hw->GetTrait(trait_id_t::PROBLEM_OUTPUT);
      bool output_set = (bool)eval_hw->GetTrait(trait_id_t::OUTPUT_SET);

      // TODO: talk to Emily about making this fitness assignment more specific to benchmark. 
      double result = 0;
      if (output_set) {
        int divisor = (int)testcases.GetOutput(testcase);
        if (divisor == 0) divisor = 1;
        result = 1 / (std::abs(output - testcases.GetOutput(testcase))/divisor);
      }
      if (result > 1000) result = 1000;

      phenotype_t & phen = phen_cache.Get(org.GetPos(), trial_id);
      phen.testcase_results.emplace_back(result);
    }
  });

  calc_score = [](org_t & org, phenotype_t & phen) {
    return emp::Sum(phen.testcase_results);
  };
  
  // Setup extra instructions
  // - Submit
  inst_lib.AddInst("SubmitResult", 
    [](hardware_t & hw, const inst_t & inst) {
      state_t & state = hw.GetCurState();
      hw.SetTrait(trait_id_t::PROBLEM_OUTPUT, state.GetLocal(inst.args[0]));
      hw.SetTrait(trait_id_t::OUTPUT_SET, 1);
    }, 1, "Submit output for given input.");
  // TODO: load input

}

void MapElitesSignalGPWorld::SetupProblem_Logic() {

  // Configure the tasks. 
  // Zero out task inputs.
  for (size_t i = 0; i < MAX_LOGIC_TASK_NUM_INPUTS; ++i) task_inputs[i] = 0;
  input_load_id = 0;

  // Add tasks to set.
  // NAND
  task_set.AddTask("NAND", [](taskset_t::Task & task, const std::array<task_io_t, MAX_LOGIC_TASK_NUM_INPUTS> & inputs) {
    const task_io_t a = inputs[0], b = inputs[1];
    task.solutions.emplace_back(~(a&b));
  }, "NAND task");
  // NOT
  task_set.AddTask("NOT", [](taskset_t::Task & task, const std::array<task_io_t, MAX_LOGIC_TASK_NUM_INPUTS> & inputs) {
    const task_io_t a = inputs[0], b = inputs[1];
    task.solutions.emplace_back(~a);
    task.solutions.emplace_back(~b);
  }, "NOT task");
  // ORN
  task_set.AddTask("ORN", [](taskset_t::Task & task, const std::array<task_io_t, MAX_LOGIC_TASK_NUM_INPUTS> & inputs) {
    const task_io_t a = inputs[0], b = inputs[1];
    task.solutions.emplace_back((a|(~b)));
    task.solutions.emplace_back((b|(~a)));
  }, "ORN task");
  // AND
  task_set.AddTask("AND", [](taskset_t::Task & task, const std::array<task_io_t, MAX_LOGIC_TASK_NUM_INPUTS> & inputs) {
    const task_io_t a = inputs[0], b = inputs[1];
    task.solutions.emplace_back(a&b);
  }, "AND task");
  // OR
  task_set.AddTask("OR", [](taskset_t::Task & task, const std::array<task_io_t, MAX_LOGIC_TASK_NUM_INPUTS> & inputs) {
    const task_io_t a = inputs[0], b = inputs[1];
    task.solutions.emplace_back(a|b);
  }, "OR task");
  // ANDN
  task_set.AddTask("ANDN", [](taskset_t::Task & task, const std::array<task_io_t, MAX_LOGIC_TASK_NUM_INPUTS> & inputs) {
    const task_io_t a = inputs[0], b = inputs[1];
    task.solutions.emplace_back((a&(~b)));
    task.solutions.emplace_back((b&(~a)));
  }, "ANDN task");
  // NOR
  task_set.AddTask("NOR", [](taskset_t::Task & task, const std::array<task_io_t, MAX_LOGIC_TASK_NUM_INPUTS> & inputs) {
    const task_io_t a = inputs[0], b = inputs[1];
    task.solutions.emplace_back(~(a|b));
  }, "NOR task");
  // XOR
  task_set.AddTask("XOR", [](taskset_t::Task & task, const std::array<task_io_t, MAX_LOGIC_TASK_NUM_INPUTS> & inputs) {
    const task_io_t a = inputs[0], b = inputs[1];
    task.solutions.emplace_back(a^b);
  }, "XOR task");
  // EQU
  task_set.AddTask("EQU", [](taskset_t::Task & task, const std::array<task_io_t, MAX_LOGIC_TASK_NUM_INPUTS> & inputs) {
    const task_io_t a = inputs[0], b = inputs[1];
    task.solutions.emplace_back(~(a^b));
  }, "EQU task");
  // ECHO
  task_set.AddTask("ECHO", [](taskset_t::Task & task, const std::array<task_io_t, MAX_LOGIC_TASK_NUM_INPUTS> & inputs) {
    const task_io_t a = inputs[0], b = inputs[1];
    task.solutions.emplace_back(a);
    task.solutions.emplace_back(b);
  }, "ECHO task");

  // Need this to happen before run, but after phenotype cache resize.
  do_pop_init_sig.AddAction([this]() {
    for (size_t i = 0; i < phen_cache.GetCache().size(); ++i) {
      phen_cache.GetCache()[i].SetTaskCnt(task_set.GetSize());
    }
  }); // 

  // Add logic problem instructions
  inst_lib.AddInst("Load-1", [this](hardware_t & hw, const inst_t & inst) {
    state_t & state = hw.GetCurState();
    state.SetLocal(inst.args[0], task_inputs[input_load_id]); // Load input.
    input_load_id += 1;
    if (input_load_id >= task_inputs.size()) input_load_id = 0; // Update load ID.
  }, 1, "WM[ARG1] = TaskInput[LOAD_ID]; LOAD_ID++;");

  inst_lib.AddInst("Load-2", [this](hardware_t & hw, const inst_t & inst) { 
    state_t & state = hw.GetCurState();
    state.SetLocal(inst.args[0], task_inputs[0]);
    state.SetLocal(inst.args[1], task_inputs[1]);
  }, 2, "WM[ARG1] = TASKINPUT[0]; WM[ARG2] = TASKINPUT[1];");
 
  inst_lib.AddInst("Submit", [this](hardware_t & hw, const inst_t & inst) { 
    state_t & state = hw.GetCurState();
    task_set.Submit((task_io_t)state.GetLocal(inst.args[0]), eval_time);
  }, 1, "Submit WM[ARG1] as potential task solution.");

  inst_lib.AddInst("Nand", [](hardware_t & hw, const inst_t & inst) {
    state_t & state = hw.GetCurState();
    const task_io_t a = (task_io_t)state.GetLocal(inst.args[0]);
    const task_io_t b = (task_io_t)state.GetLocal(inst.args[1]);
    state.SetLocal(inst.args[2], ~(a&b));
  } , 3, "WM[ARG3]=~(WM[ARG1]&WM[ARG2])");

  // setup score
  calc_score = [this](org_t & org, phenotype_t & phen) {
    // Num unique tasks completed + (TOTAL TIME - COMPLETED TIME)
    double score = 0;
    score += phen.unique_logic_tasks_done;
    if (phen.time_all_logic_tasks_done > 0) score += (EVAL_TIME - phen.time_all_logic_tasks_done);
    return score;
  };

  // Reset tasks at beginning of a trial. 
  begin_org_trial_sig.AddAction([this](org_t & org) {
    ResetTasks();
    memory_t input_mem;
    for (size_t i = 0; i < MAX_LOGIC_TASK_NUM_INPUTS; ++i) input_mem[(int)i] = task_inputs[i];
    eval_hw->SpawnCore(tag_t(), 0.0, input_mem, true);
  });

  // Logic problem needs non-default end_org_trial action.
  end_org_trial_sig.Clear();
  end_org_trial_sig.AddAction([this](org_t & org) {
    const size_t id = org.GetPos();
    phenotype_t & phen = phen_cache.Get(id, trial_id); 
    // Update logic problem phenotype info
    phen.time_all_logic_tasks_done = task_set.GetAllTasksCreditedTime();
    phen.unique_logic_tasks_done = task_set.GetUniqueTasksCredited();
    for (size_t taskID = 0; taskID < task_set.GetSize(); ++taskID) {
      phen.logic_tasks_done_by_task[taskID] = task_set.GetTask(taskID).GetCreditedCnt();
    }
    phen.score = calc_score(org, phen);
  });

  
  #ifndef EMSCRIPTEN
  // Things to record:
  //  - time all logic tasks done
  //  - unique logic tasks done
  //  - logic tasks done by task
  do_begin_run_sig.AddAction([this]() {
    pop_snapshot_stats.emplace_back("time_all_logic_tasks_completed", 
      [this]() { 
        double total = 0;
        for (size_t i = 0; i < EVAL_TRIAL_CNT; ++i) {
          total += phen_cache.Get(pop_snapshot_info.cur_org_id, i).time_all_logic_tasks_done;
        }
        return total / EVAL_TRIAL_CNT;
      }, "");
    
    pop_snapshot_stats.emplace_back("unique_logic_tasks_completed", 
      [this]() { 
        double total = 0;
        for (size_t i = 0; i < EVAL_TRIAL_CNT; ++i) {
          total += phen_cache.Get(pop_snapshot_info.cur_org_id, i).unique_logic_tasks_done;
        }
        return total / EVAL_TRIAL_CNT;
      }, "");

    if (RUN_MODE == (size_t)WORLD_MODE::EA) {
      dom_file_stats.emplace_back("time_all_logic_tasks_completed", 
        [this]() { 
          double total = 0;
          for (size_t i = 0; i < EVAL_TRIAL_CNT; ++i) {
            total += phen_cache.Get(dominant_id, i).time_all_logic_tasks_done;
          }
          return total / EVAL_TRIAL_CNT;
        }, "");

      dom_file_stats.emplace_back("unique_logic_tasks_completed", 
        [this]() { 
          double total = 0;
          for (size_t i = 0; i < EVAL_TRIAL_CNT; ++i) {
            total += phen_cache.Get(dominant_id, i).unique_logic_tasks_done;
          }
          return total / EVAL_TRIAL_CNT;
        }, "");
    }
    
    for (size_t taskID = 0; taskID < task_set.GetSize(); ++taskID) {
      pop_snapshot_stats.emplace_back("completed_"+task_set.GetName(taskID), 
        [this, taskID]() { 
          double total = 0;
          for (size_t i = 0; i < EVAL_TRIAL_CNT; ++i) {
            total += phen_cache.Get(pop_snapshot_info.cur_org_id, i).logic_tasks_done_by_task[taskID];
          }
          return total / EVAL_TRIAL_CNT; 
        }, "");
      
      if (RUN_MODE == (size_t)WORLD_MODE::EA) {
        dom_file_stats.emplace_back("completed_"+task_set.GetName(taskID), 
          [this, taskID]() { 
            double total = 0;
            for (size_t i = 0; i < EVAL_TRIAL_CNT; ++i) {
              total += phen_cache.Get(dominant_id, i).logic_tasks_done_by_task[taskID];
            }
            return total / EVAL_TRIAL_CNT; 
          }, "");
      }
    }  

  });

  #endif

}

void MapElitesSignalGPWorld::SetupWorldMode_EA() {
  
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

  // Setup dominant file stats. 
  dom_file_stats.emplace_back("update", [this](){ return GetUpdate(); }, "Update (generation) in world.");
  dom_file_stats.emplace_back("org_id", [this]() { return dominant_id; }, "Dominant organism ID.");
  dom_file_stats.emplace_back("fitness", [this]() { return agg_scores(GetOrg(dominant_id)); }, "Dominant organism fitness.");
  dom_file_stats.emplace_back("tag_sim_thresh", [this]() { return GetOrg(dominant_id).GetTagSimilarityThreshold(); }, "Tag similarity threshold for dominant organism.");
  dom_file_stats.emplace_back("inst_cnt", [this]() { return GetOrg(dominant_id).GetInstCnt(); }, "Instruction count for dominant organism.");
  dom_file_stats.emplace_back("inst_entropy", [this]() { return GetOrg(dominant_id).GetInstEntropy(); }, "Instruction entropy for dominant organism.");
  dom_file_stats.emplace_back("func_cnt", [this]() { return func_cnt_fun(GetOrg(dominant_id)); }, "Function count for dominant organism.");
  dom_file_stats.emplace_back("func_used", [this]() { return func_used_fun(GetOrg(dominant_id)); }, "Count of unique functions used by dominant organism.");
  dom_file_stats.emplace_back("func_entered", [this]() { return func_entered_cnt_fun(GetOrg(dominant_id)); }, "Count of functions entered/called by dominant organism.");
  dom_file_stats.emplace_back("func_entered_entropy", [this]() { return func_entered_ent_fun(GetOrg(dominant_id)); }, "Functions entered entropy for dominant organism");

  do_begin_run_sig.AddAction([this]() {
    // for each phenotype, add pop snapshot stats thing
    for (size_t i = 0; i < phen_traits.size(); ++i) {
      dom_file_stats.emplace_back(phen_traits[i].name + "__bin", 
        [this, i]() { 
          return GetPhenotypes()[phen_traits[i].id].EvalBin(GetOrg(dominant_id), trait_bin_sizes[phen_traits[i].id]); 
        }, phen_traits[i].desc); 
    }
    // Add dominant file. 
    AddDominantFile(DATA_DIRECTORY + "/dominant.csv").SetTimingRepeat(STATISTICS_INTERVAL);
  });
  
  
  #endif

  // One of last things to do before run: resize phenotype cache
  do_begin_run_sig.AddAction([this]() {
    std::cout << "Resizing the phenotype cache(" << POP_SIZE << ")!" << std::endl;
    phen_cache.Resize(POP_SIZE+1, EVAL_TRIAL_CNT); // Add one position as temp position for MAP-elites
  });
  
}

void MapElitesSignalGPWorld::SetupWorldMode_MAPE() {
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



  // One of last things to do before run: resize phenotype cache
  do_begin_run_sig.AddAction([this]() {
    emp::SetMapElites(*this, trait_bin_sizes);
    std::cout << "Resizing the phenotype cache (" << GetSize() + 1 << ")!" << std::endl;
    phen_cache.Resize(GetSize() + 1, EVAL_TRIAL_CNT); // Add one position as temp position for MAP-elites
  });

}

// === Run functions ===
void MapElitesSignalGPWorld::Run() {
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

void MapElitesSignalGPWorld::RunStep() {
  // could move these onto OnUpdate signal
  do_evaluation_sig.Trigger();
  do_selection_sig.Trigger();
  do_world_update_sig.Trigger();
}

// === Changing environment utility functions
void MapElitesSignalGPWorld::SaveChgEnvTags() {
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

void MapElitesSignalGPWorld::LoadChgEnvTags() {
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
void MapElitesSignalGPWorld::InitPop_Random() {
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
void MapElitesSignalGPWorld::InitPop_Ancestor() {
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
void MapElitesSignalGPWorld::Snapshot_Programs() {
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
void MapElitesSignalGPWorld::Snapshot_PopulationStats() {
  std::string snapshot_dir = DATA_DIRECTORY + "pop_" + emp::to_string((int)GetUpdate());
  mkdir(snapshot_dir.c_str(), ACCESSPERMS);
  emp::DataFile file(snapshot_dir + "/pop_" + emp::to_string((int)GetUpdate()) + ".csv");

  for (size_t i = 0; i < pop_snapshot_stats.size(); ++i) {
    file.AddFun(pop_snapshot_stats[i].fun, pop_snapshot_stats[i].name, pop_snapshot_stats[i].desc);
  }
  file.PrintHeaderKeys();

  // Loop through the population, updating file with individuals' stats. 
  for (pop_snapshot_info.cur_org_id = 0; pop_snapshot_info.cur_org_id < GetSize(); ++pop_snapshot_info.cur_org_id) {
    if (!IsOccupied(pop_snapshot_info.cur_org_id)) continue;
    org_t & org = GetOrg(pop_snapshot_info.cur_org_id);
    org.SetPos(pop_snapshot_info.cur_org_id);
    Evaluate(org);
    file.Update();
  }

}

/// Snapshot dominant program performance over many trials (only makes sense in context of EA run). 
void MapElitesSignalGPWorld::Snapshot_Dominant() {
  emp_assert(RUN_MODE == (size_t)WORLD_MODE::EA);

  std::string snapshot_dir = DATA_DIRECTORY + "pop_" + emp::to_string((int)GetUpdate());
  mkdir(snapshot_dir.c_str(), ACCESSPERMS);
  emp::DataFile file(snapshot_dir + "/dom_" + emp::to_string((int)GetUpdate()) + ".csv");

  size_t evalID = 0;
  std::function<size_t(void)> get_evalID = [&evalID](){ return evalID; };
  file.AddFun(get_evalID, "eval", "Evaluation ID of this stat-line.");
  for (size_t i = 0; i < pop_snapshot_stats.size(); ++i) {
    file.AddFun(pop_snapshot_stats[i].fun, pop_snapshot_stats[i].name, pop_snapshot_stats[i].desc);
  }
  file.PrintHeaderKeys();

  // emp::vector<double 
  org_t & dom = GetOrg(dominant_id);
  dom.SetPos(dominant_id);

  for (size_t evalID = 0; evalID < DOM_SNAPSHOT_TRIAL_CNT; ++evalID) {
    Evaluate(dom);
    file.Update();
  }
  
}

/// Snapshot map from MAP-elites (only makes sense in context of MAP-Elites run). 
void MapElitesSignalGPWorld::Snapshot_MAP(void) {
  emp_assert(RUN_MODE == (size_t)WORLD_MODE::MAPE);

  std::string snapshot_dir = DATA_DIRECTORY + "pop_" + emp::to_string((int)GetUpdate());
  mkdir(snapshot_dir.c_str(), ACCESSPERMS);
  emp::DataFile file(snapshot_dir + "/map_" + emp::to_string((int)GetUpdate()) + ".csv");

  size_t evalID = 0;
  std::function<size_t(void)> get_evalID = [&evalID](){ return evalID; };
  file.AddFun(get_evalID, "eval", "Evaluation ID of this stat-line.");
  for (size_t i = 0; i < pop_snapshot_stats.size(); ++i) {
    file.AddFun(pop_snapshot_stats[i].fun, pop_snapshot_stats[i].name, pop_snapshot_stats[i].desc);
  }
  
  file.PrintHeaderKeys();

  for (size_t orgID = 0; orgID < GetSize(); ++orgID) {
    if (!IsOccupied(orgID)) continue;
    for (evalID = 0; evalID < MAP_SNAPSHOT_TRIAL_CNT; ++evalID) {
      org_t & org = GetOrg(orgID);
      org.SetPos(orgID);
      Evaluate(org);
      file.Update();
    }
  }
}

/// Add a data file to track dominant program. Will track at same interval as fitness file. (only makes sense in context of EA run).
emp::DataFile & MapElitesSignalGPWorld::AddDominantFile(const std::string & fpath="dominant.csv") {
  auto & file = SetupFile(fpath);

  // TODO: convert to dom_stats thing
  for (size_t i = 0; i < dom_file_stats.size(); ++i) {
    file.AddFun(dom_file_stats[i].fun, dom_file_stats[i].name, dom_file_stats[i].desc);
  }

  file.PrintHeaderKeys();
  return file;
}


#endif