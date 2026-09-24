# S4b exploratoire : le neuvième signe de la grille J8 est perdu

24 septembre 2026. **Préflight d'un prototype scratch, pas défaut du moteur
v9 publié.** Un exécutable `s4b_design8` était en cours sur 08/000000
sans sol/K5. Le source voisin dans le scratchpad `s4b/ep/` du constructeur
porte le SHA-256 :
`09e73353a6c69357fcd0c50b01d06ad0e6cfebc6d66e34fbc9879592461e3529`.
L'exécutable porte le SHA-256
`61b0659556b225deadfd58ea5aa78babd93f6f5ac8ffddd79076c1c7672a78ec` ;
aucun reçu ne lie encore formellement ces deux fichiers. Le source n'est
pas sur `main` à cette lecture. Ne pas promouvoir son chrono ou son
égalité d'objet éventuelle en preuve de S4b.

Dans le chemin **DESIGN** (`:470–505`), `J=8` construit neuf bornes
`g[0]..g[8]`, mais `dn`, `dp`, `a8` et `b8` sont des `uint8_t`. Le masque
`1U << 8` est tronqué lors de l'affectation à huit bits. Ainsi, pour
tout site, `((dn[t] >> 8) & 1U) == 0` et de même pour `dp` ; la dernière
lentille `lens[7]` reste nécessairement **zéro**. Comme `T=K−2>0` à
K5/K10, `all(lens[j] >= T)` ne peut jamais être vrai : aucun certificat
DESIGN J8 ne peut fermer une graine. Le dernier seau traite en outre
comme événements des sites qui auraient eu un signe stable à sa borne
droite ; l'attribution des racines et les profondeurs candidates ne
sont plus celles de la grille voulue. Cela peut changer travail **et**
objet, pas seulement une statistique.

Le dernier seau (`:499–537`) contient alors tous les sites du cover avec
`B≠0` dans `evj` et peut comparer chaque candidat à tout ce seau : le prototype
retombe potentiellement sur un balayage quadratique par graine. Une longue
durée de ce processus ne mesurerait donc pas le design J8 corrigé.

Le diagnostic préliminaire `J=8` calculé plus haut dans ce même fichier
emploie des masques `uint64_t` sur la grille `JMAX=32` : ses comptes ne
sont **pas** invalidés par ce défaut local au chemin DESIGN. Sa grille
`-mubar + floor(2*mubar*i/32)` peut toutefois différer d'une unité,
du côté négatif, de la grille symétrique du DESIGN (`:327`, `:472`). Ses
comptes J8 ne sont donc pas exactement ceux du design, même une fois le
masque corrigé. De plus, le prototype prépare `P`, `B` et 33 signes
pour **tous** les couples graine–site avant de simuler l'arrêt anticipé
(`:323–343`), après avoir déjà payé le pipeline produit et le cœur S3 :
ses pas de warp J8 sont un modèle, pas une économie de travail réalisée.

Le processus observé n'a pas `S4B_FULL` dans son environnement. Sans cette
variable, le flux de référence `mine` n'est pas construit
(`:564–628`) alors que le code de sortie compare `mine` au produit
(`:685`, `:761`). Un code 1 dans cette configuration ne juge donc pas le
DESIGN ; seul `design_compare` compare ses sorties au produit, et il faut
conserver cet oracle au rejeu.

Correction minimale : employer au moins `uint16_t` pour les quatre
stockages de signes DESIGN, et verrouiller `J+1 <= digits(Word)` par
assertion de compilation. Une fixture à signe strictement positif et
une à signe strictement négatif en `g[8]` doivent exercer le bit 8 ;
exiger `d_cert>0` sur la sonde qui certifie avec J8, puis comparer le
multiensemble q4 complet (support, clé, profondeur, tous les IDs de
coquille) au produit, y compris les racines sur bornes et les ex æquo.
Mesurer ensuite les coûts réels de préparation et des seaux survivants,
avec le même échantillon d'arêtes avant/après correction. Aucun résultat
GPU/G4 ou borne sous-quadratique ne découle de cette porte scratch.
Ce chemin consomme les survivantes **après** le certificat S3 : même
correct, il peut réduire le q4 aval et l'atlas, mais il ne supprime pas les
formes du cœur déjà calculées. La pente défavorable de ce cœur dans les
quarts LiDAR exige donc une voie amont distincte et une mesure intégrée.
