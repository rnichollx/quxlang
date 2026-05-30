# Overview

Quxlang is a Mulit-Paradigm Systems Programming Language. In most respects, it
offers a similar niche to C++, with some noteworthy differences.

Guiding Lights:
  
  * _Deterministic Development_. The compiler should produce the same output given the same input, regardless of the environment in which it is run.
  * _Value and data semantics are the core programming model_. Various defaults in the language assume things like strong ordering and canonical representations for datatypes. We recognize that the C++ value model is superior to the reference model adopted by languages like Java and Python.
  * _Provide negative-overhead abstraction_. Abstractions should make the program faster, not slower.
  * _Don't be helpful_. Also known as "Avoid Causing Programs"; it is better to leave a problem unaddressed than provide a bad solution.
  * _Don't break code_. Design to maintain backwards compatibility of code.
  * _No training wheels_. While we want programming to be as easy as practical, sometimes decisions must be made where
    a choice must be made between what is most useful to beginners and what is most useful to experienced programmers. Quxlang is foremost a serious tool for serious development, and the utility of the language must be prioritized over ease of use for new developers.
  * _Reasonable Safety Mechanisms_. Sometimes you need sharp knives and big guns to solve big problems. Where reasonable, we can put safety mechanisms on the guns and knives. Having safety mechanisms doesn't mean we can't use shrap knives and big guns, it means we may sometimes need to use a safety lever or pull the knife out of it's sheath to use it; the language isn't going to protect you against every possible hazard, but we can put safeties up around the most hazardous bits.
