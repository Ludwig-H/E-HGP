# Contre-audit B — omissions, sonde C et portée des verdicts

23 septembre 2026. Lecture indépendante du commit `243373f6`, recoupée
avec le moteur v15 `33d51efd`. Cadre : exploration v9 hors registre,
CPU de référence, grille entière 1 mm, statut `not_claimed`. Aucun GCP
ni chrono de contrat dans cette contrelecture. Les fichiers de l'auditeur C
restent sa propriété ; cette note ne les réécrit pas.

## Ce que la nouvelle sonde établit

Le protocole reconstruit FULL après avoir retiré **une boule déjà présente**
du catalogue produit. À 8k, les 515/515 retraits de strates
`p+u <= Kmax` (six cas K5, deux K7) sont refusés ; c'est une observation
utile de sensibilité. Le sous-ensemble testé est déterministe : pour chaque
strate, `members[(j * members.size()) / take]` dans l'ordre des clés
([source](c_omission_20260923/omission_tower_probe.cpp), fonction
`mode_scale`). Ce n'est ni un échantillon aléatoire ni une énumération
exhaustive à 8k. Les « LiDAR 8k » sont des coupes/disques **sans sol**, dont
les fichiers d'entrée ne sont pas versionnés dans ce dossier ; ce ne sont
ni des trames entières ni une qualification de plusieurs séquences. Le
commit ne contient pas encore le résultat K10 annoncé comme « en cours ».

La sonde ne cherche aucune clé que le générateur n'a **jamais émise**.
Même si tous les retraits essayés étaient refusés, cela ne prouverait pas
la complétude du générateur sur ces nuages. Elle ne compare pas non plus
les payloads des tours acceptées : les 278/280 retraits de couche
supérieure acceptés ne sont pas 278 hiérarchies démontrées fausses ; ils
signifient seulement que le statut du constructeur n'a pas refusé.
Le mode `b13` affiche bien des refus, mais son code de retour ne dépend
que de la présence de D/T au catalogue initial, **pas** du fait que les
reconstructions mutilées aient effectivement été refusées.

Un contrôle local ciblé apporte plus qu'un simple statut. J'ai repris la
sonde source du commit `243373f6` sans la modifier dans le dépôt, en
ajoutant à la compilation, après `++reasons[ref ? t.reason ...]`,
l'impression de `r.tower_digest` et `mhgp9::tower_digest(t)` lorsque FULL
accepte. Avec les bibliothèques Release locales
`build/v9-audit-c-scale`, `-O1 -DNDEBUG`, deux fils, et les trois
fixtures `family <uniform|terrain|clusters> 8 5 100000 2`, les 12
retraits de couche haute acceptés donnent **neuf condensés différents**
et trois identiques. Par exemple :

```text
uniform_8/K5, index 37, p=3, q=u=3 : complete_relative
tour saine  5d211577996c3ca7
tour mutée d8745b827e7f3fb2
uniform_8/K5, index 47, p=3, q=u=3 : complete_relative
tour mutée 5d211577996c3ca7
terrain_8/K5, index 8, p=4, q=u=2 : complete_relative
tour saine  3291cfb15c0e15e9
tour mutée a1383cfd7ddac0db
```

Un condensé **différent** établit qu'au moins un champ de tour haché
diffère ; un condensé égal ne certifie pas l'égalité du payload. Le
résultat est une recherche légère de contre-exemples, pas un reçu
autonome épinglé : ses bibliothèques locales et ses sorties ne sont pas
archivées dans ce dossier. Il montre néanmoins qu'« accepté » n'est
pas synonyme de « tour inchangée ». La prochaine porte doit comparer
directement les champs FULL, et fixer le source/build/fixture.

## Le lemme « naissance visible » reste à prouver

Le rapport C affirme qu'une boule omise dont `p+u <= Kmax` doit être
refusée parce que sa naissance doit finalement fusionner. Le passage
logique est incomplet : `validate_catalogue` construit
`programs[k]` **seulement** à partir des boules conservées
([moteur](../src/tower/forest/full_ball_tower.hpp), vers 1158–1162) ;
les descentes de `prepare_static_order` naissent de ces programmes
(vers 1408–1427). Le test `live == 1` (vers 408–410 et 833–835) ne
compte que les nœuds ainsi créés. En retirant une naissance, on retire
donc aussi le nœud que l'argument invoque : la racine unique ne force
pas, à elle seule, une référence à la clé manquante.

Il manque un lemme géométrique : **pour chaque** boule B de rang haut
`p+u <= Kmax`, une autre boule conservée doit imposer une facette dont
la résolution atteint B, **ou** son retrait doit nécessairement laisser
plusieurs composantes finales. Les 515 refus, les petits essais adverses
et l'absence actuelle de contre-exemple soutiennent cette conjecture,
sans la démontrer. Ne pas inscrire « prouvé pour toute boule » au contrat.

Pour une omission isolée de coquille régulière, le coefficient d'Euler
apporte un juge nécessaire quand `p <= Kmax-3`. Pour les q2 à
`p=Kmax-1` et q3 à `p=Kmax-2`, ni ce coefficient ni le test de statut
ne sont systématiquement sensibles : appeler leurs 26–29 % de catalogue
K5 (17–18 % à K7) une **zone de détection potentiellement aveugle**,
pas autant d'erreurs de hiérarchie démontrées. Les 2/280 refus observés
dans ces strates montrent justement que `p+u > Kmax` n'implique pas
toujours l'invisibilité. Exécuter K6/K7 puis comparer la restriction
clé par clé renforce le diagnostic K5, mais ne remplace pas un oracle
indépendant si une omission commune aux deux exécutions est possible.
K11 n'est pas couvert par le domaine produit actuel : ne pas transporter
ce protocole tel quel au contrat K10.

## Portée des alternatives C

La révision 1 distingue désormais mesures locales, projections et
contrat brut : correction importante. Quelques phrases demeurent plus
fortes que leurs preuves. Le § 1 affirme qu'aucune famille ne peut tenir
K10 « sous quelque hypothèse défendable » et que seule la cible K5 reste
ouverte : R9 borne le **chemin CPU actuel**, pas une architecture GPU
résidente ou un autre générateur exact. Le tableau D5 annonce le lemme C
« prouvé ici » alors que sa propre discussion garde ouverte la borne
basse `K=p+q_min-1`. La route E0 est dite « déjà tranchée » à partir de
48,6/56,3 % **estimés par modèle**, tandis que la mesure directe est
encore demandée. Enfin, l'émission par niveau est appelée « prérequis
de 100 ms » sans borne inférieure excluant une autre organisation.
Ce sont des hypothèses ou priorités de recherche, pas des impossibilités
ni des nécessités acquises. Les 129 « vérifications » archivées sont des
verdicts argumentés sur 83 constats, non 129 tests exécutables.

## Décision d'audit proposée

Garder `complete_relative`, les objectifs K10 et K5, et une porte
séparée pour trois choses : (1) exactitude de FULL **conditionnelle** au
catalogue, (2) sensibilité du constructeur/Euler à des suppressions de
clés connues, (3) complétude du générateur jugée par un oracle indépendant
ou une preuve couvrant les clés non émises. Avant un reçu v15 G4,
fermer le faux refus K1 de la borne chrono, comparer directement les
payloads ON/OFF et exercer le recouvrement sous TSan. Les tests d'omission
ne qualifient ni la tour G4, ni la croissance sous-quadratique, ni la
complétude sur trame brute entière.

## Addendum après le lemme d'A (11 h 15 UTC)

La [preuve de la première cofacette](LEMME_PREMIERE_COFACETTE_OMISSION_20260923.md),
publiée ensuite par A dans `019e34ad`, **comble conditionnellement** la
lacune que j'avais identifiée dans l'argument initial de C. Après
contrelecture géométrique et du raccord à `full_ball_tower.hpp` : pour
`S=I_B∪U_B`, une cofacette de rayon minimal strictement supérieur à
`r(B)` existe si `|S|<n`; son bloc présent doit demander la facette
isolée `S` avant ce rayon. Deux détails doivent être explicites dans
une version registre : aucune autre graine statique pour `S` ne peut
court-circuiter la recherche, car toute boule critique de population
fermée exactement `S` est nécessairement l'unique `MEB(S)=B` ; et,
pour une coquille étendue, `S` est un sommet strict singleton de
`ShellTable`, donc son représentant est bien `S`. Les égalités au
rayon de la cofacette ne changent pas cet isolement **avant** ce rayon.

Je n'ai pas trouvé de contre-exemple sous les hypothèses du lemme :
catalogue autrement complet, coquilles admises (`u≤12`), exactitude
des miniboules et résolveur interne. Il couvre aussi plusieurs clés
retirées **toutes** de rang haut `p+u≤Kmax`, en choisissant une clé
retirée de rayon maximal. Cela ne s'étend pas aux suppressions mêlées
à la couche haute, au résolveur batch externe, ni à la preuve que le
générateur a produit les autres clés. La sonde 515/515 reste une
confirmation expérimentale, pas le fondement du lemme. Depuis la
rédaction initiale, `c19e4b49` a fermé le faux refus K1 du lecteur et
R10 a mesuré le recouvrement FULL CPU ; voir la
[contrelecture R10](CONTRE_AUDIT_B_G4_R10_ET_PASSE2_20260923.md).
