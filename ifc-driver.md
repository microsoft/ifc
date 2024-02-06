# The `ifc` Tool

The IFC SDK contains a command-line executable named `ifc`.  It is a simple driver that acts as an extendable interface for tooling around IFC files.  The pattern of invocation is

> `ifc` _cmd_ _file1_ _file2_ _..._


where:
  - _cmd_ is either a built-in sucommand, or an external extension executable
  - _file1_, _file2_, _..._ are the IFC files to be acted on

At the moment, the tool supports only one built-in command: `version`.  That is, the invocation

> `ifc` `version` _file1_ _file2_ _..._

will display each _file_ along with the version of the of IFC Specification it was generated for.

An IFC external extension `cmd` is any executable named `ifc-`_cmd_ that can be found via the `PATH` environment variable.  Such an executable will be invoked by the `ifc` driver along with the rest of the command-line arguments it was originally invoked with.  It is a convenient way to structure tooling around IFC files, making your own extension appear as if it was a built-in facility.
