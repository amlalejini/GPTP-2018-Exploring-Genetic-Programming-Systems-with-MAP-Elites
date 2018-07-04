// This is the main function for the NATIVE version of this project.

#include <iostream>

#include "base/vector.h"
#include "config/command_line.h"
#include "config/ArgManager.h"

#include "../MapElitesSignalGP_World.h"
#include "../MapElitesScopeGP_World.h"

enum class REPRESENTATION_TYPE {SignalGP=0, ScopeGP=1};

int main(int argc, char* argv[])
{
  std::string config_fname = "MapElitesGPConfig.cfg";
  MapElitesGPConfig config;
  config.Read(config_fname);
  auto args = emp::cl::ArgManager(argc, argv);
  if (args.ProcessConfigOptions(config, std::cout, config_fname, "MapElitesGP-macros.h") == false) exit(0);
  if (args.TestUnknown() == false) exit(0);  // If there are leftover args, throw an error.
  emp::Random rnd(config.RANDOM_SEED());
  
  std::cout << "==============================" << std::endl;
  std::cout << "|    How am I configured?    |" << std::endl;
  std::cout << "==============================" << std::endl;
  config.Write(std::cout);
  std::cout << "==============================\n"
            << std::endl;

  if (config.REPRESENTATION() == (size_t) REPRESENTATION_TYPE::SignalGP) {
    MapElitesSignalGPWorld world(rnd);
    world.Setup(config);
    world.Run();
  } else if (config.REPRESENTATION() == (size_t) REPRESENTATION_TYPE::ScopeGP) {
    MapElitesScopeGPWorld world(rnd);
    world.Setup(config);
    world.Run();
  }

}
