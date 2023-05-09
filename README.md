# Meld

A project for exploring how to meet DUNE's framework needs.

## Motivation

Existing data-processing frameworks for HEP experiments are largely based on
collider-physics concepts, which may be based on rigid, event-based data hierarchies.
These data organizations are not always helpful for neutrino experiments, which must
sometimes work around such restrictions by manually splitting apart events into constructs
that are better suited for neutrino physics.

The purpose of Meld is to explore more flexible data organizations by treating a
frameworks job as:

1. A graph of data-product sequences connected by...
2. User-defined functions that serve as operations to...
3. Framework-provided higher-order functions.

Each of these aspects is discussed below:

- [Data-centric graph processing](https://github.com/knoepfel/meld/wiki/Data-centric-graph-processing)
- [Higher-order functions](https://github.com/knoepfel/meld/wiki/Higher-order-functions)
