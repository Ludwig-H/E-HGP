# OutputSet: causal publication audit, working snapshot 763e2ee3

Private, bounded, header-only audit. No HGP engine, application CLI, native cloud,
GPU, GCP, heavy test or shared source was compiled, run or edited.

The 5 complete core snapshots match the source hashes before and after the
capture. HEAD was 6d2d3bc5; OutputSet and cli_options are WORKING edits differing
from the index, not code published by that HEAD. No stage.hpp is required:
the transitive project dependencies of cli_output.hpp are status.hpp,
types.hpp and reasons.def. cli_options.hpp is preserved for the refusal helper.

probe.cpp links GNU --wrap=rename. Its wrapper returns EIO on exactly the second
rename only in the failing scenario. All filesystem effects are confined to
mkdtemp fixture directories below the explicitly provided private packet path.
Each scenario starts with two regular sentinels, obtains its observation after
OutputSet destruction, and then unlinks only those two owned files and rmdir.
No production rename is intercepted. Compilation has a 10-second timeout.

Five native controls are captured in capture.json:

- success: both outputs become NEW, two renames;
- no_commit: both sentinels stay OLD, zero rename;
- writer_bad_alloc: writer raises after writing into its temporary, caught as
  memory_budget; both sentinels stay OLD;
- rename2_eio: resource_exhausted/output_unwritable, first output NEW and second
  OLD: the global all-or-nothing claim is false, although the error is returned;
- idempotent: two commits succeed with only two renames in total.

All five observe zero leftover OutputSet temporary files and equal positive FD
counts before/after. Native code 0 means those EXPECTED OBSERVATIONS match; it
does not mean that the transaction contract passes. No assert is used.

Static causal lines in snapshot/core/cli_output.hpp: commit 164--168 renames
sequentially; successful entry becomes temp_live=false; a later failure returns
without backup or rollback. Header 35--36 expressly admits this limitation,
in conflict with the unconditional all-or-nothing wording at 1,26,160.

The reader is read-only: pass the externally supplied SHA256 of manifest.json.
It refuses symlinks, foreign inventory and any payload hash mismatch BEFORE
loading capture/source JSON. It independently checks every observation, FD and
temporary count. It runs bounded in-memory negative controls: a modified payload
must fail its hash, and a falsely successful transaction verdict must fail the
causal judge. It never executes the captured binary. Normal and -O are equivalent.

Limitations: Linux/GNU header-only fault injection, one build/run, no sanitizer,
no concurrency/TOCTOU or filesystem-failure taxonomy qualification, no crash
durability/fsync guarantee. The already fixed alias/RAII guards were read
statically; this packet adds no native alias campaign or timing-performance claim.

## Text-only derivative v2

The original 11-payload packet NJyipG60 is unchanged. This derivative omits
the executable. Its raw SHA256 is recorded in capture.archive_metadata, along
with GNU g++ version 13.3.0 (Ubuntu 13.3.0-6ubuntu2~24.04.1) and identification
time. That tool identification occurred AFTER capture: the original compiler
executable was not pre-hashed, so it is not a retrospective toolchain pin.

The five core headers, probe.cpp, source pins and five native stdout records
are preserved byte for byte (capture.json only gains archive_metadata).
The v2 reader proves integrity of this TEXT archive and the causal consistency
of those historical records, NOT reexecution of a binary or verification of the
omitted executable against its recorded digest. It prints native_reexecuted=0.

Optional independent replay: use a NEW private replay directory, NOT this
closed archive. With A the absolute archive path and D that owned replay path:
  timeout 10s g++ -std=c++20 -O0 -Wall -Wextra -Werror -I"$A/snapshot" \
    "$A/probe.cpp" -Wl,--wrap=rename -o "$D/probe"
  "$D/probe" "$D"
Compile/replay artifacts must remain outside A. Compare the five semantic rows,
including refusal, mixed OLD/NEW contents, zero temporaries and unchanged
positive FD counts; FD absolute values can depend on the invoking environment.
An independently built executable need not have the historical raw binary SHA
(absolute paths/toolchain environment can differ). No engine qualification follows.
