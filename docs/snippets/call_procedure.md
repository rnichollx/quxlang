# Call procedure:


Expressions at the callsite are evaluated left-to-right. The first evaluation is the bound callable, followed by arguments, if any.

e.g. in `(a + b)(c, d + e)`, `a + b` (the callee) is evaluated first, then `c` and finally `d + e`.

Once this is complete, the bound callee is determined and functanoid selection begins.



If it differs, the conversion will construct argument objects in the same order as they are specified by the callee's signature (reverse of destructor order), NOT the order in which they appear in argument expressions at the callsite. Default expressions are evaluated during this phase in the order they occur in the callee's signature.

The conversion phase begins, with an object storage for each argument which is not a reference created in call-site order (including objects which are the result of default-expressions). After the storage is created, the move/copy constructor or default expression is called. Relocations of PRvalues are also allowed in lieu of the move constructor if it is supported by the compiler. If the constructor or default expression call returns without exception, a call to the destructor of the argument object is deferred and the deferral index added to the call deferral list.

A constructor must return VOID.

For functanoids which return a value, a temporary storage is created for that return type, a pointer to which is passed as the "RETURN" named-argument as an nvalue reference. 