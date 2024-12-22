# Resolver Conversion

This document contains instructions to refactor a resolver to use the new resolver API.

## Files to Refactor

### Old Header Example

The previous resolver format would generate a class that inherits from resolver base.

Example old resolver header file:

```cpp
... <include statements>

namespace quxlang
{
    class class_should_autogen_default_constructor_resolver : public virtual rpnx::resolver_base< compiler, bool >
    {
      public:
        using key_type = type_symbol;
        class_should_autogen_default_constructor_resolver(type_symbol cls)
            : m_cls(cls)
        {
        }

        virtual std::string question() const override
        {
            return "class_should_autogen_default_constructor(" + to_string(m_cls) + ")";
        }

        void process(compiler* c) override;

      private:
        type_symbol m_cls;
    };
}
```

### Old Source Example

Here is an example of an old source file that needs refactoring.

```cpp
#include "quxlang/compiler.hpp"
#include "quxlang/res/class_should_autogen_default_constructor_resolver.hpp"


void quxlang::class_should_autogen_default_constructor_resolver::process(compiler* c)
{

    auto callee = submember{m_cls, "CONSTRUCTOR"};
    auto exists_dp = get_dependency(
        [&]
        {
            return c->lk_entity_canonical_chain_exists(callee);
        });
    if (!ready())
    {
        return;
    }
    bool exists = exists_dp->get();

    set_value(!exists);
}
```

We can notice legacy functions like `get_dependency`, `set_value` and `ready` which are no longer used in the new resolver API. We can also notice the use of dependency objects which is abstracted away in the new resolver API.


## Refactoring Process

The first step is to identify the name, input type and output type, as described in the following sections.

### Name

The name of the resolver is the name of the class without the `_resolver` suffix. For example, if the class is named `class_should_autogen_default_constructor_resolver`, the name is `class_should_autogen_default_constructor`.

### Output Type

We can identify the output type by looking at the inherited class, which is
`public virtual rpnx::resolver_base< compiler, OUTPUT_TYPE >` or `public virtual rpnx::co_resolver_base< compiler, OUTPUT_TYPE >`.

For example, if the class inherits from `rpnx::resolver_base< compiler, bool >`, the output type is `bool`.

If the class inherits from `rpnx::resolver_base< compiler, type_symbol >`, the output type is `type_symbol`.

If the class inherits from `rpnx::resolver_base< compiler, std::string >`, the output type is `std::string`.

In general, the class with inherit from `rpnx::resolver_base< compiler, T >` where `T` is the output type.

It may also inherit from `rpnx::co_resolver_base` instead, but the order of parameters is the same.

### Input Type

We can identify the resolver's input type by looking at the class's constructor, which is usually `name_resolver(INPUT_TYPE argname) ... { ... }`. The constructor often sets a member variable for the input value. It will be useful to take note of the member variable names for later refactoring of the source file, as in the refactored version it will always be named `input`.

For example, in `foo_resolver(type_symbol cls) ` the preliminary input type is be `type_symbol`. In `foo_resolver(key_type name)` the preliminary input type is `key_type`. Sometimes the preliminary input type is an alias defined within the class. If it is an alias, we should look at the alias definition if it is in the same file. For example, if there is a `using` or `typedef` statement in the same file, we can extract the type from there.

### The Macro

The `QUX_CO_RESOLVER` macro is used in the header file, and the `QUX_CO_RESOLVER_IMPL_FUNC_DEF` macro is used in the source file.

The `QUX_CO_RESOLVER` macro format in general is `QUX_CO_RESOLVER(<NAME>, <INPUT_TYPE>, <OUTPUT_TYPE>)`. Note however that the `QUX_CO_RESOLVER` macro cannot handle commas in the input or output types. If this occurs, use a `using name_input_type = <INPUT_TYPE>;` or `using name_output_type = <OUTPUT_TYPE>;` statement to define a type alias for the input type before the `QUX_CO_RESOLVER` macro.

`QUX_CO_RESOLVER_IMPL_FUNC_DEF` is used in the source file to define the function. The format is `QUX_CO_RESOLVER_IMPL_FUNC_DEF(<NAME>)`. Note that the `QUX_CO_RESOLVER_IMPL_FUNC_DEF` macro does not take any input or output types as arguments.

### Doing the Refactor 

The first step of the refactoring process is to identify and declare the input and output types. You must tell the user what types they are before proceeding to do the refactor. Note when telling the user what the input and output types are, we should be informative, so `foo_output_type` is not informative. Informative types for example include `std::string`, `int`, `bool`, `std::map<int, string>` etc.. Avoid giving a type alias to the user here. Next, you should then follow by giving a converted header file (if one was provided) and a converted source file (if one was provided), making sure to use `using` statements if the types contain commas as the macro cannot handle commas in the input or output types. A typedef would also work, but `using` is preferred style.

When doing the refactor, we can ignore functions like `question` or `answer` as these are autogenerated by the macro. We should also ignore the `process` function as this is also implemented by the macro.

#### Refactoring the Header File

We replace the entire class with the `QUX_CO_RESOLVER` macro. The macro takes the name of the resolver, the input type, and the output type as arguments. If the input or output type contains commas, we should use a `using` statement to define a type alias for the input type before the macro. The macro will generate the class for us. The macro should be used inside the `quxlang` namespace.

#### Refactoring the Source File

The new version uses coroutines. The input argument is now always named `input`. We might refer to the original header file when doing the refactoring to see how the input type was assigned to member variables in the old version. In the new version, we use input directly.

There are also other important changes, the first
being that instead of fetching a dependency
and then testing `ready()` we use `QUX_CO_DEP(name, (args...))` to fetch the dependency and `co_await` to wait for it.
In the old
version, the `get_dependency` function would adjust the node's dependencies for the ready state automatically.
Thus, it is
no longer necessary to use `get_dependency` as it is now performed by `await_transform` via the coroutine engine. 

Instead of calling `set_value` we use `co_return` to return the value. `co_yield` is never used in a resolver.

We also use `QUX_CO_RESOLVER_IMPL_FUNC_DEF` to define the function in the source file.

Note that when using `QUX_CO_DEP` we omit the `lk_` prefix as this is autogenerated by the macro. We also do not use
`_resolver` suffix in either `QUX_CO_RESOLVER` or `QUX_CO_RESOLVER_IMPL_FUNC_DEF` nor in `QUX_CO_DEP`. `c->` is also not needed for example. See the examples below for clarification.

## Example refactoring

### Header File

Old:

```cpp
... <include statements>

namespace quxlang
{
    class class_should_autogen_default_constructor_resolver : public virtual rpnx::resolver_base< compiler, bool >
    {
      public:
        using key_type = type_symbol;
        class_should_autogen_default_constructor_resolver(type_symbol cls)
            : m_cls(cls)
        {
        }

        virtual std::string question() const override
        {
            return "class_should_autogen_default_constructor(" + to_string(m_cls) + ")";
        }

        void process(compiler* c) override;

      private:
        type_symbol m_cls;
    };
}
```

New:
```cpp

#include <quxlang/res/resolver.hpp>
#include <quxlang/ast2/ast2_entity.hpp>

namespace quxlang
{
    QUX_CO_RESOLVER(class_should_autogen_default_constructor, type_symbol, bool);
}

```

## New Source File
Old:

```cpp
#include "quxlang/compiler.hpp"
#include "quxlang/res/class_should_autogen_default_constructor_resolver.hpp"


void quxlang::class_should_autogen_default_constructor_resolver::process(compiler* c)
{

    auto callee = submember{m_cls, "CONSTRUCTOR"};
    auto exists_dp = get_dependency(
        [&]
        {
            return c->lk_entity_canonical_chain_exists(callee);
        });
    if (!ready())
    {
        return;
    }
    bool exists = exists_dp->get();

    set_value(!exists);
}
```

New:

```cpp
#include "quxlang/compiler.hpp"
#include "quxlang/res/class_should_autogen_default_constructor_resolver.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(class_should_autogen_default_constructor)
{
    auto callee = submember{input, "CONSTRUCTOR"};

    bool exists = co_await QUX_CO_DEP(exists, (callee));

    co_return exists;
}
```