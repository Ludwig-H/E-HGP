# NOTE_CLAUDE — session G4 `g4_mesure_v1` : 81/81 runs, mesures reçues

Date : 1er septembre 2026. Reçu durable :
`receipts/session_g4_20260901_d98f47296d67_1788245493/` (commit épinglé
`d98f4729`, GO re-pinné `94c74155`, contrat en six points respecté —
`campaign_status=verifie_non_decisionnel`, validateur code 0, arrêt ciblé
certifié en UNE tentative post-rapatriement, cible vérifiée `TERMINATED`
sur la génération exacte, `issue=arret_certifie_par_le_garde`). Cadre :
`public_status=not_claimed` — MESURES, jamais des claims.

## Parallélisme CPU (phase FILS, 34 runs, médianes de 2 répétitions ABBA)

À n=16000, mur 1 → 48 fils : uniform 184,3 s → 14,15 s (**×13,0**) ;
eight_clusters 232,6 s → 14,83 s (**×15,7**). Fraction série d'Amdahl
mesurée ≈ 4,4-5,7 %. La courbe sature après ~16 fils (16→48 : ×1,61-1,65
pour 3× les fils). À n=50000 : 133,9 / 81,3 / 49,0 s à 8/16/48 fils.
Répétabilité : écarts min-max ≤ 1,5 % partout, invariance du grand-livre
jugée entre TOUS les fils et répétitions.

## GPU (contrôle HISTORIQUE v5, non autoritaire — 16 contrats à digests
`digest_balls`+`digest_all` IDENTIQUES au CPU par famille)

| famille (50k) | cpu | dev | ad | idx | meilleure route |
|---|---|---|---|---|---|
| uniform | 57,1 s | 58,4 | 57,1 | 57,9 | parité (kernel 0,75 s = 1,3 % du mur) |
| eight_clusters | 62,8 s | 63,8 | 63,6 | 62,7 | parité |
| terrain | 16,8 s | 14,3 | 13,8 | **13,5** | **−19,5 %** (kernel 2,9 s ≈ 21 %) |
| scanline_single_pass | 12,9 s | 12,2 | 11,9 | **11,6** | **−9,6 %** |

Lecture honnête : le device accélère le SCAN des lanes ; le gain n'apparaît
que là où le scan pèse dans le mur (régimes surfaciques). Sur les régimes
volumiques, census/fold CPU dominent — Amdahl. Un run par route : jamais une
attribution de gain, une borne d'observation pour le futur port GPU v6.

## Frontière mémoire K=10 (sous plafond RLIMIT_AS 175 GiB, issues typées)

- uniform 200k : **78,8 Go** (0,394 Mo/pt), 225 s — classe 0 ;
- scanline_stationnaire 200k : **22,9 Go** (0,115 Mo/pt), 78 s — le régime
  surfacique est ~3,4× plus léger ;
- uniform 400k : **153,9 Go** (0,385 Mo/pt), 479 s — classe 0, linéaire ;
- uniform 800k : **abort 134 typé** (`std::bad_alloc` sous RLIMIT_AS,
  signal 6 prouvé par le superviseur) après 550 s.

Mur in-memory K=10 mesuré : **n_max ≈ 450k points** sur 180 GiB
(0,385-0,394 Mo/pt, ~458 candidats/pt à 400k — croissant avec n).

## Accord différentiel et bench

8 paires v5≡v6 (4 familles × {32000, 50000}) : objets identiques
(digest_all + 10 forêts). Bench apparié 50k à 48 fils : uniform v5 48,9 s /
v6 49,2 s (parité) ; eight_clusters v5 55,7 s / v6 62,4 s (v6 +12 % sur ce
régime — piste d'optimisation, pas un claim).

## Réponse factuelle aux deux contrats (mesures, sans claim)

1. **50 000 points : OUI, atteint et mesuré** — 49-63 s selon la famille à
   48 fils, objet prouvé identique v5≡v6, GPU v5 à digests égaux.
2. **Dizaines de millions : PAS in-memory** — le mur K=10 mesuré est
   ~450k points (bien plus dur que l'ancienne extrapolation) ; même K=5
   (~0,07-0,08 Mo/pt attendu) plafonnerait vers ~2-2,5M in-memory. Le seul
   chemin vers 10M+ reste le design STREAMÉ d'ECHELLE.md § 3 (K=10@10M ≈
   100 Go RAM + 1,3 To disque + 6-7 h ; K=5@10M ≈ 1 h + 150 Go), disque à
   attacher à la VM ; les plafonds déclarés/refus typés du checkpoint caps
   en sont la fondation. Une session K=5 (axe SMAX_PROFILE, frontière
   étendue 800k→3,2M) est en préparation à la demande de l'exploitant.
