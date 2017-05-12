# Heka Sandbox Interface

## Overview

This document describes the 1.0 release of the Heka sandbox API built on the
[Generic Sandbox Interface](/sandbox.md). The 1.0 release is a refined
implementation of its predecessor which was developed in
[Heka](https://github.com/mozilla-services/heka). The goal is to decople it from
Go and make it easily embeddable in any language. The Go version of Heka has
been deprecated and replaced by
[Hindsight](https://github.com/mozilla-services/hindsight).

## Sandbox API Changes from the Go Heka Sandbox

There are a few intentional changes between tho original Heka sandbox and this
version.

### Changes

#### Input Sandbox

1. `inject_message` accepts a numeric or string checkpoint as the second
argument
1. `process_message` receives the checkpoint value as the first argument (if it
was provided by `inject_message`).

#### Output Sandbox

1. `process_message` receives a sequence_id as the first argument (if it was
provided by `update_checkpoint`). Extended return codes have been added to
support skipping, retrying, batching, and asynchronous output.

#### Analysis/Output Sandbox

1. `read_message`
    * returns `nil` for optional header fields if they don't exist instead of an
    empty string or zero
    * added a `framed` parameter to retrive the raw message with stream framing
    * added a `size` parameter to retrive size of the raw message without having
    to copy it down
1. `timer_event` has a second parameter `shutdown` that is set to true when the
sandbox is exiting.

### Additions

#### Input Sandbox

1. [create_stream_reader](input.md#createstreamreader) function was added.
1. [is_running](input.md#is_running) function was added.

#### Output Sandbox

1. [update_checkpoint](output.md#update_checkpoint) was added for batch and
asynchronous processing.
1. [create_message_matcher](output.md#createmessagematcher) function was added.

### Removals

1. The `write_message` API was removed; messages are immutable and this API
broke that rule.
1. The `read_next_field` API was removed; instead the raw message should be
decoded and the Lua table iterated.

### Notes

1. The `read_config` API in unchanged but now has access to the entire sandbox configuration.
