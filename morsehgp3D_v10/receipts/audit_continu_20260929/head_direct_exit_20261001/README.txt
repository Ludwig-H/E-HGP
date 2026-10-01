Direct-point exit in the native head — targeted closed receipt
===========================================================
1 October 2026; auditor continuous; CPU-only, public_status=not_claimed.
Archive: archive/ (19 files: 18 TEXT payloads + manifest).
External manifest SHA256:
f7876e1b6c192fc9e8306b76f963b527b39d17635059fa2afcf60e70cc0cdd9a

What is tested
--------------
Three-node abstract PointDendrogram, not a geometric FULL export.
Two leaves each have two points, entry level1; their root is born at
level4. Its fifth direct point enters at level9. mcs2, z1, EOM,
allow_single=true. Independent analytic root stability7/3, direct
lambda1/3, selected root, labels[0,0,0,0,-1].
The reviewed native witness compiles0 and runs0. Exactly ONE line in
condense is mutated: point_rank[x] -> node_rank[v] for direct exit.
It compiles0 and runs1 with five semantic failures; stability5/2,
lambda1/2, labels[0,0,0,0,0], still the same selected root. No crash,
timeout, signal or harness failure is classified as this causal rejection.
Non-dyadic oracle values allow16*2^-52*max(1,abs(exact rational)); dyadic
values and ID/label vectors are exact. This is not real-arithmetic EOM.

Actual recording and independent auditor observations
----------------------------------------------------
Root reviewed all sources before the ONE native recording of this
packet. Fresh exclusive output; eight complete stages, two compiles,
two runs, no first_failure; GNU driver/backend and all project dependencies
hashed before/after. Binaries remain outside the archive. Static readers
normal/-O return0 and the same report. Wrong external SHA is refused.
Root separately recompiled witness and mutant in a fresh runtime, then
ran both:0/1 with the same semantic observations and binary hashes:
baseline99f7fa4daa69653c9a2aad08b91ce8e49fa58aec42012529b6c982235bebdad3
mutantc21a15ef646eb306fed2ac09676f4e8c3d4c09ed94ff0cddcfa27b1515c65b25
Reader also accepts the moved repository copy normal/-O.

Five actual-reader negative controls in RAM are refused normal/-O:
wrong last label, direct lambda1/2 instead of1/3, missing run stage,
signal-labelled exit, altered probe bytes without updating its hash.
Semantic controls consistently rehash the derived capture/manifest;
the positive original passes and original files stay unchanged.
These RAM controls and the root replay are observations of the auditor,
not extra closed payloads. Their first shell-quoted control invocation
failed with SyntaxError before executing the reader; preserved separately,
then corrected. It is not a native failure or a successful negative control.

Provenance and limits
---------------------
The initial diagnostic/preparation stays OPEN and unchanged. No old
capture was relabelled closed: archive/ was recorded afresh after review.
Its README/protocol retain their SOURCE-PREPARATION status at drafting;
the manifest and actual captures describe the later recording.
Pinned source head371d1444f35d27217999e2fe64931fb37b22d9d7aed51f7042e2bec58e206193,
probe932ae7dfc6e4c32c8a11db732c2aadac3f3abd2463ee7c989c571785b3e0ce7a.
The static reader never reruns native code or reads the original build.
System headers/libraries/libm and the full process environment are not
closed; no cross-machine bit-for-bit claim. Signal/timeout handling is
not qualified (including the documented gap around Popen).
No shared engine or registry changed. No general condensation, statistical
projection, FULL completeness, LiDAR growth, GPU/G4 or100ms contract follows.

Read (from this directory)
-------------------------
python3 -B archive/read.py archive --manifest-sha f7876e1b6c192fc9e8306b76f963b527b39d17635059fa2afcf60e70cc0cdd9a
python3 -B -O archive/read.py archive --manifest-sha f7876e1b6c192fc9e8306b76f963b527b39d17635059fa2afcf60e70cc0cdd9a
