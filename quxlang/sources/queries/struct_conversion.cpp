// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#include <quxlang/queries/specs/struct_conversion_spec.hpp>

rpnx::querygraph::coroutine< quxlang::struct_conversion_spec > quxlang::struct_conversion_impl(struct_conversion_input input)
{
    struct_inheritance_info inheritance = co_await rpnx::querygraph::request< struct_inheritance_info_query >(input.source_type);
    struct_conversion_result output;

    for (struct_subobject_record const& subobject : inheritance.subobjects)
    {
        if (subobject.type != input.destination_type || subobject.paths.empty())
        {
            continue;
        }
        output.candidate_paths.push_back(subobject.paths.front());
    }

    if (output.candidate_paths.empty())
    {
        output.status = struct_conversion_status::unavailable;
    }
    else if (output.candidate_paths.size() == 1)
    {
        output.status = struct_conversion_status::unique;
        output.path = output.candidate_paths.front();
    }
    else
    {
        output.status = struct_conversion_status::ambiguous;
    }

    co_return output;
}
