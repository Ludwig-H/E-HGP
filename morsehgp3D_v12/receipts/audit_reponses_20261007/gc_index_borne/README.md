# G-c : index borné sous collisions et file G-L7

Exploration v12, CPU de référence, FULL pi0, entrée u21, `public_status=not_claimed`.
Lecture de **repo2**, désigné par `build_r2/CMakeCache.txt` (22:50:35), et modèle Python léger ; aucun natif/GCP.
Les hashes de [capture.json](capture.json) figent les corps lus, notamment `populations.cpp` à `0c002ae8…`.
Les onze sources/tests de calcul restent identiques à la fermeture ; le pilote évolue pendant cette lecture.
Ce reçu porte sur le prototype actif, pas une livraison produit ni un gain mesuré.

**Apport nouveau confirmé :** la recherche exacte est désormais logarithmique même sous collisions totales.
La construction peut encore concentrer son tri dans une tâche ; G-L5 reste une série de dichotomies après tri des
requêtes. La file G-L7 conserve la politique de résolution. Les frontières temporelles ont aussi été corrigées.

## Recherche et construction : deux bornes différentes

Soient B les naissances d'un ordre, E celles de population complète de cardinal k (`p+m=k`), R ses représentants,
D la puissance de deux immédiatement supérieure ou égale à max(1,E), et e_j l'effectif du seau j.

- `populations.cpp:106–203` fusionne I et U, trie par `(hash,population complète)` et exige l'ordre **strict**.
  Deux populations égales ont le même hash et sont adjacentes : elles sont refusées, même si leurs naissances/rangs
  annoncés diffèrent. Ce refus est légitime : S* est dans cette population, donc son unique plus petite boule est b.
  Le filtre `p+m=k` reste présent : aucune admission implicite de toutes les naissances étendues.
- `lower_bound/find` (`populations.cpp:221–238`) fait O(log(E+1)) comparaisons, chacune au plus O(k). L'égalité finale
  porte sur tous les SiteIdx. Le hash ne décide jamais l'identité. `candidate` cherche la première ligne de hash égal ;
  `verify` teste cette ligne puis cherche exactement dans le suffixe. Ce suffixe suffit **parce que** le candidat est
  le premier de ce hash, condition garantie par l'unique producteur actuel.
- Construction : O(B+kE), tri radix des bits du seau, répertoire O(D+E), puis
  O(k Σ e_j log(e_j+1)) pour les tris locaux et O(kE) pour les fiches. Au masque zéro, tout est dans un seau :
  O(kE log(E+1)) comparaisons au pire, avec **une seule tâche de tri** (`Layout::buckets`, ligne168).
  La borne de recherche ne promet donc ni une construction linéaire ni son accélération à 48 fils.
- Hors histogrammes et petits index, les tampons tenus représentent environ `(8k+48)E + 4(D+1)` octets : fiches,
  populations denses et deux tableaux d'entrées sont conservés. `bytes_for` et l'admission utilisent toutes les
  naissances comme majorant. Le comptage et le remplissage sont des visites distinctes autorisées par le protocole.

## G-L5 et G-L7

`first_probes.cpp:53–63` n'effectue pas une fusion monotone entre deux listes : après le tri des requêtes **par seau**,
il appelle `candidate` pour chacune. Coût de recherche O(R log(E+1)), puis vérification exacte O(kR log(E+1)) au pire,
plus génération et tri des requêtes. La collision totale n'entraîne plus de produit E×R. Cette variante n'est pas
la jointure linéaire sur clés complètes proposée dans le reçu `g_l5_proposition` ; aucune qualification GPU n'en découle.

La voie active (`passes.cpp:152`) est G-L7. La file de 16 cases (`passes.cpp:64–124`) forme les mêmes traces, prépare
le répertoire à l'entrée et la première fiche du seau huit entrées plus tard, puis résout en FIFO. Chaque case garde
son représentant et sa cellule ; la vidange conserve cet ordre. Aucun nouveau tableau de tas proportionnel à R,
mais une petite file de pile par tâche active, O(16k), donc pas une mémoire littéralement nulle.

`resolve.cpp:265–294` compte toujours une sonde logique initiale, contrôle le rang initial avant un succès, puis
reprend après un échec sans rejouer cette sonde. Les sondes ultérieures utilisent le même index exact ; propositions,
certificats, sauts et census ne changent pas avec cette anticipation mémoire. La comparaison de TARG reste donc
pertinente sous politique `v12_indices` fixée, contrairement à une comparaison entre politiques de saut distinctes.

## Coûts et raccord de mesure à actualiser

**Correction par rapport au prototype décrit dans `t2c_tables` :** `passes.cpp:185–193` additionne maintenant des
`tables_ns`, `joins_ns` et `resolve_ns` disjoints, dans l'enveloppe `orders_ns`. `tower.hpp:144–152` définit aussi
prepare/count/setup/fill/workspace. Le reste correct est calculé par passe :
`wall − (prepare + count + setup + fill + workspace + orders)`. Ne pas ajouter les trois sous-durées à `orders`.
L'attribution ancienne où tables était inclus dans resolve ne décrit plus ces corps ; aucun gain ne se déduit de ce changement.

`OrderBuffers` est local à chaque appel (`stage.cpp:198`) : capacité réutilisée entre ordres, pas résultats géométriques
amortis entre trames. La sonde construit encore `MemoryBudget(o.budget)` sans cache. La reconstruction de l'index
reste dans wall. Le hash d'export actif est celui du patch1 (`9e903f72…`), qui n'était pas encore intégré à main037
lors de cette lecture. Le pilote `mes_t2c_g` est encore en préparation : pendant l'audit, ses ablations ont été
raccordées à G-L7/passes.cpp, mais ses constantes d'empreintes étaient encore celles d'avant patch1 à la lecture
`fa1b7cd6…`. Ce reçu ne qualifie pas ce pilote mouvant ; son raccord complet précède la campagne.

## Preuve bornée

`python3 -B model.py` et `python3 -B -O model.py` rendent exactement [results.json](results.json) : 3 123 requêtes,
E=0/1/2/17/64/257, trois masques dont zéro, mêmes associations naissance/rang par recherche directe et candidat
vérifié ; trois refus de populations dupliquées ; file FIFO contrôlée sur onze tailles autour de 8 et16.
Chaque dichotomie respecte la borne `bit_length(E)` itérations dans ce modèle. Hash volontairement simple,
pas simulation de temps, pas rejeu du C++ et pas qualification de concurrence native. Les portes C++ existantes
(`index_unit.cpp`) couvrent déjà masque zéro, résolution complète et doublons ; elles ont été lues, pas exécutées ici.
