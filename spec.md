## Function
### Function Return

If the execution of a function reaches the end of the function body, the function implicitly returns a default constructed object of the return type, or no object if the return type is void. However; if the return type is not default constructible, then the function triggers a fallthrough fault (which by default, terminates the program by calling RUNTIME::terminate()). However, if the function is declared NOFALLTHROUGH, then executing past the end of the body of the function without an explicit `RETURN` will always trigger a fallthrough fault, even if the return type is void or default constructible.
