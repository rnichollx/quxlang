

template < typename CoroutineProvider >
class foo
{
    CoroutineProvider provider;

  public:
    foo(CoroutineProvider provider)
        : provider(provider)
    {
    }



    auto bar() -> typename CoroutineProvider::template co_type< int >
    {
        co_await provider.baz();
        co_return 42;
    }
};

int main()
{
}