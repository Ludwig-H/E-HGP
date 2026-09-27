# Ordre des candidats et vrais drafts FULL : les décisions suivantes

27 septembre 2026, après `4badf8b7d`. Exploration CPU hors registre,
grille entière 1 mm, `public_status=not_claimed`. Moteur inchangé ;
GCP non utilisé. Ce lot vérifie deux choix d'architecture avant leur port,
sans nouvelle revendication FULL GPU ou 100 ms.

## q3/q4 : éviter le tri final sans revenir au carré

Le [prototype de lignes partagées](../audits/b_q34_ordered_rows_20260927/README.md)
conserve A dans son ordre original et construit **une liste B ordonnée par
classe de crédits A**, réutilisée par toutes les ancres de cette classe.
Cela émet directement les candidats dans l'ordre natif. Un compactage
stable après S2 conserve cet ordre, sans tri des survivants.

Ce n'est pas une liste B construite pour chaque point A : son travail est
W = somme(C_A·|B|), pas somme(|A||B|). Le nombre d'entrées de listes T
peut être bien inférieur aux E paires qu'elles représentent. Pour K fixé,
W≤K(K−1)F_B ; cette constante est payée et peut être défavorable.

La [capture close](../receipts/q34_ordered_rows_20260927/README.md) porte
25 commandes, Release/sanitizers/deux mutants par build, 17 488 vrais
rectangles de gate et 15 mesures. Sur LiDAR00 K5/s8 :

- W = 4,080 millions de comparaisons de crédits ;
- T = 1,969 million d'entrées B partagées ;
- E = 9,123 millions de paires, inchangé ;
- ajout du sidecar : 71,985 ms CPU local, ancien Pool encore payé.

Trois trames sans sol, K10, s10/12 et uniforme/terrain/amas 8k/16k/32k
sont mesurés. Sur amas, W croît de ×2,093 puis ×2,033, mais E conserve
×3,724 puis ×3,942. La représentation n'a pas résolu le résidu presque
quadratique. Aucun chrono S2/FULL n'est déduit de cette sonde.

**Décision :** comparer ce format à l'arène de bandes, sur préparation,
décodage, mémoire et coût S2 complets. Ne pas choisir les bandes seulement
sur leurs 12 octets de descripteur, ni les lignes sur le seul tri évité.
Le plan direct sans ancien Pool reste à réaliser pour cette variante.

## FULL : le prototype général régresse sur les vrais objets

Les [vrais drafts](../audits/b_full_real_drafts_20260927/README.md) sont
interceptés dans une sonde isolée, sans réécrire les décisions du moteur.
Le constructeur natif reste exécuté ; ses entrées sont copiées puis
réencodées trois fois avec les deux implémentations. Les sorties sont
comparées champ à champ à la forêt réellement publiée.

| entrée K1..5 | actions/nœuds | natif / prototype général, ms |
| --- | ---: | ---: |
| LiDAR00 sans sol, 39 885 sites | 1 541 750 | 149,38 / 296,30 |
| uniforme 8k | 629 404 | 52,14 / 92,41 |
| uniforme 16k | 1 301 794 | 130,01 / 241,30 |
| uniforme 32k | 2 660 312 | 287,51 / 554,28 |

Ces temps sont les **sommes des médianes par ordre**, pas des murs
parallèles de tour. L'hôte est partagé ; les sorties, allocations et
préparations du nouveau code sont payées. Les deux résultats demeurent
présents pendant les comparaisons. Les copies perturbent le chrono de la
chaîne instrumentée : ce n'est pas une nouvelle mesure de production.
[Reçus, RSS et capacités](../receipts/full_real_drafts_20260927/README.md).

**Décision : ne pas porter l'encodeur général scalaire tel quel.** Son
exactitude structurelle passe les tests, mais le tri d'incidences et ses
temporaires n'apportent pas de gain CPU. On ne dispose pas d'une attribution
chronométrique exclusive du surcoût au tri, ni d'un résultat GPU.

Les quatre entrées mesurées n'ont aucune continuation. C'est une propriété
observée des drafts, pas une hypothèse autorisée sur tout LiDAR. Elle ouvre
une voie plus simple, [contre-auditée mathématiquement](../audits/b_full_batch_review_20260927/README.md) :

1. Vérifier globalement qu'aucune action n'a un seul parent. Sinon garder
   la voie générale, sans refuser une continuation valide.
2. Chaque action crée alors un nœud : son ID est son ordinal d'action ;
   les offsets parents du draft sont déjà ceux de la sortie.
3. Pour chaque parent, conserver son plus petit ordinal d'apparition.
   Toute occurrence ultérieure est un réemploi interdit après fusion.
4. Garder les contrôles locaux et la réduction chronologique du premier
   motif de refus, puis écrire les plages finales disjointes.

Travail abstrait linéaire dans lots/actions/parents/contributions/nœuds,
sans tri global de parents. Une réduction minimum parallèle est possible ;
le premier worker arrivé n'est **pas** un substitut correct. Cette note
ne qualifie pas encore le prototype de cette voie en cours de préparation.

## Deux exigences à ne pas perdre au raccord GPU

Le Pool choisit ses témoins à projections égales par **ID original**.
L'entrée GPU actuelle n'a pas ce mapping : il faut le transporter une fois
avec l'index, pas le remplacer silencieusement par le rang spatial.

L'arène ne doit pas être suivie des anciens masques/flags/positions de
taille P. Prévoir des vagues complètes de tâches à mémoire bornée, puis
un compactage stable ou un retour explicite à l'ordre avant S3. Une vague
est un découpage mémoire, jamais une troncature de recherche. Conserver
P logique, E réellement testé, masses par voie et S survivants séparés.

Ces travaux préparent la parallélisation. Ils ne réduisent pas encore les
certificats S3, les lanes, le catalogue ou les images FULL. La dernière
référence G4 reste celle déjà publiée, autour de 923 ms K5 à chaud sur
00 sans sol ; les 100 ms restent ouverts.
