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
* [Input Plugins](input_plugins.md)
  * [Heka Stream Reader](heka_stream_reader.md)
* [Analysis Plugins](analysis_plugins.md)
* [Output Plugins](output_plugins.md)
