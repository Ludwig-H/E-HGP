# Contre-audit B — port u18 : arithmétique, porte manquante et reçu CLI

22 septembre 2026. Base : premier moteur `d2700314`, relu dans un worktree
détaché, sans modifier le code produit. Le profil temporel principal v9
est la grille entière isotrope 1 mm à 18 bits ; les résultats u16/2 cm
de v7/v8 ne qualifient pas ce port. Les vingt portes disponibles passent
en Release et sous Clang 18 ASan/UBSan ; leur domaine reste borné.

## Ce qui tient, et ce qui n'est pas encore qualifié

Le port FULL a élargi les clés et les minimisations de boîte au domaine
`M=2^18−1=262143`. Les majorants dérivés pour des supports u18 sont
cohérents avec les types annoncés : q3 `A≤12M⁴`, `|B_i|≤60M⁵`,
`|C|≤144M⁶` ; q4 `A≤48M³`, `|B_i|≤240M⁴`, `|C|≤576M⁵`.
Les produits de comparaison des niveaux restent sous `2^191` pour q3
et `2^278` pour q4, dans U192/U320. Le produit d'une composante de
normale par une composante de `cnum−cden·p` dans le test de plateau
peut, lui, dépasser **int128 signé** ; le port emploie S192 pour ce
produit. Ces estimations exigent encore une porte arithmétique u18
indépendante couvrant clés, niveaux, boîtes et contacts. `PROVENANCE.md`
annonce `arith_u18` comme suite, mais le commit n'enregistre que les
vingt tests actuels. Le fait qu'une borne manuelle paraisse correcte
n'est pas une qualification du chemin complet.

L'[audit gate autonome](check_plateau_u18_wide_20260922.cpp) prend
`a=(1,3,5)`, `b=(262136,262132,13)`,
`x=(262126,19,262120)`. C'est un triangle strictement aigu ; sa
normale est `(68707942707,−68707418525,−68706369965)`. Le premier
terme de `n·(cnum−cden·a)` vaut exactement
`340072132602571381029370083900622849320`, strictement supérieur
à `INT128_MAX=170141183460469231731687303715884105727`, tandis que
la somme des trois termes est **zéro**. Le gate vérifie le grand terme,
la coplanarité et `triangle_closed=true`. Il a compilé avec les
avertissements stricts et passé sous Clang O2 ainsi que Clang
ASan/UBSan ; l'ancien produit i128 aurait eu un débordement signé.
Cette fixture confirme la nécessité et un cas correct de S192, pas
tous les majorants. Une autre fixture du census q2 à centre x=200000
doit tuer l'ancien clip d'argmin à 65535 ; le code port actuel clippe
bien à `kCoordMax`, mais la porte dédiée reste à intégrer.

## Deux défauts du lanceur qui faussent une qualification

1. `tower_probe.cpp` parse un entier non négatif dans un type large,
   puis rétrécit K en `unsigned` **avant** que le cœur valide 1..10.
   La commande avec `K=4294967297` (soit `2^32+1`) a rendu **code 0**,
   `complete_relative`, `run_tower=true`, `options.K=1` et seulement
   l'ordre K1 sur un diagnostic de deux sites. L'argument fourni n'a
   pas été refusé : un rapport pourrait être étiqueté K10 par son
   ordonnanceur tout en calculant autre chose. Vérifier le domaine
   avant chaque conversion, détecter aussi le wrap du parseur décimal,
   puis ajouter les refus CLI à la suite.
2. Le texte libre `--grid=` est imprimé dans le JSON **sans échappement**
   et ne certifie pas le pas de préparation. Le diagnostic
   `--grid=1mm","sites":999,"extra":"x` a produit un objet `input`
   avec deux clés `sites` (`999`, puis `2`) et code 0. Un lecteur JSON
   peut interpréter ces doublons différemment. Échapper le champ ou
   imposer une énumération stricte ; lier le manifeste/hash des octets
   d'entrée au profil 1 mm réel plutôt qu'au libellé CLI.

Ces défauts sont à la **frontière de preuve/entrée**, pas une
contre-preuve de la géométrie du cœur. Un reçu de contrat ne doit pas
se fonder sur le seul `status=complete_relative` : avec
`--no-tower`, ce statut arrive également avec zéro ordre et digest nul.
Exiger `run_tower=true`, ordres K1..K demandé, hash/provenance, sortie
FULL et chronos de toutes les phases. Ni les 20 CTests, ni la fixture
ci-dessus ne démontrent encore une croissance sous-quadratique ou le
contrat G4.

## Addendum — nouvelle porte arithmétique encore non publiée

Dans le worktree développeur postérieur à `d2700314`, une porte
`arith_u18_gate.cpp` d'environ 1 600 lignes apporte des comparaisons
`cpp_int`/`cpp_rational` de formules distinctes, des cas extrêmes et des
refus. Les en-têtes arithmétiques modifiés ne changent **que les
commentaires** ; aucune régression d'instructions n'en découle.
Cependant `CMakeLists.txt` crée `arith_u18` avec
`mhgp9_product_executable`, sans `MHGP9_TESTING`. Dans cette configuration,
`MHGP9_MUTANT(...)` vaut toujours `false` : le mutant `level-trunc-hi`
annoncé dans `wide.hpp` n'est ni activable par la CLI de cette porte ni
exécuté par CTest. L'oracle échantillonné conserve sa valeur, mais la
preuve causale « mutant tué » est **non établie**. Il faut une cible
instrumentée distincte et un reçu du premier désaccord numérique exact.

Deux commentaires de q4 conservent aussi les anciennes bornes u16 malgré
la nouvelle ligne u18. Pour `M=262143` et le tétraèdre bien centré
`a=(0,0,0)`, `b=(0,M,M)`, `x=(M,0,M)`, `y=(M,M,0)`, les formes **brutes
non réduites** valent `det=16M³`, `N'=(8M⁴,8M⁴,8M⁴)`, donc
`|N'|²=192M⁸` (152 bits), `det²=256M⁶` (116 bits) et
`|B_i|=16M⁴` (76 bits) : les mentions `<2^146`, `<2^114`, `<2^74`
de `q4.hpp` sont fausses en u18. En translatant le même motif par
`t=M/3=87381` sur chaque axe et en prenant `L=2M/3`, le coefficient
brut `C=48L³t(t+L)` occupe 93 bits, pas `<2^90`. Les nouveaux majorants
u18 `<2^160`, `<2^120`, `<2^81`, `<2^100` restent compatibles avec
ces témoins. Le commentaire `<2^171` pour la comparaison q3/q3 dans
`level.hpp` doit également suivre la borne u18 `<2^192`.
Ces écarts sont documentaires/de preuve ; aucun débordement réel de la
voie U192/U320 n'est montré par ces témoins.
