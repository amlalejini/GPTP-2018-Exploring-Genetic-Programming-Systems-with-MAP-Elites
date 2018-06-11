#ifndef MAPE_GP_CONFIG_H
#define MAPE_GP_CONFIG_H

#include "config/config.h"

EMP_BUILD_CONFIG( MapeElitesGPConfig,
  GROUP(GENERAL, "General settings for our experiment."),
  VALUE(RUN_MODE, size_t, 0, "What mode are we running in? \n0: Standard EA, \n1: MAP-Elites"),
  VALUE(RANDOM_SEED, int, -1, "Random number seed (negative value for based on time)"),
  VALUE(POP_SIZE, size_t, 1000, "Total population size"),
  VALUE(GENERATIONS, size_t, 100, "How many generations should we run evolution?"),
  VALUE(POP_INIT_METHOD, size_t, 0, "How should we initialize the population? \n0: Randomly, \n1: From a common ancestor"),
  VALUE(ANCESTOR_FPATH, std::string, "ancestor.gp", "Ancestor program file")
)

#endif