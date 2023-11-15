# lldb_rylang.py
import lldb

def qualified_symbol_reference_summary(value, internal_dict):
    expression = "rylang::to_string({})".format(value.GetName())
    evaluated = value.EvaluateExpression(expression)
    if evaluated.IsValid():
        return evaluated.GetSummary()
    else:
        # This will print the error to the LLDB console
        print("Error evaluating expression:", expression)
        return "Error"


