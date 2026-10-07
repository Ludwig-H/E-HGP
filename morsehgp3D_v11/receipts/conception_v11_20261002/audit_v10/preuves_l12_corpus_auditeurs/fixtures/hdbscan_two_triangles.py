#!/usr/bin/env python3
"""Temoin : HDBSCAN officiel (scikit-learn, jamais reimplemente) sur les deux triangles de la these (variantes entieres).
Cible utilisateur : ABC | DEF avant la fusion globale. On liste les etiquettes pour plusieurs (min_samples, mcs)."""
import json, sys
import numpy as np
import sklearn
from sklearn.cluster import HDBSCAN

out = {"sklearn": sklearn.__version__, "numpy": np.__version__, "cas": []}
for name, shift in (("aretes_plus_courtes", 0), ("pont_plus_court", -2)):
    P = np.array([(268, 3000, 0), (268, 1000, 0), (2000, 2000, 0), (4000 + shift, 2000, 0), (5732 + shift, 3000, 0), (5732 + shift, 1000, 0)], dtype=float)
    for ms in (1, 2, 3):
        for mcs in (2, 3):
            for single in (False, True):
                h = HDBSCAN(min_cluster_size=mcs, min_samples=ms, allow_single_cluster=single, copy=True)
                lab = h.fit_predict(P).tolist()
                target = lab[0] == lab[1] == lab[2] != -1 and lab[3] == lab[4] == lab[5] != -1 and lab[0] != lab[3]
                out["cas"].append(dict(variante=name, min_samples=ms, mcs=mcs, allow_single=single, labels=lab, cible_ABC_DEF=bool(target)))
print(json.dumps(out, indent=1))
ok = sum(c["cible_ABC_DEF"] for c in out["cas"])
print("cas=%d cible_atteinte=%d" % (len(out["cas"]), ok))
