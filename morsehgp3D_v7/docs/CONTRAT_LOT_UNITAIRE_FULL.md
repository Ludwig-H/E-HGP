# FULL : regrouper un lot à une seule connexion directe

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

## Changement borné

Le [producteur horizontal](CONTRAT_PRODUCTEUR_FULL_GABRIEL.md) spécialise
le cas où un niveau contient exactement une coface de connexion Gabriel.
Il ne suppose pas qu'il n'y a aucun minimum simultané. Les lots à zéro
ou plusieurs connexions gardent le regroupement général.

Le support essentiel de cette directe possède q éléments, avec 2≤q≤4
d'après la grammaire déjà contrôlée. Ses q demandes de facettes strictes
retournent des identités de racines pré-lot. Le code historique les unissait
toutes à la première dans une DSU locale. Son unique classe non vide est
donc exactement l'ensemble de ces identités. Un tableau de quatre tokens,
trié et dédupliqué après la dernière demande, donne les mêmes parents.
La [contrelecture statique indépendante](../audits/CACHE_FULL_COURANT.md#lot-contenant-une-seule-directe--avis-statique)
précède l'implémentation ; elle ne vaut pas qualification du nouveau binaire.

Avec U racines distinctes, U=1 n'émet aucun nœud ; U≥2 émet une seule
multifusion à U parents. Ni le nombre de points couverts ni leur égalité
ne servent à identifier les racines. Aucun Gamma exhaustif, aucune cellule
de Delaunay d'ordre supérieur ni incidence supplémentaire n'est construit.

## Calendrier et budgets conservés

Toutes les q demandes sont exécutées dans l'ordre du support, même si des
racines se répètent. Les facturations et les décisions first-C restent
dans le même ordre. Dédupliquer les demandes avant leur résolution serait
un autre algorithme et changerait les frontières de refus.

Le premier token rendu est conservé avant tri. Les minima simultanés sont
installés après les demandes strictes et avant l'éventuelle multifusion.
Le suffixe commun ferme le lot, normalise l'ancre de la directe, conserve
les connexions sans fusion et installe les alias eager si cette politique
est choisie. Le cache lazy et la résolution J=1 ne changent pas.

Le plafond des parents est chargé sur U, seulement si U≥2, avant la
construction de la ligne publiée. Les plafonds de faces, portails, MEB,
supports, requêtes spatiales, successeurs et sortie ne sont pas relevés.
Sans faute mémoire, les forêts et tous les compteurs logiques doivent
coïncider avec le regroupement général, y compris sur les préfixes refusés.
Cette égalité oppose les deux regroupements **au même calendrier de
normalisation**. Le [delta suivant v2](CONTRAT_NORMALISATION_FULL.md)
versionne séparément les opérations de successeurs ; les preuves
historiques singleton ci-dessous ne lui sont pas réattribuées.

Les allocations du dictionnaire local, de ses vecteurs DSU et de ses
groupes intermédiaires disparaissent dans cette branche. Les allocations
des minima, du cache, des parents publiés et de la sortie subsistent.
Les ordinaux d'allocation ne sont pas conservés : chaque site restant doit
être soumis à un nouveau balayage de pannes persistantes. Une panne rend
toujours un refus global nommé et un certificat invalide aux arènes vides.

## Qualification du delta

Le header `21b77d29…` passe la [qualification fraîche ciblée](../receipts/full_gabriel_singleton_20260905/README.md) :
17/17 CTests Release et 17/17 ASan/UBSan, avec LeakSanitizer actif.
Les sept binaires par build sont liés à une carte stable de 584 pins,
dont les 521 headers Boost de l'oracle du digest. Cela ne qualifie pas
les 402 tests enregistrés ni toute la suite F. Le témoin antérieur
`13c6cc72…`, livré par `6126b373`, reste séparé.

Une porte propre au delta compare la branche spécialisée au chemin général
conservé, puis à l'oracle Gamma indépendant borné. Le contrôle du choix de
branche et ses compteurs d'observation n'existent que sous `MHGP7_TESTING` ;
aucune option cachée n'est ajoutée à la CLI ou à l'API produit. Le chemin
général est un différentiel, pas un oracle géométrique.

Par build, le positif ferme 181 paires et 3 320 coupes sur neuf nuages ;
472 lots spécialisés sont réellement exécutés, dont 232 q2, 192 q3 et
48 q4. Les planchers incluent 24 lots à quatre parents, 216 lots à racines
répétées, quatre avec minima simultanés, 44 lots multi-directes et un
no-op consommé ultérieurement. Les permutations et K=n sont séparés.
Le mode budgets ferme 766 paires, dont 357 refus identiques : 208 caps
exacts, 312 caps courts, 36 préfixes de demandes et quatre cas de budget
parents avec racines répétées. Les deux politiques d'alias et les caches
nul/saturé/non saturé sont comparés ; les 33 champs logiques sont vérifiés
explicitement, sans comparaison des octets de padding.

Les anciennes portes sont réexécutées sur le nouveau programme. Le
balayage frais trouve 49 allocations eager et 209 lazy (six cellules),
contre 102 et 434 dans leurs témoins antérieurs : toutes les pannes
persistantes sont refusées sans échappement ni forêt partielle. Ces nombres
portent sur les fixtures, pas sur un comptage des allocations à 8k.
La mesure mono avant/après reste une preuve séparée des tests fonctionnels.

Le [rejeu indépendant du nouveau header](../audits/receipts_full_singleton_20260905/README.md)
ajoute sa propre qualification O2 et ASan/UBSan, sans macro de test :
114 ordres, 912 sorties et 69 120 coupes par build. Les 872 sorties du
corpus historique sont réexécutées et identiques à l'ancien programme ;
cinq ordres supplémentaires exercent une naissance simultanée vérifiée
rationnellement. Le mutant perdant le quatrième token est réfuté.
Ces nombres ne s'ajoutent pas à nos planchers comme s'ils venaient de
notre porte, et les anciens mutants lazy ne sont pas réattribués au delta.

Ce changement ne résout pas le refus 32k du budget de successeurs puisque
ce travail logique reste identique. Il ne livre ni verticale ni archive
FULL intégrée. Les contrats 50k/tour entière sous 1 s, puis 100 ms, et les
dizaines de millions sur G4 restent ouverts. GCP non utilisé.
