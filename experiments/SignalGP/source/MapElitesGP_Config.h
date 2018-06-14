#ifndef MAPE_GP_CONFIG_H
#define MAPE_GP_CONFIG_H

#include "config/config.h"

EMP_BUILD_CONFIG( MapElitesGPConfig,
  GROUP(GENERAL, "General settings for our experiment."),
  VALUE(RUN_MODE, size_t, 0, "What mode are we running in? \n0: Standard EA, \n1: MAP-Elites"),
  VALUE(RANDOM_SEED, int, -1, "Random number seed (negative value for based on time)"),
  VALUE(POP_SIZE, size_t, 1000, "Total population size"),
  VALUE(GENERATIONS, size_t, 100, "How many generations should we run evolution?"),
  VALUE(POP_INIT_METHOD, size_t, 0, "How should we initialize the population? \n0: Randomly, \n1: From a common ancestor"),
  VALUE(ANCESTOR_FPATH, std::string, "ancestor.gp", "Ancestor program file"),

  GROUP(PROBLEM, "Settings related to the problem we're evolving programs to solve."),
  VALUE(PROBLEM_TYPE, size_t, 0, "What problem are we solving? \n0: Changing environment problem \n1: Testcase problem (requires TESTCASES_FPATH setting)"),
  VALUE(TESTCASES_FPATH, std::string, "testcases-squares.csv", "Where is the file containing testcases for the problem we're solving?"),

  GROUP(PROGRAM_CONSTRAINTS, "SignalGP program constraits that mutation operators/initialization will respect."),
  VALUE(PROG_MIN_FUNC_CNT, size_t, 1, "Minimum number of functions mutations are allowed to reduce a SignalGP program to."),
  VALUE(PROG_MAX_FUNC_CNT, size_t, 16, "Maximum number of functions a mutated SignalGP program can grow to. "),
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

)

#endif