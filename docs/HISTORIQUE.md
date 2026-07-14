# Historique condensé du projet

Le dépôt a successivement contenu plusieurs familles d'algorithmes :

1. `HGP-old`, proche des constructions exactes de la thèse et dépendant notamment de CGAL;
2. `E-HGP`, première tentative de régularisation entropique et de Borůvka paresseux;
3. `PERGHGPClusterer`, atlas progressif de témoins et cofaces;
4. `PowerCover3D`, champ de puissance régularisé échantillonné sur une grille cubique;
5. l'architecture actuelle à deux backends, `MorseHGP3D` et `HomogeneousLensTower`.

Les quatre premières étapes ont été utiles pour isoler plusieurs principes : distinguer proposition et certification, publier le périmètre exact, traiter les ex æquo par lots, mesurer les coûts de voisinage séparément de la réduction hiérarchique et ne pas confondre une hiérarchie sur atlas avec le HGP global.

Elles ont aussi montré les limites qui motivent le changement actuel :

- une grille introduit ses propres événements et une échelle de résolution;
- une mosaïque de Delaunay d'ordre supérieur est trop coûteuse comme structure de production;
- une régularisation locale ne rend pas spontanément la hiérarchie globale sparse;
- un atlas incomplet peut retarder les fusions sans que les poids locaux soient faux;
- une partition plate ne permet pas d'auditer la hiérarchie ordre–échelle.

Le nettoyage du dépôt ne détruit pas ces travaux. `HGP-old` reste présent intégralement comme référence directe du manuscrit, sous sa licence non commerciale propre. L'arbre complet précédant la consolidation reste disponible dans l'[instantané `0b8a1a1`](https://github.com/Ludwig-H/E-HGP/tree/0b8a1a11750b931f486ce666265eed4b6e95e2b1), avec les rapports datés, notebooks, résultats de benchmarks et les autres générations de code.

Dans l'arbre courant, `HGP-old` porte la référence historique au manuscrit et `PowerCover3D` conserve les contrats numériques et certaines primitives GPU postérieures. Aucun des deux n'est présenté comme un backend scientifique actif.
