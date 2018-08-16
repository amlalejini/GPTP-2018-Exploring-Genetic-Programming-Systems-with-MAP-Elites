
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
- `IN` - input
  - `IN[i]` - indicates input location `i`.
- `OUT` - output
  - `OUT[i]` - indicates output location `i`.

## Standard Instruction Set

Below is a table describing the set of standard SignalGP instructions included in our experiments.

| Instruction | # Arguments | Description |
| :---        | :---:       | :---        |
| `Inc`       | 1           | Increment `REG[ARG-A]` by 1 |
| `Dec`       | 1           | Decrement `REG[ARG-A]` by 1 |
| `Not`       | 1           | |
| `SetReg`    | 2           | |
| `Add`       | 3           | |
| `Sub`       | 3           | |
| `Mult`      | 3           | |
| `Div`       | 3           | |
| `Mod`       | 3           | |
| `TestEqu`   | 3           | |
| `TestNEqu`  | 3           | |
| `TestLess`  | 3           | |
| `If`        | 2           | |
| `While`     | 2           | |
| `Countdown` | 2           | |
| `Break`     | 1           | |
| `Scope`     | 1           | |
| `Define`    | 2           | |
| `Call`      | 1           | |
| `Push`      | 2           | |
| `Pop`       | 2           | |
| `Input`     | 2           | |
| `Output`    | 2           | |
| `CopyVal`   | 2           | |
| `ScopeReg`  | 1           | |
| `Dereference`| 2          | |




























