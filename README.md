# idatrace2tree

A tool for converting a trace obtained from IDA PRO to a text tree view.

## Trace recording

IDA PRO has functionality for [tracing](https://hex-rays.com/products/ida/support/tutorials/tracing/). If you are lucky enough, you can get the result of "Function tracing" without skipped records. I couldn't get it at the time of writing this program. For this reason, I abandoned the development of this application.


So, the trace results from IDA PRO can be saved to a text file, and it can already be transferred to the application input.

## Usage example

```console
$ ./idatrace2tree --input=trace.txt --output=trace_tree.txt --filters=filters.txt --type=all
```

I can describe in more detail in case someone needs it.
