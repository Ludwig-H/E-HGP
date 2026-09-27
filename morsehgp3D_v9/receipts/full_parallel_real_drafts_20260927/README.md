# Vrais drafts : encodeurs natif/W1/W4 — capture close

27 septembre 2026, base `fd1a2c7ee`, autorité `r1/`, premier essai réussi.
Audit CPU local uniquement, moteur inchangé, aucun GPU/GCP. Voir la
[méthode et les limites](../../audits/b_full_parallel_real_drafts_20260927/README.md).

Six commandes Release/Clang ASan/UBSan/LSan, puis quatre mesures Release
closes. Dix lecteurs normal/−O PASS, ainsi que le lecteur post-capture
`summarize.py`. Vingt ordres réels, 120 comparaisons natif/prototype,
chacune aussi vérifiée contre la forêt publiée. Les trois digests et
tailles des drafts restent identiques aux captures précédentes.

| capture | sites | somme médianes natif ms | W1 ms | W4 ms |
| --- | ---: | ---: | ---: | ---: |
| `ng00` sans sol entier | 39 885 | 163,973 | 168,825 | 120,793 |
| `uniform_8000` synthétique | 8 000 | 65,181 | 67,187 | 46,378 |
| `uniform_16000` synthétique | 16 000 | 147,045 | 152,448 | 109,568 |
| `uniform_32000` synthétique | 32 000 | 315,085 | 317,543 | 233,550 |

Sommes de médianes par K1..5, **pas des murs FULL**. Grain256 ; trois
rotations équilibrées des mêmes variantes sur le même draft. Allocations,
copies, threads, validation et dispersion payés ; comparaisons et
destruction des résultats exclues. Observation favorable au W4, hôte
partagé sans affinité imposée et forte variabilité : pas de gain stable
ni de contrat GPU promis. Tous les grands cas ont zéro continuation.

Chaque ordre W4 publie quatre workers parents actifs et 54 créations
de threads sur ses trois répétitions. Les sommes de scratch principal
16A excluent les auxiliaires O(W), piles, allocateur et sorties ; les trois
résultats de réencodage coexistent dans chaque répétition. Les capacités
natives ne représentent pas le pic du processus.

Génération réellement payée : chaînes instrumentées 24,855 s LiDAR,
5,258 / 11,624 / 24,984 s uniformes. Les copies de capture perturbent la
fin de chaîne ; ne pas construire un chrono de moteur fictif en soustrayant
ces sommes. Aucun coût de segmentation nouvellement mesuré.

Build LIVE épinglé, ne pas reconstruire :
`/workspaces/E-HGP/build/v9-audit-full-parallel-real-drafts-20260927-r1/{release,sanitize}`.
Chaque `capture.json` contient les hashes des sources, entrées, commandes,
sorties et huit artefacts de build. Le TSan réussi concerne le gate
structurel séparé `full_parallel_parent_20260927/tsan_r1`, pas cette sonde.
