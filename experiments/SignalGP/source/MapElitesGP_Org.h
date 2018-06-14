#ifndef MAPE_GP_ORG_H
#define MAPE_GP_ORG_H

#include "hardware/EventDrivenGP.h"

class MapElitesGPOrg {
public:
  // Useful aliases
  static constexpr size_t TAG_WIDTH = 16;
  using hardware_t = emp::EventDrivenGP_AW<TAG_WIDTH>;
  using program_t = emp::EventDrivenGP_AW<TAG_WIDTH>::Program; 
  using event_lib_t = typename hardware_t::event_lib_t;
  using inst_lib_t = typename hardware_t::inst_lib_t;
  using inst_t = typename hardware_t::inst_t;
  using hw_state_t = typename hardware_t::State;

protected:
  size_t pos; ///< Position in world.
  program_t program;

public:
  MapElitesGPOrg(const program_t & _p) : pos(0), program(_p) { ; }
  MapElitesGPOrg(const MapElitesGPOrg &) = default;
  MapElitesGPOrg(MapElitesGPOrg &&) = default;

  size_t GetPos() const { return pos; }
  void SetPos(size_t id) { pos = id; }

  program_t & GetGenome() { return program; }

};

#endif

