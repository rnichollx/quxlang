import lldb

def TypeSymbolSummary(valobj, internal_dict):
    print("lldb summary for quxlang::type_symbol")
    if not valobj.IsValid():
        return "<invalid>"

    # Determine the address of the object
    addr = valobj.GetLoadAddress()
    if addr == lldb.LLDB_INVALID_ADDRESS:
        # If it's a synthetic value or scalar, try to get value as unsigned
        addr = valobj.GetValueAsUnsigned(lldb.LLDB_INVALID_ADDRESS)
        if addr == lldb.LLDB_INVALID_ADDRESS:
            return "<unable to get address>"

    # Construct the expression to evaluate
    expr = f'quxlang::to_string(*reinterpret_cast<quxlang::type_symbol const *>({addr}))'

    # Evaluate the expression
    target = valobj.GetTarget()
    if not target.IsValid():
        return "<no target>"

    options = lldb.SBExpressionOptions()
    options.SetLanguage(lldb.eLanguageTypeC_plus_plus)
    options.SetIgnoreBreakpoints(True)
    options.SetTrapExceptions(False)
    options.SetUnwindOnError(True)
    options.SetTimeoutInMicroSeconds(5000000)  # 5 seconds timeout

    result = target.EvaluateExpression(expr, options)
    if not result.IsValid() or result.GetError().Success() == False:
        error_desc = result.GetError().GetCString() if result.GetError() else "unknown error"
        return f"<evaluation error: {error_desc}>"

    # Assuming the result is std::string, get its summary (which is the string content)
    summary = result.GetSummary()
    if summary:
        # Strip quotes if present
        if summary.startswith('"') and summary.endswith('"'):
            summary = summary[1:-1]
        return summary
    else:
        # Fallback: try to get c_str() if needed
        c_str_expr = f'quxlang::to_string(*reinterpret_cast<quxlang::type_symbol const *>({addr})).c_str()'
        c_str_result = target.EvaluateExpression(c_str_expr, options)
        if c_str_result.IsValid() and c_str_result.GetError().Success():
            return c_str_result.GetValue() or "<empty>"
        else:
            return "<unable to get string>"

def __lldb_init_module(debugger, internal_dict):
    # Get the default category
    print("Initializing quxlang lldb module")
    category = debugger.GetCategory("default")
    if not category.IsValid():
        print("Warning: Could not get default category")
        return

    # Find the type by name
    target = debugger.GetSelectedTarget()
    if not target.IsValid():
        print("Warning: No selected target")
        return

    type_name = "quxlang::type_symbol"
    ts_type = target.FindFirstType(type_name)
    if not ts_type.IsValid():
        print(f"Warning: Could not find type '{type_name}'")
        return

    # Get the canonical (de-aliased) type
    canonical_type = ts_type.GetCanonicalType()
    canonical_name = canonical_type.GetName()


    # Add to the canonical type if different
    if canonical_name and canonical_name != type_name:
        debugger.HandleCommand(f'type summary add -F quxlang.TypeSymbolSummary "{type_name}"')
        debugger.HandleCommand(f'type summary add -F quxlang.TypeSymbolSummary "{canonical_name}"')
        print(f"Added summary provider for '{type_name}' (canonical: {canonical_name})")
    else:
        debugger.HandleCommand(f'type summary add -F __lldb_summary_type_symbol "{type_name}"')
        print(f"Added summary provider for '{type_name}'")