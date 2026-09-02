# MorseHGP3D v6 — Échelle : de 10^4 à 10^7 points

Ce document est la **référence d'échelle de la v6**. Il remplace les renvois
morts vers `ECHELLE.md § 3` et `§ 8 bis` que portaient `src/core/caps.hpp` et
`src/forest/fold.hpp` (le fichier visé était celui de la v5 et n'existe pas
dans ce chantier).

Chaque chiffre porte sa nature : **[M]** mesuré dans un reçu immuable et
**sourcé** (reçu, fichier, champ), **[O]** observation locale tracée mais **non
opposable** (hors reçu, machine de développement, graine unique), **[C]** calculé
depuis le code (taille compilée, arithmétique de plafond), **[E]** extrapolation
— jamais une loi, jamais une promesse. `public_status=not_claimed` ; aucun
chiffre de ce document ne le change.

Sources des **[M]** de ce document : reçu
`receipts/session_g4_20260901_d98f47296d67_1788245493` (`frontier_resume.txt`,
`bench_resume.txt`) pour la frontière K=10 et les accélérations ; reçu
`receipts/session_g4_20260902_c8f696739b0b_1788312873` (`matrice_resume.txt`)
pour les murs K=10 et K=5 et la propriété de préfixe ; reçu
`receipts/session_g4_20260901_b97f20ea4b8f_1788293187` (`gpuv6_resume.txt`) pour
le pilote série C. Les valeurs locales K=5 à 100 000 et 200 000 points sont des
**[O]** : huit fils, machine de développement, graine 3, aucune répétition.

## 1. Vocabulaire : deux sens de « streamé »

`docs/ARCHITECTURE.md` appelle « fold streamé par K » le pipeline à deux
étages **en mémoire vive**, borné par `fold_inflight` : rien n'y touche le
disque. Le présent document appelle « streamé » un flux **sur disque** avec
runs, tri externe et reprise. Les deux n'ont ni le même coût ni les mêmes
statuts ; ne jamais lire l'un pour l'autre.

## 2. Où est le mur, aujourd'hui

| Profil | Fait | Nature |
|---|---|---|
| K=10, uniform, 48 fils, 400 000 points | 479,5 s, 146,74 Gio, statut complet sous `RLIMIT_AS` 175 Gio | [M] |
| K=10, uniform, 800 000 points | avortement à 550 s (échec d'allocation, signal 6) | [M] |
| K=10 | mur encadré par 400 000 et 800 000 points ; estimé vers 4,8 · 10^5 sur 180 Gio | [M] puis [E] |
| K=5 (`smax=6`), 50 000 points, 48 fils | 9,08 s et 3,80 Gio, contre 47,68 s et 18,08 Gio à K=10 | [M] |
| K=5, 8 fils, 100 000 et 200 000 points | 5,21 Go et 10,37 Go, soit 0,052 Mo par point et 83 boules par point | [O] |
| K=5 | mur estimé entre 2,4 · 10^6 et 3,9 · 10^6 points, **sur la seule base d'une observation locale non opposable** | [E] sur [O] |

L'écart d'un facteur 1,6 sur le mur K=5 est le traitement de la **rétention
d'allocateur** ; aucune mesure ne le tranche aujourd'hui. La session
`g4_echelle_v1` est faite pour cela.

Le passage de K=10 à K=5 est un gain sur un **objet légitime**, pas une
approximation : les `digest_forest_K1..K5` et les cardinalités d'un run
`--smax=6` sont identiques à ceux du jumeau `--smax=11` sur la même entrée
[M], propriété vérifiée par le validateur de campagne
(`tower_scope=prefix_k<K>`). En revanche `digest_balls` et `digest_all` ne
sont **pas** comparables entre profils : le nombre de candidats émis diffère
(4 061 159 contre 21 627 009 à 50 000 points) [M].

## 3. Pourquoi : la résidence, jamais le temps

Tailles compilées [C] : `BallCandidate` 144 o, `Survivor` 16 o, `BallData`
224 o, `ForestEvent` 144 o, `FacetKey` 44 o, `FidState` 32 o,
`ComponentDelta` 160 o, `DeltaMeta` 96 o.

Décomposition **estimée** du pic à 400 000 points et K=10, à fermer par les
nouveaux relevés de pic par étage (aucun reçu ne l'établit aujourd'hui) : les
`BallData` résidents pendant les dix folds (26 %), le bloc d'internement du fold
(23 %), les arènes de deltas des ordres en vol (15 %), la rétention d'allocateur
(12 % ; son indépendance vis-à-vis de `n` est une hypothèse, pas un résultat de
reçu), les événements des ordres en vol (10 %), les états de facettes (8 %).

Trois pics ne sont échantillonnés par aucun des six jalons `rss_mb` [C] : la
fusion des shards, le tri des candidats (un double exact du tableau, détruit
avant la mesure suivante) et le census (le tampon par tranche coexiste avec
le tableau final). **Depuis le palier P2, les six mêmes frontières portent
aussi `residence_hwm_mb`** (`VmHWM`, maximum historique) et
`residence_increment_mb` (les incréments `hwm[j] − hwm[j−1]`, seule quantité
imputable à un intervalle) : les pics nés et morts entre deux jalons y sont
bornés. Mesuré à `n=8000`, 8 fils, `smax=11` : incréments 678 / 553 / 0 / 729 /
504 / 0 Mo — l'étage `après_RLE` ajoute à lui seul 553 Mo au maximum, que les
instantanés ne voyaient pas (`rss_mb` y reste à 586). Attention à la lecture :
`hwm[j] − rss[j]` est un **majorant global**, pas une mesure d'étage (un étage
qui n'alloue rien affiche un écart hérité de l'étage précédent). L'écart entre le dernier jalon et le pic réel du processus
vaut +3,6 % à 50 000 points et K=5, +4,7 % à 50 000 et K=10, **+20,5 % à
400 000 et K=10** [M]. Trois points croissants ne démontrent pas une loi ; ils
suffisent à conclure que tout budget déclaré à partir des jalons est optimiste,
et d'autant plus que `n` est grand sur les cas observés.

**Sur les runs uniformes observés, le temps extrapolé est secondaire au mur
de résidence actuel.** L'exposant du mur vaut 1,097 sur 50 000 → 200 000 →
400 000 points à K=10 [M] et 1,088 à K=5 [M] ; à cet exposant, 10^6 points à
K=10 se placeraient vers 22 minutes et 10^7 à K=5 vers 48 minutes [E]. La
formule absolue « le temps n'est jamais le verrou » est **fausse** hors de ce
régime : sur les familles minces la dernière sécante mesurée vaut 1,60 à 1,76
[M] et, appliquée de 50 000 à 10^7 points, elle multiplie la durée par 15 à 35
relativement à l'exposant 1,088 — les 48 minutes deviendraient 12 à 28 heures ;
depuis 200 000 points le facteur relatif vaut encore 7,4 à 13,9. Sur les seules
familles denses observées, c'est donc la résidence qui interdit la taille avant
le temps.

## 4. Les plafonds de type, dans l'ordre où ils tirent

**Chaque garde précède les allocations qu'elle protège** — ce qui n'est pas
« tout refus précède toute allocation » : le comptage et les structures amont
sont déjà alloués quand les gardes du fold décident, le plafond de candidats
bruts est coopératif après la matérialisation possible de shards locaux, et la
limite du format de digest est latente (aucune garde ne la porte). Aucun
débordement silencieux n'a été trouvé dans le chemin produit [C].

| Verrou | Site | K=10 | K=5 |
|---|---|---|---|
| mur de résidence (échec d'allocation, pas un refus) | — | 4,8 · 10^5 [E] | 2,4 à 3,9 · 10^6 [E] |
| incidences au-delà de 2^31 − 1 | `fold.hpp` | 1,75 · 10^6 [E] | 9 · 10^6 [E] |
| événements au-delà de (2^32 − 1) / 11 | `fold.hpp` | 3,4 · 10^6 [E] | 1,0 · 10^7 [E] |
| facettes au-delà de 2^32 (format du digest) | `digest.hpp` | 4,3 · 10^6 [E] | 2,8 · 10^7 [E] |
| candidats bruts au-delà de 2^32 − 1 | `caps.hpp` | 8,4 · 10^6 [E] | 4,2 · 10^7 [E] |
| positions au-delà de 2^30 − 1 | `caps.hpp` | 1,07 · 10^9 [C] | idem |

L'ordre est **l'inverse** de celui que suggérait le commentaire historique de
`caps.hpp` : le plafond des candidats bruts, présenté comme le mur, arrive en
avant-dernier. Le mur de résidence précède le premier refus typé d'un facteur
d'environ 3,6 à K=10 et 3 à K=5 [E].

Conséquence à graver avant tout élargissement de type : le **format** de
digest lui-même cesse d'exprimer l'objet vers 4,3 · 10^6 points à K=10.
Élargir un identifiant sans versionner le digest tronquerait silencieusement,
et deux objets distincts pourraient signer pareil.

## 5. Ce qui reste fermé

- **Tuilage spatial du fold avec halo** : rejeté sur mesure, pas sur
  intuition. Le rayon maximal atteint 149 unités sur `eight_clusters` à
  32 000 points, soit 23 % du domaine [M] ; l'amplification de lecture et le
  volume d'entrées-sorties qui en découlent excèdent le gain.
- **Empreinte probabiliste ou compteur tronqué** pour l'oubli des facettes :
  interdit. Le comptage première et dernière incidence est **exact**, par
  clé complète.
- **Un seau de Morton est une localité, jamais une autorité d'unicité** : le
  théorème de co-localisation des centres ne borne pas la taille d'un seau
  (une famille cosphérique peut en concentrer un nombre arbitraire). Tout
  routage par seau exige un débordement obligatoire et une sous-partition par
  la clé complète.
- **Le reduce du fold ne se porte pas sur GPU** (`docs/GPU.md`, piste F0).

## 5 bis. Ce que les variantes déjà mesurées plafonnent

Le multi-CPU et le GPU ont un **plafond mesuré sur les variantes livrées**, ce
qui n'est pas la même chose qu'un axe épuisé. De 1 à 48 fils l'accélération vaut
13,02 sur `uniform` et 15,68 sur `eight_clusters` à 16 000 points [M], soit une
fraction séquentielle de 5,7 % et 4,4 % : il reste ×1,34 à ×1,45 par le
parallélisme seul, et le pic croît avec le nombre de fils (×1,79 de 1 à 48 [M]),
donc élargir la largeur rapproche le mur. Côté device, les variantes C1 à C5
donnent −10,4 % du mur au mieux [M], leur étage étant à 88 % du code hôte ; même
un étage nul ne dépasserait pas ×1,31.

Le palier C6 vise précisément cette résidence hôte (plusieurs gigaoctets de
staging global) et **n'a pas encore été mesuré** : aucune donnée n'établit
aujourd'hui qu'il ne déplace ni le mur ni l'exposant. Son classement comme
palier de débit seulement attend la mesure du RSS, de la mémoire verrouillée et
des pics par phase.

## 6. L'ordre de travail

Les paliers sont livrables séparément, chacun avec son code, ses portes, ses
mutants et sa mesure. Les quatre premiers ne demandent ni disque, ni nouveau
statut, ni nouveau format.

1. **Portes de préfixe** : rejouer les 23 conformités à `smax` réduit, ce qui
   rend leur porte à deux mutants aujourd'hui orphelins et rétablit une
   couverture que seule une session payante vérifiait.
2. **Vrai pic de résidence** : relever le pic historique du processus à
   chaque frontière d'étage, sans quoi aucune économie n'est vérifiable.
3. **Libérations par tranche** (livré) : rendre la mémoire des tampons dès leur
   consommation. L'hypothèse de travail est qu'elles ne déplacent pas le mur à
   48 fils (le pic du census resterait sous celui du fold) : c'est **à mesurer**,
   pas un résultat — rien ici ne porte au-delà de 8 fils.
   **CORRECTION DU 2 SEPTEMBRE, mesurée** : le gain est celui du régime
   **parallèle**, et il est **structurellement nul** à faible parallélisme —
   `expand_detail::chunked` appelle `planned_workers`, qui rend `T = 1` pour
   `threads ≤ 1`, d'où `chunk = n` et **une seule tranche** : il n'y a rien à
   libérer avant la fin de la fonction. C'est pourtant à 1 fil que le
   transitoire du census **est** le pic du processus (7864 Mo contre 6561 pour
   le fold à `n=32000`) : le seul régime où ce transitoire est le mur est celui
   où le palier ne peut pas l'attaquer. Le gain vaut
   `(1 − 1/nchunks) × survivantes × 224 o` et ne se voit sur le RSS qu'aux
   grandes tailles : glibc ne rend une tranche libérée à l'OS que si elle
   dépassait son seuil dynamique de `mmap` (mesuré à `n=2000` : la coexistence
   réelle tombe de 255 % à 155 % de `survivantes × 224 o` pour seulement
   201 Mo contre 213 Mo d'incrément de pic). La garde du palier est donc un
   **plafond sur la coexistence mesurée**, pas sur le RSS
   (`mhgp6_residence_mutant_tranches_gardees`, mutant `keep-ball-chunks`).
4. **Tri par permutation et piles hissées** : supprimer le double exact du
   tableau de candidats et deux allocations par boule.
5. **Crochets de test sur les gardes du fold** : les deux premiers verrous
   durs ne sont exerçables par aucune porte à petit `n` aujourd'hui.
6. **Point d'arrêt : la session de mesure** décide de la suite.
7. Ensuite seulement, et conditionnellement : largeur de fil découplée du
   format de digest, types au profil, fusion du census et de l'expansion,
   internement maigre.

## 7. Verrous ouverts

- **Positions dupliquées.** Le pipeline refuse tout nuage qui en contient
  (`unsupported_degeneracy`). L'index sait ranger plusieurs identités dans un
  bucket, mais **le pipeline ne sait pas produire l'objet correspondant** : le
  représentant d'un bucket est le plus petit identifiant, et accepter les
  buckets tels quels ferait disparaître des sommets étiquetés. « Un nuage réel
  en produit presque sûrement » était une déduction fautive : sur 2^48 positions
  tirées uniformément, 10^7 points n'ont qu'environ 16 % de chance de porter une
  collision, et 4,8 · 10^5 points environ 0,04 %. Un capteur réel peut être très
  non uniforme, mais cela **se mesure** : le prochain pas est une sonde en
  lecture seule sur les données cibles (sites uniques, masse des buckets non
  unitaires, multiplicité maximale, stabilité de la correspondance identifiant →
  site), puis seulement le choix entre un quotient par sites avec reprojection
  et une définition pondérée ou multiensemble. Jusque-là le refus est le
  comportement sûr.
- **Statuts.** Les cinq statuts du moteur sont conservés, `invariant_violated`
  compris : une contradiction interne n'est ni une donnée non supportée ni une
  ressource manquante. Trois vocabulaires distincts sont à tenir séparés : le
  résultat terminal de l'objet, l'état d'une tentative de campagne (terminée,
  refusée, expirée, tuée par signal) et, si le disque est un jour ouvert, l'état
  d'un point de reprise. Le manifeste atomique et l'invalidation des sorties
  provisoires restent utiles même sous huit heures : une panne, une préemption
  ou un dépassement mémoire ne sont pas des prédictions de temps.
- **Disque.** Toute variante streamée demande des centaines de gigaoctets de
  haute eau et une mutation d'infrastructure absente des scripts gardés ; le
  débit doit être mesuré au préflight, jamais supposé.
