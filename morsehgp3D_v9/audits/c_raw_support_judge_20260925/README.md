# Juge à supports indépendants sur les trames brutes de R21 (auditeur C)

25 septembre 2026. Réalisation de la porte causale spécifiée par B dans
[`b_full_raw_completeness_gap_20260924`](../b_full_raw_completeness_gap_20260924/README.md).
La chaîne jugée est celle des [épingles brutes](../c_raw_pins_20260924/README.md) :
bibliothèques CPU Release de la base `093d943ce` (`BASE.txt`), leviers
par défaut (moteur), `s=8`, 8 fils. Juge v3, après deux revues adverses.
GCP non utilisé.

## Verdict

Sur les **six cas** (b00, b01, b02 × K5, K10), le juge est **vert**. Pour
chaque cas :

- l'entrée est vérifiée (SHA-256 et FNV) et le catalogue de la chaîne
  reproduit son condensé épinglé dans `PINS_RAW.json` ;
- toutes les boules admissibles fabriquées depuis les seules coordonnées
  sont au catalogue, avec les **mêmes** clé, niveau exact, `p`, `q_min` et
  ensembles complets d'IDs d'intérieur et de coquille, soit 675 à 1 024
  clés distinctes par cas ;
- les quatre strates exigées sont non vacantes côté oracle, et leurs
  témoins sont imprimés avant l'appel de la chaîne.

Les 14 mutants sont tués, chacun par son **seul** marqueur attendu
(code 4). La fixture cosphérique retrouve exactement ses 8 boules à
coquille étendue, à K5 et à K10.

Ce PASS est une **recherche adverse bornée et indépendante**. Ce n'est :

- ni un théorème de complétude : 0,008 à 0,026 % des 2,7 à 11,7 M clés
  de chaque catalogue sont atteintes ;
- ni une preuve GPU/G4 ;
- ni une qualification multi-séquence ou float32.

## Ce que fait le juge

Source : `raw_support_judge.cpp`. L'en-tête du fichier fait foi. Le juge
fabrique ses supports depuis l'**entrée seule**, jamais depuis les boules
émises.

1. **Ancres.** 32 ancres sont figées et imprimées (ligne `ANCHORS`) avant
   tout appel de la chaîne :
   - 16 viennent d'une permutation de graine publiée
     (`5eed2026c0ffee25`) ;
   - 8 viennent de la queue de densité : plus grande distance au 10e
     voisin, départage par ID ;
   - 8 viennent des huit quantiles de distance au centre entier de la
     boîte englobante, en prenant le minimum d'un hachage de l'ID.

   Une ancre déjà prise est remplacée par la suivante de sa catégorie.
   Moins de 32 ancres donne `ANCHOR_GRID_SHORT`, code 3.
2. **Supports.** Pour chaque ancre, les partenaires sont ses 12 plus
   proches voisins plus les rangs 32, 64, 128 et 256, dans l'ordre
   `(distance², ID)`. Le juge prend tous les supports de taille 2, 3 ou
   4 qui contiennent l'ancre : 22 272 par cas.
3. **Oracle distinct du produit.**
   - Le centre est résolu par Gram/Cramer en `cpp_int`. Un support est
     positif si ses poids barycentriques sont tous strictement positifs.
   - La sphère est réduite à sa forme puissance primitive.
   - Le census couvre **tous** les sites de la trame, avec l'arbre k-d du
     juge : filtre flottant conservateur, signe entier exact aux feuilles.
     Il s'arrête au-delà de K−1 intérieurs.
   - `q_min` est calculé par les sous-ensembles positifs de taille 2 à 4
     d'une coquille de 24 sites au plus.
   - Au-delà de 24 sites, seule l'admissibilité est décidée, par cas :
     - si `p + |support| ≤ K+1`, la boule est admissible, car le support
       est lui-même positif ;
     - sinon, le juge cherche un sous-ensemble positif de taille au plus
       `K+1−p` (2 ou 3), jusqu'à 96 sites ;
     - au-delà de 96 sites, le résultat est `SHELL_DOMAIN` par clé
       distincte, code 3.
   - Une boule admissible dont la coquille dépasse 12 sites donne
     `SHELL_OVER_12`, code 1, car la chaîne aurait dû la refuser
     (`chain_shell_above_12`).
4. **Publication avant comparaison.** Avant l'appel de la chaîne, le juge
   imprime :
   - la ligne `EXPECTED`, avec les comptes par strate côté oracle ;
   - une ligne `WITNESS` par strate non vide, avec clé, `p`, `q`, support
     et IDs.
5. **Liaison.**
   - En mode `file`, `--expect-catalogue-digest` est obligatoire, sauf
     option explicite `--unpinned` ; une valeur vide ou mal formée est
     refusée.
   - `--expect-input-fnv` lie l'entrée ; la casse est ignorée.
   - Le script de campagne vérifie aussi le SHA-256 de chaque entrée.
   - La bijection ID d'entrée ↔ indice géométrique est vérifiée avant
     toute comparaison, et chaque ID du catalogue est borné
     (`CATALOGUE_ID_RANGE`).
6. **Comparaison exacte champ par champ.** Chaque écart produit un
   marqueur distinct : `MISSING_KEY`, `LEVEL_DIFF`, `INTERIOR_DIFF`,
   `SHELL_IDS_DIFF` ou `ARITY_DIFF`. Le marqueur donne la clé, l'ancre,
   le support et les IDs, et pour `LEVEL_DIFF` les deux niveaux. Aucune
   empreinte somme/xor ne remplace les IDs.
7. **Strates exigées**, toutes à boule **régulière**. Une strate vacante
   donne `VACUOUS strata=…`, code 3.
   - q2 à `p=K−1` ;
   - q3 à `p=K−2`, dont une longue (arête max ≥ 1 600 unités, 1,6 m sur
     la grille 1 mm) ;
   - q4 à `p≤K−3`.
8. **Mutants auto-vérifiés** (copies du catalogue). Le code est 4 si et
   seulement si l'unique écart est le marqueur attendu ; sinon le juge
   imprime `MUTANT_NOT_KILLED`, code 1.

   | mutant | action | marqueur attendu |
   | --- | --- | --- |
   | `drop-q2`, `drop-q3`, `drop-q4` | retrait d'une clé de la strate | `MISSING_KEY` |
   | `shell-sub` | ID de coquille substitué, taille égale | `SHELL_IDS_DIFF` |
   | `level` | dénominateur du niveau + 1 | `LEVEL_DIFF` |
   | `interior-sub` | ID d'intérieur substitué | `INTERIOR_DIFF` |
   | `ext-trim` | plus grande coquille étendue tronquée à son arité | `SHELL_IDS_DIFF` |
   | `ext-arity` | arité d'une q3 étendue faussée | `ARITY_DIFF` |

Codes de sortie : 0 conforme ; 1 écart ou mutant non tué ; 2 argument,
entrée, chaîne ou condensé ; 3 strate vacante, domaine, grille courte ou
compte étendu ; 4 mutant tué.

## Résultats

Hôte local partagé, 8 fils. Les temps n'ont aucune valeur de
chronométrage : les gadgets tournaient en même temps. Le CPU·s total est
voisin de celui du seul moteur dans les épingles (288/225/268 à K5,
895/732/864 à K10) : l'oracle pèse peu, mais il n'est pas chronométré à
part. Les comptes par strate sont ceux de la ligne `EXPECTED`, côté
oracle, imprimée avant l'appel de la chaîne ; ils sont égaux aux comptes
des boules trouvées.

| cas | positifs | trop profonds | admissibles | clés attendues | trouvées | écarts | q2 `p=K−1` | q3 `p=K−2` (longues) | q4 `p≤K−3` | étendues | boules du catalogue | condensé = épingle | CPU·s | RSS |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- | ---: | ---: |
| b00 K5 | 1 407 | 624 | 697 | 675 | 675 | 0 | 49 | 69 (29) | 56 | 0 | 2 822 052 | `11f6a8e1a7f28127` | 291 | 1,92 Gio |
| b01 K5 | 1 424 | 617 | 719 | 712 | 712 | 0 | 41 | 76 (18) | 53 | 0 | 2 747 970 | `7d385c14e5870263` | 228 | 1,87 Gio |
| b02 K5 | 1 380 | 572 | 733 | 730 | 730 | 0 | 39 | 70 (25) | 31 | 0 | 3 071 514 | `f0206be1c838c4bd` | 264 | 2,07 Gio |
| b00 K10 | 1 407 | 411 | 974 | 949 | 949 | 0 | 10 | 15 (8) | 123 | 0 | 11 387 391 | `1a315a5241510296` | 801 | 6,42 Gio |
| b01 K10 | 1 424 | 376 | 1 031 | 1 024 | 1 024 | 0 | 4 | 15 (10) | 116 | 0 | 10 663 312 | `dbbfc30b411e11ea` | 648 | 6,03 Gio |
| b02 K10 | 1 380 | 376 | 986 | 983 | 983 | 0 | 8 | 20 (9) | 54 | 0 | 11 684 593 | `8c39ed9e85d7c0fd` | 761 | 6,56 Gio |

Précisions sur les colonnes :

- Supports dégénérés : 6, 8 et 21 par trame.
- « Admissibles » compte les supports ; « clés attendues » compte les
  boules distinctes.
- Aucune coquille > 12 et aucun échec de domaine.
- Toutes les formes tiennent en i128 (`big_forms=0`).
- Les empreintes d'entrée FNV (`4120701a6194c19b`, `d2bd37fb9befdd7d`,
  `583db2f3deafe8e9`) et les SHA-256 sont ceux de `PINS_RAW.json`.
- Les 32 ancres de chaque trame et un témoin par strate (clé, support,
  IDs) figurent dans `outputs/<cas>.txt`.

## Témoins de la porte elle-même

| témoin | attendu | obtenu |
| --- | --- | --- |
| `drop-q2`, `drop-q3`, `drop-q4` sur b00 K5 | code 4, `MISSING_KEY` seul | code 4 chacun, 674/675 |
| `shell-sub`, `level`, `interior-sub` sur b00 K5 | code 4, respectivement `SHELL_IDS_DIFF`, `LEVEL_DIFF`, `INTERIOR_DIFF` seul | code 4 chacun |
| fixture cosphérique K5, `--expect-extended=8` | code 0 | 1 155/1 155, étendues 8 (q2 6, q3 1, q4 1), q4 régulières 205 |
| fixture cosphérique K10, `--expect-extended=8` | code 0 | 1 414/1 414, étendues 8 (q2 6, q3 1, q4 1), q4 régulières 287 |
| les huit mutants sur la fixture K5 | code 4, marqueur attendu seul | code 4 chacun ; `ext-trim` tronque le cube (coquille 8 → 2), `ext-arity` fausse la q3 étendue (3 → 4) |

Contrôles des options, exécutés à la main sur la fixture et les gadgets
(sorties non archivées) :

| option | code |
| --- | ---: |
| `--expect-extended=7` | 3 (`EXTENDED_COUNT`) |
| `--expect-extended=-1` | 2 |
| condensé ou FNV attendus faux | 2 (`CATALOGUE_DIGEST_DIFF`, `INPUT_FNV_DIFF`) |
| mode `file` sans condensé | 2 |
| valeur de condensé vide | 2 |
| majuscules | 0 |

## Revues adverses

**Première revue (v1).** Cinq défauts ont été confirmés, puis corrigés
en v2 :

- le test d'admissibilité de `SHELL_OVER_12` était toujours vrai ;
- les marqueurs n'identifiaient pas la boule ;
- aucune liaison aux épingles ;
- aucun plancher de boules étendues ;
- aucun mutant de boules étendues.

**Seconde revue (v2).** Trois lecteurs indépendants ont relu la v2 :
correctifs, conformité à la spécification de B, arithmétique.

- **Aucun faux vert.** La résolution, la positivité, la forme primitive,
  le census et le niveau concordent avec une re-dérivation exacte en
  fractions sur 89 000 supports de gadgets riches en égalités, jusqu'à
  2^18−1.
- **Un faux rouge.** `SHELL_DOMAIN` tombait sur toute coquille de plus
  de 24 sites avant le test d'admissibilité. C'est la même classe que le
  défaut v1, déplacée au-delà de 24 sites.
- **Écarts à la spécification :**
  - ancres perdues sans remplacement ;
  - comptes par strate et témoins non publiés avant la comparaison ;
  - strate vacante sans marqueur ;
  - mutants non auto-vérifiés ;
  - liaison facultative ;
  - témoin q4 acceptant une coquille étendue ;
  - IDs du catalogue non bornés ;
  - marqueurs incomplets ;
  - aucun mutant de niveau ni d'intérieur ;
  - commentaire de la fixture inexact.

Tout est corrigé en v3. Les gadgets des deux revues sont archivés dans
`gadgets/`, avec leurs codes attendus en v3 (`run_gadgets.sh`, 9/9
conformes) :

| gadget | K | code | constat |
| --- | ---: | ---: | --- |
| `over12_k3` | 3 | 0 | coquille de 13 sites hors fenêtre : plus de faux `SHELL_OVER_12` (510/510, 5 étendues) |
| `mine_over12` | 3 | 3 | 683/683, 30 étendues ; `VACUOUS strata=q3crit_long` sur ce petit gadget |
| `dom24`, `dom26` | 3 | 0 | coquilles de 24 et 26 sites, `q_min=3` à `p=2` : non admissibles, plus de faux `SHELL_DOMAIN` |
| `dom26` | 4 | 2 | admissible : `SHELL_OVER_12 … q<=3`, puis refus `chain_shell_above_12` |
| `domcap` | 3 | 0 | coquille de 26 sites sans paire antipodale |
| `g28b_p2` | 3 | 0 | coquille de 28 sites, non admissible |
| `g28b_p2` | 4 | 2 | admissible : `SHELL_OVER_12`, puis refus de la chaîne |
| `dedup1500` | 5 | 0 | ancres en collision : grille complétée à 32, dont 8 de queue de densité |

La campagne v2, avec le même tirage sur les trames, avait déjà zéro écart
sur les six cas. Ses sorties ne sont pas publiées : seules celles du
binaire v3 le sont.

## Portée et réserves

- **Omissions, pas intrus.** Le juge vérifie « catalogue ⊇ boules
  admissibles tirées ». Une boule en trop dans le catalogue relève du
  census de la chaîne et de la tour, pas de ce juge.
- **Aucune coquille étendue n'est tirée sur les trames brutes.** Le
  chemin étendu n'est exercé que par la fixture et par les gadgets.
- **Compte de la fixture.** `--expect-extended=8` épingle un compte
  observé avec la graine par défaut, sans le dériver :
  - la boule du cube et quatre faces (`x=−1`, `x=1`, `y=−1`, `z=1`) ;
  - la q3 et la q4 gravées ;
  - une q2 fortuite `{12,13,15}`, angle droit en 12 dans le groupe de
    cinq.

  Le recensement exhaustif par balayage (R-19) reste la bonne porte pour
  cette population.
- **q2 critique mince à K10.** Il y a 4 à 10 clés par cas, mais la
  strate est non vacante, comme la spécification l'exige.
- **Bras jugé.**
  - C'est le bras moteur.
  - Le bras par lots CPU a le même condensé de catalogue dans
    `PINS_RAW.json`. Ce condensé encode clé, niveau, arité et tous les
    IDs. Le PASS s'étend donc à ce bras modulo une collision FNV-64, pas
    par égalité littérale.
  - Un cas R21 GPU qui reproduirait l'épingle en hériterait au même
    titre.
- **Code partagé avec le produit.** Le juge n'emprunte au produit que :
  - `run_tower_chain`, qui fournit le catalogue jugé ;
  - `build_cloud_index`, qui ne sert qu'à traduire l'indice géométrique
    en ID, et dont la bijection est contrôlée coordonnée par coordonnée.
    Ce contrôle porte sur l'index reconstruit par le juge, identique à
    celui de la chaîne par le code ; une traduction fausse se verrait
    en écarts d'IDs.
  - le type `Point3`.

  Tirage, résolution, census et `q_min` sont propres au juge. La fixture
  cosphérique est celle des juges q2/q3 v9.
- **Entrées et binaire.**
  - La grille 1 mm est établie par les SHA-256 des entrées v8, comme pour
    les épingles.
  - Seule l'empreinte du binaire est archivée (`judge_binary.sha256`).

## Reproduction

```bash
S=<worktree à 093d943ce>/morsehgp3D_v9   # sources de la chaîne
B=<build Release CPU de S>                # libmhgp9_chain.a, libmhgp9_gen.a, libmhgp9_gpu.a
g++ -O3 -DNDEBUG -std=c++20 -Wall -Wextra -Wpedantic -Werror -I$S -I$S/src/gen -isystem <boost>/include \
  raw_support_judge.cpp $B/libmhgp9_chain.a $B/libmhgp9_gen.a $B/libmhgp9_gpu.a -lpthread -o raw_support_judge
./run_campaign.sh <racine du dépôt> ./raw_support_judge <sortie>
(cd gadgets && ./run_gadgets.sh ../raw_support_judge <sortie des gadgets>)
```

Compilateur : GCC 13.3.0 (Ubuntu 24.04), Boost extrait de `libboost-dev`.

Fichiers :

- `raw_support_judge.cpp` ;
- `run_campaign.sh` ;
- `BASE.txt` ;
- `judge_binary.sha256` ;
- `outputs/`, avec les 22 sorties, les six rapports GNU time et
  `codes.txt` ;
- `gadgets/`, avec sept nuages, `run_gadgets.sh` et `outputs/` ;
- `SHA256SUMS`.
