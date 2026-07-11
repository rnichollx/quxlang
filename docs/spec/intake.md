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

## Serialization

The SERIALIZE function expects two objects which compare equal (`OPERATOR==`) produce the same output when serialized 
via `.SERIALIZE` through the default interface.

If there are multiple *equivalent* representations of the same object, the `SERIALIZE` function **must** always produce
the same canonical representation.

This restriction only applies to the invocations of `.SERIALIZE` using keyword arguments, such as `@OUTPUT_ITER`
 or `@INPUT`. The restriction does not apply to any overload of `SERIALIZE` that requires a non-keyword argument.

This restriction applies to the invoked callable based on the invotype, not the paratype. In particular if an overload 
paratype has non-keyword arguments, but those paramters have default values, then this restriction will still apply if
the given paratype can be invoked using only keyword arguments, but only for those invocations that are possible using
only keyword arguments.

For example:

```quxlang
::foo_hash_table STRUCT {
  <...>
  
  .SERIALIZE FUNCTION(@OUTPUT_ITER AUTO, @fast DEFAULT(FALSE))
  {
    IF (fast!!)
    {
       VAR sorted_keys AUTO := sort(THIS.iters());
       FOR IN(sorted_keys) <...> { write(OUTPUT_ITER, <...>); }
    }
    ELSE
    {
       FOR IN(THIS.iters()) <...> { write(OUTPUT_ITER, <...>); }
    }
    <...>
  }

}
```

This construct is allowed, because the `@fast` argument defaults to `FALSE` therefore the overload will behave as 
expected when invoked using only keyword-arguments.

If two objects which compare equal but do not produce the same serialized output are both used to instantiate templates,
then the program is _ill-formed_.

If `SERIALIZE` with keyword arguments only, `OPERATOR==`, or `OPERATOR!=` is called on an object which violates this 
rule, the program behavior is undefined.

## Ill-formed

An ill-formed program is a program which is nominally invalid. An ill-formed program may fail to compile or, given a
particular compiler version, such programs, if they compile, exhibit undefined behavior. Backwards compatibility 
guarantees do not apply to ill-formed programs. In particular, new diagnostics may be introduced in new compiler 
versions which cause ill-formed programs to fail to compile. Ill formed programs are not required to fail to compile.
Ordinarily, minor and patch versions are guaranteed not to break compatibility with old code. This guarantee does not
apply to programs which are ill-formed.

A program which triggers undefined behavior during constexpr evaluation is ill-formed, unless a diagnostic is required.

