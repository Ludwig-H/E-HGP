# Une réduction q2 sur les nappes 2D, jusqu'à n32k

13 septembre 2026, réponse à la reprise constructeur après `3589a2c9`.
Cadre : `exploration_v8_hors_registre`, `cpu_reference`,
`quantized_u16_input_only`, `audit_independant_math_and_architecture`,
`not_claimed`. Prototype d'audit autonome, pas un nouveau moteur produit.

## Résultat utile

La limitation u16 des rangées 1D ne bloque pas leur extension aux nappes.
Un découpage par lignes et colonnes, avec les **vrais certificats de boîtes
q2 déjà présents**, réduit les 256 millions de paires de la nappe n32k à
1 865 300 candidates pour h10. Il construit 530 706 descripteurs en flux,
dont 358 506 rejetés après certification ; 3 585 060 visites de témoins
sont payées. Le coût du census de ces candidates reste à mesurer.

La famille est celle de `sheet` : m=n/2 sites dans chaque plan x=1000 et
x=60000 ; largeur r=⌈√m⌉, parcours y puis z à partir de 1000, dernière
ligne éventuellement incomplète. Aucun ajout de sites pour compléter le
carré. Les deux facteurs portent les mêmes coordonnées transverses.
La séparation s8/10/12 est vérifiée aux trois tailles demandées ; ces
valeurs de s ne changent pas la fixture et ne comparent pas trois WSPD.

## Certificat et couverture

Fixer une ancre a=A(i,j), h=Kmax pour q2 et w=⌈h/2⌉. Partitionner B
en deux queues de lignes u<i−w et u>i+w, puis les lignes intermédiaires.
Dans chaque queue éloignée, les w lignes immédiatement voisines d'i du
bon côté fournissent chacune deux sites A(k,j), B(k,j). Ces 2w IDs sont
distincts, strictement entre les rangs des extrémités, et aucun n'est
une extrémité du sous-produit. Pour chacun, le prédicat vaut :

$$H=(k-i)(u-k)>0.$$

La coordonnée z de b n'intervient pas. Les lignes intermédiaires k sont
complètes lorsqu'une queue éloignée existe : la dernière ligne tronquée
ne fait donc pas disparaître ces témoins.

Pour chaque ligne proche u, partager ses colonnes en une queue gauche,
un intervalle central et une queue droite. Lorsque les lignes i et u
sont complètes, ou que i=u, garder |v−j|≤w. Chaque queue est certifiée
par les w sites de chaque côté A(i,k), B(u,k) strictement intermédiaires :

$$H=(k-j)(v-k)>0.$$

Si l'une des deux lignes est incomplète et i≠u, garder plutôt |v−j|≤h
et prendre h témoins dans la ligne complète. Cette précaution rend la
construction valide sur les **préfixes de grilles effectivement mesurés**.
Elle ne traite pas un ID absent comme un témoin.

Chaque proposition appelle `classify_witness_block(Q2,U,V,Z)` avec U
singleton, V la vraie boîte de la queue et Z celle des IDs proposés.
Leur boîte conserve H>0 sur cette famille, y compris lorsqu'elle relie
les deux plans. **Seul `Credit` élimine la queue** ; les autres décisions
la conservent intégralement. Le juge perturbe aussi les coordonnées tout
en conservant les rangs : les replis deviennent nécessaires et restent
sûrs. Un mutant qui remplace la certification par les seuls rangs crédite
un site non intérieur et est réfuté.

Les queues et intervalles sont disjoints, couvrent B pour chaque ancre
et gardent les IDs originaux. Le rectangle initial reste propriétaire
des paires ; aucun nouveau constructeur WSPD n'est supposé. On ne somme
pas des crédits venant de sous-produits différents : chaque rejet porte
sur sa propre population d'IDs certifiée.

## Travail payé et limites

Les vraies boîtes préfixes/suffixes de B et de chacune de ses lignes se
préparent ensemble en deux passages, 2m visites, avec O(m) mémoire. Une
queue s'interroge ensuite en O(1), sans rescanner ses points. Le prototype
visite aussi les 2m coordonnées pour les boîtes de séparation. Pour une
entrée générique, validation d'unicité et création du rangement restent
à payer ; ici le rangement est la recette déterministe de la fixture.

Par ancre : au plus 2+3(2w+1) descripteurs, O(h²) IDs proposés et O(h)
tests de boîtes. Le prototype vérifie leur unicité par petits tris :
O(mh² log(h+1)) au total, plus O(m) de préparation, mémoire O(m+h).
Avec h≤10, aucun terme quadratique en m n'est déplacé dans les queues.
Le résidu est borné par m(2w+1)(2h+1), soit O(mh²). La borne ne vaut
que pour les nappes alignées ; les replis d'autres géométries peuvent
conserver m² paires. Aucun catalogue de cellules, cofaces ou incidences
Delaunay n'est construit.

| n | h5 : candidates | h10 : candidates | Produit initial |
| ---: | ---: | ---: | ---: |
| 8 000 | 186 226 | 445 940 | 16 000 000 |
| 16 000 | 379 010 | 917 580 | 64 000 000 |
| 32 000 | 765 838 | 1 865 300 | 256 000 000 |

Le [prototype C++](sheet_rectangles_probe.cpp) utilise les deux headers
géométriques de `3589a2c9`, extraits depuis Git dans un répertoire neuf.
Le [runner](sheet_rectangles_checks.py) conserve leurs hashes dans le
[reçu](SHEET_RECTANGLES_CHECKS.json). Chaque mode normal/−O passe 42 cas
avec C++20 strict et UBSan : 24 petits cas avec couverture, chaque témoin
crédité et census indépendant, puis 18 constructions n8k/16k/32k. Le juge
petit effectue 3 533 502 tests de points par mode ; les grandes instances
ne sont **ni développées ni soumises au census**. Aucun temps n'est publié.

Cette extension complète les
[queues 1D de l'autre auditeur](../../morsehgp3D_v8/audits/P0_SOUS_RECTANGLES_ET_GROUPES.md)
et répond au verrou de taille de la fixture. Elle donne un proposeur
simple à comparer à l'arbre de boîtes général, pas une raison d'imposer
un moteur spécialisé aux nappes. Partager des ancres voisines pourrait
réduire les descripteurs ; ce gain n'est pas mesuré ici. Le choix général
des découpages, q3/q4, le census aval et la tour FULL restent ouverts.
GCP non utilisé.
