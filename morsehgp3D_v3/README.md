# MorseHGP3D v3

État : **M1 (le juge) et M2.1 (un falsificateur borné)**. Il n'y a pas de v3, et
il ne doit pas y en avoir avant que le §2 ait été tranché.

L'autorité mathématique reste `docs/SPECIFICATION_MORSEHGP3D.md` et
`docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`. Les audits de
[`audits/`](audits/) motivent les corrections ; ils ne certifient rien. Aucun
statut public, aucun SLO n'est ouvert.

---

## 1. Ce qui est établi, et par quelle mesure

**Le générateur de la v2 est condamné**, pas lent. Son voisinage est dimensionné
par une borne *a priori* (relaxation conique du théorème 4) qui vaut $+\infty$
dès qu'un cône est trop pauvre : $\lvert W_p\rvert = n-1$ à tout $K$, mesuré, et
un coût en $\Theta(n^5)$. Même le théorème 4 exact ($\theta=0$) demanderait encore
175 voisins à $n=50\,000$, soit $4{,}4\cdot10^{10}$ quadruples.

**Le rang est une profondeur d'arrangement.** C'est l'invariant qui a survécu à
tous les audits, sous ses deux formes : ancré par arête, dans le plan médiateur,
$\mathrm{rang}=4+c_e+\delta_e(t)$ avec $Z_e\leq m_e(\kappa_e+1)$ ; ancré par
point, dans le dual inversif, $\mathrm{rang}=(4-j)+\mathrm{profondeur}$ sur une
face de dimension $j$. C'est ce qui permet de calculer le rang **pendant** la
génération au lieu de l'interroger après.

**Il n'existe aucune règle d'arrêt valide fondée sur les seules distances.** Un
certificat de localité a été proposé, puis **réfuté par le juge** : un support
inconnu employant un point exclu vérifie précisément $2r\geq d_{M+1}$, donc le
maximum des rayons déjà trouvés ne le borne pas. La complétude d'un générateur
ancré exige soit l'**exhaustivité**, soit un majorant de $R(p)$ — fini
($R\leq\mathrm{diam}(X)$) mais qu'il reste à calculer.

**Mesure honnête de la fenêtre qui aurait suffi** (régime exhaustif,
$s_{\max}=11$, calculée *a posteriori* depuis le vrai $r_{\max}$) :

| $n$ | p50 | p95 | max | sphères/point |
| ---: | ---: | ---: | ---: | ---: |
| 100 | 53 | 77 | 96 | 167,7 |
| 150 | 64 | 97 | 124 | 190,9 |
| 200 | 73 | 125 | 141 | 210,3 |

Elle **croît encore** : rien n'en est extrapolé.

**Le coût d'un parcours sensible à la sortie est mesuré, et il est constant.**
En comptant les **strates** — candidats affinement indépendants dont le point
canonique a une profondeur $\leq s_{\max}-m$, c'est-à-dire ce qu'un parcours
devrait visiter — contre ce qui est réellement émis (régime exhaustif,
$s_{\max}=11$) :

| $n$ | strates | bien centrées | émises | **strates / émise** | candidats / émise |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 60 | 108 960 | 22,2 % | 7 520 | **14,49** | 273 |
| 100 | 232 544 | 23,5 % | 16 767 | **13,87** | 965 |
| 150 | 398 999 | 23,5 % | 28 637 | **13,93** | 2 889 |

Un parcours visiterait $\approx14$ strates par sphère émise, **constant en $n$**,
là où la cascade en visite 273, 965 puis 2 889 — **croissant linéairement**. Le
gain attendu est donc $\approx19\times$ à $n=60$, $70\times$ à 100 et
$208\times$ à 150. C'est l'argument chiffré pour construire le constructeur de
strates, et c'est la cible que PEL-2 doit atteindre.

---

## 2. La voie la plus pertinente pour la v3

> **Construire le sous-complexe shallow stratifié, et y lire le rang comme une
> profondeur.** Tout le reste en découle, et rien d'autre n'a survécu aux audits.

### Ce que cela veut dire précisément

L'objet **n'est pas** $V_k(p)$ vu comme un sous-ensemble de $\mathbb{R}^3$ :
pris comme ensemble, il efface les hyperplans intérieurs séparant deux cellules
toutes deux de profondeur $\leq k$. Contre-exemple minimal : $X=\lbrace p,u\rbrace$
et $k=1$ donnent $V_1(p)=\mathbb{R}^3$, dont la frontière n'a aucune 2-face —
alors que le plan médiateur $H_u$ existe et que son milieu porte la sphère
critique de support $\lbrace p,u\rbrace$.

L'objet est le **sous-complexe stratifié** : les faces de l'**arrangement** dont
la profondeur est $\leq k$, chacune avec sa dimension, et les quatre arités
traitées **séparément** (une preuve d'arité quatre ne se propage jamais à
l'arité trois).

### Les deux ancrages, et ce qui les sépare

| | ancré par **arête** (A2e) | ancré par **point** (A2p) |
| --- | --- | --- |
| dimension de l'arrangement | 2 | 3 |
| complétude de l'ancrage | **conditionnelle** à une source complète de paires diamétrales | **par construction** |
| borne de sortie | $Z_e\leq m_e(\kappa_e+1)$, classique | $\Theta(k^2)$ par point, classique |
| coût du prédicat exact | plus faible | plus élevé |

A2e est le cœur algorithmique ; sa complétude est otage de **A1-source**, que le
RNG d'ordre borné ne peut pas fournir (théorème négatif du dépôt). A2p n'a pas ce
problème et pourrait fournir ces ancres — c'est l'hypothèse **A2pe** — mais elle
n'est pas démontrée.

### Ce qui tranche, et rien d'autre

| obligation | ce qu'elle décide |
| --- | --- |
| **PEL-1** | les 2-faces de l'arrangement sont-elles exactement les arêtes utiles ? Si oui, A1-source disparaît. |
| **PEL-2** | le parcours est-il en $O(\text{sortie})$, et non $O(\text{sortie}\times m)$ ? |
| **PEL-3** | traitement exact des strates non bornées (l'énoncé actuel est probablement faux). |
| **PEL-4** | que coûte le prédicat exact en 3D contre 2D ? C'est l'arbitrage A2pe / A2e. |

Le prochain artefact décisif est donc **un constructeur exact du sous-complexe
stratifié, comparé exhaustivement à petit $n$** — pas un pipeline, pas de CUDA,
pas de réducteur. Écrire un pipeline avant de savoir si le parcours est sensible
à la sortie, ce serait refaire exactement l'erreur de la v2 : construire un
substitut, puis mesurer.

### Ce que la v3 ne fera pas

Aucune mosaïque de Delaunay d'ordre supérieur, aucun $\Gamma$ global, aucune
matrice paire–point, aucun catalogue géométrique matérialisé. En revanche le tri
global exact et le groupement des niveaux égaux sont **inévitables** : des ancres
indépendantes n'émettent pas en ordre monotone, et le réducteur exige un lot
atomique par niveau rationnel.

Le détail, les budgets et le journal des affirmations retirées sont dans
[`PROPOSITION.md`](PROPOSITION.md) ; le plan est à son §13.

---

## 3. M1 — le juge

Indépendant du chemin jugé sur les trois couches qui comptent :

| couche | choix | pourquoi |
| --- | --- | --- |
| arithmétique | signe-magnitude, chiffres de 32 bits, précision arbitraire | représentation *différente* du complément à deux de largeur fixe de la production |
| géométrie | élimination de **Gauss** | jamais les formules de Cramer du chemin jugé |
| structure | forêt reconstruite **depuis $\Gamma_k$** | tous les $k$- et $(k+1)$-sous-ensembles, jamais le catalogue jugé |

Ni division entière ni PGCD : les rationnels ne sont pas normalisés, la division
est une multiplication croisée et la comparaison un produit croisé — la partie
risquée d'un grand entier n'existe pas ici. Validé contre `__int128` **et** GMP ;
sans témoin large, le selftest **échoue** au lieu de rendre `OK`.

**Ce qu'il compare** : cardinal, doublons de support, rang, membres, tranche
triée, **niveau et centre rationnels exacts** ; par ordre, genre, arité, racines,
nombre canonique de nœuds, généalogie, les deux représentations d'adjacence
confrontées l'une à l'autre, tous les compteurs publics — et la **participation
effective** de la sphère source d'une multifusion à son lot.

**Fermeture** : `attempted = decided + rejected_domain`, planchers strictement
positifs, arguments absurdes refusés (code 2), censure inattendue = échec, garde
de domaine symétrique, lecture **hostile** et **atomique** du sujet.

**Campagne négative, fail-closed** : six fautes injectées sur une **copie d'un
sujet déjà vert**, avec une comptabilité distincte de la campagne positive — un
plancher ou un rejet de domaine ne peut donc pas tenir lieu de preuve. Chaque
faute doit être **appliquée au moins une fois** et déclencher **exactement une
fois son garde**, sans aucun diagnostic étranger : membres non triés, numérateur
tourné (même norme, donc même niveau), sentinelle invalide, `n_children` nul,
racine supprimée, et source de fusion étrangère **de même rang et de même niveau
exact** — sans cette égalité, c'est le garde de niveau qui rougirait et le garde
de contribution ne serait pas exercé.

Résultat du 8 août, grille déclarée $[0,65535]$ : `attempted=40 decided=40
rejected_domain=0 | spheres=1850 forets=82 noeuds=1909 | largeur max=158 bits`.
Reçus dans [`receipts/`](receipts/).

Deux faits produits par ce juge : il a trouvé que les tranches `I ∪ U` n'étaient
pas triées alors que le contrat l'exige (corrigé en v2), et les niveaux exacts
atteignent **158 bits** sur cette grille — donc au-delà de `__int128`, ce qui est
la raison pour laquelle la porte précédente y décidait *zéro nuage sur quarante*
en annonçant `OK`.

**Ouvert** : différentiel `Rational` contre `mpq_class`, compteurs de
vérifications réellement exécutées, provenance complète du reçu (digests des
nuages — la graine seule n'est pas un format portable), et le profil
`exact_dyadic_input`.

---

## 4. M2.1 — falsificateur borné, pas prototype

Générateur ancré par point qui énumère tous les supports de taille $\leq4$ dans
sa fenêtre. **C'est la cascade locale que le §1 condamne** : il sert de sujet
différentiel et de mesure du travail payé, jamais de voie produit, et il ne
construit ni arrangement, ni complexe stratifié, ni peeling.

Deux régimes nommés, jamais confondus : `exhaustive` — la seule complétude
disponible — et `assumed_window` — hypothèse **déclarée**, jugée séparément et
non qualifiante.

---

## 5. Construire et exécuter

```sh
cmake -S morsehgp3D_v3 -B build/v3 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v3 -j
cd build/v3 && ctest --output-on-failure     # 14 tests

./mhgp3v_arith_selftest 20000                        # __int128 et GMP
./mhgp3v_oracle --clouds 40 --seed 4242 --min-points 8 --max-points 11 \
                --max-order 3 --min-decided 30 --min-nodes 500 \
                --receipt receipts/oracle_campaign.json
./mhgp3v_oracle --subject anchored --regime exhaustive --clouds 8 --seed 90210 \
                --min-points 9 --max-points 12 --max-order 3 \
                --min-decided 6 --min-nodes 60
./mhgp3v_oracle --inject merge_source_foreign --clouds 4 --seed 4242 \
                --min-points 8 --max-points 10 --max-order 2 \
                --min-decided 1 --min-nodes 1        # doit ROUGIR
```

GMP n'est pas une dépendance de l'oracle : il n'intervient que comme second
témoin de la validation arithmétique.

`census_tukey_shallow.py` produit un reçu complet (provenance, digests, jeu de
directions, convention de demi-espace, identité de campagne). Il mesure un
minorant de l'ensemble où la borne tangente **non contrainte** de la v2 vaut
$+\infty$, et **rien d'autre** : l'ensemble où la borne à centre convexe échoue
est vide, puisque $R\leq\mathrm{diam}(X)$. Nuages : Stanford bunny, reconstruction
fusionnée et **dix captations brutes recalées** — le cas multi-captation que la
proposition doit traiter. Les données ne sont pas versionnées ; le reçu porte
leur origine et leur digest.
