# Quxlang Language Stability

## No warranty or SLA (unless you pay)

Quxlang and QXC is an open source project provided for free with absolutely no warranty (by default). This document does not include or imply a warranty. This document describes an aspirational goal, not a legally binding contract. Quxlang is current a solo passion project "as time allows" with no funding or guaranteed support.

## Language Versioning

The Quxlang language is intended to remain stable, so that code written in one version will continue to work in future versions without modification. However, the future compatibility is intended for code within a file _declared with the same Quxlang version_. For example, `LANGAUGE QUXLANG EN 0.1;` is a version declaration. Code in that file should continue to work and behave the same on newer Quxlang compiler versions. If code is moved to a file declared with a different version, compatibility cannot be guaranteed. 

### Syntax Versions

However, in most cases, Quxlang code will continue to remain valid across versions with minimal changes. The main changes which are expected to break compatibility between Quxlang versions are new operators. For example, `"++->"` might be parsed as `OPERATOR++` `OPERATOR->`, but if a new operator `++->` is added, then a new Quxlang version might cause it to be parsed as `OPERATOR++->` instead.

### Symbol Merging Limitations

It is expected that code will be cross compatible with all code from the same major version across files. In some cases, it may not be possible to merge symbols accross files declared in different Quxlang versions. For example, if Quxlang 0.3 changes function overload selection rules from 0.2, it may not be allowed to declare a function overload in a version 0.2 file and declare a different overload of the same functum in a version 0.3 file.

### Link-Language Versions

It is possible that some versions may introduce link-language versions. Whereever possible, Quxlang will aim to support new features without a link-language version break. There may be rare cases where certain versions or features cannot be used together in the same binary. If two versions introduce a link-language version break, they are expected to have different major version numbers. For example, Quxlang 2.0 and Quxlang 3.0 might have features which are incompatible in the same binary.

## Scope of Backwards Compatibility

Quxlang backwards compatiblity guarantee does not apply:

* To the "standard library".
* To versions of Quxlang considered experimental, beta, or unreleased git revisions.
* To behavior which in the opinion of the Quxlang developer(s) is a defect in the QXC compiler or Quxlang language specification.
* To any behavior which is left undefined by the language specifications, but which the current compiler implementation happens to guarantee based upon its implementation.

## Support periods for language versions

Support for Quxlang major versions is intended to be maintained in QXC for at least 15 years.
Minor stable Quxlang versions are expected to have 5 year support, extended to 15 years for designated LTS releases.

## Support periods for compiler versions

Standard stable qxc releases have 3 year support, designated LTS releases can extend that to 15 years.


## Limitations of long=term support and fixes