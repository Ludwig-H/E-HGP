# Atlas → terminales → certificats → FULL : prototype CPU borné

Ce paquet différentiel conserve une couture complète **privée et non
intégrée** depuis de vrais census : atlas exact, terminales géométriques,
pivots, graphes, certificats composables, histoires FULL, contributions
datées et cartes verticales. `public_status=not_claimed`. GCP non utilisé,
aucun GPU ni contrat de vitesse revendiqué.

Le chemin terminal de l'extraction est le helper scalaire **test-only**
`Geometry`, confronté aux BallId directs du vrai Builder statique. Le census
complet exact reste une précondition de l'API. La gate ROOT ajoute ses juges
bornés indépendants et une comparaison du payload FULL avec la référence ;
cela ne promeut pas une complétude WSPD ou une exactitude industrielle globale.

## Résultats fermés

| Qualification | O2 et SAN | Portée |
| --- | --- | --- |
| Extraction | 11 commandes chacun | 12 census, 52 307 terminales directes, 30 422 marques, 35 862 arêtes réduites ; six fautes d'état réfutées |
| Certificat MSF composable | 4 commandes chacun | 56 variantes, 522 couples de coupes, 65 refus, quatre mutations ; graphes abstraits, pas nouvelle géométrie |
| Couture FULL ROOT | 16 commandes chacun | 114 census : 96 petits, 12 haut ordre K10, 6 uniform32 ; sept fautes/réjections causales |

Les 62 commandes de qualification hors smoke sont closes ; les sorties C++ correspondantes
O2/SAN sont identiques. La petite smoke initiale reste séparée : quatre
commandes rapportées par sorties d'outil combinées, 30 contrôles sur pair/
singleton, pas de faux stdout/stderr recomposés.
Soit 66 commandes en comptant ces quatre commandes historiques séparées.

ROOT compare les nœuds et parents, les populations I/U, les contributions
avec date et masque, et les images verticales via une correspondance
explicite d'identités natives. **Ce n'est pas une égalité brute des numéros
internes des deux exports.** Les identités ne sont jamais quotientées par
les ensembles de points couverts. Compteurs comparatifs (répétitions entre
routes/threads comprises) : 237 840 nœuds/images verticales, 150 240
contributions datées ; 29 784 coupes et 15 594 832 contrôles verticaux d'oracle
sur les seuls petits cas et cas haut ordre. Uniform32 reste différentiel,
avec zéro appel d'oracle exhaustif.

Deux voies sont confrontées : graphe de naissances natives puis compaction,
et graphe des hubs avec admission suivie de sa projection pondérée. Chacune
utilise trois largeurs de fenêtres ; les consultations/export sont comparés
avec 1 et 4 workers. À n32, 135 des 180 certificats projetés de hubs diffèrent
de leur certificat natif apparié tout en redonnant le même objet FULL testé.
Ce n'est pas une exigence d'égalité des arêtes MSF.

Les sept causes ROOT conservées sont : contribution de naissance mal datée,
image verticale fausse, parent natif dupliqué, date du nœud erronée, feuille
descendante invalide, masque de contribution faux, ancre native fausse.
Le lecteur vérifie chaque cause et code 4, les refus CLI 2/2, ainsi que la
matrice exacte des fixtures/permutations/s8/10/12 réellement exécutées.

## Lire le code et reproduire

Les fichiers nouveaux sont dédupliqués par SHA, pas masqués derrière un
format compressé. Le moyen simple de les lire ensemble consiste à extraire
l'arbre autonome, qui conserve tous les includes relatifs :

```bash
python3 -B morsehgp3D_v7/receipts/atlas_graph_full_20260911/verify.py
python3 -B -O morsehgp3D_v7/receipts/atlas_graph_full_20260911/verify.py
python3 -B morsehgp3D_v7/receipts/atlas_graph_full_20260911/verify.py --extract /tmp/mhgp7-atlas-graph-full-copy
```

La destination doit être nouvelle. Les sources principales apparaissent ici :

| Source | Chemin sous `/tmp/mhgp7-atlas-graph-full-copy/` |
| --- | --- |
| Extraction | `build/v7_atlas_graph_20260911/atlas_graph.hpp` |
| Certificat composable | `build/v7_composable_msf_20260911/composable_msf.hpp` |
| Export FULL | `build/v7_graph_full_20260911/graph_full.hpp` |
| Juge FULL ROOT | `build/v7_graph_full_20260911/tower_gate.cpp` |
| Recorder FULL exact | `build/v7_graph_full_20260911/record_tower.py` |

Dans cette copie, les deux recorders sans Boost peuvent être relancés dans
de nouveaux sous-dossiers : `build/v7_atlas_graph_20260911/record.py --out
replay_o2` et `build/v7_composable_msf_20260911/record.py --out replay_o2` avec
`python3 -B`. Pour le TU ROOT, exemple de compilation fraîche depuis la copie :

```bash
g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -pthread -isystem /chemin/boost/include -I build/v7_atlas_graph_20260911 build/v7_graph_full_20260911/tower_gate.cpp -o /tmp/mhgp7-atlas-graph-full-new-gate
/tmp/mhgp7-atlas-graph-full-new-gate --selftest
/tmp/mhgp7-atlas-graph-full-new-gate --high
/tmp/mhgp7-atlas-graph-full-new-gate --uniform32
```

Le recorder ROOT original garde son chemin Boost historique explicite. Les
commandes capturées ne sont pas réécrites comme si elles avaient été lancées
dans cette copie ; une compilation fraîche est une nouvelle qualification.
G++/libc/Boost sont externes, sans ELF ni vendor embarqués. ROOT épingle le
compilateur et les headers préprocesseur avant/après, pas tout le linker/
sysroot runtime. Le petit MSF ne capture pas de pin système/compilateur :
cette limite reste explicitement conservée.

## Dépendance différentielle et intégrité

Le seul parent requis pour lire/extracter ce paquet est
[rank_atlas_20260911](../rank_atlas_20260911/README.md), manifeste
`341c8c228a9d008084010db7b010adac77beefba123fa8a59d1d7c9023652013`.
Les fichiers corefriend et Atlas byte-identiques sont référencés par
chemin/taille/SHA dans `MANIFEST.json`, pas recopiés. Les ajouts T2 et le TU
original sont conservés avec leurs propres empreintes ; origine déclarée
f2bea998, aucune consultation Git nécessaire au replay.

`freeze.json` garde l'inventaire approuvé par ROOT. Le lecteur vérifie sa
correspondance exacte au manifeste, les quatre fichiers de paquet, tous les
objets/fichiers parent, l'inventaire physique sans objet caché, les captures,
les sources réellement compilées, les fermetures pré/post et les causes.
Il ne compile rien et n'accède ni à Git ni au réseau. Un parent absent ou
altéré est un refus. L'extraction crée tous les fichiers de source/capture,
sans lien symbolique : l'arbre obtenu est autonome du parent.

Les cartes de pins de l'extraction O2 et SAN sont strictement identiques :
les trois sources T2 étaient déjà présentes avant la première capture.
Le premier preflight du lecteur a réfuté l'hypothèse documentaire inverse ;
cet échec de lecteur est conservé en privé, ce n'est pas un échec moteur.
Chaque carte et chaque snapshot est vérifié séparément. L'échec
historique ROOT sans chemin Boost est seulement rapporté comme tel : son TU
initial et ses flux n'avaient pas été figés, ils ne sont pas inventés ici.

## Limites de résidence et d'industrialisation

Cette preuve conserve encore tout `targets[R]`, φ[A], les graphes et les
marques. Les fenêtres MSF réduisent leur propre entrée intermédiaire ; elles
ne prouvent pas une résidence globale fenêtrée de la génération à l'export.
Les graphes/histoires sont construits séquentiellement ; 1/4 workers portent
les consultations, pas une parallélisation déjà réalisée de toutes phases.
Aucune mesure RSS/VRAM/latence industrielle ni borne universelle
sous-quadratique en nombre de points n'est revendiquée. Aucun code actif
n'est changé par ce paquet et aucun contrat 50k/G4 n'est acquis ici.
