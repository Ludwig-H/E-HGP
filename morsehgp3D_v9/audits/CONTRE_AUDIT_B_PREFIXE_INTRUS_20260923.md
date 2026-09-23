# Contre-audit B — préfixe d'intrus FULL par BallKey

23 septembre 2026. Lecture indépendante de
[`INTRUS_FULL_PREFIXE_EXACT_20260923.md`](INTRUS_FULL_PREFIXE_EXACT_20260923.md)
au moteur `5ab4326c`. **Lemme exact confirmé ; fréquence des clés et gain
G4 inconnus.** Aucun cache n'est porté ici.

Le `CloudIndex` range ses feuilles par intervalles contigus de rang Morton
et `intruder_work` visite gauche avant droite. Le rejet `power≥0` conserve
les contacts hors de la suite des intérieurs. La première réponse est donc
le plus petit rang intérieur hors facette ; après une réponse `z`, les
intérieurs antérieurs à `z` ne peuvent être que dans la facette. Les
retester exactement reconstruit un **préfixe complet**, utilisable et
extensible pour la même BallKey sur le même index immuable. L'oracle A
passe 1 336 782 paires en modes normal et `-O`, mais n'exerce pas encore
le vrai radix/borne ni les durées de vie du constructeur.

Portes d'implémentation précises :

- Le stade différé `(BallKey,S,z)` doit copier la clé, les rangs Morton
  de `S`, sa longueur K et `z` **avant** que `static_terminal` remplace le
  buffer de facette par la clé suivante. Un `span` emprunté ou un tableau
  zéro-complété sans longueur peut changer l'intrus (le rang zéro est
  valide).
- Une table est propre à un `CloudIndex`/nuage ; même BallKey sur un autre
  nuage peut avoir un intérieur antérieur supplémentaire. Comparer les
  clés exactement après le hash ; une collision de hash est un miss, pas
  une autorité. Une écriture d'entrée n'est visible qu'une fois complète.
- Si un hit évite la descente, conserver `intruder_queries` comme nombre
  **logique** : `prepare_external_batch` le relie aux étapes du résolveur.
  Publier séparément hits, descentes réelles, nœuds économisés et tests de
  puissance ajoutés pour matérialiser/étendre le préfixe.
- La réponse `−1` n'alimente pas ce préfixe. Le raccourci qui lit les
  intérieurs d'un `BallData` catalogué suppose une **liste globale
  complète**, que `run_tower_chain` recense mais que la seule validation
  du constructeur ne reconstruit pas. Chercher le minimum **en rang
  Morton**, jamais le premier ID stocké.

Fixtures différentielles à tuer : `I=(2,5,9)`, `S=(2,7)` donne 5 puis
`S'=(7,8)` doit donner 2 (le dernier intrus seul serait faux) ; ajouter
un contact de puissance zéro entre 2 et 5, une mutation du buffer de S,
une collision de hash, deux indices différents pour la même BallKey et
des IDs de points permutés relativement aux rangs. Comparer chaque
intrus rendu, pas seulement le digest final.

Sur le cas G4 statique 000000/K10, **7,097 M requêtes et 403,430 M
visites d'index** s'étalent sur K2..10. Sans histogramme des répétitions
de BallKey **dans le même worker**, ces volumes ne prédisent aucun hit.
La voie différée demande en outre la copie de la facette (jusqu'à 40 o),
en plus de la clé (~80 o) et du préfixe (44 o) ; avec K, z, état et
alignement, compter plutôt **au moins ~176–192 o par entrée** avant la
table et l'allocateur. À 4 096 entrées ×48 workers, cela représente
environ 34,6–37,7 Mo décimaux, non un budget gratuit. Échantillonner les
clés par hash en gardant tous leurs appels, puis mesurer fréquence par K
et worker, octets et temps total avant de réserver ce cache sur G4.
