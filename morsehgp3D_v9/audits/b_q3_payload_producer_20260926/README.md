# Producteur des intérieurs q3, pendant le census existant

Audit B, 26 septembre 2026 ; base `52ff41802`.

## Conclusion utile pour les 100 ms

Le census portable q3 peut restituer les **IDs originaux complets des
intérieurs sans refaire de prédicat de puissance ni repartir de la racine**.
La complétude est testée contre un census global indépendant. Les IDs ainsi
produits permettent de reconstruire les `BallData` régulières sans recensus.
Ce résultat complète `../b_census_payload_20260926/`, qui étudiait seulement
le consommateur en ayant payé auparavant un census global producteur.

Ce n'est pas encore un gain de temps : le prototype ajoute **12 à 15 %** au
census CPU émulé des échantillons mesurés. Aucun passage CUDA/G4, aucun
raccord à FULL et aucune baisse du temps de la tour ne sont qualifiés ici.
Il est raisonnable d'en étudier le raccord GPU pour économiser le recensus
aval, pas de mettre la variante actuelle par défaut sur la foi de ce test.

## Objet exact et mémoire

`producer.hpp` inclut le produit sans le modifier ni remplacer ses symboles.
La variante audit `mhgp9::audit_q3_payload::census_range` reprend uniquement
`gpu::q3_census_range`, hors passe fusionnée L15.

Pour chaque chunk, les ballots `inside` et `zero` sont ceux du census
existant. Si le chunk sature le seuil `K−1`, il ne produit aucun nouvel ID.
Sinon, ses bits `inside` font écrire les IDs originaux dans huit mots de
scratch **partagés par le groupe**, à la position `depth + rang_du_bit`.
Une graine finalement rejetée ne publie jamais son préfixe, même non vide.
Une graine acceptée publie le `LaneRecord` inchangé et un sidecar d'IDs.

L'invariant est `depth < K−1`. Pour K≤10, `depth≤8`. Le prototype réserve
donc 32 octets de scratch par groupe et **32 octets physiques par record**.
À K5, seulement trois IDs au plus sont utiles : 12 octets logiques, pas
32 octets obligatoires dans un futur format spécialisé. Les cellules
inutilisées portent `absent32` ; ce n'est jamais un ID d'intérieur.

Les synchronisations protègent les lectures du scratch et sa réutilisation
par la graine suivante. Ce contrat de partage est documenté, mais il n'est
**pas testé par une exécution CUDA**. Un tableau privé par thread ne peut
pas remplacer le scratch partagé. Les compteurs `PayloadWork` sont ceux
d'un groupe et doivent être relevés une seule fois par groupe sur GPU.

Chaque groupe reste indépendant. Il n'y a ni recensement supplémentaire,
ni tableau de voisins par nuage, ni nouveau produit cartésien. En notant
C les chunks déjà visités, S les graines, E les records acceptés, le
surcoût discret est O(C + KS + KE), à K fixé. Cette borne additionnelle ne
prouve rien sur le nombre de graines ou la complexité du générateur global.

## Preuves exécutées

Release GCC et Clang ASan/UBSan/LSan :

- 484 censuses de comparaison au produit ; statuts, compteurs de census et
  tous les champs des records identiques ;
- 233 records q3, 422 IDs d'intérieur comparés à un scan global exact de
  tous les points originaux par `gen::ExactBall` ; 20 871 tests de puissance
  dans ce juge ;
- 227 `BallData` régulières construites à partir de **ces IDs produits**,
  comparées aux listes du census global indépendant de la tour ;
- six records avec coquille de plus de trois sites : exclus du test
  consommateur direct ; le fallback global reste à raccorder et qualifier ;
- K2/K3/K5/K10, profondeur acceptée maximale 8, permutations rang/ID,
  rotations exactes des fixtures, tableaux de 31/32/33/63/64/65/66 sites et
  intérieurs aux positions 31/32/33 ;
- 515 graines rejetées et 15 IDs de préfixes incomplets jetés. Les slots
  non publiés restent au canari initial ;
- deux mutants compilés meurent causalement avec `interior_ids_differ`,
  pas par crash : omettre le dernier intérieur ; écrire le rang spatial
  au lieu de l'ID original.

Le constructeur `Packet::certify` de l'ancien prototype n'est pas utilisé.
La préparation de l'index du consommateur et ses censuses de référence sont
des oracles hors chronométrage, jamais du travail caché dans le producteur.

Avant la capture, un premier essai de compilation a été refusé pour trois
conversions u32→coordonnée signée sans cast ; corrigé après validation u18.
Un premier préflight a aussi refusé une fixture de couverture : ses
intérieurs saturaient tous le premier chunk et ne créaient aucun préfixe
jeté. Les positions ont été corrigées. Ces préflights ne sont pas les
résultats de qualification conservés dans `results/`.

## Mesures, et limites de l'échantillonnage

52 lignes = 13 cas × quatre passages, ordre des deux variantes ABBA.
Une mesure porte sur **128 arêtes consécutives dans le rang de l'index
d'audit, stratifiées sur l'ensemble du nuage**, pas sur les arêtes filtrées
par la WSPD ou S3. Cet échantillonnage peut sélectionner des sauts entre
sous-arbres ; il présente beaucoup de graines rejetées. Il ne constitue
**pas** un estimateur sans biais de la production réelle.

Le temps inclut tous les censuses des graines de ces arêtes, rejetées
comprises ; il exclut construction de l'index, cover/ordre, allocations des
fixtures, comparateurs et oracles. Les chiffres sont des médianes de quatre
sommes de durées, sur CPU local partagé, pas des millisecondes GPU.

| Nuage sans sol 1 mm | K | Graines | Records | Census initial (ms) | Avec IDs (ms) |
|---|---:|---:|---:|---:|---:|
| 08/000000, 39 885 sites | 5 | 13 863 | 62 | 3,143 | 3,531 |
| 08/000100, 35 551 sites | 5 | 52 757 | 38 | 11,677 | 13,449 |
| 08/000200, 45 845 sites | 5 | 10 961 | 48 | 2,531 | 2,895 |
| 08/000000, 39 885 sites | 10 | 13 863 | 104 | 3,247 | 3,709 |

Ces trois trames appartiennent à **une seule séquence**. Elles ne satisfont
pas la demande de plusieurs séquences. Les fichiers complets servent au
contexte géométrique de chaque census ; seules 128 arêtes sont traitées.

Les synthétiques uniforme, terrain et amas sont également mesurés à
8k/16k/32k. Pour **ces mêmes diagnostics à 128 arêtes**, les tests de census
croissent respectivement :

| Régime | 8k | 16k | 32k |
|---|---:|---:|---:|
| uniforme | 123 929 | 287 398 | 607 478 |
| terrain | 86 634 | 217 310 | 464 199 |
| amas | 64 741 | 164 796 | 307 654 |

Les deux variantes font exactement le même travail de census. Les rapports
au doublement sont ici inférieurs à quatre ; cela **ne qualifie pas le
caractère sous-quadratique d'un générateur complet**, dont le nombre
d'arêtes varie lui aussi. Ces mesures isolent la charge du port producteur.

## Raccord restant : ne pas perdre l'identité du sidecar

Le prototype ne couvre pas L15, `lanes_task_slab_index`, les records q3
stockés en fin de slab en ordre inversé, le tri q4, le staging/gather GPU,
ni les tris de `Presentation` et du catalogue. Un ID de payload ou handle
possédé doit accompagner chaque record à **tous** ces déplacements.
Juxtaposer un tableau d'IDs après le tri ne suffit pas.

Les records avec coquille supplémentaire doivent conserver un fallback
global tant que leur coquille complète n'est pas transportée. La provenance
du compte et du sidecar doit être scellée par le producteur et son owner :
un compte forgé en même temps qu'une liste incomplète ne se détecte pas
par le seul test d'appartenance des IDs présents.

Une optimisation directe à essayer ensuite est de prélever les IDs **après**
`q3_census_chunk` à partir de `depth_après − depth_avant`. Cela peut éviter
le popcount additionnel sur les chunks rejetés. Elle n'est pas implémentée
ou qualifiée par ce reçu ; ne pas lui attribuer les chronos ci-dessus.
Le test GPU décisif doit mesurer le producteur **et** le consommateur,
copies, taille du sidecar, tris et fallback compris, sur la chaîne entière.

## Reproduction et clôture

Le build est épinglé après capture :
`/workspaces/E-HGP/build/v9-q3-payload-producer-audit-20260926`.

```sh
python3 -B morsehgp3D_v9/audits/b_q3_payload_producer_20260926/readback.py \
  morsehgp3D_v9/audits/b_q3_payload_producer_20260926/results
python3 -O -B morsehgp3D_v9/audits/b_q3_payload_producer_20260926/readback.py \
  morsehgp3D_v9/audits/b_q3_payload_producer_20260926/results
```

Les deux lectures passent. Le lecteur est LIVE : sources, exécutables,
bruts et sorties sont re-hachés. Il dépend des builds et bruts non versionnés ;
ce n'est pas une archive autonome. `run.py` doit toujours recevoir un
répertoire de sortie neuf et un build neuf pour une autre qualification.
Aucun fichier du moteur, aucune qualification historique et aucune VM GCP
n'ont été modifiés par cet audit.
