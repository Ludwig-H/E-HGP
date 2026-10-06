# Levier C : arithmetique etroite de la feuille device (6 octobre 2026)

Cadre : phase=exploration_v11_hors_registre, backend=cpu_reference, profile=quantized_u21_input_only,
public_status=not_claimed. GCP non utilise. Aucune branche, aucun commit, aucun push.
Base 905fad2e1 (origin/main), worktree /tmp/v11-wf-C, construction /tmp/v11-wf-C-build (Release, u21).
Correctif : /workspaces/E-HGP/build/v11-persist/wf_gpu/C.patch.

## Conception

Une seule condition nouvelle, l'**etendue de feuille** : D = max sur les axes de (max - min) de l'enveloppe des
sites de la feuille ET de la fermeture [lo, hi] de sa boite. `Leaf::prepare` la mesure dans la boucle de
chargement existante (aucune boucle nouvelle) ; si D > 2^20 (`kNarrowSpan`), la feuille n'ouvre aucun prefixe
(masque initial nul, sans branche nouvelle) et `run_leaf` rend `kUnresolved` : elle est rejouee par leaf.cpp,
comme toute feuille non certifiee aujourd'hui. Aux profils B <= 20 la condition est automatique et le test est
elimine a la compilation (`if constexpr (kBits > 20)`).

Sous D <= 2^20, trois predicats chauds passent de i128 a i32/i64, sans changer ni l'ordre des tests ni une decision :

1. **J2, `center_line_meets`** (leaf_device_predicates.hpp). Differences f, g en i32 ;
   p0 = fc - sum L_k f_k recalcule comme sum f_k (a_k + b_k - L_k) (L = lo + hi, identite exacte), sans les carres
   absolus ; produits croises, membres gauche et droit du lemme Z en i64 au lieu de quinze produits i64 x i64 -> i128.
2. **q4** (leaf_device.hpp). `orientation4(p0..p3)` = sign((u x v).sv) et le `det` de la fabrique u.(v x sv) sont le
   meme produit mixte : calcule une fois, en i64 (u, v, sv, u x v reutilises ; un produit vectoriel et un produit
   scalaire i128 de moins). Le test du cube de cote 2^20 des quatre sites (voie Wide) est implique par D <= 2^20 et
   retire ; l'invariant `det == 0` apres E4, deja inatteignable (meme valeur que l'orientation nulle), aussi.
3. **q3** (leaf_device.hpp, et `strictly_acute` de leaf_device_predicates.hpp). Triangle aigu par uu, vv, uv
   (`acute_dots` : (a-b).(c-b) = uu - uv, (a-c).(b-c) = vv - uv) au lieu de trois produits scalaires de differences ;
   uu, vv reutilises ; t = uu v - vv u en i64 au lieu de i128.

Non touches : `side()` (recensement), `center_in_box`, `canonical`, les certificats ; la seconde passe et les puits.

## Preuve d'exactitude (bornes, aussi en commentaire dans le code)

Hypothese : D <= 2^20 ; coordonnees < 2^B, hi <= 2^B, B <= 24.

- J2 : |f_k|, |g_k| <= D (i32) ; a_k + b_k - L_k : entiers < 2^26 (i32). 4 p0 = |2a - L|^2 - |2b - L|^2 avec
  |2a_k - L_k| <= |a_k - lo_k| + |a_k - hi_k| <= 2D, donc |p0|, |p1| <= 3 D^2 = 3*2^40. |cr| <= 2 D^2 = 2^41.
  |g_k p0|, |f_k p1| <= 3 D^3 = 3*2^60 ; left <= 6*2^60 < 2^63. right = sum_{j != k} w_j |cr_kj| <= 2 D * 2 D^2 =
  2^62 (cr_kk = 0). Les valeurs sont donc exactes en i64 et egales a celles de la forme i128 : memes comparaisons.
- q4 : |u_j|, |v_j|, |sv_j| <= D ; |(u x v)_j| <= 2 D^2 ; |det| <= 3 * 2 D^2 * D = 6*2^60 < 2^63. h = 2 det^2 reste
  calcule en i128 (i128(det) * det). Quatre sites de la feuille : high - low <= D <= 2^20 sur chaque axe, donc le test
  `high - low > 2^20` retire ne pouvait plus etre vrai.
- q3 : uu, vv <= 3 D^2, |t_j| <= 3 D^2 * D + 3 D^2 * D = 6*2^60 < 2^63 ; n = t x w reste en i128 (|w| <= 2 D^2).
- `acute_dots` : identites exactes ; dot < 3*2^(2B) (i64) sans hypothese d'etendue.
- Feuille large : aucune decision device ; leaf.cpp la traite (memes emissions, memes compteurs par construction du
  repli existant). Le registre et le dump restent ceux du CPU.

## Portes jouees (codespace, u21, Release)

- `mhgp11_catalogue_leaf_narrow` (nouvelle, tests/catalogue/leaf_device_narrow_test.cpp, label fast) :
  `mhgp11_test_ok tests=3 controles=320038`.
  - `line_reference` : 200 000 triplets dans une fenetre d'etendue 2^20 (au sommet du cube : sites jusqu'a 2^B - 1,
    fermeture jusqu'a hi = 2^B ; a l'origine ; quelconque), coins exacts ou tirages, boite = fenetre entiere ou boite
    de demi-cote 2^r placee pres de la droite des centres (centre circonscrit i128 independant) ; confrontation a
    num::center_line_meets ; planchers d'issues (degenere >= 2000, disjoint et rencontre >= 20 000).
  - `acute_and_triple` : 40 000 quadruplets d'etendue 2^20 : `strictly_acute` contre la definition a trois produits
    aux sommets, produit mixte i64 contre u.(v x sv) en i128 et contre `orientation4`.
  - `span_refusal` : feuille d'etendue exactement 2^20 (site lointain a 2^B - 1 - 2^20) et feuille dont la fermeture
    hi = 2^B porte l'etendue a 2^20 exactement : `kOk` et quinze compteurs egaux a leaf.cpp (prefixes, droites J2,
    candidats q4 et emissions exerces) ; etendue 2^20 + 1 par un site, ou par la seule fermeture : `kUnresolved` sans
    prefixe, leaf.cpp la traite.
  - Compile aussi (analyse) a u18 et u24 sous -Wall -Wextra -Wpedantic -Werror.
- Portes rapides du catalogue et des voies :
  `ctest -R "full_leaf_lanes|catalogue_(cache|single_pass|leaf|small_pair|parallel)"` : 36/36 vertes (rejouees sur
  la construction finale, porte unitaire nouvelle comprise) ; dont
  `mhgp11_tower_full_leaf_lanes` et sa jumelle `_opt` (CPU 16379, feuille hote 32763, lot 49147, lot sans reservoir
  180219 : memes dumps et registres).
- `python3 tools/check_style.py` : `style_ok fichiers=532`. Manifestes : `manifeste_ok module=catalogue mutants=72
  plancher=72`, `manifeste_ok module=tower mutants=151 plancher=151`.
- ng00 (lidar_ng00, 39 885 sites), voie lot hote 49147 a 4 fils, contre la voie CPU 16379 :
  K5 feuilles 16 dump 3a2bfb4f9f48... ; K10 feuilles 24 dump 61a4245b91d9... (identiques aux references) ;
  `catalogue_work` identique au CPU dans les deux cas ; `unresolved` 0 sur 353 456 (K5) et 530 259 (K10) feuilles :
  aucune feuille de ng00 ne depasse l'etendue 2^20 en u21.

## Mutants (run_mutants.py, u21 Release, --jobs 2 --build-jobs 2)

Catalogue (tests/mutants/catalogue.json, plancher 68 -> 72) : `mutants_ok module=catalogue mutants=4 tues=4`.

| id | mutation | porte | verdict |
| --- | --- | --- | --- |
| j2_etroit_second_plan_faux | p1 sur (a, b) au lieu de (a, c) | catalogue_leaf_narrow_line_reference | TUE |
| aigu_sommet_c_oublie | `vv > uv` devient `vv > 0` | catalogue_leaf_narrow_acute_and_triple | TUE |
| etendue_seuil_double | seuil 2 * 2^20 | catalogue_leaf_narrow_span_refusal | TUE |
| etendue_sans_fermeture | hi de la boite hors de l'etendue | catalogue_leaf_narrow_span_refusal | TUE |

Tower (tests/mutants/tower.json, plancher 148 -> 151) : `mutants_ok module=tower mutants=3 tues=3`.

| id | mutation | porte | verdict |
| --- | --- | --- | --- |
| q4_produit_mixte_positif | `det == 0` devient `det <= 0` | tower_full_leaf_lanes | TUE |
| q3_numerateur_etroit_faux | t = uu v - uu u | tower_full_leaf_lanes | TUE |
| j2_etroit_largeur_fausse | rayon pris sur l'axe k | tower_full_leaf_lanes | TUE |

Incident de campagne, corrige : la premiere campagne catalogue a laisse survivre `j2_etroit_second_plan_faux`
(generateur trop faible : boites minuscules quasi toujours disjointes, fenetres entieres quasi toujours
rencontrees ; un decalage du second plan ne changeait aucune issue) et `aigu_sommet_c_oublie` etait INVALIDE
(parametre inutilise sous -Werror). Le generateur place maintenant la boite pres de la droite des centres avec des
planchers d'issues ; la mutation de l'aigu garde vv. Seconde campagne : 4/4 tues.

## SASS (nvcc 12.9, sm_120, u21, /tmp/v11-wf-cuda-obj.sh puis /tmp/v11-wf-sass.py)

| noyau | instr | BSSY | CALL | registres | LDL | STL |
| --- | --- | --- | --- | --- | --- | --- |
| count base 905fad2e1 | 11 832 | 134 | 3 | 164 | 358 | 311 |
| count levier C | 10 776 (-8,9 %) | 134 | 3 | 166 | 354 | 310 |
| fill base | 9 920 | 93 | 0 | 168 | 326 | 289 |
| fill levier C | 8 744 (-11,9 %) | 93 | 0 | 168 | 324 | 288 |

Pile 4144 / 4032 octets inchangee, aucun debordement. IMAD.WIDE/IMAD.HI de la region count : 1489 -> 1229.
164 et 166 registres s'arrondissent tous deux a 168 (granularite d'allocation) : meme occupation.

Lecon de reconvergence confirmee et evitee : une premiere version mesurait l'etendue dans une boucle separee apres
le chargement, avec `unresolved = unresolved || ...` : BSSY count 134 -> 148, fill 93 -> 107, LDL +90 (relecture de
P depuis la memoire locale). La bissection (chaque morceau retire a son tour) a designe cette seule boucle ; fondue
dans la boucle de chargement (min/max en registres, `wide` separe d'`unresolved`, masque initial par
`& (u64(wide) - 1)`), les BSSY reviennent exactement a la base.

## Mesures hote appariees (codespace partage, voie lot hote 49147, W1, ng00 K5/16)

Temps (count_ns, ms), paires alternees base/variante, sondes /tmp/v11-wf-base-full_bench et
/tmp/v11-wf-C-build/mhgp11_full_bench :

- base : 4789, 6466, 6673, 6652, 6897, 8339, 9982, 8580 (min 4789, mediane 6785)
- variante : 6110, 6015, 6444, 6604, 6361, 6086, 8983, 6893 (min 6015, mediane 6403)

Charge 4 a 12 sur 8 coeurs partages par quatre agents : ces temps sont DESCRIPTIFS et ne tranchent rien (la meilleure
base est plus rapide que la meilleure variante, les medianes disent l'inverse). Mesure deterministe a la place,
callgrind (instructions executees, meme commande, une passe chacune) :

| fonction (inclusive) | base | levier C | ecart |
| --- | --- | --- | --- |
| run_leaf<ScratchSink> | 37,47 G | 34,62 G | -7,6 % |
| extend<0> | 32,88 G | 29,99 G | -8,8 % |
| q4 | 6,03 G | 5,07 G | -16,0 % |
| extend<0> propre + q3 (q3 integre dans la variante) | 23,25 G | 21,33 G | -8,3 % |
| prepare (run_leaf propre) | 3,50 G | 3,54 G | +1,1 % |
| programme | 93,90 G | 91,05 G | -3,0 % |

Sur x86 un produit i64 x i64 -> i128 est une seule instruction : l'hote sous-estime le gain du passage a i64 ; la
deduplication du produit mixte et des produits scalaires y est visible (q4 -16 %).

## Effet attendu sur G4 (ESTIMATION, non mesuree)

Sur GPU un produit signe 64 x 64 -> 128 coute environ huit a douze instructions SASS contre deux ou trois pour un
produit i32 x i64 -> i64. Profil Nsight connu : center_line_meets 23 % du comptage avant la memoire J2 (environ
10 a 12 % apres, 42 % des tests J2 evalues sur ng00 K5), q4 environ 10 %. ESTIMATION : comptage -6 a -12 %
(K5/16 : 39 -> 34 a 37 ms ; K10/24 : 226 -> 200 a 212 ms), seconde passe -5 a -10 % sur un temps deja petit ;
BSSY et occupation inchanges, donc pas de risque de reconvergence attendu. A confirmer sur G4 par la session
appariee ci-dessous.

Masques et parametres a comparer sur G4 (base 905fad2e1 contre ce correctif, ordre Williams, a chaud) :
`mhgp11_full_bench XYZ IDS DUMP K FEUILLE 256 0 4294967295 18446744073709551615 FILS MASQUE` sur lidar_ng00
(et ng01, ng02) avec K5/16 et K10/24 (aussi K5/24), FILS 48 :
- 81915 (lot GPU) : leaf_batch.count_ns, fill_ns, executor_ns, domain_ns, unresolved (doit rester 0) ;
- 49147 (lot hote) et 16379 (CPU) : dumps 3a2bfb4f9f48... (K5/16), 61a4245b91d9... (K10/24) et catalogue_work
  identiques entre voies a taille egale ;
- 81915 + 131072 = 213 051 (lot GPU sans reservoir) en controle.

## Risques

- Feuilles d'etendue > 2^20 rejouees par leaf.cpp : nulles sur ng00 en u21 (1 km a 1 mm). Au profil u24 (grille plus
  fine) ou sur des nuages epars, des feuilles larges peuvent apparaitre et basculer sur le CPU (chemin lent mais
  exact) ; le compteur `leaf_batch.unresolved` le montre. Un repli i128 dans la feuille couterait des BSSY.
- Registres du comptage 164 -> 166 : meme arrondi a 168, occupation inchangee ; a reverifier si un autre levier
  ajoute des registres.
- Les temps hote du codespace ne mesurent rien ici (charge) ; seul callgrind et le SASS sont probants localement.
- La porte unitaire est seulement compilee (non executee) a u18 et u24 ; les campagnes de mutants sont a u21.
