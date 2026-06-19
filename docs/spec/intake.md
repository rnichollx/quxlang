# Intake

## Routines

A routine represents the lowest level compiled unit that exists in the _abstract machine_ for a given target. A particular _target_ cannot have multiple different versions of the same routine. 

Routines _do not have addresses_, a _procedure_ is a compiled and addressable
unit of code which corresponds to a routine. There can be multiple procedures associated with the same routine.

## Linkspace

A linkspace represents a collection of compiled procedures in a particular lowering environment.

There are several important linkspaces:

* The _constexpr linkspace_: This describes the environment used during constexpr execution, which mostly is intended to match the abstract machine.
* The _baseline linkspace_: Used by the runtime to initialize the program before control is passed to the _main linkspace_.
* The _main linkspace_: The linkspace in which the program is usually executed.

