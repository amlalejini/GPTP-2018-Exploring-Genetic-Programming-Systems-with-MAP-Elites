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


    emp::vector<std::pair<input_t, output_t> >& GetTestcases() {
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
            for (size_t i = 0; i < split_line.size() - int(contains_output); i++) {
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

template <typename INPUT_TYPE, typename OUTPUT_TYPE>
class RuleBasedTestcaseSet : public TestcaseSet<INPUT_TYPE, OUTPUT_TYPE> {
private:
    using input_t = typename TestcaseSet<INPUT_TYPE, OUTPUT_TYPE>::input_t;
    using output_t = std::set<OUTPUT_TYPE>;
    using TestcaseSet<INPUT_TYPE, OUTPUT_TYPE>::test_cases;

    emp::vector<std::function<output_t(input_t)> > groups;

    // Pretty sure we can refactor this out
    emp::vector<emp::vector<output_t> > correct_choices;

public:

    emp::vector<emp::vector<output_t> >& GetCorrectChoices() {
        return correct_choices;
    }

    void LoadTestcases(std::string filename) {
        TestcaseSet<INPUT_TYPE, OUTPUT_TYPE>::LoadTestcases(filename, false);

        for (auto test_case : test_cases) {
            for (int i = 0; i < groups.size(); i++) {
                correct_choices[i].push_back(groups[i](test_case[0]));
            }
        }
    }

    void AddGroup(std::function<output_t(input_t)> func) {
        groups.push_back(func);
        correct_choices.push_back(emp::vector<output_t>());
        for (auto test_case : test_cases) {
            correct_choices[correct_choices.size() - 1].push_back(func(test_case.first));
        }
    }

    int GetNFuncs() {
        return groups.size();
    }

    emp::vector<int> GetBestPossible(emp::vector<size_t> choices) {
        emp::vector<int> count;
        for (int i = 0; i < groups.size(); i++) {
            count.push_back(0);
            for (size_t choice : choices) {
                if (correct_choices[i][choice].size()){
                    count[i]++;
                }
            }
        }
        return count;
    }

};

#endif
