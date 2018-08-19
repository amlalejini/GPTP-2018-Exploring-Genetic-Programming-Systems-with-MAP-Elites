
[![DOI](https://zenodo.org/badge/136976599.svg)](https://zenodo.org/badge/latestdoi/136976599)

## Navigation

- [Overview](#overview)
  - [Repository guide](#repository-guide)
  - [Authors](#contribution-authors)
  - [Publication abstract](#abstract)
- [Data and Analyses](#data-and-analyses)

## Overview

This repository is associated with our 2018 Genetic Programming Theory and Practice (GPTP) workshop contribution, citation pending.

### Repository guide

- [analysis/](https://github.com/amlalejini/GPTP-2018-Exploring-Genetic-Programming-Systems-with-MAP-Elites/tree/master/analysis)
  - This directory contains our fully detailed statistical analyses as well as the
    code necessary to reproduce publication figures.
- [data/](https://github.com/amlalejini/GPTP-2018-Exploring-Genetic-Programming-Systems-with-MAP-Elites/tree/master/data)
  - This directory contains the experiment data used by our analyses.
- [experiment/](https://github.com/amlalejini/GPTP-2018-Exploring-Genetic-Programming-Systems-with-MAP-Elites/tree/master/experiment)
  - This directory contains the source code (C++) for all experiments as well as
    a few utility scripts (Python).

### Dependencies

This code depends on [Empirical](https://github.com/devosoft/Empirical). These specific experiments were run using the version in [this commit](https://github.com/amlalejini/Empirical/tree/38d6ab1a0f70d588385dc81f7d1885ee3bc87a97).

### Contribution Authors

- [Emily Dolson](http://emilyldolson.com)
- [Alexander Lalejini](http://lalejini.com)
- [Charles Ofria](http://ofria.com)

### Abstract

> MAP-Elites is an evolutionary computation technique that has proven valuable for exploring and illuminating the genotype-phenotype space of a computational problem.  In MAP-Elites, a population is structured based on phenotypic traits of prospective solutions; each cell represents a distinct combination of traits and maintains only the most fit organism found with those traits.  The resulting map of trait combinations to fitness allows the user to develop a better understanding of how each trait relates to fitness and how they interact with each other.  While MAP-Elites has not been demonstrated to be competitive for identifying the optimal Pareto front, the insights it provide do allow the users to better understand the underlying problem.  Such insights extend into the underlying structure of the problem representations, such as the value of connection cost or modularity to evolving neural networks.  Here, we extend the use of MAP-Elites to examine genetic programming representations, using aspects of program architecture as traits to explore.  We further discuss how this approach can promote more complex and efficient solutions.

## Computational Substrate

In this work, we evolved simple linear genetic programs.
See the publication for a fully detailed description of the linear GP representation
used. 

Details about the instruction set used in our experiments can be found [here](http://lalejini.com/GPTP-2018-Exploring-Genetic-Programming-Systems-with-MAP-Elites/documentation/instruction_set).

## Data and Analyses

The data used in our analyses can be found in this repository: [./data/](https://github.com/amlalejini/GPTP-2018-Exploring-Genetic-Programming-Systems-with-MAP-Elites/tree/master/data)

[The fully detailed data analyses can be found **here**.](http://lalejini.com/GPTP-2018-Exploring-Genetic-Programming-Systems-with-MAP-Elites/analysis/stats.html)


