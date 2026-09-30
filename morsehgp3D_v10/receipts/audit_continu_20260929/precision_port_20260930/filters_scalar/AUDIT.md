# Filtres locaux futurs u24/u32 — audit scalaire clos, 30 septembre 2026

Lecture seule de R=`/workspaces/E-HGP/build/v9-open-worktree`, HEAD
`bdc0b8f08ee37ee351996b29c5974f78415bc036`. Aucun moteur modifié, aucune
compilation, aucun build/GCP. Les domaines u24/u32 ne sont pas actuellement
admis par le catalogue/tour u18 : **ce ne sont pas des défauts démontrés du
moteur u18 qualifié**. Cette note complète les largeurs arithmétiques déjà
publiées ; elle ne répète pas leur tableau.

## 1. Verrou concret : le `.02` n'est pas transportable

`src/cloud/site_tree.cpp:64,66–75,130–180,183–215` fabrique un centre global
approché `a+N/D`, puis utilise une marge absolue `.02` pour nearest et la
boule fermée. `src/tower/tower.cpp:27,152–164,502–510,836–843,915–933`
réutilise cette hypothèse pour l'acceptation MEB, la décroissance et les sauts.
La preuve de ces corps est expressément u18 ; élargir seulement les entiers
ne la transporte pas.

Trois fixtures **causales à petit centre équivalent** évitent de mêler ce
verrou aux débordements des coefficients bruts. Dans chacune, les côtés
exacts et leurs intermédiaires tiennent en i128 ; tous les sommets sont de
vrais contacts du cercle MEB d'un triangle strictement aigu.

| Fixture | Centre relatif exact | Écart approché d'un contact à l'ancre | Erreur `.02` |
|---|---|---:|---|
| u24 perdu : a=(15000010,0,15000010), b=(15000010,15000010,0), c=(0,0,0) | N=(-15000010,15000010,-30000020), D=3 | +0.0625 | c écarté |
| u24 faux intérieur : a=0, b=(15000005,15000005,0), c=(15000005,0,15000005) | N=(30000010,15000005,15000005), D=3 | −0.0625 | b/c classé intérieur sans clé exacte |
| petite géométrie u18 translatée : a=(4e9,4e9,4e9), b=a+(200005,0,0), c=a+(40001,200005,0) | N=(1000025,840021,0), D=10 | +0.03814697265625 | c écarté |

La troisième configuration a une étendue de seulement 200005 par axe ;
une origine globale élevée suffit à faire échouer le filtre. Un dispatch
« géométrie locale u18 » qui conserverait l'addition flottante de l'origine
globale serait donc incorrect. Le repère doit changer **avant les opérations**.

Le fichier `normal.json` conserve aussi quatre presentations q3 polynomiales
non réduites et deux contacts q2 u32 à écarts ±512. Pour les premières, on
ne suppose pas que l'ancien `side_key` est assez large : ce sont des preuves
scalaires du filtre, pas une qualification native des coefficients bruts.

La preuve ne dépend pas de la forme de l'index : trois sites tiennent dans une
seule feuille (kLeaf=16). Aucun élagage de boîte ne peut sauver la mauvaise
classification du site par ces branches.

### nearest entier : départage canonique faux, pas une distance K fausse prouvée

q=(2000000025,2000000025,2000000025),
x=(800000010,400000005,2000000025),
a=(4000000050,2000000025,2000000025).
L'ordre Morton96 réel est x(ID0),q(ID1),a(ID2). Les distances exactes de x
et a à q sont toutes deux 4000000100000000625. L'approximation de x dépasse
celle de a de 512, et `.02` s'arrondit à zéro à cette échelle.
Avec count=2, le corps actuel rend [ID1,ID2] au lieu de [ID1,ID0].
La suppression finale de `cand` suffit à provoquer la perte, même si x fut
visité avant la baisse de borne. C'est une violation du contrat exact
`(distance, indice)` ; cet exemple ne prouve pas une mauvaise valeur de D_K.

## 2. Remplacement certifié minimal — pas une marge arbitrairement grossie

Préparer le centre relatif rho=N/D **une seule fois par sphère**. Les sites
et extrémités des boîtes sont traduits exactement par l'ancre, en entier,
avant conversion double. Tous les écarts de sites u32 tiennent exactement
en double ; stocker encore les extrémités globales u32 en double est sans
perte. Il n'est pas nécessaire de doubler la mémoire de toutes les boîtes.

Préparer des intervalles certifiés rho_i=[l_i,u_i] avec conversion entière
bornée, division et arrondis extérieurs. Pour les nouveaux entiers larges,
un `nextafter` autour d'une somme de limbes quelconque n'est pas une preuve
de conversion : fournir un convertisseur certifié (extraction des bits
dominants/reste ou borne d'erreur du Horner). Aucun overflow/nonfinite n'est
accepté comme certificat. Dans la micro-preuve, Python convertit directement
chaque entier, puis les opérations binary64 RN sont encadrées par nextafter ;
ce n'est pas une gate FENV ni CUDA.

Pour chaque site, produire [d_lo,d_hi] et pour l'ancre [R_lo,R_hi].

- Rejeter seulement si d_lo>R_hi.
- Accepter un intérieur strict seulement si d_hi<R_lo.
- Toute autre situation, contacts compris, passe au côté exact.

Pour une boîte relative [b_lo,b_hi], la distance minimale à **l'intervalle**
du centre est un minorant de la distance minimale au centre réel :
delta_i=max(0,b_lo_i−u_i,l_i−b_hi_i), puis somme delta_i² arrondie vers le bas.
Rejeter un nœud seulement lorsque ce minorant dépasse R_hi. Ainsi le coût
large n'est payé qu'aux sites ambiguës, pas à chaque boîte lointaine.

Pour nearest, garder la count-ième plus petite **borne supérieure** des
sites déjà visités, U. Alors le vrai niveau count-ième est ≤U ; un nœud ne
peut être rejeté que si son minorant est >U. Conserver les sites de borne
inférieure ≤U, puis trier exactement `(clé, ID)`, égalités incluses. La même
règle vaut au filtrage final de l'ancien tampon. La preuve suit directement
du fait qu'il existe count sites de distances vraies ≤ leurs bornes
supérieures ≤U ; elle ne suppose pas un classement approché correct.

`check.py` vérifie ces inclusions avec Fraction pour les fixtures, des boîtes
non ponctuelles, et le départage nearest. Les intervalles relatifs sont
invariants à translation entière commune : même N,D et mêmes écarts exacts,
donc mêmes opérations flottantes. Cela supprime la mauvaise amplification
par l'origine ; cela ne supprime pas l'erreur à grand rayon, encore encadrée.

Pour descente et saut dans tower : comparer des intervalles de rayons
disjoints, et séparer les bornes de distances de part et d'autre du rang K.
S'ils se chevauchent, conserver les comparaisons exactes existantes.
La proposition Welzl double reste une proposition de coût : ses heuristiques
ne nécessitent pas toutes une réécriture tant que le vérificateur est exact.

## 3. Catalogue : localiser les polynômes AVANT multiplication

Le catalogue n'utilise pas `.02` : dominance, droite des centres et appartenance
à boîte sont des tests entiers. `generator.cpp:96–102,122–143,496–558,650–654`
conserve cependant des normes/globales ou produits trop étroits pour les futurs
domaines. La translation après calcul d'une norme débordée ne répare rien.

Pour toute origine O commune, avec A'=A−O et Q'=Q−O :

  |A|²−|B|²−(lo+hi)·(A−B)
  =|A'|²−|B'|²−(lo'+hi')·(A'−B').

Les différences de distances et les P0/P1 du zonogone sont donc exactement
invariants. Recalculer `lx2` localement avant la feuille ; inutile de maintenir
`Ctx.X2` en i64 global en prétendant ensuite localiser le résultat. Les 324
contrôles exacts ici recouvrent cette identité, coins et P0/P1 compris.

### Dispatch natif réellement prouvé

Choisir O=Q.lo dans le repère T6 et certifier
L≥max(|X_i−Q.lo_i|,h_i) pour **tous les sites de la liste parentale** et les
trois axes. La petite largeur de Q ne suffit pas : des sites conservés peuvent
rester loin de Q. La borne peut provenir d'une enveloppe possédée de la liste
parentale, maintenue avec elle ; ne pas introduire un scan par paire.

Pour le filtre D-loc, les clés de réservoir sont ≤27L² ; les produits 2hX'
sont ≤2L², les normes ≤3L² et le membre droit ≤12L². Donc L≤2^29 garantit
chaque opération et la somme en i64 signé. Au-delà, ce même noyau a une voie
i128 ; ce n'est pas une raison d'utiliser plusieurs limbes partout. Le domaine
u32/T6 donne L≤2^38 pour les boîtes de cette construction.

Pour la droite des centres, |u_i|,|v_i|≤2L,
|P0|,|P1|≤18L², |c_ij|≤8L²,
|v_kP0−u_kP1|≤72L³ et RHS≤32L³.
Avec L≤2^38, ces dernières valeurs sont ≤2^121 environ, donc i128 signé
suffit sur **tout le futur domaine u32/T6** après promotions explicites de
P0/P1, normes et produits croisés. Aucun entier à 256/512 bits n'est requis
pour ce zonogone. Les intermédiaires de calcul des P0/P1 restent aussi bien
dans i128. Cela évite un widening aveugle de cette étape coûteuse.

Appartenance du centre : annuler l'origine avant produit,
(Q.lo_i−64a_i)D ≤64N_i <(Q.hi_i−64a_i)D,
équivalent à `center_in_box` avec les mêmes faces demi-ouvertes. Les membres
ont encore besoin des largeurs exactes du profil lorsqu'ils sont grands ;
un dispatch par **borne effective des coefficients et différences** peut
garder la voie i128 locale. Ne pas multiplier aD global puis espérer annuler.

Les midpoints q2 et la coupe binaire en i64 restent sûrs en u32/T6 ; le stockage
des coordonnées/boîtes n'est pas, à lui seul, un obstacle. Pour les côtés de
sites proches, préparer un tag de largeur de N,D et des écarts : leurs bornes
effectives autorisent la voie i128 ; les sites lointains sont d'abord éliminés
par les intervalles, non forcés à emprunter un calcul large.

## 4. Deux raccourcis flottants qu'on peut probablement conserver avec preuve

`tower.cpp:443–462`, orientation filtrée : l'hypothèse « w exact en double »
cesse en u32, mais la marge 16u·mag est assez large pour une nouvelle preuve.
Si w est exact en i128 puis converti avec erreur ≤u, cc exact en trois limbes
et converti par Horner positif avec erreur ≤gamma_3, chaque produit a erreur
≤gamma_5 et sa somme ajoute gamma_2. L'erreur relative au mag calculé est
≤gamma_7/((1−gamma_5)(1−gamma_2))<16u. Coefficients et produits restent finis,
sans sous-normaux pour ces entiers. Le même seuil est alors conservable,
avec cc calculé exactement **avant conversion**, et repli large à proximité
du plan. Ceci est une preuve conditionnelle de port, pas sa qualification.

`generator.cpp:779–786` et `tower.cpp:1758–1774` : les bandes de tri et la
fusion utilisent des marges **relatives** (2^-40 et 1e−9), contrairement à
`.02`. Avec num≤5 limbes/den≤4, Horner de puissances de deux puis division
donne une erreur relative <2^-48, largement sous ces seuils. Les valeurs
positives critiques de sites entiers distincts sont ≥1/4 et ≤3(2^32−1)²
pour les supports MEB admis : aucun overflow ou sous-normal double. Une
conversion générique de TOUS les nouveaux limbes et une preuve mise à jour
suffisent vraisemblablement ; le commentaire historique <2^-50 ne doit pas
être reconduit tel quel. Le comparateur exact et les réparations restent requis.

## 5. PGCD : optimisation possible, aucune dispense de largeur

Diviser tous les N_i,D par leur PGCD positif conserve exactement centre,
côtés/signes et rayon. Le tétra régulier a une réduction spectaculaire à
N_i=M,D=2. Mais les deux tétras stricts u24 de `gcd_controls` échouent encore :

- près du régulier, PGCD=2, num réduit 194 bits et den réduit 146 bits ;
- contrôle à PGCD=1, mêmes besoins 194/146, tous quatre barycentriques >0.

La réduction doit précéder les carrés et être faite sur des coefficients
déjà exacts ; elle ne récupère ni overflow préalable ni `resize(false)`
ignoré. Elle change aussi la représentation num/den publiée historiquement
par `emitted_level`, pas leur valeur : cette compatibilité doit être explicite.
Ne pas payer un PGCD systématique en espérant qu'il sauvera tous les cas.

## 6. Clôture et prochaine action

Lecteurs normal et −O : code0, **614 contrôles**, résultats identiques, zéro
échec. Empreintes des six sources dans les deux reçus. Le préflight réduit
`s=3299786` avait une mauvaise attente issue d'un calcul Python `**2` non
fidèle aux `x*x` du noyau : gap reproduit .015625, pas .03125 ; rejeté,
source et sorties FAIL normal/−O conservées séparément. Il n'est pas compté
comme contre-exemple du filtre. Les fixtures principales ci-dessus emploient
exactement `dx*dx+dy*dy+dz*dz`.

Prochaine action utile : un petit port isolé du centre/filtre relatifs avec
ces trois régressions causales et le départage exact, oracle indépendant,
sanitizers et marges certifiées. Puis dispatch catalogue i64/i128 sous garde
de L réel. Mesurer proportion de replis et coût total, pas seulement les
opérations évitées. Aucun besoin G4 établi pour ces verrous ; ni gain FULL,
ni sous-quadraticité globale, ni contrat100ms acquis ici.

SHA256 des sources :

- site_tree.cpp 32296818941943102cf7afbaef6665ffc80f5ca5564a274ad00ae25e8c096eec
- site_tree.hpp 3cb0a70d7a05a6665e0c59676eccbe1b3ac16097ed336fc0007a97ad6bbb8f35
- generator.cpp d5996feaf0df9e9ff274eeb6831b08b54b1a1ca4fd28c7fb81aeff9595dd8662
- tower.cpp d919bee049838ca9744218565afeb62b69e04c1a1437f42869a9f7d1fb2e39ae
- geometry.cpp 0e98cf5050c64e880386736de9aa47779fae377db0c9fe7acc64dfcf6c039de2
- geometry.hpp e5954da39ab6c216bfb05535158cf862eca9a4ba78a81f7dd0c33b6be1fed673
