# Finition commune CPU/CUDA — contrelecture avant publication

7 octobre 2026. Prototype local hors de `main` (`9428db65b`), sources identifiées par `capture.json`.
Lecture statique, aucun banc, compilation ni transfert de qualification GPU. Les tests du développeur mentionnés
ci-dessous ont été lus, pas rejoués par cet audit. Cadre v12 hors registre, `full_pi0`, u21, `not_claimed`.

La nouvelle finition répond à la direction de CST-0233 : rangs de coordonnées, tris par base stables, rangs de
niveaux et populations par préfixes, table triée. Le pilote est commun aux exécuteurs Pool et CUDA. Le correctif
n'est pas encore publié ; aucune clôture de CST-0233 ni baisse de temps ne découle de cette lecture.

## Points mathématiques contrôlés

1. **Ordre des supports.** Les sites reçoivent leurs rangs lexicographiques 1..n. Les quatre rangs d'un support sont
   triés, puis complétés par zéro : un support qui est préfixe d'un autre le précède. `pack4` prend assez de bits
   pour représenter n, jusqu'à 32 bits par composante ; les quatre composantes tiennent donc dans 128 bits.
   Cela réalise la comparaison par positions du catalogue, sous unicité des sites déjà garantie par Cloud.
2. **Deux ordres distincts.** La table de recherche est triée par SiteIdx, avec n pour case absente ; ce choix met
   l'absence après tout site, comme `kNone` dans `tail_less`. Il ne faut pas confondre cette table avec l'ordre
   canonique des boules par niveaux puis positions. Le prototype maintient cette distinction.
3. **Niveaux exacts.** q2 emploie la norme carrée divisée par quatre ; q3 le produit des trois côtés carrés divisé
   par quatre fois la norme carrée du produit vectoriel ; q4 la somme des carrés des numérateurs de Cramer sur
   quatre fois le déterminant carré. Les multiplications sont élargies avant réduction de largeur. Le comparateur
   emploie les produits croisés entiers ; la clé flottante ne tranche que sous le certificat F4.
4. **Réparation des chaînes.** Après le tri stable (clé F3, positions), chaque voisin incertain passe en exact.
   Une inversion entraîne le tri exact de toute sa chaîne de voisinages incertains, puis une nouvelle vérification.
   Sous la borne F4 héritée, une frontière certifiée sépare les valeurs exactes de ses deux côtés ; on ne doit donc
   pas limiter le repli à la seule paire inversée. `collect_chains` prend les chaînes entières, une fois chacune.
5. **Plateaux et CSR.** Le drapeau de début de niveau lit le voisin global, même à une frontière de tuile.
   `rang = prefixe_exclusif(drapeau) + drapeau` donne un rang constant sur tout plateau ; seul son premier élément
   fournit sa représentation rationnelle. Les longueurs de population sont préfixées séparément et le total doit
   égaler les incidences. Les plages de copie sont disjointes.
6. **Stabilité du tri.** Les bases des seaux viennent de leurs totaux et des tuiles précédentes. Dans une tuile,
   les paquets sont parcourus dans l'ordre et `claim` donne les places par ordre de voie, y compris sur CUDA.
   Sauter un octet dont ET=OU est une passe identité et conserve l'ordre secondaire.

Les témoins livrés dans le prototype couvrent notamment une égalité rationnelle de formes différentes dont les
clés F3 sont inversées, ainsi qu'un plateau de plus de sept tuiles. La concordance de leurs intentions avec ces
conditions est acquise par lecture ; leurs résultats natifs et appareil restent à contre-juger à la livraison.

## Coûts à conserver dans la prochaine mesure

Le rassemblement des lots CPU copie encore les enregistrements/populations. Le repli de chaîne rapatrie tous les
verdicts/indices/clés puis trie les chaînes sur l'hôte. Les tableaux de tri coexistent avec niveaux exacts, clés,
CSR et sortie ; leur pic doit être mesuré. Le préfixe supérieur parcourt les sommes de tuiles avec un seul warp :
sa longueur est proportionnelle au nombre de tuiles, pas logarithmique. Aucune de ces observations ne prouve une
régression ; la campagne appariée doit publier coûts, pics et nombre d'éléments réparés.

Le mur de catalogue doit inclure préparation de la finition, téléchargements et matérialisation des niveaux.
La conservation du premier représentant rationnel doit être vérifiée octet pour octet contre la voie CPU publiée,
et la sémantique FULL reste un jugement distinct. Les 100 ms restent ouverts.
