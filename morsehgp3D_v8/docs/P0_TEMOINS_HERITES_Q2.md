# Transmettre aux enfants les témoins certifiés du front q2

17 septembre 2026, note du constructeur ; vingt-et-unième tranche implémentée et qualifiée le même jour.
Exploration v8 hors registre, `cpu_reference`, `quantized_u16_input_only`,
`implementation_v8_p0`, `not_claimed`. GCP non utilisé. Périmètre : voie q2
seule, pas q3/q4 ni tour HGP FULL. Défaut du moteur inchangé ; le levier est
une option explicite de `WspdFrontProposals`.

## Où passe le temps après la fenêtre élargie

La [fenêtre élargie](P0_SURPROPOSITION_TEMOINS_Q2.md) a divisé le temps q2
par environ deux hors rangées. Des compteurs de cycles posés sur une copie
jetable du moteur (uniforme, K = 10, s = 8, fenêtre 2K petits facteurs,
mono-fil) répartissent ce qui reste :

| Poste | n = 8 000 | n = 32 000 |
| --- | ---: | ---: |
| Descente du proposeur depuis la racine | 15,3 % | 17,1 % |
| Fenêtre de propositions et tests H | 26,0 % | 25,0 % |
| Reste du front (séparation, scissions, émission) | 4,8 % | 4,8 % |
| Census q2 et collecte des supports | 54,0 % | 53,1 % |

Un profil d'instructions (callgrind, extraits archivés au niveau des
fonctions) attribuait 51 % au filtre du front et faisait de la descente son
premier poste : il la surestime nettement. Les cycles font foi.

À uniforme n = 32 000, K = 10, le front visite 13,3 millions de produits pour
4,4 millions de candidates et 1,18 million de supports. La moitié des produits
sont des scissions internes : leur recherche n'a pas rejeté, et les témoins
qu'elle a certifiés sont jetés. Le [plan de refonte](PLAN_DE_REFONTE.md) nomme
cette perte (§ 3, point 2) : transporter « au plus h_q − 1 identifiants déjà
certifiés du parent vers ses enfants ».

## Théorème et mécanisme

**Théorème H.** Soit z crédité sur le produit A×B : `h_minimum(A.box, B.box, {z}) > 0`.
Pour tous nœuds A' ⊆ A et B' ⊆ B, `h_minimum(A'.box, B'.box, {z}) > 0`, et z
n'appartient ni à A' ni à B'.

Preuve. `h_minimum` est, axe par axe, le minimum exact du produit
(z − a)(b − z) sur les intervalles des boîtes ; les boîtes serrées des
descendants sont incluses dans celles des ancêtres ; le minimum sur une partie
est au moins le minimum sur le tout. Un site de A ou de B annule H en se
choisissant lui-même comme extrémité : il n'est jamais crédité, et A' ∪ B' est
inclus dans A ∪ B.

Mécanisme. Un produit non rejeté transmet à ses deux enfants la liste des
**rangs** crédités : au plus Kmax − 1, donc neuf rangs de 32 bits portés par
valeur dans la tâche, jamais dans l'objet front. L'enfant part de ce compte,
saute sans test tout rang proposé déjà dans sa liste, ajoute ses nouveaux
crédits. Son rejet reste certifié par Kmax rangs **distincts**, tous universels
sur ses boîtes : c'est le certificat historique, obtenu par un proposeur mieux
informé. Le census repart toujours de zéro. Copier le seul compteur compterait
deux fois un même site : c'est le mutant principal de la porte, et il perd un
support sur une fixture gravée de cinq points.

**Domination.** À chaque position du balayage, les rangs certifiés connus avec
héritage contiennent ceux de la fenêtre seule : l'arrêt avec héritage n'est
jamais plus tardif. Par récurrence sur l'arbre des produits, les produits
visités, les rectangles émis et les candidates sont des sous-ensembles de ceux
de la même fenêtre seule ; la masse rejetée est supérieure ou égale ; les
supports sont identiques. Deux énoncés voisins sont **faux** et ne doivent
jamais être gravés : l'ensemble crédité n'est pas un sur-ensemble (l'arrêt
précoce peut retenir un rang hérité à la place d'un rang de la fenêtre), et le
**nombre** de produits rejetés n'est pas monotone (un ancêtre rejeté cache ses
descendants rejetés ; grille 4×4×4, K = 2, fenêtre 2K : 515 rejets sans
héritage, 491 avec, pour une masse rejetée de 1 011 puis 1 099 paires).

Structure. Sous la voie q2 seule, le seuil de recherche est Kmax pour tout
produit et la population extérieure croît strictement vers les enfants : une
recherche sautée n'a que des ancêtres sautés ou diagonaux, une tâche diagonale
n'a que des ancêtres diagonaux. Aucune des deux ne peut recevoir de liste ; le
moteur le vérifie par une erreur logique plutôt que par un chemin mort. Le
travail d'un produit ne dépend que de sa tâche : compteurs identiques en mono,
pour tout plan de jobs et tout dispatch, quel que soit le nombre d'ouvriers.

## Options, refus et compteurs

```text
WspdFrontProposals{window_factor, small_factor_limit, inherit_witnesses}
```

- Défaut `{1, max, false}` : moteur de la tranche 20 compteur pour compteur,
  ce que juge la campagne différentielle contre le build épinglé
  `v8_front_proposals_20260917` pour les trois fenêtres (front, census, frère,
  ordre, conjoint, Pool, rappels, condensé et plan de jobs). Une seule
  grandeur déterministe change, pour toute option : la tâche du front passe de
  32 à 72 octets, donc le stockage des jobs (544 puis 1 224 octets pour 17
  jobs). Ce transport a un coût sur le chemin par défaut : un microbanc de
  relecture, front seul contre la bibliothèque épinglée, donne environ +3 %
  d'instructions et +2 à +3,5 % de temps mur à n = 8 000, dans le bruit sur la
  chaîne q2 complète ; la campagne différentielle en donne la mesure close.
- `inherit_witnesses` exige le mode `MidpointSamples` et la voie q2 seule
  (masque actif égal à 1), comme la fenêtre élargie : aucun juge indépendant
  des bornes Ξ n'existe encore. La liste porte des rangs ; un niveau par rang
  suffira au port q3/q4, les crédits étant emboîtés (q4 ⊂ q3 ⊂ q2). Refus
  au-delà de 2^32 sites, les rangs étant stockés sur 32 bits : seul le
  prédicat `wspd_proposals_fit` est jugé, aucune porte ne pouvant construire
  un tel nuage ; son site d'appel dans le front ne l'est que par relecture.
- Cinq compteurs, nuls sans l'option et fusionnés par `merge_work` sous la
  garde statique de taille : crédits reçus (somme des longueurs de liste sur
  les recherches), rangs reçus proposés à nouveau, part de ceux-ci dans les
  intervalles d'extension, rejets prononcés alors que nouveaux crédits et
  rangs revus restaient sous Kmax, crédits finaux des rectangles émis.
- Théorèmes de registre, vérifiables sur un reçu sans aucun juge : sites
  proposés = rangs dans les facteurs + rangs reçus revus + tests H (sur la
  seule extension, faute de compteur de tests H propre, seule l'inégalité
  crédits + rangs revus ≤ rangs proposés − rangs dans les facteurs est
  observable) ; crédits reçus pairs et au plus
  (Kmax − 1) × recherches ; **nouveaux crédits + crédits reçus / 2 = Kmax ×
  produits rejetés + crédits des rectangles émis**, chaque produit cherché étant
  rejeté, émis ou scindé, et ses crédits reçus une fois par chacun de ses deux
  enfants. Le compteur de rejets majore le contrefactuel vrai (rejets que la
  fenêtre seule n'aurait pas obtenus), que seul le rejeu indépendant calcule.

## Juges

- Rejeu C++ indépendant du front q2 avec héritage (descente racine, listes
  dans un vecteur croissant, H strict par force brute sur les 64 paires de
  coins), comparé au moteur compteur par compteur et rectangle par rectangle ;
  il calcule aussi le contrefactuel vrai.
- Modèle Python indépendant, troisième implémentation écrite par un relecteur
  de la réfutation de conception : douze lignes de compteurs gravées.
- Oracle de sûreté par force brute : toute paire absente de la couverture
  résiduelle possède au moins Kmax sites strictement intérieurs.
- Largeur des rangs stockés. Tous les autres nuages des portes ont au plus 128
  sites : un moteur tronquant ses rangs à 8 bits les passait tous et perdait
  des supports dès n = 300 (relecture indépendante). Un nuage gravé de 320
  sites, jugé par le rejeu et par l'oracle, avec un plancher sur le plus grand
  rang reçu reproposé, le tue. La frontière des 16 bits demande plus de 65 536
  sites : elle est jugée hors CTest par la campagne `frontier` (n = 70 000),
  où l'empreinte de supports d'une exécution avec héritage doit égaler celle
  de sa jumelle ; le mutant 16 bits y perd des milliers de supports.
- Dix mutants causaux de l'héritage, tous tués ; les quatre premiers sont non
  sûrs et doivent chacun perdre un support que le moteur conserve : compte sans
  identifiants ; déduplication dans la seule fenêtre historique ; liste
  rémanente du front lue par le second enfant ; liste rémanente transmise après
  une recherche sautée ; doublon non compté comme proposé ; nouveaux crédits non
  transmis ; liste tronquée d'un rang ; doublons d'extension non distingués ;
  registre d'émission sans les rangs reçus ; rangs reçus transmis mais ignorés
  par le seuil.
- Fixtures nommées de cinq points, K = 2 : rejet par la seule union d'un rang
  reçu et d'un rang neuf ; témoin unique reproposé (profondeur 1, la paire doit
  rester) ; compteur de rejets strictement supérieur au contrefactuel vrai.
- Portes jobs et dispatch : registre de crédits par job, qui restitue les rangs
  reçus par la tâche racine de chaque job ; listes non vides à travers les
  graines et à travers la file de donation, de façon déterministe.
- Porte des reçus : lecteur attaqué sur de vraies captures, y compris ses deux
  branches qu'aucune capture de fumée n'atteint, le plancher des tailles
  d'intérêt (lignes réelles à n = 256 lues comme une capture d'échelle) et le
  différentiel en septuplets (sondes du même build en doublure du build
  épinglé : c'est le lecteur qui est jugé, pas le moteur précédent) ; un mutant
  à message vérifié par condition de domination et par théorème de registre,
  chacun devant s'appliquer au moins une fois.
- Cinq entrées q2 contre l'oracle force brute, supports égaux.

## Ce qui n'est pas retenu

Mesures de cadrage archivées avec les reçus de la tranche (dossier `cadrage/` :
prototype, moteur à deux leviers en patch sur `8190e7ab`, sorties brutes). Ce
ne sont pas des qualifications. Compteurs déterministes ; toute variante qui
rend un résultat rend l'empreinte de supports de la référence. Temps sur
machine partagée, rapportés à la fenêtre 2K petits facteurs : deux passes
consécutives par variante pour le prototype (`measure_variants`,
`measure_matrix`), trois répétitions entrelacées pour le moteur réel
(`resume_measure`).

| Variante | Source | Temps q2 | Candidates | Verdict |
| --- | --- | ---: | ---: | --- |
| Réutiliser le pivot du parent sans redescendre | prototype | ×0,94 à ×6,00 | ×1,04 à ×23,0 | Écarté : le recentrage est essentiel |
| Le réutiliser si le parent a au moins un crédit | prototype | ×0,91 à ×1,16 | ×1,00 à ×1,97 | Écarté |
| Sauter la recherche des produits de masse 1 | prototype | ×0,88 à ×0,93 | ×1,00 à ×1,25 | Arbitrage front contre census, non instruit |
| Reprise exacte de la descente | moteur réel | ×0,94 à ×0,98 | ×1,00 | Exacte, mesurée, non portée |
| Témoins hérités | moteur réel | ×0,87 à ×0,95, rangées ×1,01 | en baisse | Porté |

Le saut des produits de masse 1 échange des tests du front contre des candidates
du census ; le prototype y comptait mal ses recherches et la sonde a refusé
trois de ses lignes, conservées telles quelles dans l'archive. Piste non
instruite, à mesurer à part si elle doit l'être.

La reprise exacte mérite sa ligne. Les boîtes de deux frères de l'index sont
disjointes ; tout nœud dont la boîte fermée contient le milieu est donc sur le
chemin glouton de la racine, et reprendre la descente depuis le premier
ancêtre de la feuille pivot du parent qui contient le milieu donne le **même
pivot**. Les pas de descente tombent à 17 à 38 % de la référence, tous les
autres compteurs sont identiques. Le prototype la mesurait à ×0,89 à ×0,92
(et ×0,85 à ×0,92 pour sa variante « dernier chemin ») : ces rapports sont
**remplacés** par la mesure du moteur réel, ×0,94 à ×0,98, aux mêmes compteurs
de descente. La référence du prototype, instrumentée, est plus lente que le
moteur de 11 à 12 % (2 559 contre 2 301 ms à uniforme 8 000, 13 545 contre
12 046 ms à 32 000) et ses pas de descente portaient un surcoût propre : la
reprise y retirait 204 ms, elle n'en retire que 46 au moteur. Hypothèse, non
mesurée : les pas épargnés sont ceux du haut de l'arbre, en cache et bien
prédits, et la remontée paie un test de contenance par niveau. Pour ce gain,
quatre compteurs, un lien de parent dans l'index et des mutants que seuls des
compteurs peuvent tuer : non porté. À réévaluer à grande échelle, où l'arbre
s'approfondit.

Deux autres pistes restent fermées par leur coût. La descente exacte plafonnée
à K coûte 38 à 91 bornes conjointes par rectangle (audit du plafond) : sur les
rectangles restants elle paierait autant que le census qu'elle évite. Le
certificat de bloc du chemin est dominé par la fenêtre 2K.

## Résultats clos

Onze captures closes, 854 mesures, lectures normal/−O identiques : lire les
[reçus](../receipts/q2_front_inheritance_20260917/README.md). Temps mur du
pipeline q2 complet, exécution avec héritage rapportée à sa **jumelle** de
même fenêtre, 13 configurations par famille à n8k/16k/32k (K5/10, s8/10/12,
un et quatre workers) :

| Famille | Fenêtre historique | 2K petits facteurs | 4K tous produits | Candidates à 2K |
| --- | ---: | ---: | ---: | ---: |
| Uniforme | ×0,50 à ×0,63 | ×0,86 à ×0,93 | ×0,93 à ×0,96 | 76 à 82 % |
| Amas | ×0,57 à ×0,72 | ×0,88 à ×0,96 | ×0,94 à ×0,99 | 79 à 85 % |
| Terrain | ×0,68 à ×0,75 | ×0,89 à ×0,96 | ×0,93 à ×1,11 | 84 à 88 % |
| Rangées | ×0,94 à ×1,04 | ×0,99 à ×1,03 | ×0,95 à ×1,07 | 100 % |

Héritage et fenêtre élargie se recouvrent : l'héritage rend le plus à la
fenêtre historique, la fenêtre 2K ayant déjà trouvé une partie des mêmes
témoins. Ensemble, à 2K petits facteurs, le temps q2 rapporté au front
historique vaut ×0,35 à ×0,47 (uniforme), ×0,43 à ×0,56 (amas), ×0,58 à ×0,65
(terrain) ; à uniforme 32k, K = 10, 29,0 s deviennent 12,1 s puis 10,5 s. Le
moteur sans héritage égale le build épinglé de la tranche 20 sur les douze
septuplets différentiels, en temps ×0,96 à ×1,03 (observations uniques).

## Limites

C'est une constante, pas un exposant : la croissance du travail restant est
celle de la référence. Les rangées n'y gagnent rien. Des témoins universels y
existent pourtant, entre sites d'une même rangée, et des rangs y sont hérités ;
mais la fenêtre de l'enfant les repropose presque tous, aucun produit
supplémentaire ne tombe (produits visités, masse rejetée et candidates
identiques à la jumelle), seuls des tests H sont épargnés, et les candidates
qui restent sont les paires entre rangées, sans aucun témoin universel : le
[README des reçus](../receipts/q2_front_inheritance_20260917/README.md) en
donne les compteurs. La tâche du front passe de 32 à 72 octets pour toute
option. Aucune borne sous-quadratique générale, aucun contrat FULL ni G4 n'en
découle.
