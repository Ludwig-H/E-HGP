LiDAR halo: exact checks at observed column argmax anchors

Scope: grille 1 mm u18, CPU local diagnostic, no HGP engine, no GCP.
The six entries are raw/nonground x frames 08/000000, 08/000100, 08/000200;
they remain ONE sequence, not six independent SemanticKITTI scenes.
K is 5 or 10, includes the site itself. The whole entry is scanned.
The fixed nonground mask is inherited, not semantically requalified.

The untouched original archive is /tmp/lidar-halo-locality.i8TpNR3i.
Its external manifest.json SHA256 is
d7aa259033edb4ffebd60b359087c4873fd670f155efc651c4c8e7923fbdb741.
Original halo.py SHA256 is
4556cee1d5fa74792e58ea3518fdd0ed42a431d66e53263b2ffccdb36fbdfcdd.
Neither halo.py nor record.py nor the original archive reader was executed
or imported. All original archive files and relevant live inputs were
hashed before and after. Original normal/-O semantic equality was checked.

For each of the 12 FULL entry/K cases, select the FIRST archived argmax in
each of the four count columns. At most four anchors are selected per case.
For each unique selected anchor compute all n squared integer distances;
the Kth value (rank K-1, self included) is exact. Apply these exact tests:
quarter_lower: 4*d2 <= 5*dK2; quarter_upper: d2 <= 5*dK2;
one_lower: d2 <= 2*dK2; one_upper: d2 <= 8*dK2.
Every selected dK2 and all four counts must equal the archived vector row.
Since coordinates are in [0,2^18), squared distance <= 3*(2^18-1)^2;
even the largest coefficient 8 keeps all intermediates below 2^63.

Actual runs normal/-O (UTC is captured in run_receipt.json): code 0 both,
byte-identical JSON, 12 cases, 48 column maxima, 28 unique anchor scans,
2,124,208 site-distance evaluations per run. Each run took approximately
2.2 seconds on the local machine, diagnostic execution time only.
JSON contains exact coordinates, site IDs, every checked row, and source,
input, vector, runtime pins before/after. No large input payload is copied.
NumPy is the only numerical dependency; no SciPy/KD-tree is used here.
The tool output is combined stdout/stderr, not a separate stderr capture.

Each reported archived maximum is now exactly REALIZED by a checked anchor.
It is a proven lower bound on the true maximum occupation, not a proof of
the GLOBAL maximum: the other anchors are still sourced from cKDTree.
q25/q50/q75 and other nearest-rank quantiles are independently computed
over the archived whole vectors, not promoted to globally exact halo data.
No conclusion on MEB classes, exact alpha, scaling, hierarchy, FULL or G4.
In particular, a small median does not erase large realized tail counts.

Reader: python3 -B read.py ROOT EXTERNAL_MANIFEST_SHA256 (also -O).
It hashes the external manifest and closed payload inventory BEFORE JSON
or numerical imports, verifies LIVE external pins, then independently
rechecks selected anchors by SORTING full integer squared-distance arrays
(not the probe's partition selection). It never imports check.py or halo.py.
It also rejudges the argmax indices and archived-vector quantiles. No native
or original generator is replayed. It rehashes payloads and inputs afterward.
This compact receipt is LIVE-input dependent, not an autonomous data archive.
Closed scripts/captures must not be rewritten. Readback is read-only.
