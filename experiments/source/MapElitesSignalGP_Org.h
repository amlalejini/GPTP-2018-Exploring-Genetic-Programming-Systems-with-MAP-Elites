#ifndef MAPE_SIGNALGP_ORG_H
#define MAPE_SIGNALGP_ORG_H

#include <algorithm>

#include "hardware/EventDrivenGP.h"

class MapElitesSignalGPOrg {
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
  enum HW_TRAIT_ID { ORG_ID=0, PROBLEM_OUTPUT=1, ORG_STATE=2, OUTPUT_SET=3 }; 

  /// Struct to keep track of the genome, which includes everything that we directly mutate/evolve.
  struct Genome {
    program_t program;      ///< Program that defines organism behavior. 
    double tag_sim_thresh;  ///< Minimum tag similarity threshold. 

    Genome(const program_t & _p, double _s=0) : program(_p), tag_sim_thresh(_s) { ; }
    Genome(Genome && in) : program(in.program), tag_sim_thresh(in.tag_sim_thresh) { ; }
    Genome(const Genome & in) : program(in.program), tag_sim_thresh(in.tag_sim_thresh) { ; }

    bool operator==(const Genome & in) const { return program == in.program && tag_sim_thresh == in.tag_sim_thresh; }
    bool operator!=(const Genome & in) const { return !(*this == in); }
    bool operator<(const Genome & other) const {
        if (program == other.program) {
          return tag_sim_thresh < other.tag_sim_thresh;
        } else {
          return program < other.program;
        } 
    }
  };

protected:
  size_t pos;       ///< Position in world.
  genome_t genome;  ///< Organism genome.
  
  /// Information about organism genome
  /// (e.g. program stats, stuff that doesn't change in context of environment)
  struct GenomeInfo {
    bool calculated; ///< Have we already calculated this information for this organism? 
    double inst_entropy;
    double inst_cnt; 

    GenomeInfo() : calculated(false), inst_entropy(0), inst_cnt(0) { ; }
    GenomeInfo(GenomeInfo && in) : calculated(in.calculated), inst_entropy(in.inst_entropy), inst_cnt(in.inst_cnt) { ; }
    GenomeInfo(const GenomeInfo & in) : calculated(in.calculated), inst_entropy(in.inst_entropy), inst_cnt(in.inst_cnt) { ; }

  } genome_info;

public:
  MapElitesSignalGPOrg(const genome_t & _g) : pos(0), genome(_g), genome_info() { ; }
  MapElitesSignalGPOrg(const MapElitesSignalGPOrg & in) : pos(in.pos), genome(in.genome), genome_info(in.genome_info) { ; }
  MapElitesSignalGPOrg(MapElitesSignalGPOrg && in) : pos(in.pos), genome(in.genome), genome_info(in.genome_info) { ; }

  /// Retrieve the position of the organism (which is whatever was set via SetPos). 
  size_t GetPos() const { return pos; }

  /// Update the position of the organism. 
  void SetPos(size_t id) { pos = id; }

  /// Retrieve the genome of this organism. 
  genome_t & GetGenome() { return genome; }

  /// Retrieve the program for this organism (from within the genome). 
  program_t & GetProgram() { return genome.program; }

  /// Retrieve the tag similarity threshold for this organism (from withint he genome).
  double GetTagSimilarityThreshold() const { return genome.tag_sim_thresh; }

  /// Reset genome information (i.e., flag that it is no longer accurate). 
  void ResetGenomeInfo() { genome_info.calculated = false; }
  
  /// Calculate genome information, filling out genome_info member variable. 
  void CalcGenomeInfo() {
    // - inst entropy
    emp::vector<inst_t> inst_seq;
    program_t & prog = GetProgram();
    for (size_t fID = 0; fID < prog.GetSize(); ++fID) {
      for (size_t i = 0; i < prog[fID].GetSize(); ++i) {
        inst_seq.emplace_back(prog[fID][i].id);
      }
    }
    genome_info.inst_entropy = std::max(emp::ShannonEntropy(inst_seq), 0.0);
    
    // - inst count (genome length)
    genome_info.inst_cnt = inst_seq.size();
    
    genome_info.calculated = true; 
  }

  /// Retrieve genome instruction entropy. If not calculated, calculate.
  double GetInstEntropy() {
    if (!genome_info.calculated) CalcGenomeInfo();
    return genome_info.inst_entropy; 
  }

  /// Retrieve genome instruction count. If not calculated, calculate.
  double GetInstCnt() { 
    if (!genome_info.calculated) CalcGenomeInfo();
    return genome_info.inst_cnt; 
  }

  /// Retrive function count of genome. 
  double GetFunctionCnt() { return GetProgram().GetSize(); }



};

#endif

