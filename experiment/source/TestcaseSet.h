/**
*   @note The original version of this class was written by Emily Dolson and can be found here: https://github.com/emilydolson/map-elites-gp/blob/master/source/TestcaseSet.h
*/

#ifndef TEST_CASE_SET_H
#define TEST_CASE_SET_H

#include <iostream>
#include <fstream>
#include <set>
#include <algorithm>

#include "base/array.h"
#include "base/vector.h"
#include "tools/string_utils.h"
#include "tools/Random.h"
#include "tools/random_utils.h"

template <typename INPUT_TYPE, typename OUTPUT_TYPE>
class TestcaseSet {
protected:
    using input_t = emp::vector<INPUT_TYPE>;
    using output_t = OUTPUT_TYPE;
    using test_case_t = std::pair<input_t, output_t>;
    emp::vector<test_case_t> test_cases;

public:
    TestcaseSet(std::string filename) {
        LoadTestcases(filename);
    }

    TestcaseSet() {}

    input_t & GetInput(size_t id) { 
        emp_assert(id < test_cases.size());
        return test_cases[id].first; 
    }

    output_t & GetOutput(size_t id) {
        emp_assert(id < test_cases.size());
        return test_cases[id].second;
    }

    emp::vector<std::pair<input_t, output_t> > & GetTestcases() {
        return test_cases;
    }

    emp::vector<size_t> GetSubset(int trials, emp::Random * random) {
        return emp::Choose(*random, test_cases.size(), trials);
    }

    std::pair<input_t, output_t> operator[](int i) {
        return test_cases[i];
    }

    void LoadTestcases(std::string filename, bool contains_output = true) {
        std::ifstream infile(filename);
        std::string line;

        if (!infile.is_open()){
            std::cout << "ERROR: " << filename << " did not open correctly" << std::endl;
            return;
        }

        // Ignore header
        getline(infile, line);

        while ( getline (infile,line)) {
            emp::vector<std::string> split_line = emp::slice(line, ',');
            input_t test_case;
            for (size_t i = 0; i < (split_line.size() - (size_t)contains_output); i++) {
                test_case.push_back(std::atoi(split_line[i].c_str()));
            }
            output_t answer;
            if (contains_output) {
                answer = std::atoi(split_line[split_line.size()-1].c_str());
            }
            test_cases.push_back(std::make_pair(test_case, answer));
            // std::cout << emp::to_string(test_case) << " " << answer << std::endl;
        }
        infile.close();
    }

};

#endif
