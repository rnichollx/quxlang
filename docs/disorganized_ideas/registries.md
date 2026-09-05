# Quxlang REGISTRY

!!!! warn
    This feature is not yet implemented.

Quxlang has a concept of a _REGISTRY_.

A registry, essentially, allows the program to pre-register objects and potentially enumerate all registered objects at runtime.

Suppose for example that we want to use dynamic typing and register a "deserialization" handler for each type.

e.g.

```quxlang

::debug_print TEMPLATE(@T CLASS) FUNCTION(@ARG CONST->VOID)
{
  ...
}
```

We would like to be able to look up a given type and detect whether or not it has a "debug_print" handler registered.

In C++ this might be acomplished via using a `std::map<std::type_index, debug_print_function>` plus a registration function:

```C++
std::shared_mutex global_debug_map_mutex;
std::map<std::type_index, debug_print_function> global_debug_map;

template <typename T>
int register_type_debug_printer()
{
  static int side_effect = []() { 
    std::unique_lock lck{global_debug_map_mutex};
    global_debug_map[typeid(T).index()] = debug_print_function<T>();
    return 0; 
  }();
  return side_effect;
}
```

This would allow later code to locate the registered debug printer for a given type.

One weakness of this is that it involves lazy initialization. A similar approach can be implemented for global initilization using constructor objects:

```C++
// Global for initalization side-effects
to_debug_string_init<mytype> mytype_debug_init;
```

This would allow later code to locate the registered debug printer for a given type.

However, this approach has a few glaring weaknesses. First, Quxlang uses global reachability analysis, meaning a global object which is never accessed is not compiled into the program and therefore cannot be relied upon for side-effects. Second, this introduces runtime intialization which must be done at program start, despite the fact that this type of operation could potentially be known at compile time.

Quxlang _registries_ allow us to move these design patterns into the compiler.

We can declare a registry as follows:

```quxlang
::my_debug_print_registry REGISTRY[TYPE_INDEX:PROCEDURE(@ARG CONST->VOID; RETURN std::string)];
```

And we can _register_ values with the registry using a REGISTER statement:

```quxlang
::mytype STRUCT
{
   REGISTER my_debug_print_registry[TYPE_INDEX_OF(mytype)] := debug_print;
   
   ::debug_print FUNCTION(@ARG CONST->VOID): std::string
   {
     RETURN ".x = " + to_string(.x) + ", .y = " + to_string(.y);
   }
   
   .x VAR I32;
   .y VAR I32;
   
   .CONSTRUCTOR FUNCTION()
   {
   }
}
```

`REGISTER` statements at evaluated at compile time during _reachability analysis_. If an object of a structure type is instantiated anywhere in the program, the compiler will execute any associated registrations at compile time.

A consequence of this is that retrieval of registered values can be optimized at compile time with a known set.

The compiler might choose between inline comparisons (small number of entries) or lookup tables (large number of entries) to optimize retrieval of registered values depending on how many implementors there are.

If there are two or more REGISTER statements that register the same index value with different projected values, this will produce a _lowering-error_.

If a procedure is registered into a REGISTRY, the compiler may produce a multiversioned registry using program steppings. This may cause lookup of registry stored procedure pointers to produce a different result between _early-init_ and _post-detect_ or ACTIVE_MAIN phases of execution.