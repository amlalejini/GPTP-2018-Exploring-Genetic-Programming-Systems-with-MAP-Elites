#ifndef MAPE_GP_CONFIG_H
#define MAPE_GP_CONFIG_H

#include "config/config.h"

EMP_BUILD_CONFIG( MapElitesGPConfig,
  GROUP(GENERAL, "General settings for our experiment."),
  VALUE(WORLD_STRUCTURE, size_t, 0, "What mode are we running in? \n0: Well-mixed, \n1: MAP-Elites"),
  VALUE(RANDOM_SEED, int, -1, "Random number seed (negative value for based on time)"),
  VALUE(POP_SIZE, size_t, 1000, "Total population size"),
  VALUE(REPRESENTATION, size_t, 0, "0 = SignalGP, 1=ScopeGP"),
  VALUE(GENERATIONS, size_t, 100, "How many generations should we run evolution?"),
  VALUE(POP_INIT_METHOD, size_t, 0, "How should we initialize the population? \n0: Randomly, \n1: From a common ancestor"),
  VALUE(ANCESTOR_FPATH, std::string, "ancestor.gp", "Ancestor program file"),

  GROUP(EVALUATION, "Settings related to evaluating SignalGP programs."),
  VALUE(EVAL_TRIAL_CNT, size_t, 3, "How many independent trials should we evaluate each program for when calculating fitness?"),
  VALUE(EVAL_TRIAL_AGG_METHOD, size_t, 0, "What method should we use to aggregate scores (to determine actual fitness) across fitness evaluation trials? \n0: Fitness = Min trial score \n1: Fitness = Max trial score \n2: Fitness = Avg trial score"),
  VALUE(EVAL_TIME, size_t, 256, "How many time steps should we evaluate organisms during each evaluation trial?"),

  GROUP(EA_SELECTION, "Settings used to specify how selection should happen."),
  VALUE(SELECTION_METHOD, size_t, 0, "Which selection scheme should we use to select organisms to reproduce (asexually)? Note: this is only relevant when running in EA mode. \n0: Tournament \n1: Lexicase \n2: Random "),
  VALUE(ELITE_CNT, size_t, 0, "How many elites should we select to reproduce no matter what (0=no elite selection)?"),
  VALUE(TOURNAMENT_SIZE, size_t, 2, "How big are tournaments when performing tournament selection?"),

  GROUP(MAP_ELITES, "Settings specific to MAP-Elites"),
  VALUE(USE_MAPE_AXIS__INST_ENTROPY, bool, true, "Should we use instruction entropy as a MAP-Elites axis?"),
  VALUE(MAPE_AXIS_SIZE__INST_ENTROPY, size_t, 20, "Width (in map grid cells) of the instruction entropy MAPE axis?"),
  VALUE(USE_MAPE_AXIS__INST_CNT, bool, false, "Should we use instruction count as a MAP-Elites axis?"),
  VALUE(USE_MAPE_AXIS__FUNC_USED, bool, true, "Should we use functions used as a MAP-Elites axis?"),
  VALUE(USE_MAPE_AXIS__FUNC_CNT, bool, false, "Should we use function count as a MAP-Elites axis?"),
  VALUE(USE_MAPE_AXIS__FUNC_ENTERED, bool, false, "Should we use number of functions entered (repeats are counted) as a MAP-Elites axis?"),
  VALUE(USE_MAPE_AXIS__FUNC_ENTERED_ENTROPY, bool, false, "Should we use entropy of number of functions entered as a MAP-Elites axis?"),
  VALUE(MAPE_AXIS_SIZE__FUNC_ENTERED_ENTROPY, size_t, 20, "Width (in map grid cells) of the functions entered entropy MAPE axis?"),

  GROUP(PROBLEM, "Settings related to the problem we're evolving programs to solve."),
  VALUE(PROBLEM_TYPE, size_t, 0, "What problem are we solving? \n0: Changing environment problem \n1: Testcase problem (requires TESTCASES_FPATH setting) \n2: Logic tasks problem"),
  VALUE(TESTCASES_FPATH, std::string, "testcases/examples-squares.csv", "Where is the file containing testcases for the problem we're solving?"),

  GROUP(CHG_ENV_PROBLEM, "Settings specific to the changing environment problem"),
  VALUE(ENV_TAG_GEN_METHOD, size_t, 0, "How should we generate environment tags (true and distraction)? \n0: Randomly\n1: Load from file (ENV_TAG_FPATH)"),
  VALUE(ENV_TAG_FPATH, std::string, "env_tags.csv", "Where should we save/load environment tags to/from?"),
  VALUE(ENV_STATE_CNT, size_t, 8, "How many environment states are there?"),
  VALUE(ENV_CHG_SIG, bool, true, "Does the environment produce a signal on a change?"),
  VALUE(ENV_DISTRACTION_SIGS, bool, false, "Does the environment emit distraction signals?"),
  VALUE(ENV_DISTRACTION_SIG_CNT, size_t, 8, "How many environment distraction signals are there?"),
  VALUE(ENV_CHG_METHOD, size_t, 0, "How should the environment change? \n0: Probabilistically every time step \n1: At a fixed time cycle?"),
  VALUE(ENV_CHG_PROB, double, 0.125, "With what probability should the environment change (only relevant when ENV_CHG_METHOD = 0)?"),
  VALUE(ENV_CHG_RATE, size_t, 16, "How often should the environment change (only relevant when ENV_CHG_METHOD = 1)?"),
  VALUE(ENV_SENSORS, bool, false, "Should we include active-polling environment sensors in the instruction set?"),

  GROUP(TESTCASES_PROBLEM, "Settings specific to test case problems."),
  VALUE(NUM_TEST_CASES, size_t, 10, "How many test cases should we use when evaluating an organism?"), 
  VALUE(SHUFFLE_TEST_CASES, bool, false, "Should we shuffle test cases used to evaluate agents every generation? "),

  GROUP(PROGRAM_CONSTRAINTS, "SignalGP program constraits that mutation operators/initialization will respect."),
  VALUE(PROG_MIN_FUNC_CNT, size_t, 1, "Minimum number of functions mutations are allowed to reduce a SignalGP program to."),
  VALUE(PROG_MAX_FUNC_CNT, size_t, 32, "Maximum number of functions a mutated SignalGP program can grow to. "),
  VALUE(PROG_MIN_FUNC_LEN, size_t, 1, "Minimum number of instructions a SignalGP function can shrink to."),
  VALUE(PROG_MAX_FUNC_LEN, size_t, 32, "Maximum number of instructions a SignalGP function can grow to."),
  VALUE(PROG_MAX_TOTAL_LEN, size_t, 512, "Maximum number of *total* instructions a SignalGP program can grow to. "),
  VALUE(PROG_MIN_ARG_VAL, int, 0, "Minimum argument value a SignalGP instruction can mutate to."),
  VALUE(PROG_MAX_ARG_VAL, int, 15, "Maximum argument value a SignalGP instruction can mutate to."),

  GROUP(MUTATION, "Settings specifying mutation rates."),
  VALUE(ARG_SUB__PER_ARG, double, 0.005, "Rate to apply substitutions to instruction arguments."),
  VALUE(INST_SUB__PER_INST, double, 0.005, "Per-instruction rate to apply instruction substitutions. "),
  VALUE(INST_INS__PER_INST, double, 0.005, "Per-instruction rate to apply instruction insertions."),
  VALUE(INST_DEL__PER_INST, double, 0.005, "Per-instruction rate to apply instruction deletions."),
  VALUE(SLIP__PER_FUNC, double, 0.05, "Per-function rate to apply slip-mutations."),
  VALUE(FUNC_DUP__PER_FUNC, double, 0.05, "Per-function rate to apply function duplications."),
  VALUE(FUNC_DEL__PER_FUNC, double, 0.05, "Per-function rate to apply function deletions."),
  VALUE(TAG_BIT_FLIP__PER_BIT, double, 0.005, "Per-bit rate to apply tag bit flips. "),
  VALUE(EVOLVE_HW_TAG_SIM_THRESH, bool, false, "Are we evolving SignalGP tag similarity thresholds?"),

  // VALUE()

  GROUP(HARDWARE, "Settings for SignalGP hardware"),
  VALUE(HW_MAX_THREAD_CNT, size_t, 8, "What is the maximum number of threads that can be active at any one time on the SignalGP hardware?"),
  VALUE(HW_MAX_CALL_DEPTH, size_t, 128, "What is the maximum call depth for SignalGP hardware?"),
  VALUE(HW_MIN_TAG_SIMILARITY_THRESH, double, 0.0, "What is the minimum required similarity threshold for tags to successfully match when performing tag-based referencing?"),

  GROUP(DATA_TRACKING, "Settings relevant to experiment data-tracking."),
  VALUE(DATA_DIRECTORY, std::string, "./output", "Location to dump data output."),
  VALUE(STATISTICS_INTERVAL, size_t, 100, "How often should we output summary statistics?"),
  VALUE(SNAPSHOT_INTERVAL, size_t, 1000, "How often should we take a population snapshot?"),
  VALUE(DOM_SNAPSHOT_TRIAL_CNT, size_t, 100, "How many trials should we do in dominant snapshot?"),
  VALUE(MAP_SNAPSHOT_TRIAL_CNT, size_t, 10, "How many trials should we do in a map snapshot?"),
)

#endif