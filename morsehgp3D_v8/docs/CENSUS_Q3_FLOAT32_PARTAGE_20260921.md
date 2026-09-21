# Compter ensemble les témoins de plusieurs triangles q3

21 septembre 2026. `exploration_v8_hors_registre`, `cpu_reference`,
`lossless_float32_input_only`, `native_q3_shared_prefix`, `not_claimed`.
Suite des [clés et prédicats natifs](IDENTITE_FLOAT32_ET_EVENEMENTS_Q4_20260921.md).
Cette entrée traite **une arête fournie** et un sous-arbre de graines du
même index. Ni front WSPD natif, ni génération de toutes les arêtes,
ni q4 natif complet, ni catalogue, ni FULL ne sont qualifiés par ce lot.
Un rejet par saturation q3 ne rejette pas la famille q4 correspondante.
Ne pas utiliser les seules sorties de cette entrée pour accéder à q4,
ni transférer ses comptes au seuil q4.

## L'objet et ce que l'on veut éviter

Pour une arête (a,b), chaque troisième site x forme un triangle candidat.
S'il est strictement aigu, sa boule est acceptée lorsque son nombre de
sites strictement intérieurs est inférieur à Kmax−1. Tous les sites du
nuage peuvent être témoins, y compris ceux qui ne formeraient pas un
triangle aigu avec (a,b). Les points exactement sur la sphère ne comptent
pas comme intérieurs, mais doivent tous être rendus dans la coquille.

L'entrée `run_float32_q3_edge_census` propose deux modes :

- `Individual`, référence par défaut : préparer chaque triangle valide,
  compter ses témoins depuis la racine, puis collecter sa coquille s'il passe ;
- `SharedPrefix` : compter d'abord des blocs de témoins communs à tout
  un groupe de triangles possibles, puis transmettre le travail acquis.

La positivité est testée au relais, pas par la préparation d'une clé
lourde. Un groupe rejeté peut contenir des graines invalides : son nombre
de sites n'est pas un nombre connu de triangles aigus. La sortie est un
support positif, sa profondeur exacte et une vue de sa coquille globale.
Pas de liste d'IDs intérieurs, de déduplication des boules ni de propriété
par la plus longue arête. Le callback peut demander la clé à l'émission,
mais ce coût n'est pas caché dans une opération prétendument gratuite.

## Borner tous les centres d'un groupe

La boîte X contient les troisièmes sites possibles. Le centre d'un
triangle strictement aigu est dans son triangle : il est donc dans la
boîte de l'enveloppe convexe de a, b et X. Cette affirmation est
**conditionnelle aux seuls supports positifs**, pas une certification de
validité de tous les points de X.

Avec $d=b-a$, $u=x-a$, $D=d\cdot d$, $U=u\cdot u$, $E=d\cdot u$,
poser $G=\lVert d\times u\rVert^2$ et
$W=U(D-E)d+D(U-E)u$. Le centre vaut $a+W/(2G)$ lorsque G est positif.
Le calcul en intervalles sur X peut donc resserrer la boîte initiale
lorsque sa borne inférieure de G est strictement positive. Sinon il
garde la boîte initiale. Une intersection vide ne devient pas un nouveau
certificat d'absence de graines : on revient conservativement à cette boîte.

Les coordonnées float32 sont converties exactement en double depuis
leurs mots, y compris les sous-normaux. Les calculs sont arrondis vers
l'extérieur. Une division peut temporairement produire une borne infinie ;
l'intersection avec la boîte géométrique **finie** précède toute requête.
Aucun quotient infini ne participe ensuite au calcul d'une puissance.
Les endpoints sont élargis en nombres double normaux près de zéro pour
rester sûrs sous FTZ/DAZ. Aucun epsilon ne décide un signe.

Cette préparation n'est payée qu'une fois par bloc X traité. Les petits
blocs sont relayés avant cette préparation. La variante plus fine dans
le plan médiateur proposée par l'audit A n'est pas portée ici : sans
propriété de plus longue arête, sa borne sur le paramètre diffère. Il ne
faut pas importer une constante valable seulement sous cette propriété.

La [contrelecture indépendante B, publiée ensuite à74fb0a6a](../audits/DIALOGUE_AUDITEUR_B.md)
confirme la sûreté et précise une meilleure factorisation candidate.
Avec m=(a+b)/2 et h=u−(E/D)d, la même identité s'écrit
$c=m+\xi h$, où $\xi=D(U-E)/(2G)$ et $\lambda=2\xi$ dans la notation de A.
Pour tout triangle aigu, $0<\xi<1/2$ car $G-D(U-E)=E(D-E)>0$.
La borne λ≤2/3 exige en revanche la propriété de plus longue arête.
Le repli m+[0,1/2]h(X), intersecté avec le hull, reste donc une piste
valide sans cette propriété ; le quotient scalaire peut le resserrer.
B constate que notre enveloppe actuelle est parfois beaucoup plus large.
Cette variante et les fixtures axiales/tournées proposées restent à porter
et mesurer ; elles ne modifient pas les sources gelées de cette capture.
Une intersection certifiée vide pourrait également rejeter un groupe sans
graine positive ; la présente version conserve son repli conservateur.

## Interroger une boîte de témoins

Pour un centre c et le point connu a de la sphère, la puissance de z est
$(z-a)\cdot(z+a-2c)$. Cette forme ne nécessite ni rayon approché, ni
carré de gros coefficient, ni quotient pendant les visites de témoins.

La puissance est une somme indépendante sur les trois axes. Pour chaque
axe, elle est affine en c : les deux extrémités de l'intervalle du
centre suffisent. À centre fixé, elle est une parabole convexe en z :
son minimum est au centre ramené dans l'intervalle Z, son maximum à une
des deux extrémités de Z. Six paraboles et18évaluations avec arrondi
extérieur bornent donc le bloc entier. Ce coût est explicitement compté.
Les sommets sont continus : aucun floor/ceil de la vieille grille u16.

Une borne supérieure strictement négative prouve que tout le bloc Z est
intérieur pour chaque graine valide de X. Une borne inférieure strictement
positive prouve l'extérieur. Tous les autres cas restent indécis, contacts
compris. Cette enveloppe peut être large : la sûreté n'est pas un gain
de performance en soi. Au relais individuel, la boîte X devient singleton ;
les feuilles Z utilisent le prédicat exact filtré déjà qualifié.

## Le compte ne se transmet jamais sans sa position

Chaque cadre conserve `(nœud X, compte, curseur Z)`. Le curseur suit le
préordre immuable de l'index ; tout ce qui le précède est entièrement
classifié, avec le même compte strict pour chaque graine valide de X.

Un bloc Z intérieur crédite puis saute à son `escape`. Un extérieur
saute sans crédit. Une ambiguïté interne descend vers le fils gauche.
Une feuille Z ambiguë force la division de X **avant de consommer cette
feuille**. Chaque enfant reçoit la copie du même ticket figé. Les seuls
témoins sautés sans borne sont a et b : ils sont sur toutes les sphères.
On ne saute jamais tous les sites de X comme s'ils étaient cette ancre.

Au relais, chaque graine valide copie à nouveau ce ticket. Le parcours
individuel reprend au curseur, pas à la racine avec un crédit. Le ticket
modifié par une première graine n'est pas transmis à la suivante. Le
compte sature au seuil uniquement pour rejeter ; un compte accepté est
exact. Aucun ticket public librement fabricable ne permet de changer
l'index, l'arête ou le seuil après certification.

La coquille fait ensuite l'objet d'une **nouvelle traversée globale**,
y compris si le compte est déjà à EOF. Seuls des signes strictement
non nuls permettent d'écarter un bloc de cette collecte. Tous les
contacts des préfixes, a et b compris, doivent réapparaître.

Limite d'exercice : avec ce même arbre pour X et Z et des décisions de
bloc seulement strictes, une graine valide ne peut pas être relayée avec
le curseur déjà à EOF. Son propre site est un contact ; il impose un
relais/division avant consommation. `relays_at_eof` reste donc nul pour
les supports valides dans cette version. Le traitement défensif du cas
EOF n'est pas une branche positive qualifiée par une fixture impossible.

## Objets, parallélisation et comptabilité

Le contexte conserve l'index partagé et possède une copie des options.
La boîte de centres ne contient ni pointeur vers le producteur ni cache
mutable. Les cadres X font24octets sur la cible locale ; leur pile est
réservée selon la profondeur médiane de cet index, plus1. Ce n'est ni
un plafond de recherche ni l'ancienne pile49 des coordonnées u16.
Z se parcourt sans pile. Le tampon de coquille est réutilisé par appel.

L'index est partageable, les états et callbacks doivent rester privés.
Le callback reçoit des vues synchrones, à copier s'il veut les conserver.
Une exception conserve les émissions et compteurs déjà produits ; aucune
transaction n'est promise. Les arguments invalides sont rejetés avant
toute modification du registre. Ces objets permettent un futur partage
de tâches X possédant leur contexte ; **aucun ordonnanceur multi-CPU ou
GPU nouveau n'est implémenté ici**.

Les compteurs distinguent préparations communes, requêtes X×Z, divisions
de X et Z, crédits, population brute rejetée, relais, supports valides,
census individuel, collecte et callback. Les sous-registres séparent
boules, puissances et les deux familles de bornes. La capacité de pile
et de coquille se réduit par maximum, pas par somme. Cela n'est pas le
RSS ni la mémoire ajoutée par le consommateur. `count_outside_nodes`
comprend aussi les contacts singleton exclus du seul compte strict.

La discipline `-ffp-contract=off -fno-fast-math -frounding-math` est
imposée par ces lanceurs autonomes. Avant raccord à CMake ou au GPU,
porter explicitement ce contrat de compilation et ses contrôles ; ne
pas supposer qu'il accompagne automatiquement les headers. Mesurer aussi
le taux de repli entier sur les trames réelles et la pile par worker.

Sur succès, la population de graines se partitionne exactement en sites
relayés et sites rejetés par bloc. Les relayés se partitionnent en a/b
éventuels et supports examinés ; les supports valides en acceptés et
rejetés par saturation individuelle. Les crédits hérités ne sont pas
repayés artificiellement comme de nouvelles visites géométriques.

## Preuve et mesures closes

Le [relais indépendant A](../audits/q3_prefix_relay_20260921/README.md)
a orienté cette architecture, mais ses nombres ne qualifient pas le port.
Le nouveau juge reconstruit centres, profondeurs et coquilles en fractions
exactes ; Individual et SharedPrefix sont confrontés au même oracle.
Release et Clang ASan/UBSan/LSan passent chacun14commandes de qualification :
18fixtures,612appels Fraction,1798supports,8610IDs de coquille, coquille
cosphérique complète de30sites ;37refus CLI et16corruptions de résultats.
Les458contrôles natifs incluent224contrôles de bornes et96appels complets
sous quatre arrondis et quatre états FTZ/DAZ, plus32appels concurrents
sur quatre lecteurs. Ce n'est pas une porte ThreadSanitizer.

Deux mutants compilés sont tués en37commandes par une géométrie erronée
avec sortie normale : compter à nouveau le préfixe avec son crédit
change la profondeur2 en3 et perd un support ; commencer la coquille
au curseur du compte perd a et b. La référence Individual reste correcte.
Un préflight avait des attendus manuels faux pour deux puissances ;
sa trace est conservée, sans modification du code produit pour le corriger.
Les sources antérieures de boules/index/clés et CMake u16 restent inchangées.

La [campagne et ses reçus](../receipts/float32_q3_census_20260921/README.md)
comptent36observations : colonne et bande étroite, n8k/16k/32k, K5/10,
Individual et SharedPrefix de grains1/8. Tous les sites sont traités
contre UNE arête ; index, partage, résidu, coquilles et digest sont payés.
Les24comparaisons de sorties entre modes concordent. La colonne a aussi
un oracle analytique à grande taille ; la bande a un différentiel, pas
un oracle exhaustif indépendant à32k. Aucun front ni paramètre s n'intervient.

Exemple K10/32k, colonne :640422visites globales avec Individual contre
601avec SharedPrefix/grain1 ;31998préparations de supports contre11.
Les temps observés de census sont1501,299ms contre1,344ms, auxquels
s'ajoutent respectivement13,486ms et13,190ms d'index. La bande donne
1640,404ms contre1,393ms de census, avec22,364ms et20,639ms d'index.
Une seule observation par configuration sur hôte partagé : pas un gain
stable revendiqué, ni un budget transférable à une trame entière/G4.
Ces familles allongent leur étendue avec n à largeur bornée ; elles ne
sont pas des scans LiDAR. Voir l'[analyse complète](../receipts/float32_q3_census_20260921/ANALYSE_CROISSANCE.md).

Sur les24doublements, aucun des122postes publiés n'atteint×4.
Le maximum des visites totales vaut×2,117 pour Individual et×1,062
pour Shared/grain1 ; celui des préparations×2,001 et×1,052.
Les comparaisons des tris de l'index donnent le maximum tous postes,
×2,218. C'est une croissance sous-quadratique observée sur ces familles,
pas une preuve globale ; les compteurs de capacité ne sont pas du travail CPU.

Le coût est proportionnel aux préparations, visites et sorties réellement
réalisées, avec une pile O(log n), hors stockage de l'index et coquilles.
Le nombre de visites et de graines sur **toutes** les arêtes n'est pas
borné sous-quadratiquement par ce résultat local. Catalogue, intérieurs,
FULL et contrat G4 restent ouverts. Pas de nouvelle mesure GCP dans ce lot.
