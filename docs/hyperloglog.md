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
void **add** (key)

*Arguments*
- key (string/number) The key to add to the HyperLogLog.

*Return*

none

____
bool **count** ()

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
