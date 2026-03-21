Call = Conversion, Binding, Invocation

A call is the full procedure that evaluates prearguments, selects a function, and performs any argument adaptations needed
to produce the selected argument types. An invocation is the exact-passing case of that procedure, where the arguments
are already of the required types and are passed as-is. The one exception is that `->T` may be passed directly to a
`NEW&& T` or `DESTROY&& T` parameter during invocation in order to supply placement-style storage such as a constructor
or destructor `THIS` slot.

For a function ensig(overload) to be callable with a given invotype, each preargument type MUST ((have a corresponding
 parameter in the ensig), AND either (the parameter is a concrete argument type and the pre-argument type must be either(:
     I. Implicitly convertible to the corresponding parameter type, OR
     II. Bindable to the corresponding parameter type, OR
     III. Identical to the corresponding parameter type.,
   ) OR (then the parameter is a template type and the pre-argument type MUST match the template constraints)))
AND ( each parameter in the ensig MUST ((have a corresponding preargument type in the invotype) OR (have a default expression))).

A type is bindable to another type if:
   1. Both types are references to the same underlying type, and the qualifiers of the From reference type are
      qualification-convertible to the qualifiers of the To reference type. (A "reference-requalification binding"), or
   2. The From type is a Value type, and the To type is a reference type to the same underlying type which is either
      TEMP& qualified or a reference type which is implicitly qualification-convertible from a TEMP& reference to the underlying
      type. (A "temporary materialization binding"), or
   3. The To type is a value type, and the From type is a reference to the same underlying type. The reference
      objectization binding proceeds through either `TEMP& To` or `CONST& To` as the effective OTHER type. The From
      type must be reference-requalification-bindable to one of those two effective OTHER types, and the constructor
      of the underlying must be invokable with that effective OTHER type as the OTHER argument. (A "reference
      objectization binding").

A type From is implicitly convertible to another type To if:
  1. The From type is bindable to the To type, or
  2. The CONSTRUCTOR of the type To is bind-only-callable with an argument of type From as OTHER., or
  3. The type From has an OPERATOR TO which produces a type that is bindable to the type To.

A function ensig(overload) is invokable with a given invotype if all preargument types are identical to the
corresponding parameter types which must exist, and all parameter types have corresponding preargument types, except
that `->T` is also permitted for a parameter of type `NEW&& T` or `DESTROY&& T`.
