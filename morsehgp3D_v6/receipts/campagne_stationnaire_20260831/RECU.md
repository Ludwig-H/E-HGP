# Reçu — campagne stationnaire J3 (31 août 2026, pin du correctif cover)

Cadre : `phase=exploration_v6_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `public_status=not_claimed`.
Provenance complète dans `META.txt` (SHA du pin, hash du binaire, toolchain,
commande, heures, hashes des 36 sorties). 36/36 runs code 0, stderr vides
(`STATUS.txt`). Compteurs déterministes seulement ; les temps de cette
machine ne sont pas des mesures. Analyse : `PENTES.txt`
(`bench/pentes.py`, pentes sécantes locales par terme, jamais une somme).

## Portée

Quatre familles × {8000, 16000, 32000} × graines {3, 4, 5} : `uniform`,
`eight_clusters` (dilatées de référence), `terrain_stationnaire`,
`scanline_stationnaire` (régimes synthétiques stationnaires, physiquement
motivés — REGIMES.md § 2). Le grand-livre publié couvre notamment
`W_sweep1` (= `tests_coeur`, masse du scan cœur+corde de la passe 1, sur le
cover q4 au COEFFICIENT 4 corrigé), `W_scan_q3`, racines du sweep, `P_role`,
ancres, seeds, candidats, sorties. Termes encore `(candidat J3)` non
couverts : `H_rect/H_scan/M_anchor`, `T_input` (la fabrication des familles
stationnaires est elle-même quadratique en interne — coût d'entrée, hors
pipeline), `V_census`, HWM par rôle.

## Verdict par régime (pentes sécantes, deux pas, trois graines)

- **uniform** : tous les termes publiés dans `[1,03 ; 1,20]`, étendues
  ≤ 0,03. Sous-quadratique par compteurs sur tout le grand-livre publié.
- **eight_clusters** : tous les termes < 1,70 ; les plus hauts sont
  `ancres_q4` (1,67–1,69 au second pas, étendue 0,02 — les paires
  inter-amas, effet connu) et `sweep_hors_corde` (1,47–1,62). Étendues
  minuscules : le signal est reproductible. Sous-quadratique partout ;
  la croissance des ancres inter-amas est la cible naturelle de la route M.
- **terrain_stationnaire** : sorties et la plupart des termes ~1,0–1,4 ;
  MAIS `W_sweep1` atteint **2,08** au second pas sur la graine 5
  (1,32/1,30 sur les graines 3/4 ; étendue 0,78). Aucune pente n'est
  établie pour ce terme : queue lourde mono-graine.
- **scanline_stationnaire** : sorties ~1,0 ; MAIS `W_sweep1` atteint
  **2,41** au second pas sur la graine 5 (0,91/1,21 ailleurs ; étendue
  1,49) et `seeds_q4` 1,54 sur la même graine. Même diagnostic.

## Conséquence contractuelle (docs/GRAND_LIVRE.md § 3)

Le déclencheur E6 est ACTIVÉ proprement : `W_sweep1` présente une pente
≥ 2 sur au moins une graine des deux régimes stationnaires de surface. Ce
n'est pas un échec de correction : c'est le signal mesuré, reproductible et
falsifiable que le grand-livre était conçu pour produire. Le prochain
chantier justifié est la réduction de la masse de scan par seed sur les
ancres lourdes (Tier R par rectangle, moteur plan par ancre lourde, ou
crédits résiduels du contrat 2), PRÉCÉDÉ de sa sonde contrefactuelle
appariée, et d'un diagnostic de la queue (distribution de `m_e` par octave,
part des ancres lourdes dans `W_sweep1` — compteurs candidat J3 à câbler).
Toute conclusion « le régime est sous-quadratique » reste bornée aux termes
publiés qui le sont ; aucune pente n'est promue pour `W_sweep1` sur les
familles stationnaires de surface.

Nota : `W_sweep1` mesure le cover COEFFICIENT 4 (correctif P0 du 31 août) —
la masse est structurellement plus grande que le coefficient 3 v5 ; les
comparaisons aux diagnostics v5 (`q4_core_site_tests` sur coef 3) changent
d'assiette et ne se comparent pas terme à terme.

## Ce que ce reçu ne dit pas

Aucun temps, aucun claim GPU, aucune exactitude HGP, aucune extrapolation
au-delà de 32000, aucune pente pour les termes `(candidat J3)` non publiés.
GCP non utilisé.
