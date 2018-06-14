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

class MapElitesGPWorld : public emp::World<MapElitesGPOrg> {
public:
  enum WORLD_MODE { EA=0, MAPE=1 };
  enum PROBLEM_TYPE { CHG_ENV=0, TESTCASES=1 };
  enum POP_INIT_METHOD { RANDOM=0, ANCESTOR=1 };

  using org_t = MapElitesGPOrg; 
  using hardware_t = typename org_t::hardware_t;
  using program_t = typename org_t::program_t;
  using inst_lib_t = typename org_t::inst_lib_t;
  using event_lib_t = typename org_t::event_lib_t;
  // using genome_t = typename 
  
protected:
  // Localized configurable parameters
  // == General Group ==
  size_t RUN_MODE;
  int RANDOM_SEED;
  size_t POP_SIZE;
  size_t GENERATIONS;
  size_t POP_INIT_METHOD;
  std::string ANCESTOR_FPATH;
  // == Problem Group ==
  size_t PROBLEM_TYPE;
  std::string TESTCASES_FPATH;
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

  emp::SignalGPMutator<org_t::TAG_WIDTH> mutator;

  inst_lib_t inst_lib;
  event_lib_t event_lib;

  emp::Ptr<hardware_t> eval_hw;

  // Run signals
  emp::Signal<void(void)> do_evaluation_sig;
  emp::Signal<void(void)> do_selection_sig;
  emp::Signal<void(void)> do_world_update_sig;

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
  Init_Mutator(); 
  Init_Hardware();
  Init_Problem();       // Initialize problem
  
  // - SetMutateBeforeBirth
  // - Setup data tracking
  // - Setup problem
  // - Setup selection
  //   - MAPE vs. EA
  // - Init population
}

void MapElitesGPWorld::Init_Configs(MapElitesGPConfig & config) {
  RUN_MODE = config.RUN_MODE();
  RANDOM_SEED = config.RANDOM_SEED();
  POP_SIZE = config.POP_SIZE();
  GENERATIONS = config.GENERATIONS();
  POP_INIT_METHOD = config.POP_INIT_METHOD();
  ANCESTOR_FPATH = config.ANCESTOR_FPATH();

  PROBLEM_TYPE = config.PROBLEM_TYPE();
  TESTCASES_FPATH = config.TESTCASES_FPATH();

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
    return mutator.ApplyMutations(org.GetGenome(), r);
  });
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
  // TODO
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

#endif