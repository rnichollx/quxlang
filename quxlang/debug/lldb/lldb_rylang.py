# Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com
# lldb_quxlang.py
import lldb

def qualified_symbol_reference_summary(value, internal_dict):
    expression = "quxlang::to_string({})".format(value.GetName())
    evaluated = value.EvaluateExpression(expression)
    if evaluated.IsValid():
        return evaluated.GetSummary()
    else:
        # This will print the error to the LLDB console
        print("Error evaluating expression:", expression)
        return "Error"


