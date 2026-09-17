#!/usr/bin/env python3
"""Compteurs du modele Python independant du front q2 (independent_front_model.py) sur les
fixtures gravees de tests/wspd_front_inheritance_gate.cpp. Le modele a ete ecrit par un
relecteur de la refutation de conception du 17 septembre 2026, a partir du contrat et de
la lecture du code, sans partager ni code ni structure avec le moteur C++ ni avec le rejeu
C++ de la porte. Ecrit independent_front_model_constants.json a cote de ce script."""
import contextlib, importlib.util, io, json, pathlib, sys

here = pathlib.Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("independent_front_model", here / "independent_front_model.py")
model = importlib.util.module_from_spec(spec)
with contextlib.redirect_stdout(io.StringIO()):  # The model prints its own audit matrix when loaded.
    spec.loader.exec_module(model)

grid = [(100 * x, 100 * y, 100 * z) for x in range(4) for y in range(4) for z in range(4)]
clouds = {
    "grid4": grid,
    "k2_reproposed": [(57, 60, 4), (9, 55, 2), (50, 32, 2), (16, 51, 1), (63, 27, 1)],
    "k2_union": [(30, 41, 3), (23, 51, 2), (46, 59, 1), (15, 57, 3), (35, 34, 1)],
    "k2_overcount": [(5, 2, 0), (16, 1, 0), (31, 0, 0), (36, 5, 0), (52, 7, 0)],
}
keys = ["visits", "searches", "steps", "proposed", "in_factors", "h", "credits", "rejected", "emitted", "rej_mass",
        "res_mass", "ext_products", "ext_proposals", "ext_credits", "ext_rejections", "inh_credits", "dups",
        "ext_dups", "inh_rej_defB", "inh_rej_true", "emitted_credits"]
rows = []
for name, points in clouds.items():
    built = model.build(points)
    for kmax in ([2, 5, 10] if name == "grid4" else [2]):
        for factor in (1, 2):
            for inherit in (False, True):
                work, _, rectangles, _ = model.run(points, kmax, 8, factor=factor, limit=None, resume=False,
                                                   inherit=inherit, order_nodes=built)
                rows.append(dict(cloud=name, K=kmax, factor=factor, inherit=inherit,
                                 **{key: work[key] for key in keys}, rectangles=len(rectangles)))
target = here / "independent_front_model_constants.json"
target.write_text(json.dumps(rows, indent=1) + "\n", encoding="utf-8")
print(f"{len(rows)} rows written to {target.name}")
sys.exit(0)
