#ifndef MAPE_GP_WORLD_H
#define MAPE_GP_WORLD_H

#include <iostream>

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

class MapElitesGPWorld : public emp::World<MapElitesGPOrg> {
public:
  enum class WORLD_MODE { EA=0, MAPE=1 };
  enum class PROBLEM_TYPE { CHG_ENV=0, TESTCASES=1 };
  enum class POP_INIT_METHOD { RANDOM=0, ANCESTOR=1 };
  enum class CHGENV_TAG_GEN_METHOD { RANDOM=0, LOAD=1 }; 
  enum class ENV_CHG_METHOD { PROB=0, CYCLE=1 };
  struct OrgPhenotype;

  using org_t = MapElitesGPOrg; 
  using phenotype_t = OrgPhenotype;
  using hardware_t = typename org_t::hardware_t;
  using program_t = typename org_t::program_t;
  using genome_t = typename org_t::genome_t;
  using inst_lib_t = typename org_t::inst_lib_t;
  using event_lib_t = typename org_t::event_lib_t;
  using tag_t = typename hardware_t::affinity_t;
  using trait_id_t = typename org_t::HW_TRAIT_ID;


  struct OrgPhenotype {
    // Generic
    double score;

    // For changing environment problem
    double env_match_score;

    // For logic 9 problem
    // TODO
    // For testcases problem
    // TODO

    void Reset() {
      score = 0;
      env_match_score = 0;
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
  // == Problem group ==
  size_t PROBLEM_TYPE;
  std::string TESTCASES_FPATH;
  // == Changing environment group ==
  size_t ENV_TAG_GEN_METHOD;
  std::string ENV_TAG_FPATH;
  size_t ENV_STATE_CNT;
  bool ENV_DISTRACTION_SIGS;
  size_t ENV_DISTRACTION_SIG_CNT;
  size_t ENV_CHG_METHOD;
  double ENV_CHG_PROB;
  size_t ENV_CHG_RATE;
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
  // == Hardware group ==
  size_t HW_MAX_THREAD_CNT;
  size_t HW_MAX_CALL_DEPTH;
  double HW_MIN_TAG_SIMILARITY_THRESH;
  // == Data tracking group ==
  size_t POP_SNAPSHOT_INTERVAL;

  emp::SignalGPMutator<org_t::TAG_WIDTH> mutator;

  inst_lib_t inst_lib;
  event_lib_t event_lib;

  emp::Ptr<hardware_t> eval_hw;

  PhenotypeCache<OrgPhenotype> phen_cache;

  size_t eval_time;

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
  emp::Signal<void(void)> do_evaluation_sig;
  emp::Signal<void(void)> do_selection_sig;
  emp::Signal<void(void)> do_world_update_sig;

  // TODO: when caching phenotypes, make cache max_env size + 1; use last position for MAP-elites eval
  //  - Use GetSize() for max capacity

  // Data-tracking signals
  emp::Signal<void(size_t)> do_pop_snapshot_sig;

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

  void SetupProblem_ChgEnv();
  void SetupProblem_Testcases();

  // === Changing environment utility functions
  void SaveChgEnvTags();
  void LoadChgEnvTags();

  // === Eval hardware utility functions ===
  void ResetEvalHW() {
    eval_hw->ResetHardware();
    // TODO: add signal for onreset hardware
    eval_hw->SetTrait(trait_id_t::PROBLEM_OUTPUT, -1);
    eval_hw->SetTrait(trait_id_t::ORG_STATE, -1);
  }

  // === Evaluation functions ===
  /// Evaluate given agent.
  void Evaluate(org_t & org) {
    begin_org_eval_sig.Trigger(org);  //? Can I keep trial ID local? 
    for (size_t trial_id = 0; trial_id < EVAL_TRIAL_CNT; ++trial_id) {
      begin_org_trial_sig.Trigger(org);
      do_org_trial_sig.Trigger(org);
      end_org_trial_sig.Trigger(org);
    }
    end_org_eval_sig.Trigger(org);
  }

  /// Used to poke the world as I develop it. 
  void Test() {
    // TODO: run environment for a bit, check changing
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
  Init_Mutator();       // Configure SignalGPMutator, mutation function.
  Init_Hardware();      // Configure SignalGP hardware. 
  Init_Problem();       // Configure problem.
  // - Setup problem
  // - Setup data tracking (but only in native mode)
  //    - Pop snapshot
  //    - Fitness file
  //    - Dominant file
  // - Setup selection
  //   - MAPE vs. EA
  //      - Setup phen cache size
  /*
  For map-eliltes:  --> Need branch w/these signals in SignalGP- 
  eval_hw->OnBeforeFuncCall([this](hardware_t & hw, size_t fID) {
    functions_used.emplace(fID);
  });
  eval_hw->OnBeforeCoreSpawn([this](hardware_t & hw, size_t fID) {
    functions_used.emplace(fID);
  });
  */
  // - Init population

  // Configure run signals. 
  do_world_update_sig.AddAction([this]() {
    if (update % POP_SNAPSHOT_INTERVAL == 0) do_pop_snapshot_sig.Trigger(update);
    Update(); 
  });

  // Generic evaluation signal actions. 
  // - At beginning of agent evaluation. 
  begin_org_eval_sig.AddAction([this](org_t & org) {
    eval_hw->SetProgram(org.GetProgram());
  });

  Test();
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

  PROBLEM_TYPE = config.PROBLEM_TYPE();
  TESTCASES_FPATH = config.TESTCASES_FPATH();

  ENV_TAG_GEN_METHOD = config.ENV_TAG_GEN_METHOD();
  ENV_TAG_FPATH = config.ENV_TAG_FPATH();
  ENV_STATE_CNT = config.ENV_STATE_CNT();
  ENV_DISTRACTION_SIGS = config.ENV_DISTRACTION_SIGS();
  ENV_DISTRACTION_SIG_CNT = config.ENV_DISTRACTION_SIG_CNT();
  ENV_CHG_METHOD = config.ENV_CHG_METHOD();
  ENV_CHG_PROB = config.ENV_CHG_PROB();
  ENV_CHG_RATE = config.ENV_CHG_RATE();

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

  HW_MAX_THREAD_CNT = config.HW_MAX_THREAD_CNT();
  HW_MAX_CALL_DEPTH = config.HW_MAX_CALL_DEPTH();
  HW_MIN_TAG_SIMILARITY_THRESH = config.HW_MIN_TAG_SIMILARITY_THRESH();

  POP_SNAPSHOT_INTERVAL = config.POP_SNAPSHOT_INTERVAL();

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
  SetMutFun([this](org_t & org, emp::Random & r) {
    return mutator.ApplyMutations(org.GetProgram(), r);
  });
  // TODO: setup mutation set (vector of mutation functions) that we can add mutate functions to

  SetAutoMutate(); // Mutations will occur before deciding where a new organism is placed. 
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

}

/// Initialize selected problem. 
void MapElitesGPWorld::Init_Problem() {
  switch(PROBLEM_TYPE) {
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
    case (size_t)ENV_CHG_METHOD::PROB: {
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

  // TODO: fitness/phenotype updates
}

void MapElitesGPWorld::SetupProblem_Testcases() {
  // TODO
}

// === Run functions ===
void MapElitesGPWorld::Run() {
  switch(RUN_MODE) {
    case (size_t)WORLD_MODE::EA: 
      // EA-mode does the same thing as MAPE-mode during a run. (we leave the break out and drop into MAPE case)
    case (size_t)WORLD_MODE::MAPE: {
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

#endif