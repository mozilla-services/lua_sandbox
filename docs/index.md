# Overview

Sandboxes provide a dynamic and isolated execution environment
for data parsing, transformation, and analysis.  They allow access to data
without jeopardizing the integrity or performance of the processing
infrastructure. This broadens the audience that the data can be
exposed to and facilitates new uses of the data (i.e. debugging, monitoring,
dynamic provisioning,  SLA analysis, intrusion detection, ad-hoc reporting,
etc.)

The Lua sandbox is a library allowing customized control over the Lua execution
environment including functionality like global data preservation/restoration on
shutdown/startup, output collection in textual or binary formats and an array of
parsers for various data types (Nginx, Apache, Syslog, MySQL and many RFC grammars)

These libraries and utilities have been mostly extracted from [Hindsight](https://github.com/mozilla-services/hindsight).
The goal was to decouple the Heka/Hindsight functionality from any particular
infrastructure and make it embeddable into any tool or language.

## Features

- small - memory requirements are as little as 8 KiB for a basic sandbox
- fast - microsecond execution times
- stateful - ability to resume where it left off after a restart/reboot
- isolated - failures are contained and malfunctioning sandboxes are terminated.
  Containment is defined in terms of restriction to the operating system,
  file system, libraries, memory use, Lua instruction use, and output size.
