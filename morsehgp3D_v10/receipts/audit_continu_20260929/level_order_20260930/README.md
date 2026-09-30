# Ordre exact des niveaux à précision élargie

30 septembre 2026. Le comparateur du futur profil u32 peut rester exact
avec huit mots de résultat de 64 bits, sous les bornes certifiées des niveaux.
Ce prototype CPU isolé ne porte pas le moteur et ne lève pas son refus u18.
`public_status=not_claimed`, hors registre. GCP non utilisé.

## Comparateur et résultats

Le [C++ figé](prototype/level_comparator.cpp) compare N_A/D_A et N_B/D_B,
avec numérateurs non négatifs de moins de 2^266 et dénominateurs strictement
positifs de moins de 2^200. Les deux domaines sont vérifiés avant multiplication.
Chaque produit croisé est inférieur à 2^466, donc tient dans huit mots.
Les produits partiels utilisent unsigned128 sans débordement : chaque somme
est au plus `(2^64−1)²+2*(2^64−1)=2^128−1`.
La retenue vers le neuvième mot est conservée et vérifiée, pas tronquée.

L'[oracle indépendant](independent/judge.py) compare exactement les deux
produits et le signe rationnel, pas seulement le résultat final.
Le panel comprend 2 444 requêtes, dont 2 172 comparaisons et 272 produits
bruts. Les 729 comparaisons géométriques sont **27² couples de niveaux**,
pas 729 supports : q2/q3/q4 réguliers et génériques jusqu'à u32, résolus
par Gram rationnel avec poids strictement positifs. Les formules de niveau
non réduit sont recoupées contre ce second calcul exact en Python.
Cela ne teste pas une construction native des niveaux.

Normal et UBSan produisent exactement les mêmes sorties. Les quatre
invocations natives — normale, UBSan et deux mutants — finissent code0 ;
huit jugements Python normal/`-O` donnent les codes attendus.

- Comparaisons : 279 égalités, 825 signes négatifs, 1 062 positifs et six
  refus de domaine, répartis entre les deux entrées.
- Produits bruts : 258 résultats exacts et 14 retenues hautes explicitement
  refusées. Ce mode admet 320×256 bits pour contrôler la capacité ; il ne
  change pas le domaine gardé du comparateur rationnel.
- Le mutant flottant perd une inégalité à la requête 3 ; les produits
  imprimés restent exacts. Le mutant qui oublie le mot haut accepte à tort
  un produit maximal à la requête 2 178. Refus numériques, pas crashes.

Le témoin flottant est une valeur rationnelle du domaine du protocole,
pas un rayon MEB u32 physiquement réalisé. Il prouve la nécessité de l'ordre
exact, sans ajouter de support géométrique natif au panel.

L'auteur du prototype a exécuté séparément un smoke UBSan de 27 contrôles,
soit une invocation native supplémentaire. Les contre-relectures du code
et du juge n'ont pas trouvé de défaut dans ce périmètre. Les rejugements
ne sont pas de nouveaux supports ou nouvelles exécutions du moteur.
La CLI peut allouer ; le noyau entier n'alloue pas et n'utilise pas de
flottants. Le protocole de sortie détaillé est un outil de preuve, pas
une représentation à stocker pour chaque comparaison en production.

## Obstacles du raccord réel

Huit sources du [snapshot bbc21eef7](independent/source_port/receipt.json)
sont copiées depuis Git et contrôlées identiques au worktree avant/après.
L'instanciation générique actuelle `Wide<5>*Wide<4>` est compilée isolément :
elle échoue bien sur l'assertion `Wide<9>` interdite. C'est un diagnostic
de port attendu, pas un échec de compilation du moteur u18.

Le port doit encore modifier les constructeurs de niveaux et leurs retours
de capacité, pas seulement le comparateur. `level_at_most` déduit aujourd'hui
d'un échec de réduction vers 192 bits que le seuil dépasse le numérateur.
Cette règle dépend de la borne du numérateur : avec N=2^200, D=2^193 et
seuil1, elle donnerait vrai après widening alors que N/D=128. Le
[témoin rationnel](independent/order_lemmas.py) exerce cette implication
conditionnelle ; il ne démontre pas un défaut actuel du profil u18 protégé.
Les distances entières, caches, échantillons de rang et exports doivent
suivre le domaine u32 ; le maximum de distance carrée demande 66 bits.

Le catalogue trie par approximations, répare les bandes, puis compare
exactement **tous les voisins** et refuse une inversion restante. Ce filet
ne répare pas une clé déjà tronquée ni toute approximation arbitraire.
Un NaN casserait l'ordre strict du tri avant le contrôle final ; les clés
valides et finies doivent être garanties avant cet étage.

La fusion de niveaux dans `point_dendrogram` utilise aussi une voie flottante
rapide, sans contrôle exact global après fusion. Sa table double regroupe
délibérément certains niveaux exacts distincts : c'est une vue métrique
avec perte d'événements intermédiaires, pas l'export exact des rangs FULL.
Le raccord précis doit préserver le rang exact de l'union des niveaux et
publier son approximation séparément.

Une voie certifiée sépare immédiatement deux intervalles disjoints et
utilise l'exact sinon. Pour grouper les cas ambigus, trier les bornes basses
et conserver le **maximum cumulé** des bornes hautes : ne pas assimiler
le recouvrement à une égalité de tri, car il n'est pas transitif. Le modèle
Fraction vérifie 2 051 intervalles, 33 fixtures et 1 805 séparations certifiées,
normal/`-O`. Ce modèle ne teste pas des convertisseurs flottants natifs.
Le tri d'un flux de M événements coûte O(M log M) ; cela ne borne pas M
en fonction du nombre de points et ne prouve pas le sous-quadratique global.

## Provenance et limites

Les sources/protocole du prototype sont gelés avant compilation. Les
binaires, hachés mais externes, sont déplacés après les captures à hashes
identiques ; les chemins enregistrés décrivent l'exécution réelle passée.
Les dépendances système ne sont pas embarquées. Ce reçu rejoue les sorties
archivées, pas une preuve LIVE de recompilation ou du moteur.

Le lecteur vérifie les fichiers avant d'importer les modules archivés,
exige les ensembles d'empreintes non vides, régénère le panel, rejoue les
huit jugements et les lemmes, puis confirme les deux refus causaux.
Il relie aussi les cinq compilations archivées à leurs modes et commandes,
le smoke à ses flux, et le refus `Wide<9>` à la commande du snapshot.
Ce contrôle de provenance n'exécute ni compilateur ni binaire historique.

```bash
python3 -B morsehgp3D_v10/receipts/audit_continu_20260929/level_order_20260930/verify.py
python3 -B -O morsehgp3D_v10/receipts/audit_continu_20260929/level_order_20260930/verify.py
```

Aucun nouveau constructeur q3/q4, tri natif complet, support/famille,
nearest, FULL, clustering, parallélisation ou GPU n'est qualifié ici.
Aucun chrono, test 8k/16k/32k ou contrat 100 ms n'est acquis.
Les sources du moteur et le registre formel restent inchangés.
