# Reconnaître une même boule et ordonner les événements q4

21 septembre 2026. `exploration_v8_hors_registre`, `cpu_reference`,
`lossless_float32_input_only`, `native_identity_and_events`, `not_claimed`.
Suite du [port des supports](BOULES_FLOAT32_Q3_Q4_20260921.md), sans
changement du moteur u16 ni de ses défauts. GCP non utilisé pour ce lot.

## Ce que cela apporte

Plusieurs triangles ou tétraèdres peuvent décrire exactement la même
boule. Ils ne doivent pas déclencher autant de collectes complètes des
points intérieurs et de la coquille. Une paire, un triangle et un
tétraèdre peuvent même décrire la même boule : les IDs du support et
son nombre de sommets ne sont donc pas une identité géométrique.

`Float32BallKey` fournit cette identité commune, construite **à
l'émission**, après acceptation du support. `Float32Q4Events` prépare
une famille autour d'un triangle aigu et compare ses événements sans
calculer ni arrondir les positions des centres. Les deux objets gardent
leurs données ; les compteurs et temporaires appartiennent à l'appelant.
Cela prépare le regroupement et le partage des tâches, sans encore
implémenter un catalogue global, un ordonnanceur ou la tour FULL.

## Clé commune : centre et rayon, pas seulement rayon

Chaque coordonnée binary32 finie est un entier exact dans l'unité
$2^{-149}$. Avec $Q=2^{149}z$, la boule est représentée par le polynôme
$A\lVert Q\rVert^2+B\cdot Q+C$, où les cinq coefficients sont entiers,
$A>0$ et leur PGCD commun vaut1. Le centre vaut $-B/(2A)$ dans cette
unité. Ce polynôme primitif identifie donc la boule entière, orientation
et facteur multiplicatif éliminés. L'égalité ne dépend ni de l'arité,
ni d'un ordre des sommets, ni d'un ID, ni de `q_min`.

Pour q2, les coefficients avant normalisation sont directement
$(1,-a-b,a\cdot b)$. Pour q3/q4, la forme relative déjà certifiée est
$S\lVert Q-a\rVert^2-L\cdot(Q-a)$ ; après orientation commune positive,
on développe $A=S$, $B=-2Sa-L$, $C=S\lVert a\rVert^2+L\cdot a$.
Le pont privé `Float32Ball::global_coefficients` utilise les mêmes
formules que les prédicats. Il ne recertifie pas la positivité et ne
crée pas une fausse requête de puissance dans les compteurs.

La capacité1728bits couvre aussi cette expansion. En posant
$M=2^{278}$, on a pour q3 $A\leq12M^4$, $|B_i|\leq60M^5$,
$|C|\leq144M^6$ ; pour q4 $A\leq6M^3$, $|B_i|\leq30M^4$,
$|C|\leq72M^5$. Chaque coefficient et intermédiaire reste sous la borne
précédente $2^{1677}$. Cela ne justifie pas d'autres expressions plus
hautes en degré, notamment le tri naïf des rayons rationnels.

Le support q3/q4 reste un objet128octets ; il n'embarque pas cinq
tableaux entiers1728bits. Une clé ne contient que ses mots actifs :

- version1, puis les cinq coefficients dans l'ordre A,Bx,By,Bz,C ;
- zéro : un en-tête0 ;
- non-zéro : en-tête `2*nombre_de_mots+signe`, nombre de bits nuls
  terminaux, puis magnitude impaire en mots32bits de poids faible d'abord.

Le PGCD est retiré **avant** cette compression par coefficient. Les
puissances de deux sont stockées, jamais jetées numériquement. Les
champs sont auto-délimités, le premier mot actif est impair et le dernier
non nul : l'encodage est unique. Ce sont des mots portables, pas un dump
d'octets natifs ; une future sérialisation doit écrire explicitement le
little-endian. Aucun décodeur non contrôlé ne permet de fabriquer une clé
produit invalide.

La voie recommandée `from_support` retourne directement une valeur et
effectue une allocation de son vecteur. Les factories autonomes q3/q4
vérifient d'abord le support pour les usages sans support préparé. Les
copies et les déplacements de la clé recopient volontairement son
vecteur immuable, y compris le retour optionnel de ces factories : coût
réel, pas un descripteur zéro-copie. Une arène de sorties et ses offsets
restent à concevoir avant un usage massif/GPU. `capacity_bytes` mesure
la capacité du vecteur, pas le RSS ni les objets de pile.

## Événements q4 : comparer sans former les grandes fractions

Pour une seed ordonnée $(a,b,x)$ strictement aiguë, poser $d=b-a$,
$u=x-a$, $n=d\times u$, $v=z-a$, $B_z=n\cdot v$. Les racines sont
proportionnelles à $P_z/B_z$, où $P_z=G\lVert v\rVert^2-W\cdot v$
et $G>0$. Une racine ne se compare que si $B_z\neq0$ : les deux signes
de B sont calculés et les sites coplanaires refusés avant le déterminant.

Ne pas former directement $P_1B_2-P_2B_1$ : ce produit a un degré9
dans les coordonnées entières. L'identité réduite donne
$G\Delta=P_2B_1-P_1B_2$, où Delta est le déterminant4×4 des lignes
$(d,\lVert d\rVert^2)$, $(u,\lVert u\rVert^2)$,
$(v_1,\lVert v_1\rVert^2)$ et $(v_2,\lVert v_2\rVert^2)$.
Le résultat de comparaison est donc
$-\mathrm{sign}(\Delta)\mathrm{sign}(B_1)\mathrm{sign}(B_2)$.
Les deux signes des dénominateurs sont nécessaires, y compris lorsqu'ils
diffèrent. Delta nul donne une égalité exacte, pas une tolérance.

Les six mineurs2×2 des deux premières lignes et la normale sont
préparés en intervalles. Chaque requête prépare les six autres mineurs,
puis six produits avec signes alternés. Degré5 seulement : chaque
somme partielle est bornée par $72M^5<2^{1397}$. En unités physiques,
les bornes restent sous $2^{652}$, sans débordement du double.
Le filtre ne décide que lorsque l'intervalle exclut0 ; sinon il calcule
le même déterminant en entier exact, dans le worker. `ExactOnly` n'utilise
pas d'arithmétique flottante. La nouvelle classe184octets n'alloue rien
et ne garde ni liste d'événements ni clé de boule ni cache mutable.

La factory paie séparément la validité q3 de la seed. `side_queries`
inclut les deux appels de chaque comparaison, même refusée ; les21
compteurs de famille, dont celui-ci, sont des sommes, avec le sous-registre de
préparation de seed. Les compteurs de clé paient4appels PGCD et5divisions
par clé produite, les mots stockés et l'arithmétique de développement.
Une exception peut laisser ces compteurs partiels ; ils ne sont pas des
transactions ni une mesure de mémoire simultanée.

## Exactitude, complexité et suite

Le PGCD binaire retire les facteurs de deux et soustrait des magnitudes
ordonnées ; ses opérandes décroissent. La division exacte descend un
diviseur aligné sans élargir un reste pleine capacité. Diviseur nul ou
négatif, reste non nul et dépassement de capacité sont refusés. Les
anciens corps addition/soustraction/produit restent inchangés.

Ces opérations ont un coût borné par le domaine binary32, indépendamment
du nombre n de points. **Cela ne borne pas le nombre de candidats,
d'événements ou de clés à produire.** Aucun nouveau test8k/16k/32k ni
chrono de trame entière n'est attribué à cette tranche numérique ; les
quadratiques résiduels du moteur précédent ne sont pas résolus ici.

Restent, dans cet ordre : bornes certifiées de blocs et partage des
graines q3, raccord au front/census natif avec les parents possédés,
catalogue commun puis collecte exacte des intérieurs/coquilles et FULL.
Le comparateur de racines n'est pas un comparateur des niveaux de rayon.
Ni `q_min`, ni toutes les incidences, ni la complétude globale ne se
déduisent d'une clé unique. Pas de nouveau backend GPU, TSan ou contrat G4.

La [qualification propre](../receipts/float32_identity_20260921/README.md)
est close : Release et Clang ASan/UBSan,28commandes chacun,229cas de clés,
960cas d'événements,141refus CLI et18corruptions détectées. Dix boules
sont identiques dans les trois arités. Les tests entiers948+9051 et les
1636cas de boules précédents passent à nouveau sur les sources étendues.
Deux mutations compilées sont tuées géométriquement en43commandes ;
relectures normal/−O concordantes, sources/dépendances/binaires épinglés.
