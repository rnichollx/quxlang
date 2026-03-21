

## Quxlang Type System Glossary

### **Argument Type**

The *argument type* is the type associated with a parameter at the point of function resolution.

* For a **non-template parameter**, it is the declared parameter type.
* For a **template parameter**, it is the *instantiated type* determined by matching the pre-argument type against the template’s constraints.

**See also:** *Pre-Argument Type*, *Callable (Functions)*

---

### **Pre-Argument Type**

The *pre-argument type* is the type of the expression evaluated at the call site, prior to any conversions, bindings, or template substitutions.
It reflects the type as produced by normal expression evaluation — for example, `MUT&` for a mutable reference variable or a plain value type for a temporary.

The distinction between **pre-argument type** and **argument type** is fundamental:

* The **pre-argument type** originates from the expression.
* The **argument type** originates from the parameter declaration (or template instantiation).

Conversions, bindings, and template deduction act on the pre-argument type to make it compatible with the argument type.

**See also:** *Bindable (Types)*, *Implicitly Convertible (Types)*

---

### **Bindable (Types)**

A type `From` is **bindable** to a type `To` when the two can be associated without invoking general implicit conversions — typically through reference adjustment or qualification change.
`From` is bindable to `To` if any of the following hold:

1. **Reference-Requalification Binding**
   Both `From` and `To` are references to the same underlying type, and the qualifiers of `From` are qualification-convertible to those of `To`.

2. **Temporary-Materialization Binding**
   `From` is a value type, and `To` is a reference to the same underlying type that is either `TEMP&`-qualified or implicitly qualification-convertible from a `TEMP&` reference.

3. **Reference-Objectization Binding**
   `To` is a value type, and `From` is a reference to the same underlying type. Reference-objectization proceeds through either `TEMP& To` or `CONST& To` as the effective `OTHER` type. `From` is reference-objectization-bindable to `To` if `From` is reference-requalification-bindable to either `TEMP& To` or `CONST& To`, and the constructor of `To` is *invokable* with that effective `OTHER` type.

**See also:** *Implicitly Convertible (Types)*, *Invokable (Functions)*

---

### **Implicitly Convertible (Types)**

A type `From` is **implicitly convertible** to a type `To` if one of the following conditions applies:

1. `From` is **bindable** to `To`.
2. The **constructor** of `To` is *bind-only-callable* with a parameter of type `From` as `OTHER`.
3. `From` defines an `OPERATOR TO` that produces a type bindable to `To`.

Implicit conversions extend bindability to include constructor and operator-based coercions that do not require explicit casts.

**See also:** *Bindable (Types)*

---

### **Invokable (Functions)**

A function is **invokable** with a given invocation type when each argument exactly matches the corresponding parameter type. In an invocation, all pre-argument types are identical to their corresponding argument types, and no argument adaptation, conversion, or binding is performed.

The one exception is for slot parameters of type `NEW&& T` or `DESTROY&& T`. Such a parameter may be invoked with an argument of type `->T`, which supplies the storage location governed by the slot. This exception exists to support placement-style construction and destruction, such as invoking a constructor or destructor with a `THIS` argument naming the target storage.

Invokability corresponds to the exact-argument-passing case of a call and represents the most direct form of function matching.

**See also:** *Callable (Functions)*, *Invocation (Calls)*

---

### **Callable (Functions)**

A function is **callable** when all of its parameters can be satisfied by the arguments supplied at a call site, either exactly or through permitted argument adaptation.
Formally, for each parameter–pre-argument pair:

* If the parameter is **concrete**, the pre-argument type must be:

  * (I) *identical* to the parameter type,
  * (II) *bindable* to the parameter type, or
  * (III) *implicitly convertible* to the parameter type.
* If the parameter is **templated**, the pre-argument type must satisfy the template’s constraints.

All parameters must either have corresponding arguments or default expressions.

**See also:** *Invokable (Functions)*, *Implicitly Convertible (Types)*

---

### **Invocation (Calls)**

An **invocation** is the exact argument-passing step in which the pre-argument types already match the parameter types, so the arguments are passed as-is and no argument adaptation is performed.

The one permitted type mismatch during invocation is that an argument of type `->T` may be passed directly to a parameter of type `NEW&& T` or `DESTROY&& T`. In that case, the pointer identifies the storage governed by the slot rather than undergoing a conversion. Aside from that slot-pointer exception, invocation requires exact type matching.

An invocation is a special case of a call.

**See also:** *Binding (Calls)*, *Conversion (Calls)*

---

### **Call (Calls)**

A **call** is the full procedure of evaluating pre-arguments, selecting a function, and producing the arguments expected by the selected parameter types. A call may therefore perform argument adaptation, including bindings and implicit conversions, before the selected function is invoked.

Every invocation occurs within a call, but not every call is an invocation.

**See also:** *Invocation (Calls)*, *Callable (Functions)*

---

### **Binding (Calls)**

A **binding** is a call where each argument can be adapted to its parameter by reference or qualification conversion only, and no implicit nonbinding conversions occur.
Bindings include cases like binding `MUT&` to `CONST&` or materializing a temporary for a `TEMP&` reference.

**See also:** *Invocation (Calls)*, *Conversion (Calls)*

---

### **Conversion (Calls)**

A **conversion call** is a call where one or more arguments require implicit nonbinding conversion, such as constructor-based or operator-based conversion, to match the parameter list.
It represents the broadest class of overload matches, occurring after invocations and bindings are considered.

**See also:** *Invocation (Calls)*, *Binding (Calls)*
