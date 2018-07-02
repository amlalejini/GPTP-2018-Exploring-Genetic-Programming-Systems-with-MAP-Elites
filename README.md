# Exploring Genetic Programming Systems with Map-Elites

## Treatments
- Different problems:
  - Testcase problems
    - Fib. 
    - Squares
    - Sum
    - Kolatz
    - Sym. Regression
    - Smallest
  - Logic '10'
  - Changing Environment
- Selection schemes
  - Tournament
  - MAP-Elites
    - Axes: instruction entropy, module (scope/function) use
  - Random select
  - Lexicase
    - Test cases
        - Testcase problems
          - Fib. 
          - Squares
          - Sum
          - Kolatz
          - Sym. Regression
        - Logic '10'
          - Each logic function
        - Changing Environment
          - Break environment into components

## Points
- MAP-Elites illuminates representation space more than tournament (lexicase?) during evolution
- What does random drift look like? 
- Case study
  - Top-left, top-right, bottom-left, bottom-right solutions in figure => look, they're different!

## Stats
- Spatial stats
  - Are these surfaces (aggregated maps) different? 

## Parameters
### Shared
- Population size = 1000
- Generations = 100k
- Population initialization: Random population initialization
- Clear the cache every generation
- Evaluation time
  - Problem:
    - Fib. = 512
    - Squares = 128
    - Sum = 512
    - Smallest = 512
    - Colatz = 512
    - Sym. Regression = 512
    - Logic '10' = 128
    - Changing Environment = 256
- Tournament size = 2
- MAPE - Inst. entropy bin count = 20
- Number of test cases:
  - Squares = 10
  - Everything else = 200
- Program constraints
  - Max total length 1024
- Mutation rate
  - Arg sub - 0.005
  - Inst sub - 0.005
  - Inst ins - 0.005
  - Inst del - 0.005
- Instruction set
  - Dereference instruction
  - Testcases
    - SubResult
  - Logic
    - Nand
    - Submit

### SignalGP
- Program constraints
  - 32 functions
- Mutations
  - No slip
  - Func dup/del = 0.05

### ScopeGP
- Program constraints
  - (?)17 scopes

## Data tracking
- Bin location
  - inst entropy
  - module used
- Phenotype/genotype values
  - Instruction entropy
  - Module Used
- Fitness

### Figures
- For each problem & representation:
  - Drift Map
  - Tournament Map
  - MAPE Map

## Stretch/future
- Run MAP-Elites over tournament/lexicase to keep track of where those algorithms have explored over all of evolutionary time. 

## Action items
- Look up original fitness function for 'smallest' benchmark
  - Maybe change our fitness function?
- Merge code base
- SIGNALGP
  - Implement lexicase
- ScopeGP
  - Implement lexicase