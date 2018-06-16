#ifndef MAPE_GP_ORG_H
#define MAPE_GP_ORG_H

#include "hardware/EventDrivenGP.h"

/// Struct describing phenotypic characteristics of MapElitesGPOrg. 
struct MapElitesOrgPhenotype {

};

class MapElitesGPOrg {
public:
  struct Genome;
  // Useful aliases
  static constexpr size_t TAG_WIDTH = 16;
  using hardware_t = emp::EventDrivenGP_AW<TAG_WIDTH>;
  using program_t = emp::EventDrivenGP_AW<TAG_WIDTH>::Program; 
  using event_lib_t = typename hardware_t::event_lib_t;
  using inst_lib_t = typename hardware_t::inst_lib_t;
  using inst_t = typename hardware_t::inst_t;
  using hw_state_t = typename hardware_t::State;

  using genome_t = Genome;

  /// Hardware trait indexes.                               
  ///   - ORG_STATE - Used to track organism state for changing environment problem.
  ///   - PROBLEM_OUTPUT - used to track organism's output to a problem.                              ///
  enum HW_TRAIT_ID { PROBLEM_OUTPUT=0, ORG_STATE=1 }; 

  struct Genome {
    program_t program;
    double tag_sim_thresh;

    Genome(const program_t & _p, double _s=0) : program(_p), tag_sim_thresh(_s) { ; }
    Genome(Genome && in)=default;
    Genome(const Genome & in)=default;
  };

protected:
  size_t pos; ///< Position in world.
  genome_t genome;
  
  /// Information about organism genome
  /// (e.g. program stats, stuff that doesn't change in context of environment)
  struct GenomeInfo {
    bool calculated; ///< Have we already calculated this information for this organism? 
    double inst_entropy;
    double inst_cnt; 

    GenomeInfo() : calculated(false), inst_entropy(0), inst_cnt(0) { ; }
  } genome_info;

public:
  // TODO: reset genome info on birth!
  MapElitesGPOrg(const genome_t & _g) : pos(0), genome(_g), genome_info() { ; }
  MapElitesGPOrg(const MapElitesGPOrg &) = default;
  MapElitesGPOrg(MapElitesGPOrg &&) = default;

  size_t GetPos() const { return pos; }
  void SetPos(size_t id) { pos = id; }

  genome_t & GetGenome() { return genome; }
  program_t & GetProgram() { return genome.program; }

  void ResetGenomeInfo() { genome_info.calculated = false; }
  
  void CalcGenomeInfo() {
    // TODO: force genome info calculation
    // - inst entropy
    // - inst count...
    genome_info.calculated = true; 
  }

  // TODO
  double GetInstEntropy() { return 0.0; }
  double GetInstCnt() { return 0.0; }
  double GetFunctionCnt() { return 0.0; }

};

#endif

