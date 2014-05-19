Lua HyperLogLog Library
=======================

The library is implemented in the _hyperloglog_ table.

Constructor
-----------
**hyperloglog.new** ()

*Arguments*

none

*Return*

A HyperLogLog object.

Methods
-------
bool **add** (key)

*Arguments*
- key (string/number) The key to add to the HyperLogLog.

*Return*

True if add altered the estimate, false if it has not.

____
double **count** ()

*Arguments*

none

*Return*

The approximated cardinality of the set.

____
void **clear** ()

*Arguments*

none

*Return*

none
