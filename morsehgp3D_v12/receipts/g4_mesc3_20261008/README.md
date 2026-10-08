# Session G4 C3 : petits nuages sur le produit (`MES-C`, régime (c)), et le cache de blocs sur trois petits nuages

8 octobre 2026. Session gardée `v12.20261008.mesc3` (`gcp-migration/v12_session.py`, commit `72f622a55`, preuve
`pushed_commit`), cible `us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f`, `--max-run-seconds 4200`. VM de 09:47:38
à 10:07:34 UTC, **arrêt certifié `TERMINATED`** par le lanceur, relu indépendamment à 10:14:46 UTC. Reçu sans
identité de compte : [`receipt.json`](receipt.json) ; sorties sous `resultats/` (lignes brutes de chaque Session
comprises) ; empreintes : `SHA256SUMS`.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference (voie CPU complète) ; cuda_g4 (catalogue, voie appareil)
objet=full_pi0
quantification=quantized_u21_input_only
public_status=not_claimed
```

**Produit mesuré** : `72f622a55`. C'est la Session recouverte, avec le cache de blocs de 8 Gio par défaut, le lot
T2-d-C, le Pool à équipe, T2-d-A et le bras « census » de T2-d-B. Le paquet et le pilote
([`pilote_c.py`](../../microbancs/mes_c_petits/pilote_c.py), schéma recouvert) sont ceux des sessions C et C2.
Configurations : voies CPU et appareil, K5 et K10, 4 et 48 fils, trois tours ; familles difficiles seules à 48 fils.
Durée : 839 s, toutes les Sessions jouées, aucun contrôle manquant.

## Verdict de `MES-C` : non tenu

| Critère | Mesure | État |
| --- | --- | --- |
| C1 (ordonnée ≤ 2 ms, CPU, K5, 48 fils, nuages réels) | 14,90 ms | non tenu |
| C2 (pente ≤ 3 727,2 ns par site, même configuration) | 10,06 µs par site | non tenu |
| C3 (cohorte difficile complète, K5, deux voies, 48 fils) | quasi-sphère refusée `wide_leaf` à 3 000 et 10 000 sites sur les deux voies | non tenu |

## Comparaison descriptive avec C2 (nuages réels, valeurs chaudes)

| Configuration | t médian, ≤ 150 sites (C2 → C3) | t médian, ≥ 5 000 sites (C2 → C3) | a (C2 → C3) | b (C2 → C3) |
| --- | ---: | ---: | ---: | ---: |
| CPU, K5, 48 fils | 11,13 → 10,88 ms | 115,6 → 101,4 ms | 16,45 → 14,90 ms | 11,57 → 10,06 µs |
| CPU, K5, 4 fils | 6,59 → 6,07 ms | 460,4 → 452,0 ms | −0,52 → −1,28 ms | 55,1 → 54,1 µs |
| appareil, K5, 48 fils | 4,54 → **4,26 ms** | 38,6 → **24,5 ms** | 6,05 → 5,39 ms | 3,73 → **2,18 µs** |
| appareil, K10, 48 fils | 7,4 → 5,74 ms | 139 → 92,8 ms | 8,7 → 5,37 ms | 15,9 → 10,9 µs |

Plusieurs changements séparent les deux sessions : la Session recouverte, T2-d-B et le cache de blocs. La
comparaison est donc descriptive et n'isole aucune cause. Au-delà de 5 000 sites, la voie appareil à 48 fils gagne un
tiers, et sa pente passe de 3,7 à 2,2 µs par site. Sous 150 sites, le coût fixe bouge à peine (4,3 ms sur l'appareil,
10,9 ms sur le CPU à 48 fils).

Familles difficiles à K5 et 48 fils, sur l'appareil (dernière passe) : réseau entier de 100 à 10 000 sites calculé
(10 000 sites : 165 ms, contre 232 ms en C2) ; droite calculée ; quasi-sphère calculée jusqu'à 1 000 sites (243 ms),
refusée `wide_leaf` à 3 000 et 10 000 sites. La voie large T1-c reste à faire.

## Cache de blocs sur trois petits nuages réels (pilote apparié)

Bouts v11 de 156, 1 013 et 4 618 sites, K5, 48 fils, 10 tours × 10 passes. Référence et A/A : voie par défaut, cache
allumé. Bras `sans_cache` : `--cache=0`. Le pilote joué n'avait pas encore les fermetures de l'auditeur.

| Voie | sans_cache / ref (p150, p1000, p5000) | A/A (p150, p1000, p5000) | Jugement |
| --- | --- | --- | --- |
| appareil | 0,997 / 0,999 / 1,025 | 1,000 / 1,002 / **0,973** | refusé (A/A hors de la fenêtre sur p5000) |
| CPU | 0,985 / 0,980 / 0,995 | 0,992 / 0,989 / **1,015** | refusé (A/A en limite de fenêtre sur p5000) |

Tous les intervalles de `sans_cache` contiennent 1 : sur les petits nuages, le cache n'a pas d'effet mesurable, ni
favorable ni défavorable. Les deux campagnes sont refusées par leur contrôle A/A, comme la règle le prévoit. Le bruit
de ces mesures courtes, de quelques millisecondes, dépasse la fenêtre de ±1,5 %.

## Lecture

Le régime (c) reste dominé par le coût fixe. Sur les nuages réels de 100 à 300 sites, la voie appareil est le meilleur
chemin (4,3 ms), et l'essentiel en est le catalogue (environ 3,7 ms en C2 : parcours, feuilles, émission et fin
d'étage, faits de lancements et de synchronisations). Sur la voie CPU, le catalogue de 150 sites coûte environ 8 ms à
4 fils : c'est son travail combinatoire (de l'ordre de 385 000 préfixes et 150 000 tests de census pour un nuage de
156 sites, mesure locale). La coordination des fils n'en est pas la cause. Le critère C1 (2 ms) demande un travail sur
le catalogue lui-même.

## Ce que cette session n'établit pas

Ni le budget de 2 ms, ni la voie large (quasi-sphère), ni l'effet isolé de chacun des changements depuis C2. GCP
utilisé pour cette seule session, arrêt certifié.
