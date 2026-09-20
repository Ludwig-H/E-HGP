# Préparation avant gel

Le prototype a d’abord été compilé et essayé sur six points. Les modes
disque/positivité donnaient les mêmes deux seeds rejetés sur quatre,
avec125/77 nœuds et160/109 tests de témoins. Entrée :

```text
6
8 10 10
12 10 10
10 13 10
10 11 12
10 11 8
10 11 7
```

SHA256 des53 octets de cette entrée, avec saut de ligne final :
`e50302f3517fbefe9be175384a225af11578aed21cb0b5c3415de6c7a6cda190`.
Cette fumée n’est pas l’autorité de la qualification close.

Avant le gel, un arrêt conservateur UNKNOWN a été ajouté lorsque tous
les témoins sont décidés sous le seuil. Une subdivision ne pourrait
alors plus produire DEEP, mais pourrait encore reconnaître OUT ; ce
choix n’est donc pas une équivalence de listes de rejets.

Le binaire et l’entrée de fumée restent dans `.build/preflight/`, hors
versionnement. Les qualifications et78 mesures utilisent la source
figée5c383924cb27dcac499383980640c1f55662afd94525705b172687ea0181a9cf.
Les portes mathématiques ont également été exécutées pendant la
préparation ; leurs exécutions closes figurent ensuite dans la capture.
Aucun essai produit, aucune qualification héritée.
