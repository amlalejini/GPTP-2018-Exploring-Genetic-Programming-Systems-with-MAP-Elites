#ifndef TASK_SET_H
#define TASK_SET_H

#include <iostream>
#include <string>
#include <algorithm>
#include <functional>
#include <map>
#include <unordered_set>

#include "base/Ptr.h"
#include "base/vector.h"
#include "control/Signal.h"
#include "tools/BitVector.h"
#include "tools/Random.h"
#include "tools/random_utils.h"
#include "tools/math.h"
#include "tools/string_utils.h"
#include "tools/map_utils.h"


/// Task library for logic9 changing environment experiments.
///  - A library of tasks with common input/output types.
template<typename INPUT_T, typename OUTPUT_T>
class TaskSet {
public:
  struct Task;  // Forward declare task struct.

  using task_input_t = INPUT_T;
  using task_output_t = OUTPUT_T;
  using gen_sol_fun_t = std::function<void(Task &, const task_input_t &)>;

  struct Task {
    std::string name;
    size_t id;
    std::string desc;
    emp::vector<task_output_t> solutions;
    emp::vector<size_t> completed_time_stamps;
    emp::vector<size_t> credited_time_stamps;
    size_t wasted_completions; ///< Completions *before* receiving credit.
    gen_sol_fun_t generate_solutions;

    Task(const std::string & _n, size_t _i, gen_sol_fun_t _gen_sols, const std::string & _d)
      : name(_n), id(_i), desc(_d), wasted_completions(0), generate_solutions(_gen_sols)
    { ; }

    size_t GetCompletionCnt() const { return completed_time_stamps.size(); }
    size_t GetCreditedCnt() const { return credited_time_stamps.size(); }
    size_t GetWastedCompletionsCnt() const { return wasted_completions; }
  };

protected:

  emp::vector<Task> task_lib;
  std::map<std::string, size_t> name_map;
  size_t unique_tasks_credited;     ///< How many unique tasks have been credited?
  size_t unique_tasks_completed;    ///< How many unique tasks have been completed?
  size_t total_tasks_credited;      ///< How many total tasks have been credited?
  size_t total_tasks_completed;     ///< How many total tasks have been completed?
  size_t total_tasks_wasted;
  size_t time_all_tasks_credited;
  size_t time_all_tasks_completed;
  bool all_tasks_credited;
  bool all_tasks_completed;
  // task_input_t task_inputs;

  bool sollision;
  std::unordered_set<task_output_t> solution_set; ///< Used for detecting collisions between solutions.

public:
  TaskSet()
    : unique_tasks_credited(0),
      unique_tasks_completed(0),
      total_tasks_credited(0),
      total_tasks_completed(0),
      total_tasks_wasted(0),
      time_all_tasks_credited(0),
      time_all_tasks_completed(0),
      all_tasks_credited(false),
      all_tasks_completed(false),
      sollision(false)
    { ; }
  ~TaskSet() { ; }

  const std::string & GetName(size_t id) const { return task_lib[id].name; }
  const std::string & GetDesc(size_t id) const { return task_lib[id].desc; }
  size_t GetSize() const { return task_lib.size(); }

  size_t GetID(const std::string & name) const {
    emp_assert(emp::Has(name_map, name), name);
    return emp::Find(name_map, name, (size_t)-1);
  }

  Task & GetTask(size_t id) { return task_lib[id]; }

  size_t GetUniqueTasksCredited() const { return unique_tasks_credited; }
  size_t GetUniqueTasksCompleted() const { return unique_tasks_completed; }
  size_t GetTotalTasksCredited() const { return total_tasks_credited; }
  size_t GetTotalTasksCompleted() const { return total_tasks_completed; }
  size_t GetTotalTasksWasted() const { return total_tasks_wasted; }
  size_t GetAllTasksCompletedTime() const { return time_all_tasks_completed; }
  size_t GetAllTasksCreditedTime() const { return time_all_tasks_credited; }
  bool AllTasksCredited() const { return all_tasks_credited; }
  bool AllTasksCompleted() const { return all_tasks_completed; }

  bool IsTask(const std::string name) const { return emp::Has(name_map, name); }

  void AddTask(const std::string & name,
               const gen_sol_fun_t & gen_sols,
               const std::string & desc = "")
  {
    const size_t id = task_lib.size();
    task_lib.emplace_back(name, id, gen_sols, desc);
    name_map[name] = id;
  }

  /// Reset tasks without changing inputs.
  void Reset() {
    unique_tasks_credited = 0;
    unique_tasks_completed = 0;
    total_tasks_credited = 0;
    total_tasks_completed = 0;
    total_tasks_wasted = 0;
    time_all_tasks_credited = 0;
    time_all_tasks_completed = 0;
    all_tasks_credited = false;
    all_tasks_completed = false;
    for (size_t i = 0; i < task_lib.size(); ++i) {
      task_lib[i].completed_time_stamps.resize(0);
      task_lib[i].credited_time_stamps.resize(0);
      task_lib[i].wasted_completions = 0;
    }
  }

  bool IsCollision() const { return sollision; }


  /// Set inputs. Reset everything.
  void SetInputs(const task_input_t & inputs) {
    sollision = false;
    solution_set.clear();
    Reset();
    for (size_t i = 0; i < task_lib.size(); ++i) {
      task_lib[i].solutions.resize(0);
      task_lib[i].generate_solutions(task_lib[i], inputs);
      for (size_t s = 0; s < task_lib[i].solutions.size(); ++s) {
        if (emp::Has(solution_set, task_lib[i].solutions[s])) {
          sollision = true;
        } else {
          solution_set.emplace(task_lib[i].solutions[s]);
        }
      }
    }
  }

  /// Submit possible solution, checking against all tasks.
  /// If submission is indeed a solution, record information about task completion.
  /// Return whether or not submitted solution was a solution.
  bool Submit(const task_output_t & sol, size_t timestamp=0, bool credit=true) {
    bool success = false;
    for (size_t i = 0; i < task_lib.size(); ++i) {
      Task & task = task_lib[i];
      for (size_t s = 0; s < task.solutions.size(); ++s) {
        if (task.solutions[s] == sol) {
          success = true;
          task.completed_time_stamps.emplace_back(timestamp);
          if (task.GetCompletionCnt() == 1) unique_tasks_completed++;
          total_tasks_completed++;
          if (credit) {
            task.credited_time_stamps.emplace_back(timestamp);
            if (task.GetCreditedCnt() == 1) unique_tasks_credited++;
            total_tasks_credited++;
          } else if (!task.GetCreditedCnt()) {
            // If you did it, but didn't get credit, increment wasted completions (total and task)
            task.wasted_completions++;
            total_tasks_wasted++;
          }
        }
      }
    }
    if (!all_tasks_credited && unique_tasks_credited == GetSize()) {
      time_all_tasks_credited = timestamp;
      all_tasks_credited = true;
    }
    if (!all_tasks_completed && unique_tasks_completed == GetSize()) {
      time_all_tasks_completed = timestamp;
      all_tasks_completed = true;
    }
    return success;
  }

};

#endif
