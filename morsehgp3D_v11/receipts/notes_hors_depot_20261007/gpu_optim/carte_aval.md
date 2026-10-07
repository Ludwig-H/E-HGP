# Carte des étages aval des sorties (attach, output, write) : GPU, levier CPU ou rien

Rédigé le 6 octobre 2026 à 01:26 UTC (`date -u`), en lecture seule. GCP non utilisé. Aucun commit, aucune branche,
aucun build : un binaire Release existant (`build/v11-persist/b21-s10/mhgp11`) a été lancé, et deux petites
bibliothèques de mesure ont été compilées dans le scratchpad (minuteur de `fsync` et échantillonneur `SIGPROF`).

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only
public_status=not_claimed
```

Trois sortes de chiffres, toujours étiquetées :

- **[G4]** : mesuré sur G4, avec le reçu cité ;
- **[local]** : mesuré sur le codespace (AMD EPYC 7763, 8 vCPU, disque local), le 6 octobre entre 01:20 et
  01:26 UTC. Le binaire est `b21-s10/mhgp11` (u21, `-O3`, sans `-march`), construit depuis le worktree
  `v11-impl-l3` à S10. Le moteur n'a pas changé jusqu'à `df904711a`. Le `manifest_sha256` des sorties locales
  `full` et `supports` (ng00, K5) est **identique** à celui du reçu G4 (`aabd6491…`, `d7c3107b…`), donc les octets
  écrits sont les mêmes. Une prise unique par configuration : ce sont des ordres de grandeur, jamais un temps G4 ;
- **[estimation]** : un raisonnement, jamais présenté comme une mesure.

## 1. Verdict

**Aucun étage aval ne mérite le GPU.** Le seul gros poste est `write`, et il est dominé par un **SHA-256 scalaire
sériel** (logiciel, sans les instructions SHA du processeur), puis par le disque. Un SHA-256 sur un flux unique ne
se parallélise pas, et le disque ne va pas plus vite avec un GPU.

Les leviers sont CPU et petits :

1. SHA-NI ;
2. hachage en recouvrement de la sérialisation ;
3. vrais tampons d'écriture ;
4. colonnes groupées dans l'écrivain `points` ;
5. balayage du lemme D parallélisé par requêtes d'ancêtres.

Aucun ne touche au contrat des 100 ms : celui-ci porte sur `domain`+`tree`. À l'inverse, **l'écriture de
`MHGP11FUL1` ne tiendra jamais en 100 ms** : 300 Mo pour ng00 à K5. Le disque de démarrage est provisionné à
290 Mo/s par `gcp-migration/deploy.sh`, ce qui fixe un plancher d'environ 1,04 s si `fsync` doit tout vider
[estimation].

## 2. Faits mesurés

### 2.1 G4 (reçus)

Source : `receipts/developpement_20261005/qualification_sorties/claudequalmesure/sorties_g4.json` (source
`b319efc84`) et `origin/main:…/qualification_finale/claudefinmesure/sorties_g4.json` (source réelle `38b76701b`,
étiquette de métadonnée `b319efc84` erronée et déclarée). Médianes de trois prises chaudes, K5, en ms :

| Trame, sortie | W | attach | output | write | fichier | débit apparent de write |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| ng00 full | 1 | 0 | 0 | 2 154 | 300 883 482 o | 140 Mo/s |
| ng00 full | 48 | 0 | 0 | 2 163 | 300 883 482 o | 139 Mo/s |
| ng00 supports | 1 | 72,5 | 189,9 | 218,1 | 30 837 304 o | 141 Mo/s |
| ng00 supports | 48 | 75,4 | 45,3 | 218,5 | 30 837 304 o | 141 Mo/s |
| ng01 full | 48 | 0 | 0 | 1 820 | 255 161 594 o | 140 Mo/s |
| ng01 supports | 48 | 61,4 | 37,7 | 170,4 | 25 503 976 o | 150 Mo/s |
| ng02 full | 48 | 0 | 0 | 2 363 | 328 855 058 o | 139 Mo/s |
| ng02 supports | 48 | 80,4 | 48,9 | 234,9 | 32 463 880 o | 138 Mo/s |
| ng00 full K10 | 48 | 0 | 0 | 10 864 | 1 457 125 554 o | 134 Mo/s |
| ng00 supports K10 | 48 | 241,1 | 189,0 | 745,5 | 86 459 568 o | 116 Mo/s |

Ce que le tableau établit :

- **`write` est strictement sériel** : W1 et W48 sont égaux à 0,5 % près, pour `full` comme pour `supports`.
- **`attach` est sériel** (W1 72,5 ms, W48 75,4 ms). L'assemblage (`output` de `supports`) passe de 190 à 45 ms
  sur 48 fils, une accélération de 4,2 seulement.
- **Le débit apparent de `write` est constant**, 134 à 150 Mo/s, quels que soient le format et la taille. C'est la
  signature d'un coût par octet unique : le SHA-256 en est le candidat (§ 2.2).
- Les sorties `points` et `plat` n'ont **aucune** mesure d'étage sur G4. Le README de `qualification_finale` dit :
  « Les sorties `points` et `plat` ne sont pas mesurées par ce banc. »
- Faits de machine : AMD EPYC 9B45 avec le drapeau `sha_ni`
  (`receipts/audit_deep_20261004/…/g4_session5_scale_20260929/vm_facts.json`, `lscpu`) ; GPU RTX PRO 6000
  Blackwell Server Edition, `compute_cap` 12.0 ; disque de démarrage `hyperdisk-balanced`, 3 600 IOPS, 290 Mo/s
  provisionnés (`gcp-migration/deploy.sh`, lignes 13–14). `bench/sorties_g4.py` publie dans un `tempfile.mkdtemp`,
  donc a priori sous `/tmp` sur ce disque.
- Voie GPU existante (`receipts/developpement_20261004/gpu_g4/README.md`) : ouverture du contexte 78 ms, retour
  4 Go/s. Ces coûts fixes dépassent à eux seuls `attach` et `output`.

### 2.2 Local (codespace), ng00, K5

**Stages [local], une prise par ligne :**

| Sortie | W | attach | output | write | fichier | fsync total |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| full | 8 | — | — | 1 906 à 3 357 | 300,9 Mo | 44 ms |
| supports | 1 | 100,0 | 290,1 | 292,1 | 30,8 Mo | 10 ms |
| supports | 8 | 101 à 107 | 88 à 101 | 289 à 303 | 30,8 Mo | 10 ms |
| points | 1 | 116,5 | 590,1 | 495,1 | 36,8 Mo | 12 ms |
| points | 8 | 102 à 109 | 171 à 177 | 478 à 496 | 36,8 Mo | 10 à 12 ms |
| plat | 1 | 111,6 | 595,0 | 81,8 | 0,32 Mo | 1,5 ms |
| plat | 8 | 101 à 118 | 226 à 239 | 80 à 83 | 0,32 Mo | 1,4 à 1,8 ms |

Le `fsync` est mesuré par `LD_PRELOAD` (minuteur autour de `fsync`). Une prise `supports` lancée juste après
`full` a vu un `fsync` de 2,7 s : c'est la vidange des 300 Mo précédents, et le disque local n'est pas
représentatif.

**SHA-256 du dépôt contre SHA-NI [local] :**

- `io::Sha256` du dépôt, compilé en `-O3` dans un petit banc : **246 à 262 Mo/s** sur 256 Mio ;
- `openssl speed -evp sha256`, blocs de 16 Kio, qui utilise SHA-NI : **1 527 Mo/s**, soit un rapport de 6 ;
- conversion petit-boutiste des `u64` (boucle de `FileWriter::u64s`) : 4,8 Go/s, négligeable.

**Appels système [local]** (`strace -c`, sortie `full`, ng00, K5) : **73 460 appels `write` pour 300 883 482
octets, soit 4 096 octets par appel**, plus 4 `fsync`.

`FileWriter::open` appelle `setvbuf(file_, nullptr, _IOFBF, 1 << 16)`. Avec un pointeur nul, glibc ignore la
taille demandée et alloue un tampon de `st_blksize` octets. Le « tampon fixe de 64 Kio » et la « frontière de
vidage indépendante du système de fichiers » écrits dans `docs/SORTIES.md` § 9 et `src/io/writer.cpp` sont donc
**inexacts sous glibc**. Les octets écrits n'en dépendent pas : c'est une imprécision documentaire et un coût
d'appels système, pas une faute de correction.

**Profil `SIGPROF` [local]** : 1 kHz de temps CPU, pile de 8 cadres, échantillons rattachés à `api::publish`.

| Sortie | Échantillons | SHA-256 du fichier | `tree_k_sha256` | Sérialisation (`u32s`/`u64s`, boucles) | Géométrie exacte (`birth_sphere`) |
| --- | ---: | ---: | ---: | ---: | ---: |
| full | 1 523 | 78 % | 5 % | 13 % | 3 % |
| supports | 259 | 49 % | 30 % | 21 % | — |
| points | 292 | 10 % | 26 % | 58 % | — |
| plat | 82 | 2 % | 98 % | — | — |

À lire avec ces précautions :

- Les piles tronquées à 8 cadres perdent une partie des échantillons. Les pourcentages valent pour les échantillons
  attribués : ce sont des ordres de grandeur.
- La ligne `plat` montre que son `write` local (≈ 81 ms pour 0,32 Mo) est **entièrement la signature d'arbre**
  `tree_k_sha256` : environ 576 000 nœuds, de l'ordre de 16 Mo hachés [estimation par le format de la signature].
- La ligne `points` montre que l'écrivain `MHGP11PT` appelle `u32s`/`u64s` **élément par élément** : `parent`,
  `rank`, `point_id` et chaque niveau. Chaque appel remet à zéro un tableau de pile de 4 Kio
  (`std::array<u8, kChunkBytes> chunk{}`), puis passe par `fwrite` et `Sha256::update` pour 4 ou 24 octets. Cela
  fait environ 2,3 millions d'appels pour ng00 [estimation par les comptes : 576 371 nœuds × 2, 573 173 niveaux × 2,
  39 885 sites].

Les autres étages :

- **`attach`** : le profil local donne `AttachmentBuilder::cells()` (balayage DSU croissant, une requête par graine)
  pour environ 75 % des échantillons de l'étage, puis `window()`.
- **`output` de `supports`** : `Assembly::postorder()` (sériel), `ball_supports`, `count_body`, `fill_body`.
- **`output` de `points`** : `AncestorIndex::lca`, `build_incidences`, `HangBuilder::rival`, `fill_roots`,
  `by_birth`.
- **Tête plate** : en plus de la pendaison, `bracket_plateaus` et `num::divide` (grands entiers), soit environ 50 ms
  en local à W8.

## 3. Opportunités, par gain attendu

Les gains G4 sont des **estimations** : les proportions locales sont transposées au temps G4 mesuré. Rien ne remplace
la mesure de protocole du § 5.

### O1. SHA-256 par les instructions SHA (SHA-NI), repli scalaire conservé

- **Étage** : `write` (toutes les sorties) et `tree_k_sha256`.
- **Coût actuel** : `write` de `full` 2 154 à 2 163 ms à W48 sur ng00 [G4]. La part du SHA y est de 78 % + 5 %
  [local].
- **Gain attendu [estimation]** :
  - si la part de 80 % se transpose, le SHA coûte environ 1,7 s sur G4. Au rapport de 6 mesuré en local
    (`openssl`), il en reste environ 0,3 s : **gain d'environ 1,4 s sur `full` ng00** (de 2,16 s vers environ
    0,8 s), environ 7 s sur `full` ng00 K10 ;
  - `supports` : SHA du fichier et signature font environ 79 % de 218 ms, soit un gain d'environ 140 ms ;
  - `plat` : `write` presque entier, soit un gain d'environ 60 ms ;
  - `points` : environ 35 % de `write` ;
  - sur G4, une part du temps est peut-être du `fsync` (§ 5), ce qui réduirait ce gain.
- **Exactitude** : SHA-256 reste SHA-256, l'empreinte est identique par définition. Les portes existantes
  s'appliquent aux deux voies : `mhgp11_io_unit_sha256` (vecteurs FIPS 180-4, découpages), `mhgp11_io_sha256`
  (différentiel contre `hashlib`) et les mutants `sha_tronque` et `sha_longueur_en_octets`. Il faut ajouter un
  différentiel scalaire contre SHA-NI sur tailles et découpages aléatoires, et forcer chaque voie dans la matrice.
  Les identités `mhgp11_cli_full_identity` et `supports_route*` restent les juges à l'octet.
- **Risque** : faible. Le point délicat est le choix de la voie. Il faut soit `__attribute__((target("sha,sse4.1")))`
  avec une détection `cpuid` au démarrage, soit une option de compilation (`MHGP11_MARCH=znver5`, qui implique
  `-msha`). La règle « aucune détection implicite de la machine » vise `--fils`, mais elle est à confirmer pour ce
  cas.
- **Effort** : petit, environ 100 lignes d'intrinsèques (`_mm_sha256rnds2_epu32`, `_mm_sha256msg1/2_epu32`) dans
  `src/io/sha256.cpp`, plus une porte.
- **Protocole G4** : voir le § 5 (P1 et P2).

### O2. Recouvrement : hacher pendant qu'on sérialise et qu'on écrit ; signature d'arbre en parallèle

- **Étage** : `write`.
- **Coût actuel** : dans `api::publish`, tout est en série : sérialisation, SHA, `fwrite`, puis
  `tree_k_sha256(...)` dans le manifeste, puis le commit. La signature d'arbre seule fait environ 80 ms en local
  (`plat`).
- **Idée** : la sérialisation remplit des tampons de taille bornée et admis au `MemoryBudget`. Un fil hache le flux
  dans l'ordre pendant que le fil appelant écrit. `tree_k_sha256` et `geometry_sha256` sont calculés sur un fil du
  pool, en même temps que le fichier de données.
- **Gain attendu [estimation]** : `write` passe d'une somme (sérialisation + SHA + appels système + signature) au
  maximum de ces termes. Après O1, le gain est d'environ 0,1 à 0,3 s sur `full`. Pour `supports`, `points` et
  `plat`, la signature (environ 30 %, 26 % et 98 % du `write` local) disparaît du chemin critique : de l'ordre de
  60 à 80 ms en local, moins sur G4 après O1.
- **Exactitude** : les octets et leur ordre ne changent pas ; seul le moment du calcul change. Le manifeste reste
  écrit après la fermeture contrôlée des fichiers (mutant `manifeste_avant_donnees`). TSan s'applique au nouveau
  fil.
- **Risque** : faible à moyen (concurrence dans `io`).
- **Effort** : moyen.
- **Protocole** : P1, avec TSan sur les portes `mhgp11_io_*` et `mhgp11_cli_*_determinism`.

### O3. Vrais tampons d'écriture, au lieu de 73 460 `write` de 4 Kio

- **Étage** : `write`.
- **Coût actuel** : 73 460 appels de 4 096 octets pour `full` ng00 [local, strace]. Le temps noyau sous strace,
  0,36 s, est gonflé par strace ; il n'y a pas de mesure G4.
- **Idée** : passer à `setvbuf` un tampon appartenant au `FileWriter` (1 Mio admis au budget), ou contourner stdio
  par `write(2)` sur des blocs de 1 à 4 Mio, avec les mêmes contrôles d'erreur. Corriger au passage `SORTIES.md`
  § 9 et l'en-tête de `writer.cpp`.
- **Gain attendu [estimation]** : 50 à 150 ms sur `full` ng00, quelques ms sur les autres sorties.
- **Exactitude** : octets identiques. Les portes d'échec d'écriture restent juges : `mhgp11_io_transaction_*`,
  `_write_failure` et le mutant `ecriture_non_controlee`.
- **Risque** : faible.
- **Effort** : petit.
- **Protocole** : P1, avec `strace -c` dans une passe séparée non chronométrée.

### O4. Écrivain MHGP11PT par colonnes groupées

- **Étage** : `write` de `points`.
- **Coût actuel** : 478 à 496 ms en local à W8 pour 36,8 Mo, dont 58 % de sérialisation élément par élément. Il n'y
  a pas de mesure G4.
- **Idée** :
  - grouper `point_id`, `parent` et `rank` sur la pile par paquets de 1 024 avant `u32s`, comme le fait déjà
    `Words` dans `write_full` ;
  - remplir les niveaux par paquets de mots ;
  - supprimer la remise à zéro inutile du tableau de pile de `u32s`/`u64s` (`chunk{}` devient non initialisé, ou
    statique au fil).
- **Gain attendu [estimation]** : environ 250 ms en local, donc de l'ordre de 100 à 200 ms sur G4.
- **Exactitude** : octets identiques. `mhgp11_cli_points`, `mhgp11_points_vs_python*` et le lecteur
  `bench/mhgp11_formats.py` restent les juges.
- **Risque** : faible.
- **Effort** : petit.
- **Protocole** : P1 pour la sortie `points`, puis `file_sha256` égal avant et après.

### O5. Balayage du lemme D par requêtes d'ancêtres indépendantes (CPU parallèle)

- **Étage** : `attach`.
- **Coût actuel** : 61 à 80 ms à K5 et 241 à 253 ms à K10 ng00, sur G4 et à W48. Il est sériel (W1 égal à W48).
- **Idée** : `ClosedAncestorSweep::query(seed)` après `advance(r-1)` rend le plus haut ancêtre de la graine de rang
  au plus r-1. Si les rangs sont croissants au sens large le long d'un chemin vers la racine (invariant de forêt,
  à citer depuis `FULL_FORESTS.md`), c'est une requête d'ancêtre de niveau, indépendante d'une cellule à l'autre :
  - levée binaire, ou `AncestorIndex`, déjà utilisé par `points` ;
  - cellules traitées en parallèle à positions fixes ;
  - réduction déterministe des registres (`touched`, `continuations`, `merged` par paires (nœud, rang)
    dédupliquées par tri) ;
  - compaction de `prior` par sommes préfixes.
- **Gain attendu [estimation]** : 75 ms vers environ 10 à 20 ms à W48 sur K5, et environ 250 ms vers environ 40 ms
  à K10.
- **Exactitude** : contrôles I1 à I4 inchangés. `MHGP11SP` est identique à l'octet par les deux voies (portes
  `mhgp11_api_supports_route*`). Garder le balayage DSU comme juge différentiel, et ajouter un mutant (« ancêtre
  au rang r au lieu de r-1 »).
- **Risque** : moyen. L'équivalence avec la DSU suppose la monotonie des rangs, y compris sur les plateaux de même
  rang. Elle demande un lemme écrit et une fixture d'égalité.
- **Effort** : moyen.
- **Protocole** : P3.

### O6. Sérialisation parallèle de MHGP11FUL1, après O1

- **Étage** : `write` de `full`.
- **Coût actuel** : sérialisation 13 % et géométrie exacte 3 % de `write` [local]. Après O1, ce terme devient le
  premier poste CPU de l'écrivain [estimation].
- **Idée** : découper par plages de nœuds. Chaque fil sérialise sa plage (dont `birth_sphere`) dans un tampon borné
  et admis au budget ; on écrit et on hache dans l'ordre (O2).
- **Gain attendu [estimation]** : environ 0,2 s sur ng00 K5 et environ 1 s sur K10.
- **Exactitude** : concaténation dans l'ordre, octets identiques (`mhgp11_cli_full_identity`,
  `mhgp11_cli_full_determinism` à W1, W2, W4 et W48).
- **Risque** : faible à moyen. Mémoire : un tampon par fil, ou une fenêtre de quelques blocs.
- **Effort** : moyen.
- **Protocole** : P1.

### O7. `fsync` : amorcer l'écriture disque pendant la sérialisation

- **Étage** : commit (dans `write`).
- **Coût actuel** : **inconnu sur G4**. 44 ms en local, ce qui ne représente rien. Plancher théorique de 1,04 s
  pour 300 Mo à 290 Mo/s si le cache ne s'est pas vidé avant le `fsync` [estimation].
- **Idée** : appeler périodiquement `sync_file_range(fd, …, SYNC_FILE_RANGE_WRITE)` sur les blocs déjà écrits. Le
  `fsync` final reste, donc la durabilité du § 9 est inchangée.
- **Gain attendu [estimation]** : de 0 à environ 1 s sur `full`, selon ce que P1 montrera. Rien sur `plat`.
- **Exactitude** : sans effet sur les octets.
- **Risque** : faible ; propre à Linux.
- **Effort** : petit.
- **Protocole** : P1, avec le minuteur `fsync`.

### O8. Assemblage des supports : postordre parallèle

- **Étage** : `output` de `supports`.
- **Coût actuel** : 45 à 51 ms [G4, W48], avec une accélération de 4,2 seulement sur 48 fils.
- **Idée** : remplacer `Assembly::postorder()` sériel par un parcours d'Euler à sommes préfixes.
- **Gain attendu [estimation]** : 10 à 20 ms.
- **Exactitude** : octets identiques (identité `MHGP11SP`).
- **Risque** : faible.
- **Effort** : moyen.
- **Priorité** : basse.
- **Protocole** : P3.

## 4. Pistes écartées

- **GPU pour SHA-256** : un SHA-256 sur un flux unique est intrinsèquement sériel (Merkle-Damgård). Seul un hachage
  par morceaux (arbre de Merkle, BLAKE3) se paralléliserait, et il changerait le champ `sha256` du manifeste et les
  reçus. C'est un changement de contrat, hors objectif. Sur CPU, SHA-NI donne un facteur d'environ 6 sans rien
  changer.
- **GPU pour `attach`** : 75 ms sur G4, données résidentes côté hôte. Le contexte coûte 78 ms et le retour 4 Go/s
  (reçu `gpu_g4`). Le coût fixe dépasse l'étage, et la parallélisation CPU (O5) suffit.
- **GPU pour l'assemblage, la pendaison H^r_{K+1} et la tête plate** : les étages sont de quelques dizaines de ms
  à W48 [G4 pour l'assemblage ; estimation pour les deux autres, non mesurés sur G4]. Leurs décisions sont exactes,
  sur des racines et grands entiers (`two_vs_two`, `num::divide`), et leur accès mémoire est irrégulier (LCA). Même
  verdict que pour les feuilles du catalogue (3,2 fils actifs par warp) : pas de gain plausible.
- **Compression** (zstd ou nvCOMP) : elle change les formats. `MHGP11FUL1` est « inchangé » par contrat pour rester
  comparable aux reçus immuables, et `MHGP11SP`, `MHGP11PT` et `MHGP11ET` sont normatifs. Elle ajoute aussi du CPU
  au chemin critique tant que le SHA n'est pas réglé.
- **`mmap` de sortie** : il n'évite ni la lecture des octets par le SHA, ni le `fsync` (`msync`). Il remplace
  seulement les `write`, ce qu'O3 fait plus simplement.
- **`O_DIRECT`** : contraintes d'alignement pour un gain nul une fois O7 en place, et cache contourné.
- **Supprimer `fsync`** : interdit (contrat de durabilité, `docs/SORTIES.md` § 9, étape 4).

## 5. Protocole de mesure sur G4 (session gardée `v11_session.py`, rien lancé ici)

Commun à P1–P3 :

- données : ng00, ng01, ng02, K5, W1 et W48 ; ng00 en K10 ;
- prises : une froide puis trois chaudes, médianes ;
- avant et après chaque levier, sur le même binaire de référence et dans la même session ;
- relevés de machine : `grep -c sha_ni /proc/cpuinfo`, `sysctl vm.dirty_*`, `df -T` du dossier de travail ;
- identité :
  - `file_sha256` et `manifest_sha256` égaux aux reçus (`aabd6491…` pour `full` ng00 K5, `d7c3107b…` pour
    `supports` ng00 K5, et de même pour ng01, ng02 et K10) ;
  - portes `mhgp11_io_*`, `mhgp11_cli_full_identity`, `mhgp11_api_supports_route*`, `mhgp11_cli_points`,
    `mhgp11_cli_plat`, `mhgp11_*_vs_python` ;
  - mutants des modules `io`, `api` et `cli` ;
- reçu ancré au commit.

**P1 (décomposition de `write`, à faire avant tout levier).**

- Ajouter au banc `bench/sorties_g4.py` les sorties `points` et `plat`, qui manquent.
- Lancer chaque appel sous `LD_PRELOAD` d'un minuteur de `fsync`. C'est 10 lignes de C, compilé sur la VM, et la
  bibliothèque du scratchpad (`fsynctime.c`) en est le modèle.
- Faire une passe `strace -c -e write,fsync`, non chronométrée.
- Faire une passe de profil : `perf` s'il existe sur la VM, sinon l'échantillonneur `SIGPROF` par `LD_PRELOAD`
  utilisé ici (`prof.c`).
- Variante sans disque : `--dossier` sur un tmpfs de plus de 2 Go, s'il peut être monté par le script gardé. Elle
  sépare le CPU du disque.
- Sortie : la part SHA / sérialisation / appels système / `fsync` / signature, en ms G4.

**P2 (O1 à O4, O6, O7).** Mêmes appels après chaque levier pris seul, puis cumulés.

- Critère de livraison écrit d'avance : `write` de `full` ng00 K5 à W48 baisse d'au moins 30 % en médiane, sans
  aucune empreinte changée.
- Pour O1 : un banc de débit SHA du dépôt (scalaire contre SHA-NI) sur 256 Mio, sur G4.

**P3 (O5, O8).**

- `attach` et `output` à W1, W8 et W48.
- `MHGP11SP` identique à l'octet par la voie DSU et par la voie parallèle (registres I3 et I4 égaux).
- Invariance W1, W4 et W48, permutation et réétiquetage (`cli_supports_scale_verdict`), TSan.

## 6. Questions ouvertes

1. Le contrat de 100 ms porte-t-il seulement sur `compute` (`domain`+`tree`, API en mémoire) ? L'écriture de
   300 Mo de `MHGP11FUL1` est physiquement hors de portée en 100 ms sur le disque provisionné (plancher d'environ
   1 s [estimation]).
2. Quelle est la part du `fsync` dans les 2,16 s de G4 ? Elle est inconnue. Le calcul local (SHA à 250 Mo/s, soit
   environ 1,2 s pour 300 Mo) plus le plancher disque (environ 1,04 s) dépasse déjà 2,16 s. Le Zen 5 de G4 est donc
   plus rapide en SHA scalaire, ou le cache s'est vidé en partie avant le `fsync`. P1 tranche.
3. Une détection `cpuid` de SHA-NI est-elle admise par la doctrine (« aucune détection implicite de la machine »),
   ou faut-il une option de construction explicite (`MHGP11_MARCH`) ?
4. Faut-il un format compact `MHGP11FUL2`, décision de l'utilisateur ?
   - Aujourd'hui, chaque indice est un `u64`, et chaque entier exact s'écrit signe, nombre de mots, puis mots.
   - Un format avec indices `u32` et niveaux par rang dédupliqués serait plusieurs fois plus petit
     [estimation non mesurée].
   - Il casse la comparabilité octet à octet avec les reçus ; `full` resterait alors une sortie de diagnostic.
5. Faut-il relever le débit provisionné du disque de la VM (`BOOT_DISK_THROUGHPUT=290`), ou écrire les sorties de
   banc sur SSD local ou tmpfs ? Cela modifie `gcp-migration/`, sous scripts gardés et avec un coût.
6. Doc : corriger `docs/SORTIES.md` § 9 et `src/io/writer.cpp` sur le tampon de 64 Kio, que glibc n'honore pas
   avec un pointeur nul (§ 2.2).
7. Les sorties `points` et `plat` n'ont aucune mesure d'étage sur G4. Il faut les ajouter au banc avant toute
   conclusion sur la pendaison et la tête.
