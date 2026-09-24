# Trace S2→S3 autonome, un seul quartier sans sol (audit B)

Prototype **audit-only**, hors registre et sans modification des sources du
moteur. L'entrée est le quartier physique `x≥0,y≥0` de la trame SemanticKITTI
`08/000200` sans sol, densité globale `1/4`, grille commune 1 mm/u18 : **1 288
sites exactement**, K10/s8/W8. Le programme refuse tout autre effectif ; il
n'exécute ni la tour FULL ni GPU/G4, ne mesure aucun gain de temps et ne lance
aucune trame complète.

## Données et lien au reçu

Entrées publiées dans
`../s4a_cpu_scene02_physical_panel_20260924/inputs/` :

- `quarter_quarter_x_nonneg_y_nonneg.u32le`, SHA-256
  `33630aea9492d3b059001e10c74ba30bec143dd7a1a0be6b25f8701b2f2a5e8f` ;
- `quarter_quarter_x_nonneg_y_nonneg.raw_return_ids.u32le`, SHA-256
  `d473e6314cf322c9213998906d3c43f59ee5044b809a8545fe4febe9113e053a`.

Le comparateur est le stdout S3
`../s4a_cpu_scene02_physical_panel_20260924/runs/attempt_0042_quarter_quarter_x_nonneg_y_nonneg_s3.stdout`.
Le `BUILD_PROVENANCE.json` de ce panneau attache ce stdout au binaire moteur
SHA-256 `eea3040cf4150599ca7ab5c71a4f0e3738f2975483584527f339d3d2600d08e9`, reconstruit
depuis le commit `7ceadffad1de860e30325ae357ba3243f269d48b`. `verify.py`
contrôle les SHA des deux entrées, cette identité binaire historique et les
ledgers du stdout. Le sidecar, lui, a été compilé dans le clone isolé au HEAD
`605f39be4982b17a213113546a19728dc38e87fd` : **ce n'est pas le binaire
historique**. Son SHA-256 était
`f4af8dab5b5e1e066a4a261c822730077019fbd1266d0b8b246271c9f11e1c67`
et celui de sa bibliothèque `mhgp9_gen`
`e17932e499905ae2ae389b243b1fdf973303091719d55514379abce978f395b0`.
Les fonctions publiques S2/S3 appelées et `front.cpp` n'ont pas changé entre
les deux commits ; `wspd_q34.cpp` a néanmoins reçu des changements S4,
ledger et `after_front`. Les comptes observés sont confrontés au reçu, pas
un chrono transféré du binaire historique.

## Construction et rejeu borné

Depuis la racine du clone isolé :

```sh
cmake -S morsehgp3D_v9/audits/b_s2_trace_20260924 \
  -B /tmp/mhgp9-audit-s2trace-build -DCMAKE_BUILD_TYPE=Release \
  -DMHGP9_BOOST_INCLUDE_DIR=/workspaces/E-HGP/build/v7_boost_gate/extracted/usr/include
cmake --build /tmp/mhgp9-audit-s2trace-build --target mhgp9_b_s2_trace -j2
/tmp/mhgp9-audit-s2trace-build/mhgp9_b_s2_trace \
  morsehgp3D_v9/audits/s4a_cpu_scene02_physical_panel_20260924/inputs/quarter_quarter_x_nonneg_y_nonneg.u32le \
  morsehgp3D_v9/audits/s4a_cpu_scene02_physical_panel_20260924/inputs/quarter_quarter_x_nonneg_y_nonneg.raw_return_ids.u32le \
  morsehgp3D_v9/audits/b_s2_trace_20260924/quarter_1288.trace.tsv \
  morsehgp3D_v9/audits/b_s2_trace_20260924/quarter_1288.segments.tsv
python3 -B morsehgp3D_v9/audits/b_s2_trace_20260924/verify.py
```

Le binaire refuse de remplacer des fichiers existants : supprimer ou choisir
de nouveaux chemins de sortie après avoir conservé les anciens reçus. Il
reconstruit l'index une fois, puis le front public `make_wspd_front_jobs`
mass-first avec 512 jobs, concaténés en ordre de job comme le produit ; S2 et
S3 utilisent leurs fonctions CPU publiques. Le seul rejeu supplémentaire est
le core diamétral `Q34EdgeCover::make_diametral` et son prover public pour
attacher (F_e) et le masque post-core à chaque arête. Ce coût d'audit n'est
pas inclus dans une proposition de chemin produit.

## Sorties et vérifications

`quarter_1288.trace.tsv` (55 657 lignes de données, 2 198 079 octets,
SHA-256 `6bb07e89ed89b514278e58e7d635459d8ec50f67af82d260d7ecf22e119d1d6e`)
porte l'ordinal S2, l'ordinal du rectangle, les deux IDs locaux et bruts,
le masque S2, (F_e), puis les masques post-core et post-S3. Les bits 2/4
désignent q3/q4. `quarter_1288.segments.tsv` (52 842 rectangles,
1 020 158 octets, SHA-256
`6f3428e189891fd5be179272dbe3e183575b2795b16fd44e3ba724a24f4c1af3`)
donne `[begin,end)` pour **chaque** rectangle, même vide ou terminal, dans
la même numérotation ; dernier `end=55 657`. Les ordinals sont des ordinals
de ce rejeu et ne doivent pas être joints aux ordinals d'un run GPU distinct.

La relecture indépendante des fichiers retrouve R=52 842, P=95 830, S=55 657,
`core_builds=dead_core_loads=S`, Σ(F_e)=1 151 766,
`core_closed_edges=15 657`, aucun différé ni masque élargi, IDs bruts
uniques, et offsets couvrant chaque arête exactement une fois. Le prover
core recoupe les 11 734 preuves q3 et 15 940 preuves q4 du reçu.

| Masse (F_e) de baseline | Sites cumulés |
| --- | ---: |
| q3 prouvé par core | 416 215 |
| q4 prouvé par core | 599 911 |
| union des deux (sans double compte) | 662 790 |
| intersection des deux | 353 336 |
| arêtes complètement fermées par core | 610 738 |

Ces masses sont **des expositions du travail du core déjà payé**, pas un
travail réellement économisé. q3 et q4 se chevauchent et ne s'additionnent
pas ; une preuve pré-core indépendante doit identifier ses propres arêtes
et sommer leurs (F_e) avant de parler de coût évité. Un shadow peut joindre
sa décision exacte aux ordinals de `trace.tsv`, puis compter séparément q3,
q4, union et fermetures de l'arête entière. Les 171 rectangles de 16
survivants ou plus ne portent que 5 223 arêtes et 236 483 (F_e) (20,5 %
de la masse totale), raison de pondérer toute priorité de groupe par (F_e)
plutôt que par sa seule cardinalité.

La relecture vérifie la structure, les mappings, les sommes et la concordance
au reçu historique ; les décisions géométriques du sidecar réutilisent les
API publiques du moteur et ne constituent pas un oracle géométrique autonome.

## Jointure d'un futur shadow pré-core

`join_shadow.py` accepte un TSV **clairsemé** avec exactement deux colonnes,
`s2_ordinal<TAB>proved_mask` (en-tête compris). Une arête absente a masque 0.
Chaque bit prouvé doit être demandé par le masque S2 de cette arête ; le
lecteur refuse les doublons, ordinals hors `[0,S)`, lignes mal formées et bits
absents de S2. Il contrôle le SHA du TSV de trace contre `RESULT.json` avant
la jointure. Exemple neutre, sans prétendre disposer d'un certificat :

```sh
python3 -B morsehgp3D_v9/audits/b_s2_trace_20260924/join_shadow.py \
  morsehgp3D_v9/audits/b_s2_trace_20260924/fixtures/empty.tsv
```

La sortie neutre vérifiée donne `full_closed_edges=0`,
`eligible_core_skip_F=0`, `q3_proved_F=0`, `q4_proved_F=0`. Un ordinal n'est
compté dans `eligible_core_skip_F` que si son `proved_mask` ferme **toutes**
les voies encore demandées par S2 ; les masses q3 et q4 sont publiées
séparément, avec union et intersection, et ne sont pas additives. Ce sont des
**éligibilités conditionnelles à la validité de la preuve shadow**, pas des
économies mesurées : le sidecar ne branche pas la preuve dans le moteur.

Contrôles de refus, exécutés en lectures normale et `-O` : la fixture
`forbidden_q4.tsv` tente le bit q4 à l'ordinal 124 dont S2 n'a demandé que
q3, et se termine au code 1 avec `decision proves a lane absent at ordinal
124`. `duplicate.tsv` et `out_of_bounds.tsv` se terminent également au code
1 avec leurs causes respectives ; le fichier vide termine au code 0.
