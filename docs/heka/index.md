# Heka Sandbox

## Overview

This is the 1.0 release of the Heka sandbox API.  It is a refined implementation
of its predecessor which was developed in [Heka](https://github.com/mozilla-services/heka).  
The goal is to decople it from Go and make it easily embeddable in any language.
Although this could be ported back to the Heka Go implementation a new design
of that infrastructure has been created to replace it called 
[Hindsight](https://github.com/trink/hindsight).

## Table of Contents

* [Message Matcher](message_matcher.md)
* [Input Sandbox](input.md)
  * [Heka Stream Reader](heka_stream_reader.md)
  * [Heka JSON](heka_json.md)
* [Analysis Sandbox](analysis.md)
* [Output Sandbox](output.md)
  * [Heka JSON](heka_json.md)

## Sandbox API Changes from the Go Heka Sandbox

There are a few intentional changes between tho original Heka sandbox and this version.

### Changes

1. `read_message` now has a `framed` parameter to retrive the raw message with stream framing.

#### Input Sandbox

1. `inject_message` accepts a numeric or string checkpoint as the second argument
1. `process_message` receives the checkpoint value as the first argument (if it was provided by `inject_message`).

#### Output Sandbox

1. `process_message` receives a sequence_id as the first argument (if it was provided by `update_checkpoint`).
   Extended return codes have been added to support skipping, retrying, batching, and asynchronous output.

#### Analysis/Output Sandbox

1. `timer_event` has a second parameter `shutdown` that is set to true when the sandbox is exiting.

### Additions

#### Input Sandbox

1. A [Heka Stream Reader](heka_stream_reader.md) Lua module was added.
1. A [Heka JSON](heka_json.md) Lua module was added.

#### Output Sandbox

1. [update_checkpoint](output.md#update_checkpoint) was added for batch and asynchronous processing.
1. A [Heka JSON](heka_json.md) Lua module was added.

### Removals

1. The `write_message` API was removed; messages are immutable and this API broke that rule.
1. The `read_next_field` API was removed; instead the raw message should be decoded and the Lua table iterated.

### Notes

1. The `read_config` API in unchanged but now has access to the entire sandbox configuration.
