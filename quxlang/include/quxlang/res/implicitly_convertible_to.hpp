//
// Created by Ryan Nicholl on 10/27/23.
//

#ifndef QUXLANG_implicitly_convertible_to_RESOLVER_HEADER_GUARD
#define QUXLANG_implicitly_convertible_to_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/canonical_type_reference.hpp"
#include "quxlang/data/type_symbol.hpp"
#include "quxlang/res/resolver.hpp"

namespace quxlang
{
    struct implicitly_convertible_to_query
	{
		type_symbol from;
		type_symbol to;

		RPNX_MEMBER_METADATA(implicitly_convertible_to_query, from, to);
	};


    QUX_CO_RESOLVER(implicitly_convertible_to, implicitly_convertible_to_query, bool);
} // namespace quxlang

#endif // QUXLANG_implicitly_convertible_to_RESOLVER_HEADER_GUARD
