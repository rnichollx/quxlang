# Quxlang Modules

Quxlang supports multiple modules. Modules are like libraries. There are several related concepts that should be understood together:

* Source Module
* Logical Module
* Target Mappings
* External Module Rename
* Internal Module Rename

## Source Module

A source module is a named folder containing the content of the module. It lives in the build source tree under the `modules` directory. The name of the folder is the name of the source module.

## Logical Module & Target Mappings

When building a target, logical modules must be mapped to source modules. Example mappings:

```
  boost: boost_1_75_0
  foolib: foolib
  barlib: barlib2.7
```

These represent the canonical module name within a target. The left side is the logical module name, and the right side is the source module name. The source module name is the name of the folder in the `modules` directory.

Different targets can have different logical module to source module mappings. This allows different targets to use different versions of the same module, or to use different modules with the same logical name. E.g. one could configure "foolib" -> "foolib_1.63" target for Linux and "foolib" -> "foolib_1.69_win32alpha" for Windows.


## External Module Rename

An external module rename is a declaration that a import name should be used to import a particular logical module. This is used to resolve conflicts between logical module names. External Module Renames are part of the Target Build Configuration, not the module source code.


When configuring a target, it might be the case the foolib and barlib both depend on bazlib, but they depend on incompatible versions of bazlib. Or perhaps they both depend on a library named `math`, but by two different vendors with compeltely different interfaces. In this case, we don't want "IMPORT math;" in foodlib and barlib to import the same module. We want to import different modules. This is where external module renames come in.

External module renames allow changing which logical module is actually imported by a given import statement without modifying the module source code. It is therefore part of the target configuration, not part of the module source code. 

## Internal Module Rename

An internal module rename allows importing a module using a different (usually shorthand) name. This is part of the module source code, not the target configuration.

Example:

```
IMPORT foo_filesystem_utilities AS fs;
```

This allows importing the `foo_filesystem_utilities` logical module using the name `fs` instead of `foo_filesystem_utilities`. This is useful for shortening long module names, or for renaming a module to a name that is more descriptive of its purpose.