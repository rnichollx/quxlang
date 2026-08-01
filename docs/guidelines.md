# Const

Avoid declaring local const objects, this prevents move operations.

Generally, you should declare query results as `const &` if they are read-only. If the object is modified, instead prefer
by-value, potentially using std::move.