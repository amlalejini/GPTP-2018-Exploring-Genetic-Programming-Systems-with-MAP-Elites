#ifndef MAPEGP_PHENCACHE_H
#define MAPEGP_PHENCACHE_H

  /// Utility class used to cache phenotypes during population evaluation.
  template <typename PHENOTYPE_INFO>
  class PhenotypeCache {
    public:
      using phenotype_t = PHENOTYPE_INFO;
    protected:
      size_t org_cnt;   ///< How many organisms do we need to track? 
      size_t eval_cnt;  ///< How many evaluations per organism are we tracking? 

      emp::vector<phenotype_t> phen_cache;
      
    public:
      PhenotypeCache(size_t _org_cnt=0, size_t _eval_cnt=0) 
        : org_cnt(_org_cnt), eval_cnt(_eval_cnt),
          phen_cache(org_cnt*eval_cnt)
      { ; }

      /// Resize phenotype cache. 
      void Resize(size_t _org_cnt, size_t _eval_cnt=1) {
        org_cnt = _org_cnt;
        eval_cnt = _eval_cnt;
        phen_cache.clear();
        phen_cache.resize(org_cnt * eval_cnt);
      }

      /// Access a phenotype from the cache
      phenotype_t & Get(size_t org_id, size_t eval_id) {
        emp_assert(org_id < org_cnt);
        emp_assert(eval_id < eval_cnt); 
        return phen_cache[(org_id * eval_cnt) + eval_id];
      }
    
  };

#endif