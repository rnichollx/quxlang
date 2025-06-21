```quxlang
::std IMPORT stdlib;

// vs
IMPORT stdlib AS std;
```


Decision: 

use `::foo IMPORT foo;`, allow `IMPORT foo;` as shorthand for `::foo IMPORT foo;`
