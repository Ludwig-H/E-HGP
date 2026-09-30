# Bande à K fixé : supprimer les ancêtres témoins redondants

Résultat : **la réduction à l'antichaîne minimale conserve couverture,
laminarité en rayon et monotonie avec η**, sous le protocole actuel et à
univers fixe. Elle ne retarde jamais l'entrée par rapport au bras initial.
Cette note et son prototype sont un reçu d'audit ; aucun moteur, catalogue,
bras développeur, note active ou build n'est modifié.

## Définition et préconditions

Fixer K, le nuage, son univers de témoins et la forêt FULL. Pour x, noter
`a = α_K(x)²`, `B_η = (1+η)² a`. Chaque témoin sélectionné est un triplet
`(boule, β, v)`, avec `a ≤ β ≤ B_η`, x dans sa boule fermée et **v vivant
après toutes les activations du plateau β**. L'univers géométrique contient
toutes les incidences I/U des boules de population ≥ K et `p+q_min ≤ K`.
Les événements FULL de fusion, notamment `p+q_min = K+1`, restent dans la
forêt ; la réduction ne supprime ni boule ni nœud du moteur.

Dans l'ensemble S des nœuds sélectionnés distincts, garder A, les nœuds
sans descendant strict également sélectionné. Calculer `J = LCA(A)`, puis
`e = max(a, naissance(J))` et son ancêtre vivant à la coupe fermée e.
Fixer cet ancrage une fois : x est un singleton avant e, puis suit seulement
l'ascendance. Les nœuds internes ont les mêmes droits que les feuilles.

## Preuve générale

1. **Première couverture préservée.** Tout nœud f témoin à β = a reste dans
   A. Si un descendant strict d de f était vivant à une date β ≥ a, f serait
   déjà né à β et d aurait quitté sa branche vivante : contradiction. Les
   nœuds distincts vivants à une même coupe fermée sont incomparables.
2. **Couverture à l'entrée.** J est donc ancêtre d'au moins un témoin de
   première couverture. À e ≥ a, la composante vivante obtenue contient
   celle qui couvrait x à a ; la croissance des boules conserve sa couverture.
   Le témoin plus tardif éventuellement ignoré n'a pas à être actif à e.
3. **Emboîtement en rayon.** L'ancrage fixé ne fait que monter dans FULL.
   Les singletons avant entrée et les points déjà entrés suivent donc des
   partitions emboîtées. Aucun vote ni nouveau choix de propriétaire à une
   coupe ultérieure n'est nécessaire.
4. **Monotonie avec η.** Quand la bande s'élargit, chaque nouveau témoin
   a une date strictement supérieure à l'ancienne borne B. Il ne peut être
   descendant d'un ancien nœud sélectionné vivant à sa propre date ≤ B.
   Les anciens minima restent donc minima ; les ajouts sont soit des ancêtres
   redondants, soit de nouvelles lignées incomparables. A ne décroît pas,
   J monte et e ne diminue pas. Après l'entrée du bras de bande plus large,
   les deux bras suivent le même ancêtre du même témoin initial ; avant cette
   entrée, le bras plus large laisse des singletons. Ses partitions raffinent
   donc celles du bras de bande plus étroite.
5. **Comparaison au bras initial.** Le LCA de S est ancêtre du LCA de A.
   Ainsi `e_réduit ≤ e_initial`. À coupe commune, le bras initial raffine
   le réduit ; dès l'entrée initiale, les deux ancrages ont la même ascendance
   vivante. À η = 0, aucun ancêtre distinct ne peut être vivant avec son
   descendant au même plateau : la réduction reproduit exactement le bras
   initial, donc son comportement de première couverture.

La preuve suppose des parents de dates non décroissantes, des témoins
vivants aux dates propres, toutes les incidences et un univers figé. Elle
ne prouve aucune continuité d'une projection dure lorsque les coordonnées,
le catalogue ou l'univers changent. Le protocole reste propre à un K fixé.

## Exemple géométrique au η par défaut

K2, trois sites entiers `(0,0,0)`, `(8,0,0)`, `(0,3,0)`, x = `(8,0,0)`.
Les trois boules diamétrales ont β = `16`, `9/4`, `73/4`. La dernière a
les trois sites sur sa coquille, `p=0`, `q_min=2` : elle appartient bien à
l'univers fort K2. À son plateau, son centre est dans la racine FULL.
La première couverture de x vaut a = 16 ; la bande η = 1/8 va jusqu'à
`81/4`, donc sélectionne son premier nœud et la racine tardive.

Le bras initial entre à `73/4`. L'antichaîne ignore la racine, déjà représentée
par le premier nœud, et entre à **16**. Avant la fusion `73/4`, x n'est jamais
couvert par la branche issue de la paire verticale : l'oracle Γ indépendant,
avec toutes les paires et leur unique union de trois sites, le vérifie. Cet
exemple montre un retard sans concurrent couvrant indépendant, avec un vrai
nœud de fusion binaire et une vraie coquille, sans subdivision artificielle.

Le second triangle `(0,0)`, `(2,0)`, `(0,3)`, η = 1, conserve une ambiguïté
réelle au sommet de l'angle droit : ses deux premières lignées restent dans
A et son entrée reste `13/4`. Les deux autres entrent à `1` et `9/4`, au
lieu de `13/4`. Des dates d'entrée plus précoces rendent de la masse disponible
plus tôt ; elles ne garantissent pas davantage de clusters. Sur ces petits
triangles, les partitions de points peuvent encore coïncider avant fusion.
Condensation/EOM, rappel et qualité doivent être comparés séparément.

## Vérifications bornées et coût

`check.py` passe normalement et avec `-O`, résultats identiques :

- 482 contextes abstraits, 2 892 bandes, arbres binaires, continuation unary,
  dates simultanées, ordre des témoins inversé et doublons. L'antichaîne par
  intervalles Euler est comparée à un oracle pairwise indépendant.
- Six exports natifs, 507 contrôles de coupe : **88 incidences fortes complètes**,
  deux fixtures de couverture interne K3/K5, deux quasi-égalités K3 et les
  deux triangles. K3/K5 gardent leurs propriétaires internes aux dates 25 et
  105625 ; les quasi-égalités conservent leurs deux lignées et leur attente.
- Normalisation du plateau fermée, couverture par ascendance initiale,
  comparaison au bras initial, emboîtement en rayon et raffinement avec η.
  Un enfant dont le parent naît au même β est refusé comme témoin vivant.

Les quatre exports K3/K5 sont relus depuis leurs captures scellées, sans
réexécuter le moteur. Les deux triangles ont chacun une exportation u18 de
trois sites avec le binaire déjà capturé, sans compilation. Leurs reçus
ferment 433 dépendances avant/après et héritent du reçu de compilation
développeur. Les scripts, inputs et sources consumer sont hachés avant/après
aux deux lectures. Le premier passage η = 1 est conservé historiquement.

Le prototype prépare Euler en O(V), puis trie les nœuds sélectionnés de
chaque point. Le retrait des ancêtres est linéaire après le tri : dans l'ordre
préfixe, un nœud a un descendant sélectionné exactement lorsque le prochain
nœud sélectionné est dans son intervalle. Travail : O(V + D + Σ d_x log d_x),
plus au plus D requêtes LCA et leur coût ; D compte **toutes** les incidences
parcourues, d_x celles sélectionnées pour x. Hors univers D préexistant,
le prototype conserve les A pour l'audit : mémoire additionnelle
O(V + n + Σ |A_x| + max d_x). Une version streaming qui émet l'ancrage et
les compteurs sans conserver les minima peut viser O(V + n + max d_x),
hors structure de requêtes LCA du contexte. Aucune borne de D ni gain de
temps/mémoire LiDAR n'est acquis.

**La bande complète doit être fermée avant décision**, jusqu'à B_η, même si
l'entrée mathématique réduite e est plus ancienne. Il existe toujours un
look-ahead : ce n'est pas un algorithme causal sur le seul préfixe jusqu'à e.
Retard de calcul et date d'entrée mathématique sont des notions différentes.

Pour une forêt partielle à racines distinctes, les témoins de racines
différentes restent incomparables. Il n'existe pas de LCA fini : le prototype
refuse explicitement ce cas. Une extension pourrait déclarer e = +∞ et
laisser x singleton à toute coupe finie, sans fabriquer de fusion ; cette
extension n'est ni implémentée ni validée ici. Une forêt FULL complète d'un
nuage fini, K ≤ n, a une racine commune à rayon fini. Les infinis liés à λ
au rayon nul relèvent encore des conventions de condensation, hors preuve.

Reçu : `normal_receipt.json`, `optimized_receipt.json`, `comparison.json`,
`SHA256SUMS`. Replay consumer portable : `python3 -B check.py` et
`python3 -B -O check.py`, dans une copie de ce dossier avec sorties neuves.
Les captures natives exigent les dépendances explicites de leurs reçus.
Aucune campagne, GCP, qualification statistique, performance ou test EOM.
