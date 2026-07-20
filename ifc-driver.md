# The `ifc` Tool

The IFC SDK contains a command-line executable named `ifc`.  It is a simple driver that acts as an extendable interface for tooling around IFC files.  The pattern of invocation is

> `ifc` _cmd_ _file1_ _file2_ _..._


where:
  - _cmd_ is either a built-in subcommand, or an external extension executable
  - _file1_, _file2_, _..._ are the IFC files to be acted on

At the moment, the tool supports three built-in commands: `version`, `archive`, and `extract`.

The invocation

> `ifc` `version` _file1_ _file2_ _..._

will display each _file_ along with the version of the IFC Specification it was generated for.

The invocation

> `ifc` `archive` [`--name` _name_] `-o` _archive_ _file1_ _file2_ _..._

packages the IFC files _file1_, _file2_, _..._ into a single archive IFC file named _archive_ (an IFC file whose translation unit sort is `Archive`).  Each input IFC file is embedded verbatim, retaining its own complete file structure.  The archive's table of contents records, for each member, its **sort** and **canonical name** — the module name for a named module or the header-unit name for a header unit, read from that member's own unit descriptor — together with a normalized relative **filepath**, and its offset and size within the archive.  A member is identified by its sort and canonical name together, so a named module and a header unit may share a name; no two members may share both.  Members are stored by canonical name with sort breaking ties.  Input paths containing parent traversal, or paths that normalize to the same extraction destination, are rejected.  The optional `--name` gives the archive itself a canonical name.  If `-o` is omitted, the first file argument names the archive to create and the rest are the IFC files to package.  The inputs must be non-archive IFC files; archives cannot be nested.

The invocation

> `ifc` `extract` [`--force`] [`-o` _dir_] _archive_ (`--all` | _selector1_ _selector2_ _..._)

recovers IFC files from an _archive_ created by the `archive` command.  With `--all`, every contained member is extracted.  Otherwise, a positional _selector_ is the canonical name of a named module, while header units are selected with the repeatable options `--quote-header` _name_ and `--angle-header` _name_.  Their arguments are bare header-name payloads: `--quote-header detail/config.h` selects the member named `"detail/config.h"`, and `--angle-header vector` selects the member named `<vector>`.  Keeping the delimiter form in the option avoids shell removal of quotes and interpretation of angle brackets as redirection.  Each member is written to its recorded filepath beneath _dir_ (the current directory by default), recreating intermediate directories without following archive-controlled symbolic links or reparse points.  Existing files are left unchanged unless `--force` is supplied, in which case each completed temporary file atomically replaces its destination.  This is the inverse of `archive`, so `extract` followed by a byte comparison is a convenient way to test archive round-tripping.

An IFC external extension `cmd` is any executable named `ifc-`_cmd_ that can be found via the `PATH` environment variable.  Such an executable will be invoked by the `ifc` driver along with the rest of the command-line arguments it was originally invoked with.  It is a convenient way to structure tooling around IFC files, making your own extension appear as if it was a built-in facility.
