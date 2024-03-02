# Notes

Need to write a new interface for the incremental compilation.

Thoughts:

Each Resolver should be a Question(Inputs) -> Answer(Output) function.

We should cache the indirect questions and answers, e.g.

Questions should be declared like,

question ( inputs ) -> output type

The problem is that a question needs to access state, you could view a question as,

question(state, inputs) -> output

But this doesn't work well with caching, since any chance to the state results in a cache miss for all inputs.

Instead, we should treat state separately by only interacting with state in a few special ways.

e.g. we can cache the result of executing a question q1 that asks q2 and q3 in a logical format like so:

```
q1("foo") -> 4
  q2("foo.x") -> 2
  q2("foo.y") -> 2
```

Thus, to determine if q1("foo") is still "4", we need to determine if q2("foo.x") and q2("foo.y") are still "2".

Supposing we want to cache multiple results based on multiple program states, we can create a tree like so:

```
  q1("foo")
    ask(q2,"foo.x")
      2 -> ask(q2,"foo.y")
        2 -> answer(4)
```

Supposing that the result of q1("foo") is the sum of q2("foo.x") and q2("foo.y"), we can add a result for q2("foo.y")-> 4 as follows:



```
  q1("foo")
    ask(q2,"foo.x")
      2 -> ask(q2,"foo.y")
        2 -> answer(4)
        4 -> answer(6)
```

Supposing the answer of q2("foo.x") becomes 4, we can update the tree as follows:



```
q1("foo")
    ask(q2,"foo.x")
    2 -> ask(q2,"foo.y")
        2 -> answer(4)
        4 -> answer(6)
    4 -> ask(q2,"foo.y")
        4 -> answer(8)
```

