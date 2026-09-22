# Élargissement du moteur entier à 18 bits par coordonnée (grille 1 mm)

Décision utilisateur du 22 septembre 2026 : le contrat temps (1 s puis 100 ms,
K5 et K10, LiDAR sans sol de 30 000 à 60 000 sites) se poursuit sur le
**moteur entier** élargi de 16 à 18 bits par coordonnée ; le profil float32 sans
perte reste qualifié hors contrat temps. Cadre : `exploration_v8_hors_registre`,
`backend=cpu_reference`, nouveau profil `quantized_u18_input_only` (grille
isotrope de pas paramétrable, 1 mm par défaut, translation calculée sur la
trame brute entière), `public_status=not_claimed`. GCP non utilisé pour cette
tranche. Note de conception ; l'inventaire exhaustif des dépendances 16 bits
est joint en annexe au fur et à mesure du port.

## Objet et invariants

Le moteur calcule le **même objet** qu'aujourd'hui (flux de candidats q3/q4
exact en entier, puis, à terme, la tour) sur des coordonnées entières
`0 ≤ x, y, z < 2^18 = 262 144`, soit ±131 m à 1 mm. Toutes les décisions
restent des prédicats entiers exacts (i64, i128), sans flottant ni jitter ;
aucune borne n'est « élargie à l'aveugle » : chaque majorant commenté sous la
forme « M = 65 535 : … < 2^k » est recalculé avec M = 262 143 (chaque facteur
en M gagne deux bits) et doit tenir dans le type qui le porte, sinon l'échelle
ou le type change explicitement. Les entrées u16 actuelles (2 cm) restent
valides (leur plage est incluse) et doivent produire des **sorties, digests et
compteurs géométriques bit-identiques** : c'est la régression de référence du
port (toutes les portes, les reçus `ground_baseline` et `ground_phase1`, les
digests des campagnes spatiales).

## Décisions de conception

1. **Un seul type de coordonnée** : `Coordinate` (stockage `std::uint32_t`)
   dans `src/core/types.hpp`, avec `coordinate_bits = 18` et
   `coordinate_limit = 2^18 − 1` ; `Point3` et `Box3` le portent. Toute
   entrée est validée à `prepare_cloud` (refus explicite au-delà de la limite,
   jamais de troncature ni de repli). Les clés d'unicité du nuage compactent
   trois fois 18 bits (54 bits) dans un `u64`.
2. **Index spatial** : les coupes au milieu de l'extension positive halvent au
   plus 18 fois par axe : profondeur ≤ 54 (contre 48), piles dimensionnées par
   `3·coordinate_bits + 1` = 55 cadres, jamais une constante 48/49 en dur.
3. **Bornes** : recalcul systématique (annexe), par degré en M :
   séparation et distances (degré 2, i64 : 3·M² < 2^38), lentille du front et
   citron (degré 4 : Ξ < 2^76, 16 Ξ < 2^80, i128), boules exactes (q3 : 360·M⁶
   < 2^117 ; q4 : intermédiaires < 2^117 ; q2 : < 2^98 ; i128, gcd et négation
   sûrs sous 2^127), census q3 (A ≤ 12 M⁴ < 2^76, |B| ≤ 60 M⁵ < 2^96, |C| ≤
   144 M⁶ < 2^115, bornes < 2^117), mineurs q4 et orientation d'enveloppe
   (< 2^97 et < 2^119), atlas de centres (échelle Q = 2^20 : |w| < 2^19, |T| <
   2^40, valeur par axe < 2^60, somme < 2^62 : trop près de i64 ; **l'échelle
   passe à Q = 2^18** avec `max_depth = 18`, ce qui redonne < 2^60 par somme ;
   centre q3 en i128 : |p|, |q| < 2^73, |det| < 2^113, test de cellule
   Q·|x| < 2^131 : **dépasse i128**, à réduire (voir port) ; disque du domaine
   96 M² Q² < 2^76 ✓).
4. **Entrée** : lecteur `.u32le` (12 octets par site, petit-boutiste, valeurs
   < 2^18 exigées) à côté du lecteur `.u16le` ; empreinte d'entrée FNV
   calculée sur les valeurs entières (identique pour un même nuage quel que
   soit le conteneur) ; profil publié dans les sorties de sonde
   (`quantized_u18_input_only`) ; les payloads « grille 1 mm » des reçus
   sans sol ([lidar_ground_20260921](../receipts/lidar_ground_20260921/README.md),
   maximum par axe 159 832) sont les premières entrées.
5. **Portes** : chaque fixture « extrême » à 65 535 est doublée à 262 143 ;
   les oracles rationnels (Boost) ne supposent aucune largeur ; les assertions
   de taille de registres sont inchangées (aucun compteur n'est ajouté) ; le
   mutant « limite de plage non vérifiée » (accepter 2^18) doit être tué par
   la porte du nuage.
6. **Reçus** : une campagne appariée u16 (2 cm) avant/après port (bit-identité)
   puis la première campagne 1 mm sur les trois scènes sans sol (K5, K10, W1
   et W8), même lanceur, nouveau profil.

## Hors périmètre

Les briques float32 (`src/core/float32_*`, `src/lanes/float32_*`,
`src/spatial/float32_index.*`) ne changent pas ; le brouillon de raccord
global float32 non commis n'est pas repris (consigne du 22 septembre : aucun
développement float32 pour l'instant).

## Annexe A. Inventaire (quatre lentilles, lecture seule, HEAD 93ba5bb4)

Quatre relectures indépendantes (types/nuage/index/front/q2 ; voies q3/q4 ;
formats/sondes/lecteurs/préparateurs ; portes/oracles/docs) ont recensé 415
dépendances de la largeur 16 bits. Ce qui casse à 18 bits, et non ce qui
tient de justesse, est la seule liste qui compte :

| dépendance | pourquoi elle casse à M = 262 143 | décision |
| --- | --- | --- |
| clés d'unicité `(x<<32)|(y<<16)|z` (`prepared_cloud.cpp`) | les champs se chevauchent : (0,1,0) et (0,0,65536) reçoivent la même clé | trois champs de 18 bits (54 bits) ; contre-fixture gravée |
| plage d'entrée garantie par le type `uint16_t` | un stockage plus large admet des valeurs que les preuves ne couvrent pas | refus explicite hors [0, 262 143] à `prepare_cloud`, jamais un clamp |
| carrés `D=(e−a)^2` stockés en `uint32_t` (`q2_prepared_bounds.hpp`, `q2_joint_bounds.hpp`) | 262 143² = 68 718 952 449 > 2^32 : troncature silencieuse par `static_cast`, signes faux | constantes en `uint64_t` (96 et 192 octets) |
| test de cellule du centre q3 (`q4_local.cpp`, `contains`) | numérateurs et dénominateur < 2^117, donc `scale·x` demande 2^137 > i128 | localisation par division longue exacte (`scaled_floor`), aucun produit 128×21 bits |
| disque de la carte des centres à Q = 2^44 (`q4_center_map.cpp`) | 96·M²·Q² = 2^130,6 > 2^127 | Q = 2^42, profondeur ≤ 42 (cellules réelles inchangées ; témoin hors Local28) |
| piles fixes `3·digits(uint16_t)+1 = 49` et réserves 49/97, profondeur 48/96 | l'index atteint 54 niveaux (18 coupes par axe) | `coordinate_bits`, `max_index_depth = 54`, `index_stack_frames = 55` dans `types.hpp`, dérivés partout (moteur, portes C++, validateurs Python) |
| lecteurs `.u16le` (6 octets par site) | ne portent pas 18 bits | lecteur `.u32le` (12 octets, valeurs < 2^18 exigées) à côté ; empreinte d'entrée sur les valeurs, identique quel que soit le conteneur |

Tout le reste tient : chaque borne de degré k en M gagne 2k bits ; les
bornes i64 restent < 2^42 (distances, scores, 4H), les bornes i128 < 2^121
(déterminant q4 shallow 5 760·M⁶), la puissance q3 < 2^117 (360·M⁶) ; les
commentaires de preuve ont été réécrits avec M = 262 143 (annexe B). L'atlas
Local28 garde Q = 2^20 : ses bornes de blocs restent < 2^62 en i64 (marge
d'un bit, l'échelle ne peut plus monter), et garder Q préserve bit à bit les
compteurs de partition sur les entrées u16.

## Annexe B. Choix d'implémentation

- **`Coordinate = std::int32_t`**, pas `uint32_t` : avec `uint16_t`, une
  différence `a.x − b.x` se promeut en `int` signé ; un stockage non signé
  de 32 bits l'aurait fait s'enrouler silencieusement. Le type signé garde
  exactement la sémantique des promotions historiques ; la plage est
  vérifiée à l'entrée, et une initialisation à accolades depuis un entier
  non signé devient une erreur de compilation (ce qui a exposé et forcé la
  relecture de toutes les clés de test).
- **Division longue pour le centre q3** : q = ⌊num/den⌋ puis 20 doublements
  du reste r < den (< 2^118) ; le quotient final est ⌊scale·num/den⌋ et le
  reste nul dit si la valeur est entière. Le test de cellule fermée devient
  `left ≤ ⌊v⌋` et `⌊v⌋ < right ou (⌊v⌋ = right et v entière)`. Le centre d'une
  graine aiguë vérifie |u| ≤ 1/2 en coefficients réels (λ_min du Gram = h²),
  donc |quotient| ≤ scale ; une garde à 2^100 rend l'arithmétique totale.
- **Profil publié par les sondes** : `quantized_u16_input_only` quand toutes
  les coordonnées lues tiennent sur 16 bits, `quantized_u18_input_only`
  sinon ; les reçus u16 restent comparables au JSON près, seuls les octets
  retenus (Point3 6 → 12, Box3 12 → 24, bornes q2 48 → 96) changent, et ils
  sont exclus des empreintes logiques.
- **Validateurs Python** : constantes `MAX_INDEX_DEPTH = 54`,
  `INDEX_STACK_FRAMES = 55` ; les vérifications d'égalité sur la pile de
  témoins acceptent 49 (reçus antérieurs au 22 septembre 2026) et 55.
- **Familles synthétiques** (`front_fixtures.hpp`, `p0_fixtures.hpp`) : les
  masques `& 65535U` sont des recettes épinglées (mêmes nuages, mêmes
  empreintes), pas une largeur ; seuls les types et le refus de plage
  changent.
