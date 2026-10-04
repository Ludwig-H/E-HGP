# Euler à K+2 et restriction J1 — contrelecture mathématique

**Favorable : aucun défaut matériel trouvé dans les mécanismes relus.** Source WIP du snapshot **4 octobre 2026, 14:50:18.534697 UTC**, base `66372e621dcee58daaa7d7309875ab157894acf4`, acteur alors `81e32cd7019f666aa5bf1f768ef29d6ee6fbecb1`. `SOURCE.json` ancre onze copies relatives (dix initiales, puis la dépendance feuille du même snapshot) ainsi que le hash du manifeste de snapshot. Les deux fichiers du juge `catalogue_euler.cpp/.hpp` sont ensuite rapprochés du commit publié **77db5738eb2dd5bc84ecdc4d85ade833124c58f8** : octets strictement identiques. Ce rapprochement de sources ne transfère aucune qualification native, ni le reste de ce commit. Aucun code produit/notes modifié, aucun natif, build, fit ou G4 exécuté.

## J3 : pourquoi la formule et K+2 sont corrects

Pour `1<=k<=n`, la somme sur les parties non vides F de taille au moins k,
`sum_F (-1)^(|F|-k) C(|F|-1,k-1)`, vaut 1 par l'identité binomiale. Regrouper F selon sa MEB exacte b. Les singletons contribuent `n[k=1]`. Pour une boule de rayon positif, F a cette MEB si et seulement si `F=A∪J`, où `A⊆U`, `c∈conv(A)` et `J⊆I`. La condition porte sur les sites actifs de coquille : des intérieurs stricts ne certifient pas le centre d'une MEB de rayon inchangé.

Pour `s=|A|`, la somme sur J est
`sum_(j=0..p, j+s>=k) (-1)^(j+s-k) C(p,j) C(j+s-1,k-1)`.
La différence finie donne zéro si `k<=p` ou `k>p+s`, et sinon
`(-1)^(s-(k-p)) C(s-1,k-p-1)`. C'est exactement [add_contributions](source/morsehgp3D_v11/bench/catalogue_euler.hpp#L94). Une coquille régulière `m=qmin` ne possède que A=U contenant le centre, d'où la formule régulière.

Un terme non nul impose `p<=k-1`. En dimension 3, `qmin<=4`, donc `p+qmin<=k+3<=K+3`, admission de `Cat_(K+2)`. Tous les ordres `1..min(K,n)` sont ainsi vérifiables ; les ordres K+1/K+2 ne le sont pas en général. Le modèle réalise la nécessité de cette marge : tétraèdre régulier `(0,0,0),(8,8,0),(8,0,8),(0,8,8)` et intérieur `(4,4,4)`. À k=2, la boule de qmin4/p1 contribue −1 et manque de Cat₃ ; sa somme Euler vaut 2, contre 1 avec Cat₄.

Pour une partie A de coquille, `c∈conv(A)` équivaut à contenir un support minimal de cardinal 2..4, affinement indépendant et de poids strictement positifs. Carathéodory puis réduction d'un support minimal justifient la paire antipodale, le triangle strictement aigu dont le plan contient c, ou le tétraèdre contenant strictement c. Les marques de [mark_supports](source/morsehgp3D_v11/bench/catalogue_euler.hpp#L137), puis leur fermeture en OU, sont donc exactes, y compris quand plusieurs arités de supports minimaux existent sur la même coquille. Le premier support par cardinal/ordre des SiteIdx fournit bien qmin puis S*.

## Refus, bornes et rangs

[euler_totals](source/morsehgp3D_v11/bench/catalogue_euler.hpp#L296) inspecte d'abord les cardinalités des coquilles étendues non omises. Une coquille >24 produit un refus avant allocation du brouillon, tâches et calcul des contributions. Cette pré-passe reste du travail et les deux catalogues ont déjà été construits : « avant calcul » concerne le **juge**, pas tout le banc. Aucune somme partielle n'est alors publiée. À m=24, les 2²⁴ bits occupent 2 Mio par worker ; les brouillons et `Totals` simultanés sont admis ensemble avant leurs allocations.

Les marqueurs `1u<<i` gardent `i<=23`. Le comptage par cardinal tient en u64. Une contribution absolue est au plus `2^(2m)` ; moins de 2³² boules et m≤24 donnent des sommes de magnitude <2⁸⁰, très loin de i128 signé. Les comptes de supports sont bornés par `2^32*(C(24,2)+C(24,3)+C(24,4))<2^64` ; les autres réductions sont également bornées par le nombre de boules et la taille des populations effectivement jugées. Ces preuves supposent un `Catalogue` valide construit sous ses contrats et le `Pool` borné existant ; elles ne sont pas une qualification d'objets forgés ou de l'exécution concurrente.

**J1** est la restriction définitionnelle au même Cloud/sites unitaires : filtrer `p+qmin<=K+1` dans Catₖ′ redonne Catₖ. La [jointure](source/morsehgp3D_v11/bench/catalogue_euler.hpp#L421) compare niveau exact/S*, puis les champs et listes ; elle renumérote les seuls niveaux retenus. Le brut doit être celui du premier support **filtré**, pas nécessairement celui du grand catalogue. Au niveau 25 de compensation5, le triangle premier dans le grand catalogue donne `409600/16384` ; la paire conservée à K1 donne `100/4`. Même rationnel, octets bruts différents. [Les lignes 463–471](source/morsehgp3D_v11/bench/catalogue_euler.hpp#L463) recalculent bien depuis S* lorsque le premier global était filtré. Les rangs denses/ordre du grand catalogue sont vérifiés ; avec des omissions de harnais, le contrôle des anciennes tables est volontairement désactivé, ce que la documentation annonce.

## Indépendance et limites des portes

Le juge natif appelle les primitives `num` du produit pour reconstruire les sphères et les supports : cette voie n'est pas un deuxième noyau numérique indépendant. [euler_oracle.py](source/morsehgp3D_v11/tests/catalogue/euler_oracle.py#L75) recoupe néanmoins le résultat sur petits nuages avec Gram/Gauss Fraction, énumération exhaustive des supports et coquille globale, sans C++ ni import R2. Pour les coquilles étendues de taille ≤8, il recoupe en plus la fermeture de supports positifs avec une formulation à poids faibles. Les six K, les sommes aux ordres non jugés, parts régulières/étendues, recomptages bruts et voies séquentielle/production font l'objet d'attendus explicites. Ce sont des obligations de qualification à jouer, pas des résultats natifs acquis par nous.

[euler_limits.py](source/morsehgp3D_v11/tests/catalogue/euler_limits.py#L96) grave les limites pertinentes. Compensation5 perd ensemble une paire −1 et un triangle +1 à K1 tout en conservant Euler/J1 ; à K2, l'écart apparaît. Les géométries D/T13 et D/T23 ont effectivement p=4/p=9, coquilles régulières 2/3, et annulent toutes leurs contributions jusqu'aux ordres 5/10. Les limites 24/25, omissions d'un seul catalogue, refus et planchers ont des attendus. Les neuf mutations Euler sont bien dirigées contre omission commune, défaut K-dépendant/J1, contributions, fermeture des mots hauts, supports q4, nombre d'ordres et terme singleton. Leurs ancres sont présentes et uniques ; compilation et mort natives restent hors de ce contrôle.

Euler/J1 ne certifient jamais le catalogue ou FULL : compensations, omissions communes de contribution nulle, erreurs numériques partagées et listes I/U incomplètes restent possibles. Le juge ne balaie pas le Cloud global pour vérifier cette complétude. Ces limites sont **déjà déclarées** dans le snapshot ; ce reçu ne les présente pas comme de nouveaux défauts.

## Contrôle autonome et fermeture

`check.py` n'importe aucun produit ou référence : il calcule les MEB de toutes les parties sur huit nuages de 1..6 sites, puis regroupe directement les poids binomiaux selon leur MEB. **3 623 gardes**, 194 parties non vides / 83 boules critiques ; égalité avec J3 par boule et par ordre, J1 et Catₖ₊₂, fermeture booléenne empaquetée recoupée par le prédicat de sur-ensemble jusqu'à 10 sites. D/T13 et D/T23 sont vérifiés seulement pour leurs deux boules témoins et leurs contributions régulières, sans campagne exhaustive de leurs parties. Les gardes m24/25 sont scalaires : pas une preuve d'allocation native ni un replay des grandes coquilles.

```
python3 -B -S check.py > normal.json
python3 -B -O -S check.py > optimized.json
cmp normal.json optimized.json
sha256sum -c SHA256SUMS
```

Résultats normal/−O identiques. `EXPECTED.json` grave statut, comptes et témoins ; `AFTER.json` revérifie sources figées et ancres ciblées. `SHA256SUMS` inclut chaque payload, uniquement lui-même est exclu. Pas de nouveau chronomètre, statut public, test FULL ou transfert de qualification vers une évolution LIVE.
