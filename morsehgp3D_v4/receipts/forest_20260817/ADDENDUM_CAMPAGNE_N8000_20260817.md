# Addendum — première campagne à taille contractuelle : n=8000

Date : 17 août 2026. Premier run à une taille d'intérêt du contrat
(`docs/TEST_PLAN_MORSEHGP3D.md` § 3.1), moteur CPU de référence
mono-thread, pipeline filtré complet (scan du cover + W₄ par ancre +
deux passes de profondeur + fold à macro-lots + deltas + rendu).

## Mesure (uniform, n=8000, smax=11, seed 3, 1 thread CPU du conteneur)

```text
candidats émis (q2/q3/q4)   : 313 095 / 1 425 847 / 1 396 285
candidats tués à la gen     : 34 005 697 (q3) + 139 744 809 (q4)
ancres tuées W₄             : 354 156
boules uniques              : 3 134 427   (mortes en profondeur : 31 176)
clés au census complet      : 3 103 251
événements                  : 3 126 158
fusions / nœuds             : 19 465 140 / 1 974 086
désaccords                  : 0 (hors régime jugé — n > 120)

t_gen        : 169,7 s   (52 %)
t_tri        :   1,9 s
t_count-only :  19,3 s
t_census     :  21,4 s
t_fold       : 112,0 s   (35 %)
TOTAL        : ~324 s = 5,4 min
```

## Lecture des pentes (contre n=1600, facteur 5 sur n)

- événements ×5,9 : l'objet reste en ~n·polylog — ~390 événements par
  point, ~1 candidat émis par événement (aucun excès structurel) ;
- `t_gen` ×7,5 : le poste dominant, dominé par les ÉVALUATIONS de
  candidats tués (174 M — ~56 par candidat émis) : c'est exactement la
  marge des deux filtres auditeurs restants (boule intérieure O(1),
  préfixe axial streaming ≤ 16 groupes par seed) ;
- `t_fold` ×8,7 : la pente la plus raide se confirme — `std::map` dans
  `build_forest`/`build_render` ; la réécriture par tri est le second
  chantier d'échelle, GPU-alignée ;
- extrapolation n=32000 : ~25 min mono-thread par run — la matrice
  contractuelle complète (4 familles × 3 tailles × 2 profils = 24 runs)
  tient dans une session G4 de 4 h sur 48 cœurs.

## Session G4 gardée prête (autorisation utilisateur du 17 août)

`gcp-migration/session_campagne_v4_scale_g4.sh` — sur le modèle des
sessions gardées existantes : `set_max_run_duration` (4 h) → clé OS
Login éphémère (TTL 250 min) → `start_and_verify` (handoff de
génération) → rejeu INDÉPENDANT des 89 portes sur la VM (échec = pas de
campagne) → matrice 24 runs mono-thread en parallèle (chaque run borné
`timeout 10800`, floors anti-vacuité, résultats cat au fil de l'eau) →
rapatriement scp → arrêt certifié par trap `stop_and_verify` sur
EXACTEMENT la génération démarrée, fail-closed. La v4 n'ayant pas de
cible CUDA, la G4 sert de ressource CPU (48 vCPU / 180 Go) et de
matériel contractuel des reçus — aucun débit GPU revendiqué, aucun
benchmark ne promeut `public_status=exact`. Ce conteneur n'a ni
`gcloud` ni identifiants (le README GCP impose le terminal local) : le
script est prêt à lancer par l'opérateur.
