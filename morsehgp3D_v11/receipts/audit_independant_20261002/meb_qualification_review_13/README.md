# MEB1 : campagne fermée en échec — 2 octobre 2026

**MEB1 ne qualifie pas l'extension MEB.** Sa source exécutée est
`ab04bc7b1ef61b9996eb7ec15db4fd4e5f2d511a`. La campagne conserve ses échecs,
son archive et sa fermeture G4. Le correctif de sérialisation publié ensuite
dans `25792084eb4e672c5222d62f5b2ae87bd2ee4948` n'a pas été exécuté par MEB1.
Aucun build, produit natif ni appel GCP n'a été lancé par cet audit.

Les copies précèdent les dérivations : [sources avant](sources_before.json),
[paquet avant](package_before.json), [reçu original avant](extra_before.json),
[sources après](sources_after.json). Toutes les pièces LIVE capturées sont
inchangées à la recoupe finale. Le contexte fourni par l'auditeur math/API
est conservé, sans réécriture, sous [contexte_math_api](contexte_math_api/SOURCE_BEFORE.json) ;
sa première erreur de chemin de capture y reste attribuée à l'audit.

| Configuration | Portes passées / sélectionnées | Échecs |
| --- | ---: | ---: |
| GCC Release B18 | 268 / 270 | 2 |
| GCC ASan/UBSan B24 | 193 / 195 | 2 |
| GCC TSan B21 | 193 / 195 | 2 |
| profils B21 et B24, chacun | 193 / 195 | 2 |
| poison B21 | 194 / 196 | 2 |
| mutants | 18 / 18 | 0 |
| style | 2 / 2 | 0 |
| **matrice principale** | **1 254 / 1 266** | **12** |
| **complément ASan18 num/index/tower** | **53 / 55** | **2** |

Clang est absent et facultatif. Par rapport aux 1 149 portes du précédent
index, la nouvelle sélection ajoute 19 portes tower par configuration native
et trois portes du harnais mutants, soit 117 portes ; 12 de ces nouvelles
portes échouent. Le complément précédent de 36 portes gagne 19 portes,
dont deux échouent. Ces différences de sélection ne transfèrent aucune
qualification de l'index vers MEB. Les unités tower, refus, propriété, budgets,
concurrence, starvation et oracles Fraction passent dans la capture ; les
deux portes IO empêchent le verdict de qualification complète.

Les [14 échecs](failed_gates_initial.json) sont exclusivement
`mhgp11_tower_bench_io` et `mhgp11_tower_bench_io_opt`. Hors ASan, l'enfant
réussit puis `meb_io_test.py:42` → `meb_semantic.py:132` refuse
`centre MEB hors domaine`. Sous ASan24 et ASan18, `meb_io_test.py:41` refuse
`tiny native success` : le retour enfant est non nul, sans diagnostic enfant
exposé dans le JUnit. Le wrapper retourne 1, attendu 0.
Ces symptômes sont compatibles avec le défaut C++20 établi dans
[le sérialiseur ab04](contexte_math_api/failed_pin/morsehgp3D_v11/bench/meb_probe.cpp)
ligne 37 : référence aux coordonnées du Point temporaire rendu par valeur.
[La version 257](contexte_math_api/fixed_pin/morsehgp3D_v11/bench/meb_probe.cpp)
conserve ce Point localement. Ceci n'est ni un défaut démontré du calcul de
MEB ni un diagnostic ASan démontré par les traces. La garde sur le centre
reste correcte : une MEB portée positivement a son centre dans l'enveloppe
convexe de ses sites, donc dans leur boîte de coordonnées.

Les [mutants observés](mutants_observed.json) comprennent 141 mutations :
136 morts par code, trois par ligne attendue, deux refus de construction
core hérités, aucun signal/délai. Les dix mutants tower sont tous morts par
juge, sans refus de construction. Les 18 portes du harnais ne sont pas
141 portes CTest. Les sorties complètes LastTest sont recoupées par le lecteur.

Le [paquet](raw/package/package.tar.gz) de 26 447 056 octets a le SHA publié
`f059bb237a8a0948a219834ea988d44475691bbaf69ee4a5aacb1fbf016c7dca`.
Ses 2 578 fichiers réguliers, 46 067 505 octets expansés, sont exactement les
blobs Git ab04 du périmètre emballé : [inventaire](package_git_inventory.json).
L'archive de résultats a 121 fichiers et 3 010 625 octets ; son
[MANIFEST](extraits/results/MANIFEST.sha256) couvre exactement les 120 autres
fichiers, hashes vérifiés. Les [provenances](binary_pins.json) conservent les
hashes des exécutables, caches et flags ; les exécutables natifs eux-mêmes
ne sont pas archivés. Les fichiers d'entrée uploadés ne subsistent pas dans
le dossier package/data, où reste leur SHA256SUMS ; manifest et hashes du
reçu sont recoupés. Aucun chrono de grand nuage n'en découle.

Les trois groupes sont fermés avec codes **1 / 1 / 2** et temps mur
**172,323 / 15,362 / 0,038 s**. Le banc est refusé avant lancement de ses
processus natifs : **zéro essai, 18 non joués**, aucun résultat MEB/FULL/GPU.
Les métadonnées conservent `group_closed=1`, aucun groupe résiduel tué,
aucun flux tronqué, aucune interruption. Le [reçu de session](session_facts.json)
lie génération et machine arrêtée : départ 11:48:12.904−07:00, arrêt
11:55:10.913−07:00, état TERMINATED, clés retirées et réserve libérée.
Ces faits ne certifient pas l'isolation de mesures de performances absentes.

[Replays Python](logs/commands.json), normal/−O : lecteur figé, son autocontrôle
(20 positifs / 79 corruptions, zéro natif), et [notre contrôle autonome](review.py)
passent. Code 0 du lecteur signifie **preuve cohérente d'une campagne en échec**.
L'adaptateur portable redirige seulement le chemin du reçu brut vers sa copie
byte-identique ; les contrôles de hash et de transport restent actifs.
MEB2 et tout chantier ultérieur sont hors de cette capsule.

[LEDGER.json](LEDGER.json) exclut uniquement lui-même et SHA256SUMS racine.
[SHA256SUMS](SHA256SUMS) couvre tous les fichiers sauf lui-même, y compris le
SHA256SUMS imbriqué. Les reçus et capsules précédemment fermés restent intacts.
