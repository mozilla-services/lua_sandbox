# Command Line Interface Tools

## lsb_heka_cat

A command-line utility for counting, viewing, filtering, and extracting messages
from a Heka protobuf log/stream.

```
usage: lsb_heka_cat [-t|-c|-h] [-m message_matcher] [-f] [-n #] <FILE>
description:
  -t output the messages in text format (default)
  -c only output the message count
  -h output the messages as a Heka protobuf stream
  -f output appended data as the file grows
  -n output the last # of messages (simple header check so not 100% accurate)
  -m message_matcher expression (default "TRUE")
  FILE name of the file to cat or '-' for stdin
notes:
  All output is written to stdout and all log/error messages are written to stderr.

```
See the [message matcher](../util/message_matcher.html) documentation for more details about the message_matcher expression.

## luasandbox

Lua interpreter. It behaves like vanilia lua5.1 (i.e not sandboxed) and can be sandboxed by using:
```
package.sandboxed = true
```

```
usage: luasandbox [options] [script [args]].
Available options are:
  -e stat  execute string 'stat'
  -l name  require library 'name'
  -i       enter interactive mode after executing 'script'
  -v       show version information
  --       stop handling options
  -        execute stdin and stop handling options
```
