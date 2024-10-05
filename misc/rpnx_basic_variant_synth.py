# Copyright 2024 Ryan Nicholl, rnicholl@protonmail.com
# Made with AI assistance

import lldb
import re

import sys
class BasicVariant_SyntheticProvider:
    def __init__(self, valobj, internal_dict):
        self.valobj = valobj
        self.active_child = None
        self.update()

    def update(self):
        # Access m_vinf
        m_vinf = self.valobj.GetChildMemberWithName('m_vinf')
        if not m_vinf.IsValid():
            self.active_child = None
            return

        # Check if m_vinf is not nullptr
        m_vinf_address = m_vinf.GetValueAsUnsigned()
        if m_vinf_address == 0:
            self.active_child = None
            return

        # Access m_vinf->m_index
        # Since m_vinf is a pointer, we need to dereference it
        m_index = m_vinf.Dereference().GetChildMemberWithName('m_index')
        if not m_index.IsValid():
            self.active_child = None
            return

        index = m_index.GetValueAsSigned()
        if index < 0:
            self.active_child = None
            return

        # Extract the template arguments (Alloc, Ts...)
        type_str = self.valobj.GetType().GetUnqualifiedType().GetName()
        Ts = self._extract_template_arguments(type_str)
        if len(Ts) < 2:
            self.active_child = None
            return

        Ts = Ts[1:]  # Skip Alloc
        if index >= len(Ts):
            self.active_child = None
            return

        active_type_str = Ts[index]

        # Access m_data
        m_data = self.valobj.GetChildMemberWithName('m_data')
        if not m_data.IsValid():
            self.active_child = None
            return

        m_data_address = m_data.GetValueAsUnsigned()
        if m_data_address == 0:
            self.active_child = None
            return

        # Cast m_data to active_type
        active_value_expr = f'*({active_type_str}*)({m_data_address})'
        self.active_child = self.valobj.CreateValueFromExpression('value', active_value_expr)

    def num_children(self):
        return self.active_child.GetNumChildren() if self.active_child else 0

    def get_child_index(self, name):
        return self.active_child.GetIndexOfChildWithName(name) if self.active_child else -1

    def get_child_at_index(self, index):
        return self.active_child.GetChildAtIndex(index) if self.active_child else None

    def has_children(self):
        return self.active_child is not None

    def get_value(self):
        return self.active_child.GetValue() if self.active_child else None

    def _extract_template_arguments(self, type_str):
        # Use regex to extract template arguments
        match = re.match(r'.*<(.+)>', type_str)
        if not match:
            return []
        args_str = match.group(1)
        return self._parse_template_args(args_str)

    def _parse_template_args(self, args_str):
        args = []
        current_arg = ''
        depth = 0
        i = 0
        while i < len(args_str):
            c = args_str[i]
            if c == '<':
                depth += 1
                current_arg += c
            elif c == '>':
                depth -= 1
                current_arg += c
            elif c == ',' and depth == 0:
                args.append(current_arg.strip())
                current_arg = ''
            else:
                current_arg += c
            i += 1
        if current_arg:
            args.append(current_arg.strip())
        return args
