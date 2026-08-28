# Note Claude — L2 livré, verdict chiffré sur le GPU, et les verrous de L3 (28 août 2026)

Ancrage : HEAD local `615b9bcc` (réducteur vivant), session G4 14 au pin
`839cf1ecafb8`, reçu `receipts/campagne_g4_v5_20260828_g0_g1/`.
Cadre : `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## 1. Ce qui est livré depuis votre ordre de travail (§ 8 bis de `ECHELLE.md`)

**Commit 1 — porte de rejeu.** `tests/delta_replay_gate.cpp`, portes
`mhgp5_delta_replay*`, mutants `drop-nonmerge` et `attach-prebatch` tués.

**Commit 1 bis — les six fixtures gravées** (`tests/fold_fixtures_gate.cpp`,
porte `mhgp5_fold_fixtures`) : étoile K = 1 de 300 arêtes, chaîne
`{0,1}`/`{0,2}`, deux simplexes K = 2 partageant `{0,1}`, plateau mono-lot à
pic transitoire 3, absorption logique d'un grand composant par un singleton,
et les cinq précédentes **à empreinte d'adressage constante**. Deux points
méritent votre lecture :

- Les fixtures de plateau et d'absorption sont construites pour que l'ordre
  des **racines logiques** *contredise* l'ordre des **clés de sortie** (D :
  `{0,2}` avant `{0,1}` ; E : `{20}` avant `{1}`). Une première version, plus
  naturelle, ne les distinguait pas — elle aurait été verte sous une
  implémentation triant par clé. C'est corrigé et le commentaire de la
  fixture donne les `fid` et les racines à la main.
- Le hachage constant n'est **pas** un mutant : ce n'est pas un défaut, et
  son verdict attendu est *aucun changement*. Il est donc câblé comme un
  crochet de stress compilé sous `MHGP5_TESTING` seulement
  (`fold_detail::fold_hash_constant()`), et la porte vérifie l'égalité du
  texte des deltas, du catalogue et de `final_canon_fid`. Si vous jugez
  qu'un crochet de stress doit malgré tout entrer au registre des mutants,
  dites-le : la règle actuelle (`tests/mutants_gate.py`) exige une porte en
  code 4 pour chaque nom, ce qu'un stress à sortie identique ne peut pas
  fournir.

**Commit 2 — réducteur vivant.** `src/forest/fold_live.hpp`,
`tests/fold_live_gate.cpp`, portes `mhgp5_fold_live*`. Un `Alias` par facette
entre sa PREMIÈRE et sa DERNIÈRE incidence (clé portée par l'alias : le
réducteur ne consulte plus le catalogue), un `Component` par composante ayant
au moins un alias, table `fid -> alias` en adressage ouvert linéaire à
**suppression par décalage arrière** (aucune pierre tombale : la table suit le
vivant, pas le cumul). Union ordonnée : racine **logique** = celle de `first`,
canonique = minimum **historique**, conteneur **physique** = record de plus
grande masse (small-to-large), record vide détruit ; lot en deux passes (gels
puis unions) ; morts après émission.

**Vos raccords de réception sont appliqués dans le même travail** (votre
`ETAT_COURANT.md` non commité du 28 août). Point par point :

| votre demande | état |
|---|---|
| `TIMEOUT` sur les six portes, mutants sortant dès la mise à mort | fait : `TIMEOUT` 300 s (nominal) et 60 s (mutants) ; deux cas **synthétiques adverses** joués en tête tuent les cinq mutants — portes passées de 36–50 s à **0,06–0,09 s** |
| commentaire `--reloc-ratio` obsolète | supprimé |
| compteur **par alias** et chaîne d'absorptions adverse | fait : `max_moves_per_alias`, plafond `ceil(log2 facettes) + 1` ; sur la chaîne (le singleton est toujours `first`) le conforme fait **1** déplacement par alias, le mutant **200** pour un plafond de 9 — le mutant de coût est désormais causal et court |
| T6 trop faible (comparaison au pic global) | fait : égalité forte `alias == compte exact du lot` **avant et après les morts**, à chaque frontière |
| `idx.used == live_alias`, bijection, listes/comptes, cycles, vacuité finale | fait : balayages structurels à cadence bornée (2 785 balayages, **0** violation) et vacuité finale vérifiée |
| rejouer les deltas, pas seulement les comparer | fait : `(catalogue du résident, deltas du **vivant**) -> partition`, comparée fid par fid à `final_canon_fid` |
| `live_bytes_peak` n'est pas de la mémoire allouée | renommé `logical_live_bytes`, et **`allocated_bytes`** ajouté (arènes, listes libres, table) |
| la ligne de résidence mélange deux témoins | corrigé : deux lignes, `fold_live_pic_absolu` (2,31 %) et `fold_live_pire_ratio` (7,29 %), chacune avec **ses** champs |
| « jamais plus grand que nécessaire » | remplacé par la mesure qui la fonde (égalité de vie par lot) ; le facteur mémoire est qualifié d'**estimation de structures choisies**, et il vaut **8,3** sur les octets alloués, pas 15 |
| `free-on-absorb` sous-débordait et a fini par signal | l'injection ne recycle plus rien : les alias orphelins ne sont ni libérés ni ré-indexés, seule la sortie diverge ; propre sous ASan **et** UBSan, code 4 en 0,06 s |

Deux points où je ne suis pas allé jusqu'à votre suggestion, et pourquoi :
`root-key-mutable` recouvre en effet `canon-not-min-on-union` sur ces nuages,
mais il mute le champ à un **autre moment** (à la relocalisation physique, pas
à l'union) et je préfère garder les deux tant que le conteneur physique n'a pas
de porte propre ; et je n'ai **pas** encore de mutant qui altère
`logical_root_fid` lui-même — les fixtures gravées D et E le couvrent
structurellement (leur ordre de deltas contredit l'ordre des clés), mais vous
avez raison qu'un mutant direct manque : je le prends au prochain lot.

Mesures du 28 août (six familles, `n` = 7 à 1 500, 58 ordres) :

| grandeur | valeur |
|---|---|
| facettes cumulées / deltas | 5 194 737 / 733 029 |
| désaccords avec le fold résident (deltas champ à champ, `batch_levels`, compteurs) | **0** |
| violations de `composantes <= alias <= pic exact` aux frontières de lot | **0** |
| violations de vie par lot, de structure, de vacuité finale | **0** (2 785 balayages) |
| rejeu T5 des deltas vivants vers `final_canon_fid` | **0** fid en désaccord |
| témoin *pic absolu* (ordre le plus gros, 733 687 facettes) | 16 929 alias, **471** composantes, **2,31 %** |
| témoin *pire ratio* (15 ordres $\ge 10^{5}$ facettes) | **7,29 %** (7 994 alias, 206 composantes, 109 721 facettes) |
| octets au témoin du pic absolu | 1,83 Mo logiques, **3,19 Mo alloués**, contre 26,4 Mo pour deux champs du résident (**× 8,3**, estimation de structures choisies) |
| relocalisations par facette / par alias | 1,09 et 8 au pire (mutant : 138 et **200** sur la chaîne adverse, plafond 9) |

Cinq mutants tués : `free-on-absorb`, `root-key-mutable`,
`canon-not-min-on-union`, `last-mark-shifted` par désaccord ;
`physical-root-is-logical-root` par les deux **plafonds de relocalisation**
(agrégé et par alias). Les six passes (conforme et cinq mutants) sont propres
sous ASan **et** UBSan.

Ce que ce pin **ne** montre **pas**, et que je ne revendique pas : aucun gain
CPU ni RSS. `reduce_fold_live` est hors du chemin produit, sa réduction reste
séquentielle, et son chronomètre démarre après la construction PREMIÈRE /
DERNIÈRE. La comparaison résident/vivant sur le même périmètre complet
(catalogue et rejeu inclus, temps par étape et RSS) est à faire avant tout
routage — je la place en tête du prochain lot, avant même L3.

`lifetime-by-hash-only` n'est pas encore tué : il porte sur le calcul
**externe** des durées de vie, que L2 remplace par deux tableaux `u32` par
facette — assumé, documenté, et **hors** de la revendication de résidence.

## 2. Verdict chiffré sur le GPU (session 14, contrats 50 000)

G1 fait ce qu'il promettait sur les octets et **rien** sur le mur :

| famille | octets H2D q3, SoA → indices | mur de lane q3 | mur total |
|---|---|---|---|
| `uniform` | 19,6 → 11,2 Go | 2,37 → 2,34 s | 58,1 → 58,6 s |
| `terrain` | 31,5 → 17,8 Go | 2,37 → 1,86 s | 14,3 → 13,9 s |
| `eight_clusters` | 56,4 → 19,6 Go | 6,35 → 5,67 s | 63,6 → 63,4 s |
| `scanline_single_pass` | 28,0 → 12,3 Go | 2,04 → 1,74 s | 12,1 → 11,8 s |

Digests CPU / `--gpu` / `--gpu-wire=index` identiques sur les quatre familles.
La décomposition du mur CPU à `uniform` 50 000 (56,9 s) explique le reste, et
c'est elle que je vous soumets :

| poste | s | % du mur |
|---|---|---|
| fold (mur, `fold_inflight = 2`) | 25,36 | **44,6** |
| génération, vagues WSPD | 10,26 | 18,0 |
| préfiltre de profondeur | 7,13 | 12,5 |
| génération, rectangles q2/q3/q4 (**le seul poste device**) | 5,38 | **9,5** |
| census | 4,17 | 7,3 |
| RLE | 1,85 | 3,3 |
| expansion | 1,84 | 3,2 |

Trois conséquences que je tiens pour établies :

1. **Le device ne peut pas rendre plus de 9,5 % à `uniform`** — et il rend
   déjà moins que le CPU sur cette famille (lane q3 device 2,34 s contre CPU
   2,05 s ; c'est l'enfilement hôte, 1,45 s sommé sur les fils, pas le
   kernel, mesuré à 59 ms).
2. **`scanline_single_pass` est l'exception, et c'est la famille réaliste** :
   rectangles 6,47 s sur 12,98 s de mur, dont q4 4,98 s — le device y ramène
   q4 à 3,28 s. C'est le seul endroit où G2 se justifie par la mesure.
3. **Le pic de mémoire est le fold** : `max_fold = 17,7 Go` à 50 000 contre
   7,6 Go après census, soit ≈ 0,35 Mo par point, linéaire de 8 000 à 50 000.
   À 10 M ce poste seul demanderait des téraoctets : c'est L2–L4, pas le GPU,
   qui ouvre l'échelle.

## 3. Une observation nouvelle, à vérifier de votre côté

`pic_mesure_en_vol` vaut **2 quel que soit `fold_inflight`** (mesuré à 1, 2, 3
et 4). L'étage A (expansion, tri, internement, fusion) est produit
séquentiellement par le fil principal : au plus un étage B tourne pendant que
A prépare l'ordre suivant, donc la concurrence est structurellement bornée à
2 et le domaine `[1, 16]` est un mensonge d'interface. Comme le fold pèse
44,6 % du mur et que son étage B (reduce) en est l'essentiel (26,6 s cumulés
pour 25,4 s de mur), la question est : **pipeliner l'étage A lui-même** est-il
sain ? Les événements de l'ordre K sortent de l'expansion des boules
censusées, qui est déjà par K ; je ne vois pas d'obstacle d'objet, mais je ne
veux pas ouvrir ce chantier sans votre verrou (V31), d'autant que la
résidence de L2 rend justement plusieurs réductions simultanées abordables.

## 4. Verrous demandés

- **V31** — pipeliner l'étage A par ordre (au-delà du plafond structurel de 2
  en vol) : sain, ou faut-il d'abord un invariant que je n'ai pas vu ?
- **V32** — un mutant qui, prouvablement, ne change **pas** la sortie
  (`physical-root-is-logical-root`) peut-il être tué par un plafond de coût
  tiré du théorème lui-même, ou exigez-vous un autre témoin ?
- **V33** — ordre de la suite. Je propose : (a) durées de vie externes et
  `lifetime-by-hash-only` ; (b) coutures externes (RLE multi-runs 1/2/3/7,
  join retour) ; (c) payload et reprise ; (d) pilote 1 M. Le convertisseur
  « flux → tableaux v4 » de L2 me paraît devoir attendre (b), sans quoi il
  reconstruit en RAM ce que L2 vient d'éviter — confirmez-vous ?
- **V34** — GPU : gèle-t-on G2 (compaction q4 device) hors de
  `scanline`/`eight_clusters`, où seul il se justifie par la mesure ? Et
  jugez-vous le **préfiltre + census** (11,3 s, 19,8 % du mur, prédicats
  entiers par boule) un meilleur sujet device que les rectangles (9,5 %) ?
