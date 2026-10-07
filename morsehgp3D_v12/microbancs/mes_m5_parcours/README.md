# Microbanc MES-M5 : le parcours des boîtes du catalogue en largeur sur le GPU

7 octobre 2026. Microbanc de la tranche T0 de la v12 (`morsehgp3D_v12/docs/PLAN.md`, `MES-M5`), **hors produit**. Il
conditionne, avec `MES-M2` (adopté sur G4 le 7 octobre), l'entrée de la tranche T1 (catalogue).

```text
phase=exploration_v12_hors_registre
backend=cpu_reference (identité, warp simulé) ; cuda_g4 (banc, à jouer sur G4)
quantification=quantized_u21_input_only (fixtures u32 synthétiques pour la voie large)
public_status=not_claimed
GCP non utilisé
```

Question posée par `ARCHITECTURE.md` § 4.1 : le parcours des boîtes de centres peut-il se faire **en largeur sur
l'appareil** (réservoir des $3K$ témoins par nœud, filtre G1 par couple nœud–site dans le repère du parent, compactage
stable par préfixes, enveloppe par réduction segmentée, bissection), en rendant **le même ensemble final de feuilles**
que la v11 (`PLAN.md`, règle de `MES-M5`), et en combien de temps à côté de la frontière et de la passe unique de la
v11 sur la même machine ? La règle d'adoption est écrite au § 6, avant toute mesure sur GPU.

## 1. Carte du dossier

| Chemin | Rôle |
| --- | --- |
| `include/mhgp12/traversal/bfs.hpp` | **le parcours en largeur, source unique `__host__ __device__`** : enregistrements, arithmétique exacte, sommet des $3K$ clés d'un warp, neuf corps de noyaux, mutants |
| `include/mhgp12/traversal/driver.hpp` | pilote des niveaux, écrit une fois pour les deux exécuteurs (hôte simulé, CUDA) |
| `include/mhgp12/traversal/warp.hpp` | collectives de warp ajoutées (échange par XOR, minimum, maximum, somme et préfixe sur 64 bits) au-dessus de `simt.hpp` de `MES-M2`, inchangé |
| `include/mhgp12/traversal/format.hpp` | format de vidage `MHGP12TR` v1 et son lecteur strict (primitives FNV-1a de `MES-M2`) |
| `include/mhgp12/traversal/compare.hpp` | comparaison indépendante de l'ordre : statut, grand livre, ensemble des feuilles, empreinte canonique |
| `vidage/traversal_dump.cpp` | vidage du parcours de la v11 gelée (nœuds, feuilles, grand livre), lié à `libmhgp11.a` |
| `vidage/v11_timing.cpp` | chrono de la frontière et de la passe unique de la v11 (W fils, passes chaudes), et du `walk` séquentiel |
| `host/traversal_identity.cpp` | identité sur l'hôte (warp simulé) contre les vidages, nœuds compris, mutants, portes unitaires |
| `cuda/traversal_bench.cu` | banc CUDA : exécuteur appareil du même pilote, chrono, profil par niveau, identité, mutants |
| `oracle/oracle_parcours.py` | oracle du parcours de la v11 en entiers Python exacts (arithmétique volontairement autre) |
| `fixtures/fixtures.py` | six fixtures synthétiques (u21 et u32) et leurs vidages de référence |
| `scripts/g4_traversal_bench.py` | session G4 complète (bibliothèque standard) : construit, vide, vérifie, mesure, juge ; `--selftest-judge` |
| `CMakeLists.txt` | CMake ≥ 3.20 (3.22.1 de la VM), C++20, `-Wall -Wextra -Wpedantic -Werror` côté hôte, CUDA sm_120 |
| `RAPPORT.md` | résultats locaux du 7 octobre |

Réutilisé de `MES-M2` sans copie (`MHGP12_M2_DIR`, par défaut le dossier frère `../mes_m2_feuille`) : le warp en
source unique `include/mhgp12/leaf/simt.hpp`, les primitives de vidage `include/mhgp12/leaf/dump_format.hpp`, et du
script `scripts/g4_leaf_bench.py` la session (`Session`, journaux), l'environnement, la recherche de `nvcc`, la
construction de la v11 et les empreintes. Les vidages des trames (environ 1 Go pour neuf cas) sont dérivés de
SemanticKITTI : **jamais dans le dépôt** ; seuls leurs comptes et leurs empreintes sont publiés.

## 2. Le parcours de la v11, règle par règle

Retrouvé dans `morsehgp3D_v11/src/catalogue/boxes.cpp` (moteur `ac081a06f`) ; la frontière (fixe ou adaptative) et la
passe unique ne changent que l'ordonnancement, pas l'arbre.

1. **Racine** : tous les sites, dans l'ordre `SiteIdx` (ordre de Morton du nuage préparé) ; boîte = enveloppe
   $[\min,\max+1)$ ; profondeur 0.
2. **Nœud** (boîte $B=[lo,hi)$, liste parente $L$, profondeur $d$) : refus si $d>3B$ ; puis
   - **réservoir** : les $\min(\lvert L\rvert,3K)$ sites de $L$ les plus proches du centre de $B$, clé
     $\sum_j(2x_j-lo_j-hi_j)^{2}$ puis **rang dans $L$** (le tri par insertion de `reservoir` garde le premier arrivé à
     égalité et évince le dernier : c'est exactement les $3K$ premiers de $L$ triée par (clé, rang)) ;
   - **filtre G1** : pour chaque site $x$ de $L$ dans l'ordre, les témoins sont examinés dans l'ordre du réservoir,
     arrêt au $K$-ième dominateur strict (`tests` = témoins examinés) ; $x$ est retiré s'il a $K$ dominateurs ; termes
     $2(hi_j-lo_j)(x_j-lo_j)$ et $\sum_j(x_j-lo_j)^{2}$ ; un site n'est jamais son propre dominateur ;
   - **boîte ajustée** : enveloppe $[\min,\max+1)$ de la liste retenue, intersectée avec $B$ ; le nœud est vide si la
     liste est vide **ou** si la boîte ajustée est vide.
3. **Feuille ou coupe** : axe de plus grande largeur de la boîte ajustée (le premier à égalité), largeur $w$ ;
   feuille si $\lvert\text{liste}\rvert\leq$ taille de feuille ou $w\leq 1$ (refus `wide_leaf` au-delà de `max_leaf`) ;
   sinon milieu $lo+\lfloor w/2\rfloor$, gauche $[lo,\text{milieu})$, droite $[\text{milieu},hi)$, et les deux enfants
   lisent la liste retenue du parent.
4. **Ce que le parcours transmet à la feuille** : la liste retenue (`SiteIdx` croissants) et la boîte ajustée
   demi-ouverte (`enumerate_leaf(run, ready.sites(), ready.box)`).
5. **Grand livre** (`CatalogueLedger`) : nœuds (appels de `prepare_node`, vides compris), feuilles, tests G1,
   profondeur maximale (vides comprises), feuille maximale. Ces sommes et maxima ne dépendent pas de l'ordre de visite.

## 3. Vidage `MHGP12TR` v1

`mhgp12_traversal_dump <xyz.u32le> <ids.u32le> <K> <feuille> <sortie.bin> [--crop N] [--max-leaf M]` prépare le `Cloud`
de la v11 (profil u21), puis fait **deux** parcours : une capture qui reprend mot pour mot le contrôle de
`walk`/`process`/`run_ready` mais appelle les fonctions **gelées** `prepare_node` et `split_ready`, et le `walk` gelé
lui-même avec une file différée (comme `MES-M2`). Le grand livre et les feuilles mises en file du `walk` doivent égaler
la capture (code 3 sinon) : le vidage est le parcours de la v11, pas une réécriture.

Contenu (`format.hpp`) : en-tête de 256 octets (profil, K, tailles de feuille, comptes, statut, grand livre), nuage,
**nœuds** en ordre préfixe (96 octets : boîte d'entrée, chemin, tests, empreinte de la liste retenue, candidats,
retenus, profondeur, genre), **feuilles** (80 octets, dont les 64 premiers ont la disposition de `LeafJob` de la v11),
sites des feuilles, FNV-1a final. Le **lecteur strict** (leçon de `CST-0215`) contrôle avant toute allocation la taille
du fichier contre les comptes, puis le profil, K, les tailles de feuille, les coordonnées dans $[0,2^{B})$, les boîtes
dans $[0,2^{B}]$, l'arbre entier reconstruit par une pile (enfants = moitiés de la boîte ajustée du parent, axe de coupe
au premier maximum, candidats = retenus du parent, profondeurs, chemins), les feuilles (= nœuds feuilles dans l'ordre,
débuts contigus sans débordement, sites bornés et strictement croissants, empreinte de liste égale à celle du nœud) et
le grand livre (= sommes des sections). Un refus (`wide_leaf`, profondeur) ne publie aucun préfixe.

`--crop N` garde les N premiers sites de l'ordre de Morton (une région compacte de la trame) : vidage de contrôle pour
Compute Sanitizer.

## 4. Le parcours en largeur (`bfs.hpp`, `driver.hpp`)

**Forme.** Un niveau = tous les enfants d'une même profondeur. Chaque enfant lit la liste retenue de son parent ; ses
candidats sont découpés en **tâches** de 256 sites (8 paquets de 32), **une tâche par warp**. Neuf noyaux par niveau,
tous faits de warps indépendants (aucun atomique, aucune synchronisation de bloc ; résultats indépendants de l'ordre
d'exécution des warps) :

| Noyau | Grain | Rôle |
| --- | --- | --- |
| `Select` | tâche | sommet local des $3K$ clés (clé, rang) du morceau : tri bitonique de warp par paquet, fusion par rangs (recherche dichotomique) avec le sommet courant en mémoire partagée ; paquet sauté s'il n'a aucune clé sous le seuil |
| `Merge` | enfant | réservoir = sommet des sommets locaux (même fusion) ; rien à faire si l'enfant a une seule tâche |
| `Filter` | tâche | termes G1 des témoins en mémoire partagée, test par couple (enfant, candidat) dans l'ordre du réservoir avec arrêt au $K$-ième, masques de garde par vote, comptes, tests, enveloppe locale |
| `Close` | enfant | préfixe des comptes de ses tâches (**compactage stable**), enveloppe par réduction, boîte ajustée, genre, repère de l'enfant |
| `ScanA/B/C` | tuile de 1 024 enfants | préfixes sur les enfants : parents du niveau suivant, sites de leurs listes, tâches, feuilles, sites des feuilles ; totaux du niveau |
| `Scatter` | tâche | écriture stable des sites retenus (rang = sites gardés des voies inférieures du paquet) dans la liste du niveau suivant ou dans l'arène des feuilles |
| `Emit` | enfant | enregistrements des parents du niveau suivant et des feuilles (`LeafJob`, profondeur, chemin) |

Le pilote lit les **totaux** du niveau après `ScanB` (un seul aller-retour par niveau, mémoire épinglée), dimensionne
le niveau suivant, puis lance `Scatter` et `Emit`. Les tampons sont gardés d'une passe à l'autre : en régime
résidant, aucune réservation après l'échauffement (compte publié ; une réservation pendant les passes chronométrées
refuse le banc). La profondeur est bornée par $3B$ (`CST-0205`) : 63, 72 et 96 niveaux aux profils 21, 24 et 32,
atteints tous les trois par les coquilles de l'auditeur (63 en u21, 96 en u32, § 5).

**Arithmétique** ([contrat numérique](../../docs/CONTRAT_NUMERIQUE.md), § 2 et § 3). Toutes les décisions sont
entières. Le filtre d'un enfant (réservoir et G1) s'évalue dans le **repère du parent**
$E=\overline{\text{boîte ajustée}}\cup L$ (`NUM-COUVERTURE`, `CST-0112`), d'étendue $s$ calculée par `Close` depuis
l'enveloppe de la liste et la boîte ajustée : voie native `i64` si $2s+4\leq 63$, soit $s\leq 29$ (`CST-0208`), voie
large `i128` au-delà, jusqu'à $s=33$ (boîtes fermées à $2^{32}$, `CST-0204`, bornes en `i64`). Au profil u21,
$s\leq 22$ : toujours la voie native, avec les mêmes entiers que `boxes.cpp`. La voie est uniforme sur le warp (une
tâche = un enfant). Aucune décision en flottant.

**Source unique.** Les corps de noyaux suivent la discipline « super-pas » de `simt.hpp` (`MES-M2`) : code uniforme,
blocs `MHGP12_LANES` sans collective, collectives à masque plein, `__syncwarp` entre écriture partagée et lecture par
une autre voie. Sur l'hôte, un noyau est une boucle sur ses warps, chaque warp simulé voie après voie ; sur
l'appareil, un noyau générique lance un warp par indice (4 warps par bloc, mémoire partagée propre à chaque warp).

## 5. Identité, mutants, fixtures, oracle

- **Identité** (`compare.hpp`) : même statut, même grand livre, et **même ensemble de feuilles**, rangées par boîte
  (les boîtes des feuilles sont disjointes, donc distinctes) : boîte, liste comparée comme suite exacte, profondeur,
  chemin ; empreinte canonique FNV-1a. Sur l'hôte, en plus, **tous les nœuds visités** (boîte d'entrée, chemin,
  candidats, retenus, tests, genre, empreinte de la liste), rangés en ordre préfixe.
- **Mutants** (paramètre de modèle, jamais dans le banc chronométré) ; tué = statut, grand livre ou feuilles
  différents :

| Mutant | Faute | Doit être tué par |
| --- | --- | --- |
| `temoin_perdu` | réservoir de $3K-1$ témoins | trames LiDAR |
| `repere_enfant` | repère pris sur la seule boîte de l'enfant (mutant « repère pris sur la seule boîte » du contrat § 7) | fixture u32 `coquille48_u32_k5_l24` (équivalent au profil u21 : voie native partout) |
| `compactage_instable` | sites retenus d'un paquet écrits dans l'ordre inverse des voies | trames LiDAR |
| `bissection_decalee` | milieu $lo+\lceil w/2\rceil$ | trames LiDAR |
| `ex_aequo_inverses` | ex æquo du réservoir départagés par rang décroissant | trames LiDAR |
| `axe_dernier_maximum` | axe de coupe au dernier maximum | trames LiDAR |

- **Fixtures** (`fixtures.py`, synthétiques, régénérées à chaque session) :

| Fixture | Profil | Référence | Rôle |
| --- | --- | --- | --- |
| `coquille24_k2_l16` | u21 | v11, recontrôlée par l'oracle | profondeur 60 (témoin `CST-0205`) |
| `coquille48_k5_l24` | u21 | v11, recontrôlée par l'oracle | profondeur 63 = $3B$ |
| `coquille48_k5_l8_m8` | u21 | v11, recontrôlée par l'oracle | refus `wide_leaf`, aucun préfixe publié |
| `coquille48_u32_k5_l24` | u32 | oracle | profondeur 96 = $3B$ ; repère du parent à 32 bits (voie large), boîtes d'enfant étroites : tue `repere_enfant` |
| `bord_u32_k2_l5` | u32 | oracle | boîte racine $[0,2^{32})$, repère fermé à 33 bits (`CST-0204`) |
| `uniforme_u32_k3_l8` | u32 | oracle | 300 sites uniformes : voies large et native dans un même parcours |

- **Oracle** (`oracle_parcours.py`) : réécriture indépendante des règles du § 2 en entiers Python de taille
  arbitraire ; `--check` recalcule un vidage (nœuds, feuilles, sites, grand livre) et le compare entièrement ; il
  reproduit la v11 sur les trois fixtures u21 avant de servir de référence aux fixtures u32.
- **Portes unitaires** (`--unit`) : repère fermé de 33 bits pour les sites $(0,0,0)$ et $(2^{32}-1,0,0)$ ; clé du
  réservoir $3(2^{31}-3)^{2}$ au-delà d'`i64` pour la boîte $[0,1]^{3}$ et le site $(2^{30}-1)^{3}$, repère du parent à
  30 bits (voie large) alors que la seule boîte en demande 1 ; borne $3B$.

## 6. Règle d'adoption, écrite d'avance

Écrite le 7 octobre 2026, avant toute mesure sur GPU ; jamais réécrite après les données (`PLAN.md` § 0,
`MESURE.md` § 5) ; mise en œuvre par `judge()` du script G4, dont l'auto-test (`--selftest-judge`, seize injections)
fige le comportement.

1. **Cas** : ng00, ng01, ng02 à K5/16, K5/24 et K10/24 (neuf vidages). **Décident** les six cas à feuilles de 24
   (K5/24 et K10/24), configuration GPU de la v11 (`868347`) ; les trois cas K5/16 sont mesurés et publiés, et
   l'identité y est exigée aussi. Les nuages uniformes de 8 000, 16 000 et 32 000 sites à K5/24 (tailles d'intérêt de
   `CLAUDE.md`, `uniform_u18_n*.u32le` de `{data}`) sont mesurés et publiés quand ils sont fournis, identité exigée ;
   ils ne décident pas.
2. **Identité exigée** : sur l'hôte (warp simulé) et sur l'appareil, même statut, même grand livre et même ensemble
   de feuilles que la v11 sur les neuf cas, et que la référence sur les six fixtures ; sur l'hôte, tous les nœuds.
   Chaque mutant tué : les cinq mutants « LiDAR » chacun sur au moins une trame, `repere_enfant` par la fixture
   `coquille48_u32_k5_l24` sur l'hôte **et** sur l'appareil. Portes unitaires conformes. Compute Sanitizer
   (`memcheck`, `racecheck`, `synccheck`) sans erreur sur la découpe de 4 000 sites de ng00 et deux fixtures u32.
3. **Mesure** : 1 + 5 tours ; à chaque tour et pour chaque cas (ordre tournant), un processus de chrono v11 puis un
   processus du banc GPU ; le tour 0 est jeté. GPU : médiane de 15 passes chronométrées (après 3 d'échauffement) de
   `total` = copie du nuage + tous les niveaux + rapatriement de la table des feuilles et de leurs sites
   (**transferts compris**, événements sur le flux). v11 : médiane des passes 2 à 10 de `prefix_ns + single_pass_ns`
   (frontière adaptative et passe unique, 48 fils, configuration `868347`, feuilles mises en file et traitées par
   l'exécuteur hôte au lieu du GPU, ce qui ne touche aucun des deux sous-étages) ; le grand livre du catalogue de ce
   chrono doit égaler celui du vidage.
4. **Statistique** : par cas, rapport des médianes du tour (GPU / v11), moyenne géométrique sur les 5 tours,
   intervalle à 95 % par bootstrap sur les tours (10 000 tirages, graine 20261007).
5. **Verdicts** : **adopté** si l'identité, les mutants, les portes, les fixtures et Compute Sanitizer sont conformes,
   le banc valide, **et la borne haute de l'intervalle au plus 1/4 sur chacun des six cas qui décident** ;
   **rejeté** si l'identité est en défaut (hôte, appareil ou fixture) ou si une borne haute dépasse 1/4 sur un cas
   qui décide ; **refusé** si le banc est invalide : construction, vidage, fixture ou porte unitaire en échec, tour
   manquant, résultat périmé ou incohérent avec sa commande (vidage, K, taille de feuille, nombre de passes), durée non
   finie ou nulle, réservation pendant les passes chronométrées, binaire modifié pendant la session, grand livre du
   chrono v11 différent du vidage, mutant non tué, Compute Sanitizer en erreur ou non joué, isolation du GPU non
   certifiée. Seul « adopté » permet l'adoption.

**Pourquoi 1/4.** Le budget du catalogue à K5 est de 35 à 45 ms sur ng00 (`ARCHITECTURE.md` § 3). Il contient la
feuille J3 adoptée par `MES-M2` (11,6 ms de noyau seul sur ng00 K5/24), la fin d'étage sur l'appareil (tri radix des
clés F3, rangs, CSR, table $S^{*}$ : 21 ms sur le CPU de la v11 à 48 fils, 2 à 10 ms visés sur l'appareil) et les
transferts et replis (2 à 5 ms). Il reste au parcours **10 à 20 ms**. La même étape de la v11 vaut 58 ms sur G4
(frontière 20,7 ms + passe unique 37,5 ms, ng00 K5/24, `MESURE.md` § 3.2) : 15 ms en sont un quart. La même fraction
vaut à K10, dont l'objectif global (0,3 à 0,5 s) laisse au catalogue résident 70 à 130 ms. Le seuil porte sur la
mesure la plus défavorable au GPU (rapatriement des feuilles compris, que la Session résidente ne paiera pas).

**Publié sans décider** : temps absolus, temps résidant (feuilles laissées sur l'appareil), temps des noyaux seuls
et des allers-retours par niveau (passe de profil), cas K5/16, `walk` séquentiel de la v11 à un fil, statistiques par
niveau (parents, enfants, tâches, candidats, tests).

## 7. Session G4

Commandes pour un plan de session (forme de `gcp-migration/README_V12.md`), une fois ce dossier versé sous
`morsehgp3D_v12/microbancs/mes_m5_parcours/` ; la première est celle de la session `t0a` (sources de la v11 gelée
déballées et `libmhgp11.a` construite sur la VM), à omettre si le plan la contient déjà :

```json
{"name": "source_v11", "timeout_seconds": 900,
 "argv": ["python3", "{src}/morsehgp3D_v12/microbancs/outils/source_v11.py",
          "--archive", "{data}/v11_src_ac081a06f.tar.gz",
          "--sha256", "6f3454ad9d9ad2f6bb0b846f1aaad7c1a4c509d14cd450a99381c72b04a57885",
          "--dest", "{build}/v11src", "--build", "{build}/v11build", "--jobs", "44",
          "--report", "{out}/source_v11.json"]},
{"name": "m5", "timeout_seconds": 2400,
 "argv": ["python3", "{src}/morsehgp3D_v12/microbancs/mes_m5_parcours/scripts/g4_traversal_bench.py",
          "--out", "{out}/m5", "--work", "{build}/m5", "--data", "{data}",
          "--repo", "{build}/v11src", "--v11-lib", "{build}/v11build/libmhgp11.a", "--jobs", "44"]}
```

Données de `{data}` : `lidar_ng00`, `lidar_ng01`, `lidar_ng02` (`.u32le` et `.ids.u32le`), `uniform_u18_n8000`,
`uniform_u18_n16000`, `uniform_u18_n32000` (idem) et `v11_src_ac081a06f.tar.gz` : les quatorze fichiers de la session
`t0a`. Le script refuse un `--work` placé dans `--out` : vidages, fixtures et constructions restent dans `{build}`
(environ 1,1 Go), seuls le rapport, les journaux et les JSON des prises vont dans `{out}` (environ 1 Mo sans GPU) ;
aucun `publier.py` n'est nécessaire. Durée : 6 à 8 minutes sur le codespace sans GPU (4 fils, un tour) ; estimée à 12
à 20 minutes sur G4 (constructions et vidages plus rapides, mais six tours, Compute Sanitizer et passes GPU). Codes : 0
rapport écrit (quel que soit le verdict), 2 refus avant toute mesure. Avant la session : `python3 -S -O
scripts/g4_traversal_bench.py --selftest-judge` (code 0).

## 8. Prédiction, écrite avant G4

Voir `RAPPORT.md` § 6 (chiffrée à partir des statistiques par niveau mesurées sur l'hôte et des coûts de Session de
`MES-M6`).

## 9. Limites, risques, et ce qui reste pour la tranche T1

- **Jamais exécuté sur un GPU** : le code d'appareil est compilé (sm_120, ptxas ; hôte GCC 13.3 et GCC 11.5) et son
  texte est celui de l'identité hôte, mais les collectives ajoutées (`warp.hpp`), les noyaux et l'exécuteur CUDA ne
  sont jugés que par G4. GCC 11.4 de la VM non essayé : GCC 11.5, Clang 18 et GCC 13.3 compilent sans avertissement.
- Un aller-retour par niveau (34 à 39 niveaux sur les trames, 97 sur la coquille u32) : coût publié par la passe de
  profil ; levier T1 : tailles du niveau lues sur l'appareil et lancements sans attente (graphes, grilles fixes).
- Les premiers niveaux ont peu d'enfants mais de longues listes (39 885 candidats à la racine) : le découpage en tâches
  de 256 sites les répartit sur 150 à 1 800 warps ; `Merge` y reste un warp par enfant, élagué (il ne fusionne que
  34 % des sommets locaux à ng00 K5/24) mais en série sur les sommets retenus.
- Ex æquo de clé du réservoir : départagés par le rang dans la liste parente, comme la v11 ; le mutant
  `ex_aequo_inverses` montre qu'ils existent et comptent sur les trames.
- La voie large `i128` n'est exercée que par les fixtures u32 ; aucune trame réelle u24 ou u32 n'est vidée ici.
- **Pour T1**, dans l'ordre :
  1. jouer cette session G4 (identité sur l'appareil, Compute Sanitizer, temps) ; sans « adopté », pas de port ;
  2. porter le parcours dans `src/catalogue/` comme **seul** chemin (le texte de `bfs.hpp` et `driver.hpp`, sans les
     mutants ni l'exécuteur simulé, qui restent dans le microbanc comme référence de test) ;
  3. brancher les feuilles **en flux** sur la feuille J3 de `MES-M2` : la table `Leaf` a déjà la disposition de
     `LeafJob` et l'arène des sites est sur l'appareil ; recouvrir les niveaux profonds du parcours par les feuilles ;
  4. **ordre des sites** : l'ensemble des feuilles dépend de l'ordre `SiteIdx` par le départage des ex æquo du
     réservoir (le mutant `ex_aequo_inverses` change l'ensemble des feuilles sur six trames sur neuf). Si la v12 change
     cet ordre (clé de Morton exacte sur les coordonnées normalisées, `CST-0202`, `CST-0113`), le différentiel du
     parcours contre la v11 se fait à ordre égal (nuage de la v11), et celui du produit au niveau du catalogue (les
     listes changent, les boules non) ;
  5. supprimer l'aller-retour par niveau (tailles lues sur l'appareil, grilles fixes ou graphes) si le profil G4 le
     montre coûteux, et le `Merge` en série des premiers niveaux si son temps compte ;
  6. budgéter la mémoire des niveaux et de l'arène des feuilles dans le budget de la Session (prévision, admission,
     réservation, `CST-0211`) ; refus transactionnel au-delà des indices 32 bits des tâches (déjà refusé ici, statut
     `capacité`) ; régime de plusieurs millions de sites (lots de feuilles, `ARCHITECTURE.md` § 4.6) ;
  7. qualifier u24 puis u32 sur des données réelles : la voie large `i128` n'est exercée ici que par des fixtures.
