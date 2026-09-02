# QUESTION aux auditeurs — pré-inscription de la mesure du palier KeyCSR

Date : 2 septembre 2026. Pièces : `REPONSE_AUDITEUR_COMPACTDELTA_CSR_20260902.md`
(Q2 : protocole de mesure à graver à la place du mien), fin du § 5.21 de
`REPONSE_AUDITEURS_MULTICPU_V6_20260901.md` (« nommer estimateur, niveau,
égalités et invalidations ; `1/64` suppose l'indépendance ; unanimité comme
règle d'ingénierie, ou orientations randomisées et pré-enregistrées »),
spécification de la fenêtre par le lecteur E (cartographie du 2 septembre).
Cadre inchangé : `public_status=not_claimed`. Rien ici n'est exécuté : c'est
le protocole que je grave AVANT la campagne, soumis à votre verrou. La
campagne elle-même n'aura lieu qu'après la réception du prototype sémantique
KeyCSR (porte `first_divergence` verte, commit stable) et la fermeture du
harnais de sonde que vous avez demandée (faite à `32da1550`).

## 1. Ce qui est mesuré (deux binaires, un commit)

- **Attribution** : binaire de profil (`mhgp6_profile`, `MHGP6_PROFILE_REDUCE`,
  schéma `reduce_v3` ci-dessous), `--layout=classic|csr` signé sur la ligne
  `profil_kind=`, `csr_fallback` MESURÉ par K (`storage_kind` construit ≠
  demandé) et exigé à 0, `join=1` pour l'étage B isolé.
- **Mur** : binaire `mhgp6` Release non instrumenté, `--threads=8
  --fold-inflight=2 --fold-join=0`, `--digest` off puis on, même commit,
  copies privées hachées avant/après chaque tuple (lanceur du harnais de
  sonde, réutilisé : plan gravé, réagrégation épinglée, ensemble exact).

## 2. Fenêtre `delta_payload_total` (schéma `reduce_v3`, à valider)

Les neuf fenêtres de `reduce_v2` et leur fermeture `somme` restent
intactes. Cinq SOUS-ATTRIBUTIONS nouvelles, imprimées sur la même ligne mais
**hors** de `somme` (elles recouvrent des fenêtres existantes ou vivent hors
du reduce) :

| colonne | où (classique) | où (csr) |
|---|---|---|
| `payload_reserve_ms` | `r.deltas.reserve` (dans `init`) | réserves de `delta_meta`/offsets (dans `init`) |
| `payload_tris_ms` | deux `std::sort` par racine (dans `materialisation_tri_copie`) | idem (listes du scratch) |
| `payload_append_ms` | `r.deltas.push_back(cd)` (copie profonde + allocations) | `append` dans les arènes |
| `payload_meta_ms` | affectation `batch/level/output` | `DeltaMeta` + offsets |
| `payload_destruction_ms` | destruction de `r` sur le fil B, **à l'endroit exact où elle a lieu aujourd'hui** (après `next_publish`/notify, hors verrou de publication ; seules des MARQUES sont ajoutées, le point de destruction ne bouge pas) | idem |

`delta_payload_total = payload_reserve + payload_tris + payload_append +
payload_meta + payload_destruction`, calculée par l'agrégateur, jamais par
le binaire. Une sixième colonne `payload_consomme_ms` chronomètre un
**callback de contrôle** (`on_forest`) qui parcourt toutes les clés de tous
les deltas et imprime leur XOR (sinon l'optimiseur peut l'éliminer) : elle
interdit de déplacer le travail hors de la fenêtre mesurée et n'entre pas
dans le total. `tests/profil_gate.py`, `tests/profil_contre_fixture.py` et
`validate_v6_campaign.py` apprennent `reduce_v3` (les reçus `reduce_v2`
restent immuables et jugés par leur schéma).

Télémétrie d'octets (déterministe à libstdc++ fixée, `size` et `capacity`
séparés, hors fenêtres et après publication) : classique = Σ `capacity` des
vecteurs `parents`/`born` + `deltas.capacity()·sizeof(ComponentDelta)` ;
csr = capacités des deux arènes + métadonnées + offsets ;
`storage_allocations` = nombre de changements réels de `capacity()` observés
(à partir de zéro — rectification reçue).

## 3. Plan, orientations, échauffement

- Tailles décisionnelles : **16 000 et 32 000**, `uniform` ; 8 000 en
  diagnostic ; `eight_clusters` à 32 000 avant tout GO au-delà de `uniform`.
- Par taille : un échauffement par bras (jeté), puis **six blocs appariés**
  (bras adjacents dans le même bloc), trois `AB` et trois `BA`. L'ordre des
  six orientations est **tiré** par Fisher–Yates sur `[AB,AB,AB,BA,BA,BA]`
  avec la graine `sha256("prereg-keycsr-v1:" ‖ commit)` tronquée à 64 bits,
  gravé dans `plan.txt` AVANT le premier run ; l'ordre global des tailles
  (16k, 32k, 8k) est écrit avant le départ. Aucun bloc de remplacement.
- Chaque run : processus neuf, `taskset -c 0-7`, `loadavg` avant/après,
  copie privée hachée avant/après, sorties et codes conservés.

## 4. Estimateur, règles, égalités, invalidations (pré-enregistrés)

Par taille et par bloc `b`, `R_b = delta_payload_total[csr,b] /
delta_payload_total[classique,b]`, chaque terme = Σ sur K ∈ {8, 9, 10} du
même run (K8–K10 ne sont pas trois répétitions ; chaque K est publié comme
contrôle de cohérence seulement).

- **Cible mécanisme** : `max_b R_b ≤ 0,55` sur 16 000 **et** sur 32 000
  (unanimité des six blocs : règle d'ingénierie, aucune p-value revendiquée ;
  l'égalité exacte au seuil compte comme satisfaite). La médiane des `R_b`
  et les six valeurs sont publiées.
- **Garde reduce** : `max_b (temps_fold_mur_ms[csr,b] /
  temps_fold_mur_ms[classique,b]) < 1,00` sur le binaire de profil en
  `join=1` (seul mur contenant les deux destructions), et la même garde sur
  `fold=` ; l'égalité exacte à 1,00 est un échec de garde.
- **Garde mémoire** : offsets exacts et validés, `storage_allocations`
  publié, octets possédés du payload csr ≤ classique sur chaque bloc ; RSS
  (`rss_max_kb`, processus neuf par bras, 32 000, `join=1` puis `join=0`) :
  aucune régression au-delà de la résolution A/A, secondaire.
- **Gain mur** (32 000, binaire Release, `join=0`, 8 fils, digest off puis
  on) : les six paires favorisent csr **et** la médiane du ratio mur est
  ≤ 0,97 — seulement si l'A/A préalable (six blocs classique/classique, même
  plan) montre `max_b |1 − R_b^{A/A}| < 0,03` ; sinon le mur est
  `INCONCLUSIF`.
- **Non-vacuité par run** : `csr_fallback = 0`, nombre de deltas et de clés
  strictement positifs, derniers offsets exacts, arènes réellement utilisées,
  mêmes nombres de deltas/parents/nés et même `digest_all` entre les deux
  bras d'un bloc, `first_divergence` vide sur un run témoin par taille.
- **Invalidations (bloc entier, sans remplacement)** : code de run non nul,
  hash de copie privée changé, `csr_fallback ≠ 0`, non-vacuité violée,
  `loadavg` (1 min) > 2,0 au départ d'un run du bloc, tuple incomplet. Une
  campagne comptant un bloc invalide est `INCONCLUSIF` pour la taille.
- **Verdicts** : `NO-GO_SEMANTIQUE` (toute divergence du comparateur),
  `GO_MECANISME` (cible + gardes), `NO-GO_PERFORMANCE_PALIER` (reçu valide,
  cible manquée nettement : `min_b R_b > 0,55` sur une taille décisionnelle),
  `INCONCLUSIF` (intervalle traversant la porte, A/A insuffisant, bloc
  invalide). Aucun verdict ne touche `public_status` ni ne se généralise
  au-delà de la machine, du commit, des familles et des tailles reçus.

## 5. Questions

1. Le schéma `reduce_v3` (cinq sous-attributions hors `somme` + colonne de
   consommation) et la marque de destruction sur le fil B, hors verrou, à
   l'endroit actuel, sont-ils recevables ?
2. L'unanimité `max_b R_b ≤ 0,55` avec égalité inclusive, et les
   invalidations listées (dont `loadavg > 2,0`), sont-elles la règle que vous
   souhaitez, ou préférez-vous un seuil d'invalidation différent pour la
   charge ?
3. Le tirage des orientations par graine liée au commit vous convient-il, ou
   voulez-vous une graine externe gravée par vous avant la campagne ?

GCP non utilisé par cette question.
