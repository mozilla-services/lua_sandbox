Lua Bloom Filter Library
========================

The library is implemented in the _bloom_filter_ table.

Constructor
-----------
**bloom_filter.new** (items, probability)

*Arguments*
- items (unsigned) The maximum number of items to be inserted into the filter (must be > 1)
- probability (double) The probability of false positives (must be between 0 and 1)

*Return*

A bloom filter object.

Methods
-------
bool **add** (key)

*Arguments*
- key (string/number) The key to add to the bloom filter.

*Return*

True if the key was added, false if it already existed.

____
bool **query** (key)

*Arguments*
- key (string/number) The key to lookup in the bloom filter.

*Return*

True if the key exists, false if it doesn't.

____
void **clear** ()

*Arguments*

none

*Return*

none
