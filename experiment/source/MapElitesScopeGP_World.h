#ifndef __MAP_ELITES_GP_H__
#define __MAP_ELITES_GP_H__

#include <set>
#include <sys/stat.h>

#include "Evolve/World.h"
#include "hardware/AvidaGP.h"
#include "config/ArgManager.h"
#include "base/vector.h"
#include "tools/Random.h"
#include "tools/stats.h"
#include "Evolve/World_select.h"

#include "TestcaseSet.h"

EMP_BUILD_CONFIG( MEGPConfig,
  GROUP(DEFAULT, "Default settings for box experiment"),
  VALUE(SEED, int, 55711224, "Random number seed (0 for based on time)"),
  VALUE(TOURNAMENT_SIZE, uint32_t, 2, "Number of organisms in the popoulation."),
  VALUE(POP_SIZE, uint32_t, 100, "Number of organisms in the popoulation."),
  VALUE(UPDATES, uint32_t, 1000, "How many generations should we process?"),
  VALUE(SELECTION, std::string, "MAPELITES", "What selection scheme should we use?"),
  VALUE(INST_MUT_RATE, double, 0.005, "Per-site mutation rate for instructions"),
  VALUE(ARG_MUT_RATE, double, 0.005, "Per-site mutation rate for arguments"),
  VALUE(INS_MUT_RATE, double, 0.005, "Per-site mutation rate for arguments"),
  VALUE(DEL_MUT_RATE, double, 0.005, "Per-site mutation rate for arguments"),
  VALUE(PROBLEM, std::string, "configs/testcases/examples-squares.csv", "Which set of testcases should we use? (or enter 'box' for the box problem"),
  VALUE(N_TEST_CASES, uint32_t, 11, "How many test cases to use"),  
  VALUE(GENOME_SIZE, int, 20, "Length of genome"),
  VALUE(SCOPE_RES, long unsigned int, 16, "Number of bins to make on scope axis"),
  VALUE(ENTROPY_RES, long unsigned int, 25, "Number of bins to make on entropy axis"),
  VALUE(EVAL_TIME, int, 200, "Steps to evaluate for."),
  VALUE(MAX_SIZE, int, 500, "Maximum genome length.")
)

class MapElitesScopeGPWorld : public emp::World<emp::AvidaGP> {

public:


    enum class STRUCTURE { MIXED=0, MAPE=1 };
    enum class PROBLEM_TYPE { CHG_ENV=0, TESTCASES=1, LOGIC=2 };
    enum class SELECTION_METHOD { TOURNAMENT=0, LEXICASE=1, RANDOM=2 };

    size_t EVAL_TIME;
    size_t MAX_SIZE;
    size_t WORLD_STRUCTURE;
    long unsigned int SCOPE_RES;
    long unsigned int ENTROPY_RES;
    double INST_MUT_RATE;
    double ARG_MUT_RATE;
    double INS_MUT_RATE;
    double DEL_MUT_RATE;
    size_t TOURNAMENT_SIZE;
    size_t POP_SIZE;
    size_t GENERATIONS;
    size_t N_TEST_CASES;    
    size_t SELECTION;
    size_t PROBLEM_TYPE;
    std::string TESTCASES_FPATH;

    emp::DataNode<double, emp::data::Range> evolutionary_distinctiveness;

    TestcaseSet<int, double> testcases;

    emp::vector<std::function<double(emp::AvidaGP&)> > fit_set;

    std::function<double(emp::AvidaGP&)> goal_function = [this](emp::AvidaGP & org){
        double score = 0;
        emp::Random rand = GetRandom();
        // for (int testcase : testcases.GetSubset(N_TEST_CASES, &rand)) {
        emp_assert(N_TEST_CASES <= testcases.GetTestcases().size());
        for (size_t testcase = 0; testcase < N_TEST_CASES; ++testcase) {
            org.ResetHardware();
            for (size_t i = 0; i < testcases[testcase].first.size(); i++) {
                org.SetInput((int)i, testcases[testcase].first[i]);
            }
            org.Process(EVAL_TIME);
            double divisor = testcases[testcase].second;
            if (divisor == 0) {
                divisor = 1;
            }
            const std::unordered_map<int, double> & outputs = org.GetOutputs();
            int min_output = 0;
            double result;
            for (auto out : outputs) {
                if (out.first < min_output) {
                    min_output = out.first;
                }
            }

            if (outputs.size() != 0) {
                result = 1 / (std::abs(org.GetOutput(min_output) - testcases[testcase].second)/std::abs(divisor));
            } else {
                result = 0;
            }

            // emp_assert(std::abs(result) != INFINITY);
            if (result > 1000) {
                result = 1000;
            }
            score += result;
        }
        return score;
    };

    std::function<double(size_t id)> goal_function_ptr = [this](size_t id){
        return CalcFitnessID(id);
    };

    MapElitesScopeGPWorld() {;}
    MapElitesScopeGPWorld(emp::Random & rnd) : emp::World<emp::AvidaGP>(rnd) {;}

    std::function<int(emp::AvidaGP &)> scope_count_fun = [this](emp::AvidaGP & val){ 
        std::set<size_t> scopes;
        val.ResetHardware();
        val.SetInput(0, 1);
        scopes.insert(val.CurScope());

        for (int i = 0; i < EVAL_TIME; i++) {
            val.SingleProcess();
            scopes.insert(val.CurScope());
        }
        return scopes.size(); 
    };

   std::function<int(size_t id)> scope_count_fun_ptr = [this](size_t id){ 
        return scope_count_fun(*pop[id]);
    };

    std::function<size_t(emp::AvidaGP::Instruction&)> GetInstID = [](emp::AvidaGP::Instruction & inst){return inst.id;};

    std::function<double(emp::AvidaGP &)> inst_ent_fun = [this](emp::AvidaGP & val){ 
        emp::vector<size_t> inst_list = emp::ApplyFunction(GetInstID, val.GetGenome().sequence);
        return emp::ShannonEntropy(inst_list); 
    };

    std::function<double(size_t id)> inst_ent_fun_ptr = [this](size_t id){ 
        return inst_ent_fun(*pop[id]);
    };

    std::function<size_t(size_t id)> inst_ent_bin = [this](size_t id) {
      return GetPhenotypes()[1].EvalBin(*pop[id], ENTROPY_RES);
    };

    std::function<size_t(size_t id)> scope_count_bin = [this](size_t id) {
      return GetPhenotypes()[0].EvalBin(*pop[id], SCOPE_RES);
    };

    std::function<emp::vector<size_t>() > get_pop = [this](){return GetValidOrgIDs();};

    std::function<size_t(size_t)> return_id = [](size_t id){return id;};

    void Setup(MapElitesGPConfig & config) {
        Reset();
        SetCache();
        InitConfigs(config);
        SetMutFun([this](emp::AvidaGP & org, emp::Random & r){
            int count = 0;
            for (size_t i = 0; i < org.GetSize(); ++i) {
                if (r.P(INST_MUT_RATE)) {
                    org.RandomizeInst(i, r);
                    count++;
                }
                for (size_t j = 0; j < emp::AvidaGP::base_t::INST_ARGS; j++) {
                    if (r.P(ARG_MUT_RATE)) {
                        org.genome.sequence[i].args[j] = r.GetUInt(org.CPU_SIZE);        
                        count++;
                    }
                }
                if (r.P(INS_MUT_RATE)) {
                    if (org.GetSize() < MAX_SIZE) {
                        org.genome.sequence.insert(org.genome.sequence.begin() + (int)i, emp::AvidaGP::genome_t::sequence_t::value_type());
                        org.RandomizeInst(i, r);
                        count++;
                    }
                }
                if (r.P(DEL_MUT_RATE)) {
                    if (org.GetSize() > 1) {
                        org.genome.sequence.erase(org.genome.sequence.begin() + (int)i);
                        count++;
                    }
                }

            }
            return count;
        });
        SetPopStruct_Mixed();
        SetAutoMutate();
        
        #ifndef EMSCRIPTEN
        SetupFitnessFile().SetTimingRepeat(10);
        // SetupSystematicsFile().SetTimingRepeat(10);
        SetupPopulationFile().SetTimingRepeat(10);

        emp::Ptr<emp::ContainerDataFile<emp::vector<size_t> > > tfile;
        tfile.New("traits.dat");

        auto & trait_file = static_cast<emp::ContainerDataFile<emp::vector<size_t> >& >(AddDataFile(tfile));
        trait_file.SetUpdateContainerFun(get_pop);
        trait_file.AddContainerFun(inst_ent_fun_ptr, "instruction_entropy", "Entropy of instructions");
        trait_file.AddContainerFun(scope_count_fun_ptr, "scope_count", "Number of scopes used");
        trait_file.AddContainerFun(goal_function_ptr, "fitness", "Fitness");
        trait_file.AddContainerFun(return_id, "id", "ID");
	    trait_file.AddContainerFun(scope_count_bin, "scope_count_bin", "Bin that scope count falls into");
	    trait_file.AddContainerFun(inst_ent_bin, "inst_ent_bin", "Bin that instruction entropy falls into");
        trait_file.AddVar(update, "update", "Update");
        trait_file.SetTimingRepeat(10);
        trait_file.PrintHeaderKeys();

        OnUpdate([this](size_t ud){if (ud % 100 == 0){SnapshotSingleFile(ud);}});
        #endif

        testcases.LoadTestcases(TESTCASES_FPATH);
        SetFitFun(goal_function);
        emp::AvidaGP org;
        AddPhenotype("Num Scopes", scope_count_fun, 1, 17);
        AddPhenotype("Entropy", inst_ent_fun, 0, -1*emp::Log2(1.0/org.GetInstLib()->GetSize())+1);
        if (WORLD_STRUCTURE == (size_t)STRUCTURE::MAPE) {
            emp::SetMapElites(*this, {SCOPE_RES, ENTROPY_RES});
        }

        if (SELECTION == (size_t)SELECTION_METHOD::LEXICASE) {

            for (size_t testcase = 0; testcase < N_TEST_CASES; ++testcase) {
                fit_set.push_back([testcase, this](emp::AvidaGP & org) {
                    org.ResetHardware();
                    for (size_t i = 0; i < testcases[testcase].first.size(); i++) {
                        org.SetInput((int)i, testcases[testcase].first[i]);
                    }
                    org.Process(EVAL_TIME);
                    double divisor = testcases[testcase].second;
                    if (divisor == 0) {
                        divisor = 1;
                    }
                    const std::unordered_map<int, double> & outputs = org.GetOutputs();
                    int min_output = 0;
                    double result;
                    for (auto out : outputs) {
                        if (out.first < min_output) {
                            min_output = out.first;
                        }
                    }

                    if (outputs.size() != 0) {
                        result = 1 / (std::abs(org.GetOutput(min_output) - testcases[testcase].second)/std::abs(divisor));
                    } else {
                        result = 0;
                    }

                    // emp_assert(std::abs(result) != INFINITY);
                    if (result > 1000) {
                        result = 1000;
                    }
                    return result;
                });

            }
        }

        InitPop();
    }

    void SnapshotSingleFile(size_t update) {
        std::string snapshot_dir = "pop_" + emp::to_string((int)update);
        #ifndef EMSCRIPTEN
        mkdir(snapshot_dir.c_str(), ACCESSPERMS);
        #endif        
        // For each program in the population, dump the full program description in a single file.
        std::ofstream prog_ofstream(snapshot_dir + "/pop_" + emp::to_string((int)update) + ".pop");
        for (size_t i : GetValidOrgIDs())
        {
            if (i)
            prog_ofstream << "===\n";
            prog_ofstream << "id: " << i << std::endl;
            pop[i]->PrintGenome(prog_ofstream);
        }
        prog_ofstream.close();
    }

    void InitConfigs(MapElitesGPConfig & config) {
        TOURNAMENT_SIZE = config.TOURNAMENT_SIZE();
        EVAL_TIME = config.EVAL_TIME();
        MAX_SIZE = config.PROG_MAX_TOTAL_LEN();
        SCOPE_RES = config.MAPE_AXIS_SIZE__FUNC_ENTERED_ENTROPY();
        ENTROPY_RES = config.MAPE_AXIS_SIZE__INST_ENTROPY();
        INST_MUT_RATE = config.INST_SUB__PER_INST();
        ARG_MUT_RATE = config.ARG_SUB__PER_ARG();
        INS_MUT_RATE = config.INST_INS__PER_INST();
        DEL_MUT_RATE = config.INST_DEL__PER_INST();
        POP_SIZE = config.POP_SIZE();
        GENERATIONS = config.GENERATIONS();
        N_TEST_CASES = config.NUM_TEST_CASES();    
        SELECTION = config.SELECTION_METHOD();
        PROBLEM_TYPE = config.PROBLEM_TYPE();        
        TESTCASES_FPATH = config.TESTCASES_FPATH();
        WORLD_STRUCTURE = config.WORLD_STRUCTURE();        
    }

    void InitPop() {
        emp::Random & random = GetRandom();
        for (int i = 0 ; i < POP_SIZE; i++) {
            size_t len = random.GetUInt(1, MAX_SIZE);
            emp::AvidaGP cpu;
            for (int j = 0; j < len; j++) {
                cpu.PushRandom(random, 1);
            }
            Inject(cpu.GetGenome());
        }
    }

    void RunStep() {
        evolutionary_distinctiveness.Reset();
        std::cout << update << std::endl;
        if (WORLD_STRUCTURE == (size_t)STRUCTURE::MAPE) {
            if (num_orgs < .5*GetSize()) {
                emp::RandomSelectSparse(*this, POP_SIZE);
            } else {
                emp::RandomSelect(*this, POP_SIZE);                
            }
        } else if (SELECTION == (size_t)SELECTION_METHOD::TOURNAMENT) {
            emp::TournamentSelect(*this, TOURNAMENT_SIZE, POP_SIZE);
        } else if (SELECTION == (size_t)SELECTION_METHOD::RANDOM) {
            emp::RandomSelect(*this, POP_SIZE);
        } else if (SELECTION == (size_t)SELECTION_METHOD::LEXICASE) {
            emp::LexicaseSelect(*this, fit_set, POP_SIZE);
        } else {
            emp_assert(false && "INVALID SELECTION SCEHME", SELECTION);
        }

        Update();
    }

    void Run() {
        for (size_t u = 0; u <= GENERATIONS; u++) {
            RunStep();
        }  
    }

};

#endif
