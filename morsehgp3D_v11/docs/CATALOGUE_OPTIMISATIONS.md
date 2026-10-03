# Tri indirect et cache J2 : ablation exacte

Tranche du 2 octobre 2026, CPU u18/u21/u24, `not_claimed`. Options désactivées
par défaut ; qualification native et mesures G4 propres à cette tranche requises.
La capture [parallel5](../receipts/catalogue_parallel_20261002/README.md), à
`c1046dfc7`, motive deux changements séparés. Sur les trois LiDAR, K5/W48,
le tri coûte environ0,9–1,4s. Sur08/0/u21, les deux phases parallèles valent
1,533 et1,529s, avec une tâche maximale1,527 et1,525s : le déséquilibre reste
un verrou distinct. Ces temps ne qualifient ni ces options ni FULL.

## Permutation exacte des émissions

`indirect_sort=true` trie les indices u32 des émissions, puis l'assemblage
lit les émissions selon cette permutation. Les émissions et leurs populations
restent immobiles. Ordre total : niveau rationnel exact, support canonique,
ordinal initial ; ce dernier départage seulement des clés identiques.
Le contrôle existant continue de refuser deux émissions de même clé.

Des blocs fixes de2048 indices sont triés par tas. Les fusions successives
doublent leur largeur ; chaque tuile de4096 sorties calcule ses deux co-rangs
exacts par recherche binaire. Toutes les tuiles décrivent la même fusion
stable, y compris la dernière fusion. Ni le nombre de workers ni l'ordre
d'exécution ne changent les tuiles ou les comparaisons. Sans Pool, le même
algorithme s'exécute séquentiellement. Depuis le 3 octobre 2026, une clé
binary64 par niveau (F3/F4 d'[ARCHITECTURE.md](ARCHITECTURE.md)) tranche
les comparaisons hors de sa marge prouvée ; sinon l'ordre exact décide.
La permutation est inchangée ([PERFORMANCE_FULL.md](PERFORMANCE_FULL.md)).

Pour N émissions, deux Buffer u32 et les N clés binary64 réservent 16N
octets simultanément avant le tri. Le scratch est rendu à son retour ; la permutation4N reste vivante
pendant l'assemblage. Le pic public est mesuré par MemoryBudget avec toutes
les autres réservations vivantes. Le compteur de comparaisons est local à
chaque callback puis agrégé après jonction ; il n'est publié qu'au succès.
Avec N<2^32 et L=ceil(log2 N), la borne4NL<2^39 couvre les tas, fusions
et recherches de co-rangs. Un refus rend tous les buffers privés.

Les portes prévues vérifient les frontières des blocs et tuiles, les niveaux
larges/non réduits, l'ordre des supports, les égalités, W1/W4, les injections
d'échec d'allocation et la concordance de l'assemblage complet avec Fraction.

## Mémo du seul prédicat J2

`cache_center_lines=true` mémorise le résultat exact de
`center_line_meets` pour un triplet de positions croissantes i<j<k dans
UNE feuille et SA boîte. Le rang dense est C(k,3)+C(j,2)+i. Les états sont
réinitialisés à chaque feuille, donc aussi entre les deux passes.
Les contacts et les triplets alignés gardent la réponse du prédicat initial.
Ce cache ne mémorise ni angle, ni G3, ni admission d'une boule.

La capacité fixe maximale32 donne4960 octets par workspace, réservés dans
un Buffer ; au-delà de32 sites, chaque demande appelle directement le
prédicat exact. Aucun candidat n'est supprimé par cette limite de cache.
La mémoire supplémentaire parallèle vaut au plus4960×min(W,J), et elle est
rendue avant le tri. Les17 compteurs géométriques historiques restent égaux.
Trois compteurs supplémentaires distinguent les évaluations réellement
calculées, les hits et les replis directs : demandes=évaluations+hits,
replis≤évaluations. Option inactive : hits=replis=0.

Les portes isolent le rang, les trois relations, le changement de boîte,
la réinitialisation, le repli au-delà de32, la mémoire et les défauts
d'allocation. Les comparaisons catalogue complètes portent sur la référence,
l'option séquentielle et W4, y compris les refus, puis sur les deux options
réunies. Les mutants doivent être tués par ces portes natives.

## Origine critique et protocole

Lecture R2 `865f5e64ddd08bedf6ab8f94e8bb94812e380e79`, `generator.cpp`,
SHA256 `4647e90297b5ade1195f17c715ad79841409799fbd92d056991e82a85aa34cc1`.
Ses masques de triplets dépendent aussi de G3 ; ils ne sont pas portés.
Le mémo J2 est neuf. L'idée de trier des indices est également réexaminée
dans R2 `777406b82` ; aucun PSRS, vecteur non budgété ni bande flottante
n'est repris. La clé approchée du 3 octobre est neuve et suit F3/F4. Aucune qualification R2 n'est héritée.

Le [pilote d'ablation](../bench/catalogue_optimizations.py) déclare36 essais
K5 : trois LiDAR entiers×u21/u24×quatre modes W48, puis les six couples
LiDAR/profil avec les deux options W8, et les trois tailles synthétiques
avec les deux options W48. Chaque enfant a15s ; budget propre700s.
Un échec d'un mode ne supprime aucun autre mode ; seules les omissions
de budget sont permises et conservées avant lancement. Une répétition par
configuration : diagnostic d'ablation, sans prétention de stabilité temporelle.
Les octets sont comparés intraprofil ; sémantique et anciens comptes
géométriques/q4 sont comparés entre profils, modes et nombres de workers.
Les nouveaux compteurs de cache mesurent séparément le travail évité.
Le contrat200ms reste celui de FULL, avec forêts et verticales.
