# Allocation boundaries — isolated header-only audit

The four positive controls return their exact expected values. Refusing the
first ordinary C++ allocation inside each helper then reproduces four escaped
std::bad_alloc exceptions, rather than Result/Outcome memory_budget:
read_u32le_cloud, parse_real, parse_integer_list, parse_head_configs.
This is distinct from the already archived second-rename defect.

The native process itself catches these escapes and exits 0 only if every
expected defect and positive control is reproduced, all files are unchanged,
and the descriptor counts before/after agree. That exit 0 is an audit success,
NOT a product pass. The four injected records retain default "ok"/"none"
fields only because no Outcome was returned; "returned":false is mandatory.

One fresh private root contains this closed TEXT-ONLY archive, a non-archived
build executable, and two test fixtures. Manual source creation used
apply_patch; only the private probe wrote its fresh fixtures with O_EXCL.
No existing engine, CLI, shared audit note, archive, or user file was changed.
No GPU/GCP, HGP campaign, native engine, sanitizer, or benchmark ran.

## Sources and provenance

Five complete local compiler transdependencies are frozen under snapshot/:
core/cli_options.hpp, cloud/u32le_input.hpp, core/status.hpp, core/types.hpp,
core/reasons.def. They include exactly those local dependencies. probe.cpp
compiles with C++20 and no engine object or library.

Four complete caller .cpp snapshots under callers/ are evidence of the
unprotected calls, NOT compiler inputs. source_pins.json records original
absolute paths, HEAD context, and SHA-256 before/after. The version is the
developer's working/index additions atop 6d2d3bc5, not that HEAD's committed
implementation. cli_options/u32le_input are staged additions.
There is no whole-engine qualification.

The reader preserves all u32 coordinate words. The u32le containing format
does not expand the product's u18 coordinate domain: prepare_cloud, not this
reader, judges every word. No B21/u18 numerical claim is made by this probe.

Compile command, run command, exit codes, complete merged stdout/stderr tool
output, compiler GNU version and the non-archived binary's raw SHA are in
capture.json. The compiler executable was not prehashed; this is a recorded
GNU version, not a closed toolchain proof. Eight bounded cases refuse only
four allocations; compile and run each used timeout 10s.

## Causal scope

- Reader: a valid ordinary long pathname. On this GNU build its implicit
  std::filesystem::path allocation before the local try throws first.
- Real: valid decimal 1.00000000000000000000; allocation of the terminated
  std::string escapes.
- List: valid "1,2,3"; first vector push allocation escapes.
- Configs: valid "5 1.0 eom 0\n"; first token vector allocation escapes.

All baseline values are checked before reading vector indices. FD counts
include the transient /proc/self/fd enumeration descriptor consistently.
Each row explicitly checks both fixture contents and the directory cardinality
before/after. Final fixture SHA values are recorded too. This single-failure
probe does not claim coverage of every allocation, every C runtime ENOMEM,
every platform, or every caller operation. In particular the CLI executables
themselves were not run, and no SIGABRT/CLI JSON trace is invented.

## Closed archive reader

Supply the manifest SHA obtained externally. reader.py verifies that SHA
BEFORE parsing manifest.json, refuses symlinks and unlisted files, verifies
every payload hash before loading capture or source pins, checks the eight
records, source before/after identities and snapshots, and verifies all hashes
again after reading. No assert statement is used for a gate.
In-memory controls must reject a altered hashed source and a false
"returned_memory_budget" summary, without modifying this archive.

The reader proves text/source/capture integrity and bounded capture semantics,
NOT a new native execution, omitted binary verification, or engine correctness.
Run normal and -O; both must print native_reexecuted=0.

## Optional independent native replay

Choose NEW private build and fixture directories outside this archive.
Use argv equivalent to capture.compile.argv, replacing the include/probe paths
with this archive, replacing the -o argument with the new private build/probe.
Enforce subprocess timeout 10s for compilation. Then run the new probe with
one NEW empty private fixture directory and timeout 10s. The probe refuses to
overwrite its fixture files. Compare all eight records, ignoring only GNU
implementation/path-size-dependent refused_size and platform FD baseline.
Required: four positive valid returns, four injected escaped bad_alloc,
matching FD counts, exactly two unchanged fixtures for every row.
Do not compile the four caller snapshots and do not invoke an engine/CLI.
