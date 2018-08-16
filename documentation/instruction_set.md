
# SignalGP Instruction Set Details

Here, we provide details about the instruction set used in our experiments. See
our paper for a full description of our simple linear genetic programming representation.

Each instruction in the instruction set has up to three arguments (A, B, and C), 
which can specify any of the following: memory registers, scopes, functions, inputs,
or outputs. What a particular argument position for an instruction specifies depends
on the instruction.

Throughout this document we will use the following abbreviations in our descriptions:

- `ARG-A`, `ARG-B`, `ARG-C` - The first, second, and third argument for an instruction.
- `REG` - register
  - `REG[i]` - indicates accessing register `i`.
  - `REG[ARG-A]` - indicates accessing the register specified by `ARG-A` of an instruction.
- `SCOPE` - scope 
- `FN` - function
- `SK` - stack
- `IN` - input
  - `IN[i]` - indicates input location `i`.
- `OUT` - output
  - `OUT[i]` - indicates output location `i`.

## Standard Instruction Set

Below is a table describing the set of standard SignalGP instructions included in our experiments.

| Instruction | # Arguments | Description |
| :---        | :---:       | :---        |
| `Inc`       | 1           | Increment REG[ARG-A] by 1 |
| `Dec`       | 1           | Decrement REG[ARG-A] by 1 |
| `Not`       | 1           | Logically toggle REG[ARG-A] |
| `SetReg`    | 2           | REG[ARG-A] = the value of ARG-B |
| `Add`       | 3           | REG[ARG-C] = REG[ARG-A] + REG[ARG-B] |
| `Sub`       | 3           | REG[ARG-C] = REG[ARG-A] - REG[ARG-B] |
| `Mult`      | 3           | REG[ARG-C] = REG[ARG-A] * REG[ARG-B] |
| `Div`       | 3           | REG[ARG-C] = REG[ARG-A] / REG[ARG-B] (safe division)|
| `Mod`       | 3           | REG[ARG-C] = REG[ARG-A] % REG[ARG-B] (safe mod) |
| `TestEqu`   | 3           | REG[ARG-C] = 1 if REG[ARG-A] == REG[ARG-B]; else REG[ARG-C] = 0 |
| `TestNEqu`  | 3           | REG[ARG-C] = 1 if REG[ARG-A] != REG[ARG-B]; else REG[ARG-C] = 0 |
| `TestLess`  | 3           | REG[ARG-C] = 1 if REG[ARG-A] < REG[ARG-B]; else REG[ARG-C] = 0 |
| `If`        | 2           | If REG[ARG-A] != 0: go to SCOPE[ARG-B] |
| `While`     | 2           | Repeat the contents of SCOPE[ARG-B] until REG[ARG-A] == 0 |
| `Countdown` | 2           | Repeat the contents of SCOPE[ARG-B], decrement REG[ARG-A] each time until REG[ARG-A] == 0 |
| `Break`     | 1           | Break out of SCOPE[ARG-A] |
| `Scope`     | 1           | Enter SCOPE[ARG-A] |
| `Define`    | 2           | Define FN[ARG-A], storing its contents in SCOPE[ARG-B] |
| `Call`      | 1           | Call FN[ARG-A] (which must have already been defined) |
| `Push`      | 2           | Push REG[ARG-A] onto SK[ARG-B] |
| `Pop`       | 2           | Pop SK[ARG-A] into REG[ARG-B] |
| `Input`     | 2           | REG[ARG-B] = IN[ARG-A] |
| `Output`    | 2           | OUT[ARG-B] = REG[ARG-A] |
| `CopyVal`   | 2           | REG[ARG-B] = REG[ARG-A] |
| `ScopeReg`  | 1           | Backup register ARG-A. When current scope is exited, it will be restored.|
| `Dereference`| 2          | REG[ARG-B] = REG[REG[ARG-A]]|




























