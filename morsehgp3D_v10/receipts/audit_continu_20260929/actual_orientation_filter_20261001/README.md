# Contrôle causal du filtre d orientation

Ce paquet teste directement le filtre semi-statique réellement appelé
par la certification de MEB de la tour. Il compare son signe à une
géométrie entière indépendante, puis remplace seulement la borne
`0x1p-49` par `0x1p-80`. L'objectif est de détecter une décision fausse
du filtre même si le repli exact masque celle-ci dans la sortie finale.

## Géométrie et domaine

Les quatre vecteurs et le centre sont dans [protocol.json](protocol.json).
Pour chacun, poser v=(u_z,u_x,−u_y), t=(−u_y,−u_z,u_x) et considérer
C+u, C−u, C+v, C+t. Leurs normes relatives à C sont égales.
La sonde native vérifie que le vrai `geom::center4` construit C exactement
et que D>0. Ces tétraèdres sont non coplanaires ; C est sur le segment
[C+u,C−u], donc sur deux de leurs plans de face. Ce sont des cas de
certification fermée, pas des supports q4 stricts à émettre au catalogue.

Les coordonnées sont positives et inférieures à2^21 ; les différences
de coordonnées de la fixture sont inférieures à2^19. Le signe exact est
celui de ((b−a)×(c−a))·(C−a), calculé en i128 natif sans multiplication
par D, puis recalculé en entiers Python indépendants dans le lecteur.
Les quatre faces donnent huit zéros et huit autres signes par arrondi.

## Portée des arrondis

Le protocole appelle directement le filtre sous quatre modes d'arrondi.
Il attend64 lignes par exécutable,32 orientations nulles et32 non nulles.
Le témoin doit s'abstenir ou donner le signe exact ; le mutant doit
prendre au moins une décision fausse sous FE_TONEAREST.
Le préflight GNU a observé27 décisions fausses, dont5 sous FE_TONEAREST.

Dans la tour, l'appelant coupe le filtre hors FE_TONEAREST. Les trois
autres modes sont donc un diagnostic de primitive, pas un transfert de
qualification au chemin d'appel natif. Les invariants numériques de la
sonde couvrent ces seuls cas, pas toutes les coordonnées21bits.

## Captures et provenance

[record.py](record.py) construit dans un répertoire neuf hors du paquet :
témoin GNU, mutant GNU et témoin GNU UBSan. Il conserve argv, stdout,
stderr, codes, délais, durées et empreintes des exécutables, ainsi que
les hashes des sources avant/après dans [source_close.json](source_close.json).
La compilation et les exécutions sont bornées selon le protocole.
Un signal ou un délai n'est jamais une preuve géométrique acceptée.

Les quatre headers du témoin sont les sources complètes B21.
Le site d'appel est vérifié en lecture seule au hash tower.cpp
`3ce0a14a0d6413ca8f7171e6414099334cacdd2ebdf8d13845e7a01020ff9f06`.
La seule différence du mutant est le littéral de la borne.
[preflight.json](preflight.json) conserve les essais exploratoires.
Les binaires restent hors de Git ; aucune source moteur n'est modifiée.

## Lecture et fermeture

Le SHA256 externe de manifest.json doit être fourni avant lecture.
[read.py](read.py) valide ce manifeste puis l'inventaire et tous les
payloads avant d'interpréter les captures. Il reconstruit les signes
indépendamment et exige les64 identités de requête, sans doublon ni
absence. Le code0 de la sonde mutant signifie « erreur numérique
attendue observée », jamais « moteur mutant conforme ».

Commande de lecture, avec le SHA publié à côté du paquet :

```bash
python3 -B read.py CHEMIN_DU_PAQUET SHA256_EXTERNE
python3 -B -O read.py CHEMIN_DU_PAQUET SHA256_EXTERNE
```

Le collecteur refuse un paquet déjà fermé ; ne pas le rejouer pour
écraser ses captures. Une recompilation indépendante utilise une autre
sortie. Le déplacement du paquet et un faux SHA sont testés séparément.

## Limites

Primitive CPU seulement : ni catalogue complet, ni résolution FULL,
ni rattachement de points, ni croissance LiDAR, ni float32, ni GPU/G4.
Le paquet ne qualifie pas le contrat100ms. Le filtre de production
n'a pas de défaut démontré sur ces fixtures ; c'est le mutant qui
est réfuté. `public_status=not_claimed`.
