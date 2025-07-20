# Copyright (c) 2025, Ryan P. Nicholl <rnicholl@protonmail.com> http://rpnx.net/

# This file implements LLDB synthetic provider for use with rpnx::basic_variant.

import lldb
import lldb.formatters.Logger


def prepare_name_for_lookup(name):
   """
   Basically, we need to replace >> with > >, because LLDB
   does not handle the C++ template syntax correctly.
   """
   logger = lldb.formatters.Logger.Logger()
   if not name:
     return name
   # Replace '>>' with '> >' to handle C++ template syntax
   prepared_name = name.replace('>>', '> >')
   logger >> f"Prepared name for lookup: {prepared_name}"
   return prepared_name


def get_matching_type(target, name):
    """
    Get a list of types matching the given name.
    This function uses LLDB's FindTypes API to search for types.
    """
    target = lldb.debugger.GetSelectedTarget()
    type_list = target.FindTypes(name)
    matching_types = [t for t in type_list]
    if len(matching_types) != 1:
        raise ValueError(f"Expected exactly one type matching '{name}', found {len(matching_types)}.")
    return matching_types[0]



def demangle(mangled_name):
    """
    Demangle a C++ mangled name using LLDB's demangling capabilities.
    """
    logger = lldb.formatters.Logger.Logger()
    if not mangled_name:
        return mangled_name
    target = lldb.debugger.GetSelectedTarget()
    expr = f'rpnx::demangle("{mangled_name}")'
    dem_str = target.EvaluateExpression(expr)
    logger >> f"dem str: {dem_str}"
    if not dem_str.IsValid():
        logger >> f"Demangling error: {dem_str.GetError()}"
        logger >> f"Demangling error: {dem_str.GetError().GetCString()}"
        raise ValueError(f"Demangling error: {dem_str.GetError().GetCString()}")

    return dem_str.GetSummary().strip('"')

def lookup_type_from_mangled_name(target, mangled_name):
   return get_matching_type(target, prepare_name_for_lookup(demangle(mangled_name)))


def get_child_member_with_name(valobj, name):
    """
    Get a child member of a value object by name.
    This is a helper function to simplify error handling.
    """
    logger = lldb.formatters.Logger.Logger()
    child = valobj.GetChildMemberWithName(name)
    if not print_if_error(child):
        raise ValueError(f"Failed to get child member '{name}' from {valobj.GetName()}")
    logger >> f"Retrieved child member '{name}' from {valobj.GetName()}: {child.GetValue()}"
    return child

def print_if_error(obj):
    logger = lldb.formatters.Logger.Logger()
    if not obj.IsValid():
        error_msg = obj.GetError().GetCString()
        logger >> f"Error: {error_msg}"
        return False
    return True

class TypeErasedProvider:
    def __init__(self, valobj, internal_dict):
        self.valobj = valobj
        self.update()

    def update(self):
        logger = lldb.formatters.Logger.Logger()
        # Access the type_info pointer
        logger >> f"Updating TypeErasedProvider for {self.valobj.GetName()}"
        m_vinf = get_child_member_with_name(self.valobj, 'm_vinf')
        vtable = m_vinf.Dereference()
        print_if_error(vtable)
        logger >> f"VTable: {vtable}"
        m_general_info = get_child_member_with_name(vtable, 'm_general_info')
        print_if_error(m_general_info)
        if not m_general_info.IsValid():
            raise ValueError("Failed to get m_general_info from vtable")
        type_info_ptr = get_child_member_with_name(m_general_info, 'm_type_info')

        type_info_addr = type_info_ptr.GetValueAsUnsigned()

        target = self.valobj.GetTarget()
        logger >> f"Type info address: {type_info_addr:#x}"

        # Evaluate the type name from std::type_info
        expr = f'((std::type_info const*)({type_info_addr:#x}))->name()'
        name_str = self.valobj.EvaluateExpression(expr)
        if not name_str.IsValid():
            error_msg = name_str.GetError().GetCString()
            logger >> f"Error evaluating type name: {error_msg}"

        mangled_name = name_str.GetSummary().strip('"')
        logger >> f"Mangled type name: {mangled_name}"

        # Demangle if necessary (skip on MSVC where it's already demangled)
        triple = target.GetTriple()
        is_msvc = 'msvc' in triple.lower()
        demangled_name = demangle(mangled_name)
        logger >> f"Demangled type name: {demangled_name}"

        self.type_name = demangled_name

        type = get_matching_type(target, prepare_name_for_lookup(self.type_name))
        logger >> f"Resolved type: {type}"

        self.type_obj = type


        # Resolve the void* object pointer
        void_ptr = self.valobj.GetChildMemberWithName('m_data')

        addr = void_ptr.GetValueAsUnsigned()

        if addr == 0:
            logger >> "Void pointer is null, cannot create typed object."
            self.typed_obj = None
            self.index = None
            return

        index = vtable.GetChildMemberWithName('m_index')
        index_type = index.GetType()

        self.typed_obj = self.valobj.CreateValueFromAddress("<variant value>", addr, self.type_obj)
        self.index = vtable.CreateValueFromData("<variant index>", index.GetData(),
                                                index_type)

    def num_children(self):
        logger = lldb.formatters.Logger.Logger()
        logger >> f"Number of children for {self.type_name}: {1 if self.typed_obj else 0}"
        return 2 if self.typed_obj else 0

    def get_child_at_index(self, index):

        if index == 1:
            return self.typed_obj
        if index == 0:
            return self.index
        return None

    def get_child_index(self, name):
        if name == "<variant index>":
            return 0
        elif name == "<variant value>":
            return 1
        return -1


def TypeErasedSummary(valobj, internal_dict):
    logger = lldb.formatters.Logger.Logger()
    type_info_ptr = valobj.GetChildMemberWithName('type_info')
    type_info_addr = type_info_ptr.GetValueAsUnsigned()
    logger >> f"Type info address (b): {type_info_addr:#x}"
    expr = f'(std::type_info const*)({type_info_addr})->name()'
    name_str = valobj.EvaluateExpression(expr)
    mangled_name = name_str.GetSummary().strip('"')

    # Demangle if necessary (skip on MSVC)
    target = valobj.GetTarget()
    triple = target.GetTriple()
    is_msvc = 'msvc' in triple.lower()
    demangled_name = mangled_name
    if not is_msvc:
        demangle_expr = (
            'extern "C" char* __cxa_demangle(const char*, char*, size_t*, int*); '
            f'int status=0; char* dem = __cxa_demangle("{mangled_name}", 0, 0, &status); '
            '(const char*)dem'
        )
        dem_str = valobj.EvaluateExpression(demangle_expr)
        if dem_str.IsValid() and not dem_str.GetError():
            demangled_name = dem_str.GetSummary().strip('"')

    return f'myobject<{demangled_name}>'


def __lldb_init_module(debugger, internal_dict):
    logger = lldb.formatters.Logger.Logger()
    #debugger.HandleCommand(
    #    'type summary add -F myobj_printer.TypeErasedSummary mytype'
    #)
    debugger.HandleCommand(
        'type synthetic add -x -l rpnx_variant.TypeErasedProvider "^rpnx::basic_variant<.*>$'  # Adjust the type name as needed
    )
    #debugger.HandleCommand(
    #    'type synthetic add -x -l rpnx_variant.TypeErasedProvider "rpnx::basic_variant<.*> (const)?\\s?&"'  # Adjust the type name as needed
    #)

    debugger.HandleCommand(
        'type synthetic add -l rpnx_variant.TypeErasedProvider "quxlang::type_symbol"'  # Adjust the type name as needed
    )
    debugger.HandleCommand(
        'type synthetic add -l rpnx_variant.TypeErasedProvider "quxlang::type_symbol &"'  # Adjust the type name as needed
    )
    logger >> "TypeErasedProvider and summary added to LLDB."
