# Microbancs de la tour v12 : MES-M3, MES-M4 et vidage de la v11

7 octobre 2026. Travail préparatoire de la tranche T0 de la v12 (`morsehgp3D_v12/docs/PLAN.md`, § 1), **hors produit** :
outils de vidage de la v11 gelée, microbanc `MES-M3` (plus petite boule proposée puis certifiée) et microbanc `MES-M4`
(forêt sans lots), pilote Python. Rien ici n'est un port du produit ; tout est destiné à être jugé sur G4 avant le
port des étages G, T et M.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference
quantification=quantized_u21_input_only (construction v11 liée : MHGP11_COORD_BITS=21, moteur ac081a06f)
public_status=not_claimed
GCP non utilisé (aucune commande gcloud, aucune session G4 lancée d'ici)
```

**Licence des données.** Les vidages contiennent des coordonnées SemanticKITTI (CC BY-NC-SA) : ils restent dans le
dossier de sortie, jamais dans un dépôt. Seuls des comptes, des empreintes et ce dossier de sources peuvent être versés.

## 1. Contenu

| Chemin | Rôle |
| --- | --- |
| `CMakeLists.txt` | construction (CMake ≥ 3.22, C++20 sans extensions, `-Wall -Wextra -Wpedantic -Werror`, GCC 11.4 visé) |
| `common/format.hpp` | format binaire des vidages (écriture, lecture par projection en mémoire), comparaison exacte de centres ; chaque avancée de la lecture est contrôlée contre la place restante (`CST-0225`) |
| `tests/format_reader_test.cpp` | `mhgp12_format_test <dossier>` : porte de l'admission du lecteur, neuf fichiers synthétiques dont le témoin de 88 octets de l'auditeur (code 0 conforme, 1 écart) |
| `vidage/vidage_v11.cpp` | `mhgp12_vidage` : vidage de la tour v11, lié à `libmhgp11.a` ; mesure de résolution à trois bras, et quatrième bras `replique_v12_saut` (`MES-G1`) ; profil par composante (`MES-M7`) |
| `mes_m3/welzl_proposal.hpp` | port de `DWelzl` de la v10 (proposition flottante, ne décide rien) |
| `mes_m3/meb_cert.hpp` | cœur de `LEV-MEB-CERT` : `LEM-T1` corrigé, certificat exact, canonisation, repli, juge |
| `mes_m3/mes_m3.cpp` | `mhgp12_mes_m3` : porte (témoins gravés) et banc sur vidage |
| `mes_m4/mes_m4.cpp` | `mhgp12_mes_m4` : noyau union-find sans lots, contraction `LEM-T4`, numérotation, juge, `LEM-T6` |
| `pilote.py` | pilote (bibliothèque standard, Python ≥ 3.10, jouable sous `python3 -S`) : étapes, preuves, juges |
| `tests/test_pilote.py` | porte du pilote (`CST-0018`, `0213`, `0214`) : sorties réelles des reçus G4 B et D (verdicts et statistiques publiés retrouvés), injections de l'auditeur par binaires simulés, mutant de MES-M4 rattaché à son cas, juge de MES-M3, binaires réels sur le carré |
| `tests/test_m4_preuves.py`, `tests/carre.py` | porte native de MES-M4 sur le carré K1..4 de l'auditeur : verticales correctes, fausses, absentes ; entrées hors domaine |
| `RAPPORT.md` | rapport des passages locaux et ce qui reste pour G4 |
| `out/` | sorties (vidages, journaux JSON, `rapport_mes_m3_m4.json`) ; jamais versées |

Binaires : `mhgp12_vidage`, `mhgp12_mes_m3`, `mhgp12_mes_m4` et deux **mutants causaux** compilés à part (jamais une
branche du chemin mesuré) : `mhgp12_mes_m3_mutant_sans_s_dans_f` (`LEM-T1` sans le test S ⊆ F) et
`mhgp12_mes_m4_mutant_sans_contraction` (chaque événement binaire devient un nœud).

## 2. Construire et jouer

```bash
python3 pilote.py --v11-source <dépôt>/morsehgp3D_v11 --v11-build <construction Release u21 de la v11> \
    --donnees <dossier des trames>/ --sortie out --cas ng00:5,ng01:5,ng02:5,ng00:10 tout
```

`tout` = `construire portes vider resolution m3 m3var m4 rapport` (`rapport` écrit aussi `out/tableaux.md`, tableaux
Markdown tirés du JSON, sans valeur recopiée à la main). `--processus N` rejoue chaque microbanc **et la mesure de
résolution** dans N processus (l'unité de réplication de `MESURE.md` § 5) ; les temps publiés des microbancs sont alors
des médianes entre processus, et les routes doivent être identiques d'un processus à l'autre.

**Preuves** (`CST-0018`, `CST-0213`, `CST-0214`) : aucune conformité sans preuve. `construire` hache tous les binaires
et `libmhgp11.a` ; chaque exécution hache le binaire lancé avant et après (égal à `construire`, sinon refus), vérifie
les empreintes des vidages de `vider` avant et après, garde son journal sous un nom propre à la campagne (identifiant
`c<date>_<hasard>` par invocation, jamais écrasé) avec son empreinte, et exige toutes ses phases : une ligne d'entrée du
bon cas, une ligne par ordre (comptes égaux aux sections du vidage), une fin explicite au code du processus, ni
exception ni refus. Un bloc remplacé passe dans `historique` ; les campagnes de résolution s'ajoutent. Les sources du
microbanc sont hachées au début et à la fin de chaque invocation. Un mutant n'est tué que par sa réponse géométrique
(témoin faux, écarts), jamais par un simple code, et seulement si cette réponse est **complète et rattachée à son cas**
(reçu `audit_cd_corrections_20261007/m34`) : entrée de la même trame et du même K marquée mutant, une ligne par ordre
1..K aux comptes du vidage, identité lisible, au moins un écart d'identité expliqué par la forme, les comptes ou les
nœuds, fin au code du processus ; une sortie tronquée, d'une autre trame ou d'un autre K est un refus (code 3), jamais
un mutant tué, et le juge de MES-M4 n'admet que les mutants tués, complets et sans refus. Les cas sont `trame:K` avec `trame` dans `ng00`, `ng01`, `ng02`,
`u8000`, `u16000`, `u32000` (fichiers `lidar_ng0*.u32le` et `uniform_u18_n*.u32le` du dossier de données), ou tout
autre nom `X` désignant `<données>/X.u32le` et `<données>/X.ids.u32le` (nouvelles trames de plusieurs séquences ; pas
d'empreinte de référence, l'identité `MHGP11FUL1` est alors publiée « sans référence »). Le vidage
utilise `--fils` fils (3 par défaut sur le codespace partagé) ; les microbancs sont à **un fil**. Le rapport JSON est
`out/rapport_mes_m3_m4.json` ; chaque binaire écrit aussi ses lignes JSON dans `out/<cas>/*.jsonl`.

Codes de sortie : `0` conforme ; `1` écart d'identité, porte en échec, mutant survivant ; `2` usage (et domaine des
opérandes de MES-M4, `CST-0212`) ; `3` refus (preuve absente, périmée ou non rattachée ; section absente ; entrée hors
domaine) ou invariant (vidage) ou exception (microbancs). Le pilote rend le pire code de ses étapes ; le verdict
d'adoption de MES-M3 est dans le rapport et ne change pas ce code.

## 3. Le vidage (`mhgp12_vidage`)

```text
mhgp12_vidage <xyz.u32le> <ids.u32le> <trame> <K> <feuille> <fils> <dossier>
              [--ful1 <chemin>] [--journal tous|aucun|k1,k2,...] [--chrono-resolution R] [--budget <octets>]
              [--profil-resolution] [--bras-saut]
```

Il reproduit la sonde `mhgp11_full_bench` au masque `802811` (voie CPU de référence des empreintes ; feuilles 16 à
K ≤ 5 et 24 au-delà), puis lit **tout** dans les objets de la v11 ou le recalcule par ses fonctions : aucune décision
n'est réimplantée dans le vidage.

1. Domaine (`prepare_full_domain`) et forêts publiées (`build_forests`, ordres concurrents, mêmes paramètres que la
   sonde). Option `--ful1` : le vidage `MHGP11FUL1` de la sonde, réécrit à l'octet (copie de `serialize()`), dont le
   pilote compare le SHA-256 aux empreintes de `morsehgp3D_v12/docs/MESURE.md` § 4, puis l'efface.
2. Journaux de graines de la v11 (`ForestBuilder::seed_log`, voie série, un ordre par tâche) : contrôle que la forêt
   de la voie série est **identique** à la forêt publiée, puis que les graines rejouées (point 4) sont **identiques**
   au journal de la v11, cellule par cellule et trace par trace.
3. Classification de la v11 (`classify_range`) : cellules = boules de genre 2 (traces strictes), dans l'ordre des
   boules ; contrôle que les naissances de la forêt publiée sont exactement les boules de genre 1.
4. Rejeu instrumenté de chaque trace, **même suite d'appels que `PopulationLookup::descend_each_step`** : table de
   populations (`hit`), sinon `descent_step` (plus petite boule `bounded_meb`, puis catalogue ou census), pas suivant,
   jusqu'à la naissance. Chaque partie dont la v11 calcule une plus petite boule est vidée avec sa route et B(F).
5. Option `--chrono-resolution R` : mesure de résolution à **un fil**, trois bras sur les mêmes traces (§ 5.4).
6. Option `--profil-resolution` : profil par composante du bras `replique_v12` (`MES-M7`, § 5.5), après tout le reste.
7. Option `--bras-saut` (avec `--chrono-resolution R`) : quatrième bras `replique_v12_saut` de la mesure de résolution
   et contrôle de sa forêt contre celle de la v11 (`MES-G1`, § 5.6).

Le vidage refuse, jamais en silence : code 3 (invariant) si la forêt série ≠ la forêt publiée, si une graine rejouée ≠
le journal v11, si les naissances ≠ la classification, si une route « catalogue » n'a pas son support local dans la
table (ou l'inverse), si le niveau ne décroît pas strictement le long d'une descente, si une graine n'a pas de nœud de
naissance, ou si les graines des trois bras de la mesure de résolution diffèrent ; code 2 pour une coquille de plus de
64 sites (masque de trace sur 64 bits) ou un refus de ressources de la v11.

## 4. Formats (version 1)

Tous les entiers sont little-endian. Fichier = en-tête de 64 octets, puis sections. Une section = en-tête de 24 octets
(étiquette ASCII de 8 octets complétée par des zéros, `u32` taille d'un élément, `u32` réservé, `u64` nombre
d'éléments), puis les éléments, complétés par des zéros jusqu'à un multiple de 8 octets. Un lecteur cherche les
sections par étiquette et refuse une section attendue absente, une taille d'élément inattendue, un fichier tronqué ou
des octets en trop.

| En-tête (64 octets) | Type | Sens |
| --- | --- | --- |
| `magic` | 8 octets | `MHGP12DP` |
| `version` | `u32` | 1 |
| `kind` | `u32` | 1 catalogue, 2 ordre, 3 forêt |
| `coord_bits` | `u32` | 21 (profil de la construction v11) |
| `kmax` | `u32` | K du catalogue |
| `order` | `u32` | k (0 pour le catalogue) |
| `sections` | `u32` | nombre de sections |
| `sites` | `u64` | n (0 pour une forêt) |
| `frame` | 24 octets | nom de la trame, complété par des zéros |

Un `i128` est écrit sur 16 octets en complément à deux (mot bas, puis mot haut).

### 4.1 `cat.bin` (kind 1) : Cat_K canonique de la v11

| Section | Élément | Contenu |
| --- | --- | --- |
| `SITEXYZ` | 12 octets : `u32 x, y, z` | sites par `SiteIdx` (ordre de Morton de la v11), coordonnées entières de la grille |
| `BALLS` | 32 octets : `u32 rank, p, m, q, sstar[4]` | boules dans l'ordre canonique de la v11 (niveau exact, puis S*) ; `rank` ≥ 1 rang dense du niveau ; `sstar` croissant, `0xFFFFFFFF` au-delà de q |
| `POPOFF` | `u64` | B + 1 décalages (CSR) |
| `POPVAL` | `u32` | populations : pour la boule b, I (p sites croissants) puis U (m sites croissants) |
| `NLEVELS` | `u64` | nombre de niveaux distincts, niveau nul (rang 0) compris |

Le niveau exact d'une boule se recalcule depuis S* et les coordonnées (formules de `morsehgp3D_v11/docs/MATHEMATIQUES.md`
§ 2 : q2 milieu, q3 centre circonscrit dans le plan, q4 centre circonscrit) ; les rangs se comparent entre eux, les
niveaux ne sont pas stockés.

### 4.2 `ordre_<k>.bin` (kind 2) : naissances, cellules, traces, graines, parties de descente

| Section | Élément | Contenu |
| --- | --- | --- |
| `BIRTHS` | 16 octets : `u32 key, rank, v11_node, flags` | naissances de l'ordre k par **clé croissante** (BallIdx ; SiteIdx à k = 1) ; `v11_node` = indice du nœud de naissance dans la forêt v11 (ordre canonique) ; `flags` bit 0 = coquille étendue |
| `BCENTER` | 64 octets : `i128 x, y, z, d` | centre exact de la naissance (`x/d, y/d, z/d`, d > 0 ; d = 1 pour un site) : avec `rank`, c'est ce qui fixe l'ordre canonique |
| `CELLS` | 24 octets : `u32 ball, rank, p, m, q, flags` | cellules (boules de W_k qui ne sont pas des naissances) par boule croissante, donc par rang croissant ; `flags` bit 0 = coquille étendue |
| `CELLOFF` | `u64` | C + 1 décalages des traces par cellule |
| `TRACEA` | `u64` | trace stricte I ∪ A, codée par le masque de A dans U (bit j = j-ième site de U) ; ordre de la v11 (lexicographique des A ; voie régulière : sommet omis décroissant) |
| `SEEDS` | 12 octets : `u32 key, node ; u8 end, steps ; u16` | graine de la trace telle que la v11 la calcule : clé de la naissance terminale, nœud v11 ; `end` 1 table de populations sur la trace, 2 table après des pas, 3 pas terminal ; `steps` parties vidées pour cette trace |
| `PARTOFF` | `u64` | T + 1 décalages des parties par trace |
| `PARTS` | `4k` octets : k × `u32` | parties de descente (SiteIdx croissants) dont la v11 calcule la plus petite boule, dans l'ordre de parcours |
| `PARTINF` | 8 octets : `u8 route, action, sstar_in_f, pad ; u32 ball` | `route` 1 catalogue (support local trouvé dans la table), 2 census saturé, 3 census complet ; `action` 1 saut intérieur (p ≥ k), 2 trace stricte, 3 terminal ; `ball` = B(F) dans Cat_K, sinon `0xFFFFFFFF` ; `sstar_in_f` = 1 si S*(B(F)) ⊆ F |

`ball` est exact : support local canonique de `bounded_meb` dans la table, sinon census de seuil K sur la sphère, S*
global (`global_support`) puis table. Un census saturé au seuil K implique p ≥ K, donc une sphère hors de Cat_K.

### 4.3 `foret_<k>.bin` (kind 3) : forêt publiée par la v11

| Section | Élément | Contenu |
| --- | --- | --- |
| `FNODES` | 24 octets : `u32 rank, parent, birth_key, child_count ; u64 child_begin` | nœuds dans l'ordre de la v11 (naissances canoniques, puis fusions par rang et plus petite naissance) ; `parent` et `birth_key` à `0xFFFFFFFF` si absents |
| `FEDGES` | `u32` | enfants (CSR dans l'ordre des nœuds, triés) |
| `FLOWER` | `u32` | verticales vers l'ordre k − 1, par nœud : **exigée** à tout ordre k ≥ 2, absente à l'ordre 1 (`CST-0214`) |
| `FMETA` | 3 × `u64` | naissances, racine, nombre d'arêtes |

## 5. MES-M3 : plus petite boule proposée puis certifiée

### 5.1 Voie nouvelle (`mes_m3/meb_cert.hpp`)

Pour une partie F de k sites :

1. **Proposition** : `DWelzl` de la v10 (paire éloignée puis pire point, sinon Welzl à déplacement en tête), en
   repère local (translation par le premier site : différences entières exactes en binary64). Elle ne décide rien :
   seul son support S (2 à 4 sites) sort.
2. **`LEM-T1` corrigé** (constat `CST-0101` de l'auditeur, 7 octobre) : si S ⊆ F (inclusion de multiensembles
   d'indices de sites), si S est le S* d'une boule b du catalogue (table S* → boule) et si F ⊆ P_b, alors B(F) = b,
   sans arithmétique. Énoncé correct : le rayon de B(F) est au moins celui de B(S) = b puisque S ⊆ F, et F tient dans
   la boule fermée b ; d'où l'égalité par unicité. Un échec de S ⊆ F renvoie au **repli exact**, jamais à un succès
   (le certificat exact suppose lui aussi un support tiré de F).
3. **Certificat exact** du support proposé (prédicats de `morsehgp3D_v11/src/num`, liés) : centre exact, coordonnées
   barycentriques strictement positives (q2 : milieu ; q3 : triangle strictement aigu ; q4 : centre strictement
   intérieur au tétraèdre), côté exact de **tous** les sites de F (≤ 0) ; puis **canonisation** parmi les sites de F sur
   la sphère (support de cardinal minimal, premier dans l'ordre lexicographique des SiteIdx, prédicats exacts sur le
   centre certifié) et recherche du support canonique dans la table. Si la table l'ignore (sphère hors de Cat_K, ou
   S*(b) ⊄ F), le niveau est matérialisé pour le census, comme la v11 recense alors la sphère.
4. **Repli exact** : `bounded_meb` de la v11 (la référence elle-même).

Variantes nommées de la proposition (aucune ne décide rien) :
- `--repere local|absolu` (défaut `local`) : `absolu` est le port littéral de la v10 (coordonnées absolues), `local` le
  même calcul après translation par le premier site ;
- `--proposition welzl|mere_puis_welzl` (défaut `welzl`) : `mere_puis_welzl` propose d'abord le S* de la boule du pas
  précédent (la « mère » : cellule de la trace pour la première partie, B de la partie précédente sinon), puis DWelzl.
  Par la descente stricte, S*(mère) n'est jamais dans F alors que F est toujours dans P_mère : c'est le contre-exemple
  `CST-0101` à chaque pas. Avec le test S ⊆ F, la mère est écartée et l'identité tient ; le mutant sans ce test y
  rend la mère, fausse, sur toutes ces parties. L'étape `m3var` du pilote joue cette variante sur le premier cas, avec le
  binaire normal (code 0 attendu) et le mutant (code 1 attendu : **mutant tué sur données réelles**).

Routes : `t1`, `cert_table`, `cert_census`, `repli_table`, `repli_census` ; raisons du repli :
`proposition_echouee`, `s_hors_de_f`, `support_degenere`, `barycentre_non_strict`, `site_exterieur`,
`refus_arithmetique`.

### 5.2 Référence et juge

Référence : `bounded_meb` (énumération exacte exhaustive de la v11 : paire diamétrale, puis triplets, puis
quadruplets) et recherche de son support local canonique dans la même table. Juge, sur **toutes** les parties :
identité de la sphère (centre exact par `compare_centers` et niveau exact), du support local canonique et de
l'identification au catalogue ; cohérence avec `PARTINF` du vidage. Un écart rend le code 1.

Comptes publiés par ordre : parties, parties distinctes, routes, raisons, part `t1`, part « sphère au catalogue »
(l'analogue du chiffre de 76 % de la v10), part « éligible à `LEM-T1` avec le test complet » (S*(B(F)) ⊆ F ⊆ P_b),
coquilles étendues, propositions non canoniques, sites cosphériques en plus du support, présentations de la référence ;
temps à un fil par passes entrelacées (minimum de R), au total et par route.

### 5.3 Porte et mutant

`mhgp12_mes_m3 --porte` joue dix témoins sur un nuage gravé (carré ABCD, point lointain, triangle aigu, tétraèdre
régulier) et un mini-catalogue recensé en exact (boules de diamètre AC et AB, cercle PQR, sphère du tétraèdre), dont
**`WIT-T1-CARRE`** : A = (0,0,0), B = (2,0,0), C = (2,2,0), D = (0,2,0), b la boule de diamètre AC (S* = la paire
antipodale canonique), F = {A, B} ⊆ P_b mais S ⊄ F ; B(F) a le rayon carré 1, b le rayon carré 2. Chaque témoin exige
l'identité avec la référence et, s'il y a lieu, la route et la raison attendues. Le mutant
`mhgp12_mes_m3_mutant_sans_s_dans_f` doit être **tué** (code 1) : il l'est par `WIT-T1-CARRE`.

### 5.4 Mesure de résolution (règle d'adoption de MES-M3)

La règle (`PLAN.md`) porte sur le **CPU de résolution** à K10. `mhgp12_vidage --chrono-resolution R` joue, sur les
mêmes traces, dans le même processus, à un fil :

- `v11` : `PopulationLookup::descend_each_step` de la v11 (registres de travail compris) ;
- `replique_v11` : la même descente réécrite pas à pas (`DescentBuilder::run`, `locate`, `strict_trace`) avec la plus
  petite boule de la v11 (`bounded_meb`) ;
- `replique_v12` : la même réplique avec la plus petite boule proposée puis certifiée.

Les graines des trois bras doivent être identiques trace par trace (sinon refus). Seule la plus petite boule diffère
entre les deux répliques : leur rapport isole l'effet de `LEV-MEB-CERT` sur le CPU de résolution.

**Réplication et juge** (`CST-0213`). L'étape `resolution` du pilote joue cette mesure dans `--processus` processus neufs
par cas (`--journal aucun`, `--chrono-resolution R` avec R = `--chrono-k5` ou `--chrono-k10`) ; chaque processus réécrit
les vidages, qui doivent être identiques à l'octet à ceux de `vider` (mêmes traces, mêmes parties), puis les efface ;
ses lignes doivent couvrir les ordres 2..K, graines identiques et temps finis positifs. Chaque prise garde son journal
(`<cas>/resolution/<campagne>/p<i>/resolution.jsonl`) et chaque campagne s'ajoute aux précédentes. Le temps d'un bras
dans un processus est le **minimum de R passes** de ce processus (identifié comme tel au rapport) ; l'unité de
réplication est le processus. Juge (étape `rapport`, `REGLE_M3`) : par processus, rapport des sommes sur les ordres
2..K (réplique v12 sur réplique v11) ; moyenne géométrique des rapports par processus, IC 95 % par bootstrap sur les
processus (10 000 tirages, graine fixe), comme MES-M2 ; **adopté** si la borne haute est au plus 0,60 sur ng00, ng01 et
ng02 à K10, avec au moins 5 prises valides par cas dans la dernière campagne, l'identité de MES-M3 et la porte
(`WIT-T1-CARRE`, mutant tué) rattachées aux mêmes binaires et vidages ; **rejeté** si une borne haute dépasse 0,60 ou
si l'identité est en défaut ; **refusé** si une preuve manque. Le verdict cite ses journaux et empreintes. La prise de
`vider` (`--chrono-vidage oui`) reste informative.

### 5.5 Profil de la résolution par composante (`MES-M7`, option `--profil-resolution`)

`mhgp12_vidage … --profil-resolution` joue, **après** toutes les mesures existantes (vidages, journaux,
`--chrono-resolution`), dont il ne change ni les sorties ni le protocole, une passe de plus du bras `replique_v12` à
**un fil**, instrumentée au compteur de cycles (`__rdtsc` encadré par `lfence`) ; la fréquence du compteur est estimée
par `steady_clock` sur la passe. Le pilote ne passe pas cette option. Composantes, par ordre k = 2..K :

| Composante | Contenu |
| --- | --- |
| `sonde` | table de populations (`PopulationLookup::hit`) |
| `proposition_t1` | proposition DWelzl puis `LEM-T1` (arité, S ⊆ F, table S* → boule, F ⊆ P_b) |
| `certificat` | route certificat : centre exact, barycentre strict, côtés des sites de F, canonisation, table, niveau |
| `repli` | `bounded_meb` et table |
| `census_sature`, `census_complet` | sphère du support, census de seuil k ; pour le complet, S* global de la coquille puis table |
| `saut`, `trace_stricte` | construction de la partie suivante : p ≥ k ; p < k (terminal compris) |
| `reste` | cycles de la passe moins la somme des sections (boucle, parties, lecture du catalogue, appels) |

Sortie : une ligne `profil_resolution` par ordre (cycles, occurrences, secondes, part du total et nanosecondes par
occurrence de chaque composante ; `cycles_par_section_vide`, biais d'une section, à retrancher par occurrence ;
`secondes_replique_v12_non_instrumentee`, une passe du bras sans compteur, informative), puis `profil_resolution_fin`.
**Contrôles** (refus `tower_invariant`, code 3) : deux passes non chronométrées précèdent, le bras `replique_v12`
lui-même et la copie instrumentée sans lecture du compteur ; leurs graines doivent égaler celles du vidage trace par
trace, avec les mêmes routes et les mêmes nombres de plus petites boules et de censuses ; les occurrences de la passe
chronométrée doivent égaler celles du contrôle et les comptes du vidage (`controle_vidage` : sondes, parties, routes
catalogue, census saturé et complet, sauts, traces ou terminaux). Les lectures encadrées gardent la latence d'une sonde
dans sa section, mais suppriment le recouvrement entre composantes : la passe instrumentée est plus lente que le bras
(de 18 à 28 % en local sur ng00 K5) ; ce sont les parts qui se mesurent. Les temps locaux ne décident rien : `MES-M7` se
joue sur G4, K5 et K10, ng00–02 et une trame d'une autre séquence.

### 5.6 Quatrième bras `replique_v12_saut` (`MES-G1`, option `--bras-saut`)

Seconde moitié de la règle du levier `G-L3` ([`CONTRAT_TOUR.md`](../../docs/CONTRAT_TOUR.md) § 4.3) ; le pilote et le
juge sont ceux de [`../mes_g1_saut/`](../mes_g1_saut/README.md). Avec `--chrono-resolution R --bras-saut`, chaque
répétition joue un quatrième bras après les trois autres, sur les mêmes traces, à un fil :

- **même fonction** que `replique_v12` (`resolve_replica<true>`), le saut étant un commutateur d'exécution : seule la
  tentative de saut distingue les deux bras ; la réplique v11 n'est pas touchée ;
- pour une plus petite boule **hors catalogue**, avant le census : F et les K plus proches voisins de chacun de ses
  sites (`../mes_g1_saut/voisins.hpp`, mutualisé), triés par `SiteIdx` et testés dans cet ordre au côté exact de la
  v11 (`LatticeSphere`, la voie du census) ; k sites strictement intérieurs prouvent p ≥ k, et la partie suivante est
  formée des k plus petits `SiteIdx` d'entre eux (pas valide du théorème D) ; sinon census, comme les autres bras ;
- voisins calculés **une fois** avant les passes (ligne `voisins_etage_p`, temps publié à part comme un coût de
  l'étage P), jugés contre la force brute sur 64 sites.

**Contrôle de forêt** (hors chronomètre) : les graines terminales du bras peuvent différer de celles de la v11 ; il
faut que la cible de chaque représentant soit dans la même composante que la graine de la v11 à la coupe ouverte de sa
jonction. Union-find des cibles par jonction, rangs croissants ; chaque fusion formée doit être un nœud de la forêt
publiée de la v11 (même rang, mêmes enfants), autant de fusions par rang, une seule composante finale portée par la
racine publiée. Le contrôle est d'abord joué avec les graines de la v11 (il doit reproduire la forêt publiée, sinon
refus, code 3). Une forêt différente est un **écart** : ligne `exit` « ecart », code 1, après tous les ordres. Mutant
causal du contrôle (construit par `../mes_g1_saut/CMakeLists.txt`) : `mhgp12_vidage_mutant_cibles_decalees`, chaque
représentant reçoit la cible du suivant ; il doit sortir en code 1, forêt différente à chaque ordre.

Sortie : une ligne `resolution_saut` par ordre (temps des quatre bras, rapport saut sur `replique_v12`, censuses saturés
et complets des deux bras, saturés évités et leur part, censuses restants, tentatives, sauts certifiés, candidats
testés, pas, plus petites boules, histogrammes et maximum des longueurs de chaîne, graines différentes de la v11,
contrôle de forêt) ; sans `--bras-saut`, les sorties ne changent pas. Le tri des supports de `meb_cert.hpp` passe par
`sort_support` (insertion à bornes explicites, même résultat que `std::sort`) : `std::sort` y déclenchait un faux
positif `-Warray-bounds` de GCC 13 une fois `certify` expansée dans la boucle de résolution.

## 6. MES-M4 : forêt sans lots

1. **Naissances** : numérotation canonique par (rang, centre exact) ; les cohortes de même rang sont contiguës dans
   `BIRTHS` (contrôlé) et triées par comparaison exacte des centres (produits croisés sur 256 bits).
2. **Noyau** (`D-F1`, `CONCEPTION_TOUR.md` § 4.1, port de `preuves_tour/noyau_v11.cpp`) : jonctions par rang
   croissant ; union-find par **taille** sur des événements binaires de 20 octets (rang, deux opérandes = sommets
   courants, plus petite feuille, survivant) ; attache de chaque perdant (rang compris) ; sommet par jonction ; refus si
   le nombre d'événements n'est pas `naissances − 1` (racine unique). **Domaine des opérandes** (constat `CST-0212`) :
   un opérande est un `u32` dont le bit 31 porte le genre (naissance ou événement) ; il reste 31 bits utiles. Le noyau
   exige donc au plus 2^31 − 1 naissances par ordre (d'où au plus 2^31 − 2 événements et 2^32 − 3 nœuds, sous la
   sentinelle 2^32 − 1) et refuse au-delà, sur le seul nombre de naissances, avant toute allocation (banc : code 2,
   `domaine_operandes_31_bits`). La porte vérifie la borne : 2^31 − 1 admis, 2^31 et 2^32 − 1 refusés sans allocation.
   Les décalages (représentants par jonction, enfants) sont en `u64`.
3. **Contraction** (`LEM-T4` = lemme P de `MATHEMATIQUES.md` § 10.3) : par groupe de rang (contigu), classes
   d'événements liés (l'un a pour opérande le sommet produit par l'autre) par un union-find local ; une classe = une
   multifusion N-aire ; numérotation des fusions par (rang, plus petite naissance) ; parents ; enfants en CSR triés.
   La boucle par groupe de rang est celle d'une version parallèle par tranches alignées sur les rangs.
4. **Juge** : identité avec la forêt publiée par la v11 (nombre de nœuds, rang, parent, clé de naissance, enfants et
   décalages, racine), identité des naissances canoniques avec `v11_node`, graines de rang strictement inférieur à leur
   cellule. Sur les ordres consécutifs : image de chaque naissance par **`LEM-T6`** (sommet laissé par la jonction de la
   même boule à l'ordre k − 1, remonté d'un cran si le parent a le rang de la boule ; nœud de naissance si la boule
   était déjà une naissance à k − 1) contre les verticales `FLOWER` de la v11.
5. **Preuves exigées** (`CST-0214`) : à tout ordre k ≥ 2, la section `FLOWER` est obligatoire et toutes les naissances
   de l'ordre sont jugées par `LEM-T6` (au moins une) ; une section absente, un ordre sans naissance, des en-têtes
   discordants (genre, ordre, K, trame, profil), des clés de naissance hors domaine ou non croissantes, une boule de
   cellule hors du catalogue ou des décalages décroissants rendent un refus explicite (ligne `refus`, code 3), jamais
   un succès. Le pilote exige de plus, par processus, une ligne par ordre aux comptes du vidage et `LEM-T6` complet.
6. **Porte** : `mhgp12_mes_m4 --porte [cas]` : borne du domaine des opérandes, trois témoins de plateau (ternaire
   d'une cellule, chaîné par deux cellules de même rang, plateaux disjoints puis fusion) et des hypergraphes aléatoires
   à rangs très répétés (2 à 40 naissances) contre un Kruskal par lots (sémantique de la v10,
   `preuves_tour/foret_check.py`). Le mutant `mhgp12_mes_m4_mutant_sans_contraction` doit être tué par la porte et par
   le banc sur vidage.

Temps à un fil, minimum de R passes : naissances, noyau, contraction. Option `--fils-contraction T` (pilote :
`--fils-contraction`) : **contraction parallèle** par tranches d'événements alignées sur les changements de rang, équipe
de T fils persistante et barrières entre phases (classes ; identifiants ; parents et nombres d'enfants ; décalages et
enfants triés), écritures disjointes par tranche, deux sommes préfixes par le fil 0 ; sortie exigée **identique** à la
contraction séquentielle (`identique` dans le JSON ; écart = code 1).

Domaine de chaque espace du noyau et de la contraction :

| Espace | Type | Domaine |
| --- | --- | --- |
| naissance (feuille) | `u32`, bit 31 libre | 0 à nb − 1, nb ≤ 2^31 − 1 (refus au-delà, avant allocation) |
| événement brut | `u32`, codé avec le bit 31 | 0 à nb − 2 (une union de moins que de naissances) |
| opérande | `u32` | feuille (bit 31 nul) ou événement (bit 31 mis) ; jamais la sentinelle dans le domaine |
| nœud final | `u32` | 0 à 2 nb − 2 ≤ 2^32 − 4 (au plus 2^32 − 3 nœuds) ; `0xFFFFFFFF` = absence (parent de la racine, clé d'une fusion) |
| rang, boule (clé de naissance) | `u32` | rangs et `BallIdx` de la v11, < `0xFFFFFFFF` (domaine de la v11) |
| décalages (représentants par jonction, enfants, traces, parties) | `u64` | sans borne pratique |

## 7. Sur G4

Le pilote est prêt pour une session gardée (le développeur principal la lance). Plan proposé (`{src}`, `{build}`,
`{data}`, `{out}` du plan de session v12) : K5 en un lot (`tout`, 5 processus) ; K10 en lots de moins de 30 minutes :
`construire portes vider m4 rapport` (5 processus), `construire m3 rapport` (3 processus), puis une commande
`construire resolution rapport` par trame (5 processus chacune, environ 7 minutes) ; publication par
`microbancs/outils/publier.py --exclude-suffix .bin` (vidages exclus). Le vidage tourne à 48 fils ; MES-M3, la mesure
de résolution et le noyau de MES-M4 restent à **un fil** (seule la contraction parallèle de MES-M4 utilise
`--fils-contraction`). Rapatrier `rapport_mes_m3_m4.json`, `tableaux.md` et les `*.jsonl` ; jamais les vidages (ils
dérivent de KITTI). Les temps locaux de `RAPPORT.md` ne prédisent pas G4.

Portes locales : `python3 -S -O tests/test_pilote.py [--binaires <construction>]` et
`python3 -S -O tests/test_m4_preuves.py --binaire <construction>/mhgp12_mes_m4`.
