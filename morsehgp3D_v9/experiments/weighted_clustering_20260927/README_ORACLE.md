# Qualification géométrique indépendante sur petits nuages

`qualify_geometry.py` utilise seulement `Fraction` et une résolution de Gram
par Gauss–Jordan : supports affinement indépendants de taille au plus quatre,
poids barycentriques non négatifs et containment exact. Aucun prédicat natif,
`weighted_model`, calcul flottant de miniball ou résultat de clustering ne
sert d'oracle de géométrie.

Le domaine de l'oracle est volontairement **1 à 12 sites**, pas le domaine
du moteur ou un quota sur ses sorties. La campagne fixée comporte 28 cas :
triangles entier équilatéral et obtus, carré, octaèdre, intérieur obligatoire,
trois nuages aléatoires de huit points, et K10 sur onze points. K1/2/3/5 sont
exercés là où les tailles le permettent. Aucune scène de 1 200 points ni GCP.

Pour chaque cas, sont recomputés indépendamment :

- tous les supports positifs et les boules admissibles au rang demandé ;
- leurs centres/rayons rationnels, intérieur strict, coquille complète,
  supports minimaux et q_min, y compris les coquilles non régulières ;
- tous les (K+1)-sous-ensembles, leur miniball et leur propriété de Gabriel ;
- les facettes et cofaces de Čech, les composantes du graphe de Gabriel et
  les couvertures FULL, avant et à chaque niveau critique exact.

Les comparaisons de couverture portent sur les composantes **non triviales**.
Une facette isolée est écartée ; ce n'est pas une validation de ses dates
virtuelles dans EOM. Les multiensembles de couvertures sont comparés, sans
fusionner artificiellement deux composantes ayant la même union de points.
Les identités des cofaces, niveaux et tous les masques sont aussi contrôlés :
une égalité de couverture ne remplace pas l'égalité du catalogue pondéré.

`test_qualify_geometry.py` vérifie l'oracle pur et dix corruptions d'un petit
JSON **synthétique**. Ce JSON n'est jamais présenté comme une exécution native
ou une preuve GPU. Les tests normaux et `-O` ne dépendent pas d'assertions.

## Capture et relecture

Le constructeur natif est indépendant. Après gel de toutes les sources :

```text
python3 -B qualify_geometry.py --binary /absolute/build/native_weighted_export --build-receipt /absolute/build/receipt.json --output /absolute/new/capture
python3 -B qualify_geometry.py --readback /absolute/new/capture
python3 -B -O qualify_geometry.py --readback /absolute/new/capture
```

La capture exige un nouveau répertoire privé. Elle archive neuf commandes
préalables : quatre suites Python en normal/−O (oracle, modèle, EOM, CLI
native), puis le gate natif. Suivent les 28 commandes d'export et le jugement
Fraction. Commandes exactes, retours, sorties standard/erreur, entrées et
hashes sont conservés. Un échec reste `failed`, sans réutiliser ce répertoire.

Le reçu lie les dépendances épinglées du build, son reçu et son binaire aux
sources du contrôle, du modèle et de l'EOM, avant et après les commandes.
`--readback` ne relance **aucun** exécutable géométrique : il vérifie les
pins LIVE, l'inventaire, les commandes et toutes les empreintes puis recalcule
les petits oracles à partir des sorties enregistrées. Ce n'est pas une archive
autonome détachée des sources et du build privés.

Cette qualification ne prouve ni une borne globale de coût, ni la qualité
des labels pondérés, ni la fidélité de tous les paramètres de HGP-old. Les
tests EOM et modèle archivés portent leurs propres périmètres et oracles.
