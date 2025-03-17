# TODO

## Major Cleanup

### Conversion to Co-Resolvers using Macros
All resolvers should be replaced with co-resolvers. And all co-resolvers should use macros to reduce boilerplate. This will allow later refactoring to the second version of the resolver system by editing the macro.

E.g.



Old (non-coroutine):
```quxlang
void quxlang::entity_canonical_chain_exists_resolver::process(compiler* c)
{
    //std::cout << this->debug_recursive() << std::endl;
    auto chain = this->m_chain;
    std::string name = to_string(chain);
    assert(!qualified_is_contextual(chain));
    if (chain.template type_is< module_reference >())
    {
        std::string module_name = as< module_reference >(chain).module_name;
        // TODO: Check if module exists
        auto module_ast_dp = get_dependency(
            [&]
            {
                return c->lk_module_ast(module_name);
            });
        if (!ready())
            return;
...
```

Old (coroutine):
```cpp
rpnx::resolver_coroutine< quxlang::compiler, quxlang::ast2_type_map > quxlang::type_map_resolver::co_process(compiler* c, key_type input)
{

    std::vector< ast2_named_declaration > ast_declarations;

    std::string inputname = to_string(input);

    ast2_node ast = co_await *c->lk_entity_ast_from_canonical_chain(input);

start:

    auto include_decl = [&](std::vector< ast2_top_declaration > const& decls) -> rpnx::general_coroutine< compiler, void >
    {
...
```


New:
```cpp
QUX_CO_RESOLVER_IMPL_FUNC_DEF(type_symbol_kind)
{
    QUX_CO_GETDEP(ast, entity_ast_from_canonical_chain, (input_val));

    if (typeis< functum >(ast))
    {
        QUX_CO_ANSWER(symbol_kind::functum_kind);
    }

    QUX_CO_ANSWER(rpnx::unimplemented());
}
```

#### Refactoring examples:

* `auto y = co_await *c->lk_X(input);` -> `QUX_CO_GETDEP(y, X, (input));`
* `co_return x;` -> `QUX_CO_ANSWER(x);`

