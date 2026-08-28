# Note Claude — L2 livré, verdict chiffré sur le GPU, et les verrous de L3 (28 août 2026)

Ancrage : livraison L2 `bc66ade7` (noyau initial `615b9bcc`), session G4 14 au
pin `839cf1ecafb8`, reçu `receipts/campagne_g4_v5_20260828_g0_g1/`.
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
| `free-on-absorb` sous-débordait et a fini par signal | les alias orphelins ne sont plus recyclés, mais leur record de composante entre encore dans `cfree` ; code 4 Release en 0,06 s. Claude rapporte ASan/UBSan propres, non rejoués par l'audit |

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
(agrégé et par alias). Claude rapporte les six passes propres sous ASan et
UBSan ; cet audit reçoit les codes Release, pas encore ce rejeu sanitizer.

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

## 5. Réponse de l'audit — décisions V31 à V34

Réponse fondée sur le pin `bc66ade7`. Ce pin est un progrès net et il ne faut
revenir ni au fold résident comme architecture cible, ni aux portes longues du
pin précédent. Les réserves ci-dessous servent à rendre les nouveaux témoins
causaux et la future mesure de coût attribuable.

### V31 — ne pas pipeliner A sur la seule observation d'un pic naturel à 2

Le plafond structurel à 2 n'existe pas. Le code autorise un étage A courant et
jusqu'à `fold_inflight` étages B. La porte
`mhgp5_fold_inflight_safety_gate --n=300 --threads=4` atteint déjà exactement
3 avec `fold_inflight=3` : K2 et K3 commencent leur réduction pendant la
publication de K1. Le domaine `[1,16]` est donc un plafond d'admission valide,
pas la promesse que tout nuage le saturera. Dans le scénario déterministe N1,
remplacer l'assertion `peak >= 2` par `peak == 3` recevra ce fait.

Le pic naturel à 2 signifie seulement qu'A alimente B à ce rythme sur ce
workload. Avant tout chantier, publier par K le mur et la somme d'A et de B,
les attentes `A-ready -> B` et `B-ready -> publication`, ainsi que les pics A,
B, ordres résidents, crédits ouvriers et RSS. Comparer ensuite des formes à
budget CPU constant ; plusieurs A employant chacun `threads` ne sont pas une
expérience recevable.

Mathématiquement, A(K) est indépendant entre ordres : `ix`, `balls` et les
comptages sont lus, puis événements et `FoldPrepared` sont propres à K. Une
implémentation concurrente reste néanmoins transactionnelle : résultat A
privé par K, `Stage` à adresse stable, admission de B et consommation des
fautes strictement par K, aucun K haut autoritaire avant les K bas, et jonction
de tous les A/B avant retour. Les stats `rr.expand`, les chronos et callbacks A
sont aujourd'hui partagés ; un simple `async` autour d'A créerait des races et
`P * threads + fold_inflight` ouvriers. Ajouter une fenêtre A seulement si un
micro-banc à crédits CPU et mémoire constants montre un gain.

### V32 — oui au mutant de coût, à condition de prouver d'abord sa neutralité

Un plafond déterministe déduit de small-to-large est un contrat mutant valide.
Il est même préférable à un timeout ou à un ratio de temps : chaque alias
déplacé rejoint un record de masse au moins double, donc son nombre de
déplacements est logarithmique. Le compteur maximum par alias et la chaîne
d'absorptions de `bc66ade7` sont les bons témoins.

La porte actuelle doit toutefois inverser une priorité : elle rend 4 dès que
le plafond synthétique est dépassé, alors que sa comparaison synthétique
n'exerce qu'une partie de la sortie. Pour que « tué par le coût seul » soit
vrai, comparer d'abord sur ces cas les deltas complets, niveaux de lots,
compteurs, détecteurs et partition rejouée ; toute divergence sémantique rend
1 ou 3. Le code 4 n'est autorisé qu'après égalité sémantique et non-vacuité du
compteur de déplacements. La borne agrégée peut rester un second témoin, mais
le maximum par alias porte directement le théorème.

### V33 — schéma tôt, convertisseur borné après les coutures, avant le pilote

L'ordre proposé est presque le bon, mais PREMIÈRE/DERNIÈRE externe dépend déjà
de la fusion multi-run par clé complète et du join retour. L'ordre causal est :

1. figer le wire d'occurrence à clé complète, la clé totale, les rangs/fid u64
   et le schéma du payload logique ;
2. fermer la fusion/RLE multi-run des **occurrences**, l'attribution globale des
   fid, PREMIÈRE/DERNIÈRE, le join `(event_rank_u64, slot)` et le mutant
   `lifetime-by-hash-only`, avec empreinte constante comme stress code 0 ;
3. brancher le réducteur vivant sur ces annotations et recevoir catalogue
   externe plus deltas vers partition, sans les tableaux résidents ;
4. livrer encodeur/décodeur, digest logique u64 et reprise atomique par K ;
5. exécuter le pilote 1 M après préflight d'octets, disque et RSS.

Le RLE externe complet des `BallCandidate` demeure L4 : il ne faut pas le
confondre avec la primitive multi-run des occurrences nécessaire à L3.

Le convertisseur v4 ne doit jamais entrer dans le chemin massif ni forcer le
produit à rematérialiser ses tableaux. Son **contrat** doit en revanche être
figé dès le wire, pour ne pas découvrir après les coutures qu'une information
manque. Son implémentation peut attendre l'étape 3, puis rester un adaptateur
hors produit, borné au domaine u32 et exécuté avant le pilote : il consomme le
flux par ordre et compare les quatre objets v4 aux tailles où le résident
existe. Le reconstruire en RAM dans ce seul oracle borné est acceptable ; le
faire dans le produit ne l'est pas.

### V34 — G2 ciblé par intensité, puis probe de préfiltre ; pas de routage par famille

Geler G2 comme route générale est sain, mais la frontière
`scanline/eight_clusters` n'est pas soutenue par les mesures. Au reçu q4,
`terrain` porte le signal le plus fort (`7,88 -> 3,70 s`), devant `scanline`
(`4,98 -> 3,23 s`) ; `eight_clusters` ne gagne que `9,91 -> 8,98 s` sur la
lane sans gain total établi, et `uniform` régresse (`3,12 -> 3,63 s`). Faire le
premier G2 derrière un toggle sur **terrain et scanline**, avec `uniform` comme
contrôle négatif et `eight_clusters` comme cas limite/stress de mémoire. Une
future décision de route doit dépendre de comptes déterministes de pression de
compaction, survivants, octets et VRAM, jamais du nom du générateur. Aucun reçu
actuel n'exécute encore G2 : ces temps guident l'ablation, ils ne la reçoivent
pas.

Le préfiltre plus census n'est un meilleur plafond d'Amdahl que sur `uniform`
(19,8 % contre 9,5 % pour les rectangles). Sur `scanline` et `terrain`, il
pèse environ 10 % contre près de 50 % pour les rectangles ; sur clusters,
environ 18 % contre 24 %. Il est aussi moins prêt : parcours radix irrégulier,
sorties précoces, pile variable, prédicats i128 et sortie census variable,
alors que la géométrie device ne porte encore ni nœuds, ni boîtes, ni sommes.

Ordre utile : recevoir G2 q4 stable et exact ; instrumenter visites, profondeur
de pile, sorties précoces, survivants et octets du préfiltre/census ; porter
ensuite le **préfiltre seul**, arbre résident et arithmétique exacte, avec un
bitset ou une compaction stable ; exiger un gain bout en bout incluant H2D,
kernel et D2H. Le census ne vient que si ce probe déplace réellement le mur.

## 6. Contre-audit ciblé de `bc66ade7`

Quelques raccords courts éviteront de surqualifier ce bon pin :

- la porte des fixtures rend le différentiel utile, mais son texte comparé
  omet `level`, `batch_levels`, refus et compteurs. A-300 et E-50 n'ont pas
  d'attente littérale, et le plancher global de 6 sur 7 appels ne garantit pas
  chaque motif. Ajouter un comparateur complet et des planchers nommés, ou
  réduire explicitement le claim à cinq sorties littérales plus deux stresses
  différentiels ;
- le rejeu ajouté à `fold_live_gate.cpp` ignore `output`, lots, niveaux et clés
  hors catalogue, sans contrôle négatif. Extraire le replayer strict déjà
  exercé par `delta_replay_gate.cpp` dans un utilitaire d'oracle partagé et
  l'appeler sur le catalogue résident et les deltas vivants ; conserver le
  claim actuel comme « connectivité finale conditionnelle » jusque-là ;
- D reçoit maintenant l'ordre des racines, mais ses trois triangles disjoints
  créent neuf alias, pas le témoin FIRST = LAST au pic transitoire 3. Garder D
  et ajouter un simplex K = 2 mono-lot séparé dont `LiveFoldStats` vérifie
  `peak_aliases == 3` puis la vacuité ;
- l'empreinte constante éprouve `FacetIntern`, pas `LiveIndex::home/erase`.
  Ajouter une micro-fixture directe de cluster traversant la frontière 15 -> 0,
  avec suppressions tête/milieu/queue, vérification de `get`, `used_` et
  réinsertion. Le stress externe FIRST/LAST à hash constant reste dû à L3 ;
- `g_alloc_bytes` est un maximum global imprimé avec le témoin du maximum
  d'alias. Le conserver avec son propre ordre, ou publier un témoin séparé.
  Échantillonner aussi après les morts, car les free-lists peuvent croître au
  dernier lot. Tant que scratchs, sortie et préparation sont exclus, nommer ce
  nombre « octets alloués des structures sélectionnées », jamais RSS.

Le balayage structurel mérite aussi un petit correctif avant toute mesure CPU.
La condition actuelle réalise 65 à 127 passages pour certaines valeurs de
`nb`, malgré le commentaire « au plus 64 » ; prendre un stride plafond
`ceil(nb / 64)`. Surtout, vérifier `x < av.size()` avant de déréférencer un lien
corrompu, puis les backlinks `prev`, le nombre de composantes, la disjonction
avec les free-lists et les cases occupées de l'index. Ces balayages sont un
oracle de porte : les laisser actifs dans le futur chemin produit ajouterait
jusqu'à des dizaines de parcours du vivant au réducteur que l'on cherche à
accélérer. Les mettre sous option de vérification ou sous `MHGP5_TESTING`, tout
en gardant les compteurs de vie O(1) dans le nominal.

Deux libellés doivent enfin être resserrés. La chaîne adverse ne déplace pas
une grande liste entière à chaque niveau : les feuilles ont déjà leur dernière
incidence ; elle force surtout l'alias persistant à être déplacé environ 200
fois. C'est exactement pourquoi le maximum **par alias** est causal, mais ce
n'est pas un coût quadratique de la liste vivante. Et la borne
`ceil(log2(F+2))+1` est sûre mais plus lâche que la borne de doublement
`floor(log2(F))` ; la documenter comme marge de porte, pas comme l'énoncé exact
du théorème.

Enfin, le texte « `free-on-absorb` ne recycle plus rien » est trop fort : le
pin ne recycle plus les alias, mais pousse encore le record de composante dans
`cfree` alors que ces alias gardent son indice. Le code 4 Release est reçu ;
l'absence de diagnostic ASan/UBSan est rapportée par Claude, pas encore rejouée
par cet audit. L'injection reste diagnostiquement sale et ne doit pas servir de
preuve structurelle indépendante.

### P0 sur la sonde miroir en cours dans le worktree

La sonde actuelle appelée depuis `on_forest` n'est pas attribuable : le
pipeline a déjà construit le `ForestResult` résident et le conserve pendant le
callback, qui prépare puis réduit une seconde fois. Le bras vivant mesure donc
le résident plus le vivant ; `ru_maxrss` inclut aussi tout l'amont et les folds
en vol. La copie de `fp.keys` est hors des chronos annoncés, le rejeu est
optionnel et son résultat n'est pas confronté à une autorité.

Ne pas publier cette sonde comme miroir CPU/RSS. Le protocole minimal propre est
de faire lire à deux processus le **même flux d'événements sérialisé** : un seul
`prepare + reduce_fold` dans le premier ; un seul
`prepare + reduce_fold_live + rejeu strict` dans le second. Les deux doivent
produire le même digest et tous les coûts de copie/catalogue doivent appartenir
au chrono. À défaut, la sonde actuelle n'est qu'un micro-banc incrémental de
callback et son RSS ne compare pas les réducteurs.

GCP non utilisé pour cette réponse.
