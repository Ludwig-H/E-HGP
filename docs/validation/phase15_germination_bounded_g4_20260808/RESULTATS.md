# Douze candidats par record à cinquante mille points

> **Statut : profilage.** `deployment_status = profiling_only`,
> `public_status = not_claimed`. Aucun pipeline qualifié, aucune porte ouverte
> ou fermée, aucun statut promu.

Troisième session G4 du 8 août 2026, dépôt au commit `64b6411`, régime de germe
**certifié** partout (borne tangente $D\le 2R(p)$, jeu de 26 directions, rayon
de recouvrement **prouvé** 27,570°), $K=5$, jamais de coupe-circuit
heuristique. Chaque cellule porte un délai opérationnel et rend donc une
ventilation, censurée ou non. VM relue `TERMINATED`, clé de session révoquée.

---

## 1. La cellule du contrat

`uniform_latin`, $n = 50\,000$, arité 3, délai 2 400 s :

| grandeur | valeur |
|---|---:|
| boucle de germes parcourue | **99,92 %** des 1 249 975 000 paires |
| paires retenues | 4 525 888 — **0,362 %**, soit **90,5 par point** |
| triples produits | 29 842 507 — **596,9 par point** |
| part de $\binom{50000}{3}$ | **0,000143 %** |
| supports acceptés | 2 482 617 |
| **candidats par record** | **12,02** |

Pour situer : la subdivision de produit examine 1,83 fois l'univers, soit
$2{,}16\cdot10^{11}$ candidats par record à $K=5$. **Douze.**

C'est aussi mieux que les ~24 candidats par record que le dépôt espérait au
mieux le 7 août — et ce chiffre-là reposait sur un coupe-circuit **non
certifié**, tandis que celui-ci est produit par le régime dont la complétude
est un théorème.

> **Ce que la cellule ne dit pas.** Elle est **censurée** : la boucle de germes
> d'arité trois s'est arrêtée à 99,92 %, donc son
> `completeness_guaranteed` est faux et le run n'est pas une énumération
> complète. L'arité quatre n'a jamais démarré, alors qu'elle représente 98,4 %
> de l'univers.

> [!CAUTION]
> **Erratum du schéma v1.** `dl_uniform_latin_50000.json` et les quatre cellules
> `dl_eight_clusters_{1024,2048,4096,50000}.json` ont sérialisé pour cette arité
> quatre non exécutée un objet initialisé par défaut : `proof_basis` vide,
> `support_size=0`, tous les compteurs nuls, mais
> `completeness_guaranteed=true`. Ce dernier champ est faux et ne doit pas être
> lu comme un certificat. Les artefacts historiques restent immuables. Le
> schéma `scale_probe.v2` ajoute `applicable`, `executed` et
> `floating_rejections_certified`, ce qui rend ce placeholder non certifiant.
> Ce dernier champ reste faux pour le prototype actuel. La conservation
> scalaire des paires examinées et restantes ne prouve pas l'identité d'un
> suffixe repris; sans curseur contigu, liaison audit--payload et replis exacts,
> la chaîne reste volontairement non scellable.

## 2. Ce que la borne certifiée fait quand la densité monte

Arité 3, cellules où elle est **complète** (`uniform_latin`) :

| n | paires retenues | % des paires | **par point** | triples | part de $\binom n3$ |
|---:|---:|---:|---:|---:|---:|
| 2 048 | 201 517 | 9,61 % | 98,4 | 1 103 180 | 0,0772 % |
| 4 096 | 439 811 | 5,24 % | 107,4 | 4 430 152 | 0,0387 % |
| 8 192 | 783 924 | 2,34 % | 95,7 | 9 255 414 | 0,0101 % |
| 16 384 | 1 535 092 | 1,14 % | 93,7 | 14 489 179 | 0,0020 % |
| 50 000 *(censurée à 99,92 %)* | 4 525 888 | 0,362 % | **90,5** | 29 842 507 | 0,000143 % |

**Les paires retenues sont $\Theta(n)$** : 98,4 puis 107,4 puis 95,7 puis 93,7
puis 90,5 par point, sur plus d'une décade. La fraction retenue tombe donc
comme $1/n$, et c'est le comportement que la théorie annonçait sans pouvoir le
mesurer — la borne ne peut pas mordre là où aucune boule n'atteint $s_{\max}$,
et elle mord de plus en plus à mesure que la densité l'y contraint.

## 3. Le coût qui reste, et il a changé de place

Les 2 400 s de la cellule 50 k se lisent : 1,92 µs par **paire de germe**
examinée, sur $1{,}25\cdot10^{9}$ paires. Ce n'est plus la classification qui
domine — c'est **l'énumération $O(n^2)$ de la boucle de germes**.

Et c'est une propriété connue de cette implémentation, pas de l'algorithme.
L'en-tête de `local_germination.hpp` le dit depuis `77554b2` : *« This host
reference deliberately scans the cloud for its neighbourhoods instead of
traversing the LBVH […] a device implementation must use the index. »* Comme
les paires retenues sont $\Theta(n)$, une boucle de germes indexée les
énumérerait sans parcourir les $\binom n2$. **Le prochain incrément est donc
nommé et il est mécanique : porter la boucle de germes sur le LBVH.**

## 4. `eight_clusters` : rien n'a changé, et rien ne changera

$n = 50\,000$, même délai : **21 695 paires examinées** sur 1 249 975 000, 76,1 %
retenues, 6 785 triples par paire. Aux petites tailles : 6,711 % de l'univers à
$n=512$, et une rétention de 99,2 % à 91,3 % de $n=512$ à $n=4096$ — c'est-à-dire
que la borne ne mord **pas du tout**.

C'est scellé et sans appel : le vide qui laisse grossir la boule tangente est
**intérieur** à l'enveloppe convexe, donc aucun majorant convexe de $R(p)$ ne
peut l'exclure. **Le contrat 50 k reste énoncé par famille, et
`balanced_multiscale_clusters` est l'une des trois familles de la porte P0.**

## 5. Ce que la session ne prétend pas

- Aucune cellule 50 k n'est complète ; toutes sont censurées et le déclarent.
- L'arité 4 n'a été mesurée nulle part à l'échelle. C'est 98,4 % de l'univers
  et c'est le trou de cette mesure.
- Rien ici n'est branché au produit : le flux germé n'est consommé par aucune
  session ancrée, et les consommateurs refusent toujours sa base par nom.
- Aucun `public_status` n'est promu.
