Lua Circular Buffer Library
===========================

The library is a sliding window time series data store and is implemented in the _circular_buffer_ table.

Constructor
-----------
**circular_buffer.new** (rows, columns, seconds_per_row, enable_delta)

*Arguments*
- rows (unsigned) The number of rows in the buffer (must be > 1)
- columns (unsigned)The number of columns in the buffer (must be > 0)
- seconds_per_row (unsigned) The number of seconds each row represents (must be > 0 and <= 86400).
- enable_delta (**optional, default false** bool) When true the changes made to the 
    circular buffer between delta outputs are tracked.

*Return*

A circular buffer object.

Methods
-------
**Note:** All column arguments are 1 based. If the column is out of range for the configured circular buffer a fatal error is generated.

double **add** (nanoseconds, column, value)

*Arguments*
- nanosecond (unsigned) The number of nanosecond since the UNIX epoch. The value is 
    used to determine which row is being operated on.
- column (unsigned) The column within the specified row to perform an add operation on.
- value (double) The value to be added to the specified row/column.

*Return*

The value of the updated row/column or nil if the time was outside the range of the buffer.

____
double **set** (nanoseconds, column, value)

*Arguments*
- nanosecond (unsigned) The number of nanosecond since the UNIX epoch. The value is
    used to determine which row is being operated on.
- column (unsigned) The column within the specified row to perform a set operation on.
- value (double) The value to be overwritten at the specified row/column. 
  For aggregation methods "min" and "max" the value is only overwritten if it is smaller/larger than the current value.

*Return*

The resulting value of the row/column or nil if the time was outside the range of the buffer.

____
double **get** (nanoseconds, column)

*Arguments*
- nanosecond (unsigned) The number of nanosecond since the UNIX epoch. The value is used
    to determine which row is being operated on.
- column (unsigned) The column within the specified row to retrieve the data from.

*Return*

The value at the specifed row/column or nil if the time was outside the range of the buffer.

____
double, double, double **get_configuration** ()

*Arguments*
- none

*Return*

The circular buffer dimension values specified in the constructor.
- rows
- columns
- seconds_per_row

____
int **set_header** (column, name, unit, aggregation_method)

*Arguments*
- column (unsigned) The column number where the header information is applied.
- name (string) Descriptive name of the column (maximum 15 characters). Any non alpha
    numeric characters will be converted to underscores. (default: Column_N)
- unit (string - optional) The unit of measure (maximum 7 characters). Alpha numeric,
    '/', and '*' characters are allowed everything else will be converted to underscores.
    i.e. KiB, Hz, m/s (default: count)
- aggregation_method (string - optional) Controls how the column data is aggregated
    when combining multiple circular buffers.
    - **sum** The total is computed for the time/column (default).
    - **min** The smallest value is retained for the time/column.
    - **max** The largest value is retained for the time/column.
    - **none** No aggregation will be performed the column.

*Return*

The column number passed into the function.

____
string, string, string **get_header** (column)

*Arguments*
- column (unsigned) The column number of the header information to be retrieved.

*Return*

The current values of specified header column.
- name
- unit
- aggregation_method

____
double, int **compute** (function, column, start, end)

*Arguments*
- function (string) The name of the compute function (sum|avg|sd|min|max|variance).
- column (unsigned) The column that the computation is performed against.
- start (optional - unsigned) The number of nanosecond since the UNIX epoch. Sets the
    start time of the computation range; if nil the buffer's start time is used.
- end (optional- unsigned) The number of nanosecond since the UNIX epoch. Sets the 
    end time of the computation range (inclusive); if nil the buffer's end time is used.
    The end time must be greater than or equal to the start time.

*Returns*

- The result of the computation for the specifed column over the given range or nil if the range fell outside of the buffer.
- The number of rows that contained a valid numeric value.

____
double, double **mannwhitneyu** (column, start_x, end_x, start_y, end_y, use_continuity)

Computes the Mann-Whitney rank test on samples x and y.

*Arguments*
- column (unsigned) The column that the computation is performed against.
- start_1 (unsigned) The number of nanosecond since the UNIX epoch.
- end_1 (unsigned) The number of nanosecond since the UNIX epoch. The end time must be greater than or equal to the start time.
- start_2 (unsigned).
- end_2 (unsigned).
- use_continuity (optional - bool) Whether a continuity correction (1/2) should be taken into account (default: true).

*Returns* (nil if the range fell outside the buffer)

- U_1 Mann-Whitney statistic.
- One-sided p-value assuming a asymptotic normal distribution.

**Note:** Use only when the number of observation in each sample is > 20 and you have 2 independent samples of ranks. 
Mann-Whitney U is significant if the u-obtained is LESS THAN or equal to the critical value of U.

This test corrects for ties and by default uses a continuity correction. The reported p-value is for a one-sided
hypothesis, to get the two-sided p-value multiply the returned p-value by 2.

____
double **current_time** ()

*Arguments*
- none

*Return*

The time of the most current row in the circular buffer.

____
cbuf **format** (format)
    Sets an internal flag to control the output format of the circular buffer data structure; if deltas are not enabled or there haven't been any modifications, nothing is output.

*Arguments*
- format (string)
    - **cbuf** The circular buffer full data set format.
    - **cbufd** The circular buffer delta data set format.

*Return*

The circular buffer object.

Output
------
The circular buffer can be passed to the output() function. The output format
can be selected using the format() function.

The cbuf (full data set) output format consists of newline delimited rows
starting with a json header row followed by the data rows with tab delimited
columns. The time in the header corresponds to the time of the first data row,
the time for the other rows is calculated using the seconds_per_row header value.

    {json header}
    row1_col1\trow1_col2\n
    .
    .
    .
    rowN_col1\trowN_col2\n

The cbufd (delta) output format consists of newline delimited rows starting with
a json header row followed by the data rows with tab delimited columns. The
first column is the timestamp for the row (time_t). The cbufd output will only
contain the rows that have changed and the corresponding delta values for each
column.

    {json header}
    row14_timestamp\trow14_col1\trow14_col2\n
    row10_timestamp\trow10_col1\trow10_col2\n

Sample Cbuf Output
------------------

    {"time":2,"rows":3,"columns":3,"seconds_per_row":60,"column_info":[{"name":"HTTP_200","unit":"count","aggregation":"sum"},{"name":"HTTP_400","unit":"count","aggregation":"sum"},{"name":"HTTP_500","unit":"count","aggregation":"sum"}]}
    10002   0   0
    11323   0   0
    10685   0   0
