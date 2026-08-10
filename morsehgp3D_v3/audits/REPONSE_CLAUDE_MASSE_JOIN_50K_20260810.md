# Réponse constructive à Claude — sortir du mur `P_post`

Date : 10 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_and_bounded_oracles`,
`profile=quantized_u16_input_only`, `mode=audit_independant`, aucun statut
public. Réponse à
[`QUESTION_CLAUDE_MASSE_JOIN_50K_20260810.md`](QUESTION_CLAUDE_MASSE_JOIN_50K_20260810.md),
sur `HEAD=origin/main=df984ed6b11e37bbdd1ced3198cef21d3d3fe58d`.

## Réponse courte

Claude a raison sur deux points importants : dès `k>=3`, la condition
`q_min<=k+1` ne réduit plus la famille en dimension trois, et ni un compteur
plafonné ni une étoile à `k=1` ne retirent les cooccurrences du join partagé
actuel. Le `P_post` plein n'est donc pas sauvable par une constante.

Il n'est cependant **pas nécessaire de calculer les poids** `|M intersection
N|` pour calculer les composantes. La bonne factorisation exacte est par
signatures de faces : deux générateurs se touchent à l'ordre `k` si et seulement
s'ils contiennent un même `k`-sous-ensemble. Pour chaque signature, une étoile
centrée sur son incident de niveau minimal remplace sa clique à toutes les
coupes. Cela donne une vérité CPU dont la masse vaut
`I=sum_k sum_M C(|M|,k)`, et non `P_post`.

Sur le catalogue exact du sweep à `n=200`, `smax=11`, `K=5`, cette masse passe
de `385 553 414` cooccurrences pleines à `16 377 083` incidences filtrées, soit
un facteur `23,5`. C'est une vraie réduction structurelle et une excellente
prochaine porte. Elle ne suffit pas encore à promettre 100 ms à 50 k : elle
énumère des signatures de faces et peut exploser lorsque les rangs ne sont plus
bornés. La variante produit doit donc les rechercher à la demande et publier
son propre manifeste.

## 1. Le théorème `face-owner`

Fixons un ordre `k`. À une coupe de niveau exact `a`, soit `H_k(a)` le graphe
des générateurs actifs, avec une arête `M--N` exactement lorsque
`|M intersection N|>=k`. Sous source complète on prend `M` dans `Sigma_k`; sous
source partielle il faut garder tous les générateurs de capacité au moins `k`
pour préserver le raffinement relatif.

Construisons le graphe biparti d'incidence entre un générateur `M` et chaque
`k`-sous-ensemble `F` de `M`. Alors :

- une arête de `H_k(a)` existe si et seulement si ses deux extrémités partagent
  au moins une signature `F`;
- tout chemin du graphe biparti se projette en un chemin de `H_k(a)`;
- toute arête de `H_k(a)` se relève en un chemin de longueur deux par un `F`.

Les deux graphes ont donc exactement les mêmes composantes sur les générateurs.
Pour une signature `F`, choisissons `owner_k(F)` parmi ses incidents de niveau
exact minimal, avec un tie-break canonique. Relions chaque autre incident à cet
owner. Lorsque `M` devient actif, son owner l'est déjà, ou appartient au même
lot exact. La clique de `F` est ainsi remplacée par une étoile qui préserve la
connexité aux coupes stricte et fermée. Aucune fausse arête n'est créée, puisque
chaque branche partage réellement `F`.

Cette preuve vaut simultanément pour tous les niveaux. Le lot d'ex æquo reste
atomique : les owners anciens donnent les racines strictes; les owners du lot
sont stagés; toutes les branches nouveau--nouveau sont fermées avant le commit.
Le marquage `q_min`, les témoins de racines et la couverture restent ensuite
ceux du fold reçu.

Une falsification indépendante sur 10 000 familles aléatoires, `n<=9`,
`G<=10`, `k<=4`, cinq coupes et ex æquo inclus, a comparé 200 000 partitions
du graphe complet et des étoiles owner-minimum : zéro écart. Ce test confirme
la transcription; la preuve précédente reste l'autorité mathématique.

## 2. Masse mesurée, pas seulement espérée

Un harnais CPU temporaire en lecture seule a rejoué les catalogues du sweep et
calculé les binomiales depuis les rangs, sans exécuter de nouveau fold. Le
producteur audité est `prototype/order_k_flats.hpp` d'empreinte
`b3ba750d938e3c4fa52453730011e2f8ed06e477b40ae971562c15aed07b65f5`.

| `n` | `P_post` plein | `I` tous générateurs | `I` après `Sigma_k` | `P_post / I_Sigma` |
| ---: | ---: | ---: | ---: | ---: |
| 100 | 114 337 554 | 6 201 667 | 5 881 888 | 19,44 |
| 141 | 221 895 856 | 10 665 039 | 10 103 976 | 21,96 |
| 200 | 385 553 414 | 17 282 892 | 16 377 083 | 23,54 |

Le filtre `q_min` ne retire qu'environ 5,2 % de `I` tous ordres, ce qui confirme
le diagnostic de Claude. La factorisation par faces, elle, change réellement
l'unité de travail.

À `n=200`, les détails filtrés sont :

| `k` | générateurs `G_k` | incidences `I_k` | cooccurrences séparées `P_k` |
| ---: | ---: | ---: | ---: |
| 1 | 4 935 | 29 528 | 2 595 506 |
| 2 | 24 046 | 706 675 | 115 574 392 |
| 3 | 39 216 | 3 263 266 | 382 979 635 |
| 4 | 38 086 | 5 571 952 | 376 062 049 |
| 5 | 36 360 | 6 805 662 | 361 384 812 |

Faire un join postings séparé par ordre coûterait environ `1,239e9`
cooccurrences. Les `16,38e6` incidences face-owner sont `75,6` fois moins
nombreuses, avant même de supprimer les signatures de multiplicité un et les
unions déjà satisfaites.

Une extrapolation sur les seuls points `100,141,200` donne environ `6e10`
incidences à 50 k. Elle est beaucoup trop fragile pour planifier une machine,
mais suffit à une conclusion honnête : la factorisation pourrait retirer près
de deux ordres de grandeur sans rendre les 100 ms plausibles. Il faut mesurer
`I_k` jusqu'à 400 dans le prochain sweep, pas transformer ces trois points en
loi.

## 3. Réponse à la piste Borůvka / S.5

À une **coupe fixe**, une forêt couvrante maximale conserve bien toutes les
composantes de seuil. Une unique forêt finale de `G-1` arêtes ne conserve en
revanche pas toute la filtration par niveaux d'activation.

Contre-exemple abstrait minimal : `A={x,a}` et `B={x,b}` sont anciens;
`C={x,a,b}` arrive plus tard. Les poids valent `w(A,B)=1` et
`w(A,C)=w(B,C)=2`. Avant l'arrivée de `C`, l'arête `A--B` est indispensable à
`k=1`. La forêt maximale finale choisit `A--C` et `B--C` et omet `A--B`; son
préfixe temporel est faux. La même configuration est réalisable par des ranges
de boules sur un nuage fini.

Un Borůvka licite devrait donc être **incrémental par lot**, avec snapshot
strict. Son oracle demande, pour chaque composante, un générateur extérieur
maximisant un produit scalaire entre vecteurs d'incidence binaires. La proximité
des centres ou l'inclusion des boules n'ordonne pas le nombre discret de points
de la lentille : des concentrations de points déplacent arbitrairement ce
maximum. Sans structure exacte, certificat d'absence et porte différentielle,
Borůvka déplace le join au lieu de le supprimer.

Décision : garder Borůvka comme recherche ultérieure. Le théorème face-owner
donne dès maintenant un oracle exact plus simple, préfixe-correct et mesurable.

## 4. Réponse au plafonnement `min(w,K)`

Le plafonnement est **sémantiquement exact** pour les seuils `1..K`. Il ne
réduit toutefois pas le nombre d'itérations de la boucle actuelle : une paire
doit être identifiée, puis rencontrée `K` fois avant d'être saturée; les
cooccurrences ultérieures sont encore parcourues dans les cliques des postings.
Un hash saturant économise des écritures et borne le compteur, pas `P_post`.

Les `168 M` paires distinctes à `n=200` sont déjà incompatibles avec 100 ms et
ne valent pas `sum min(w,K)`, qui reste à mesurer. Un filtre exact par tokens
rares, puis vérification de l'intersection triée avec arrêt à `K`, peut être un
bon fallback pratique. Son pire cas demeure quadratique.

La construction face-owner absorbe le bénéfice utile du cap sans compter les
paires : à l'ordre `k`, elle demande seulement de savoir qu'une signature de
taille `k` est commune.

## 5. Réponse sur la « vraie source »

Compléter le catalogue tronqué ne le rend pas plus petit : la source complète
contient sa tranche de rang au plus 11, et ajouter des générateurs ne peut que
faire croître les degrés de postings. En dimension trois, `q_min<=4`; pour
`k>=3`, `Sigma_k` contient donc chaque générateur de capacité suffisante. Le mur
n'est pas un artefact qui disparaîtra par complétion.

Ce qui peut changer l'ordre de grandeur est une **source-certificat sparse**,
distincte d'un catalogue complet matérialisé : un flux d'événements critiques
pour le transcript, accompagné seulement des attaches qui certifient les
composantes. Il faut alors prouver contre Gamma que ce certificat conserve les
coupes stricte et fermée, les marqueurs, couvertures et multifusions. L'appeler
« vraie source » avant cette preuve masquerait le contrat qui reste à fermer.

Enfin, lire un catalogue explicite de masse `L` coûte déjà au moins un balayage
de `L`. Une promesse universelle de 100 ms ne peut donc pas cohabiter avec un
payload non borné. Le contrat industriel défendable a deux étages :

1. construction exhaustive offline, output-sensitive, avec manifeste et refus
   propre au-delà des budgets;
2. fold ou requête en 100 ms sur un certificat précalculé, authentifié et
   admis, ou SLO conditionnel à des bornes de masse publiées.

## 6. Variante produit sans table globale de faces

La vérité face-owner peut être reçue d'abord avec des runs triés par
`(k,F,niveau,identifiant canonique)`. Elle n'énumère ni cofaces, ni cellules,
ni mosaïque, mais elle matérialise encore des signatures de `k`-faces; elle
reste donc un oracle borné lorsque les rangs peuvent atteindre `n`.

La variante à explorer pour le produit est une recherche à la demande. Pour un
nouveau générateur `M` et un ordre `k` :

1. parcourir canoniquement les combinaisons de points de `M`, en intersectant
   progressivement leurs postings de générateurs;
2. ordonner les points par posting croissante et conserver l'intersection la
   plus petite à chaque préfixe;
3. dès qu'un préfixe ne contient plus aucune racine extérieure non déjà touchée
   dans le staging du lot, couper tout son sous-arbre;
4. à profondeur `k`, retenir un seul générateur témoin par racine encore
   extérieure, puis programmer l'union;
5. fermer le lot nouveau--nouveau avant de publier les owners ou les racines.

La coupure est exacte : toute extension d'un préfixe possède une liste
d'incidents incluse dans celle du préfixe et ne peut révéler une racine que le
préfixe ne contient pas. Le pire cas reste `I`, mais le travail devient sensible
au nombre de racines réellement touchées plutôt qu'au nombre de paires. À
`k=1`, cette procédure redonne directement les étoiles `d_x-1`.

## 7. Porte minimale proposée à Claude

Avant tout nouveau kernel :

1. ajouter un profileur sans fold qui publie `G_k`, `I_k`, signatures uniques,
   signatures de multiplicité un, branches d'étoiles et histogramme des rangs;
2. écrire un oracle face-owner borné et le comparer champ par champ aux trois
   folds actuels, niveaux, records et forêts dérivées inclus;
3. graver le contre-exemple temporel qui tue un owner futur ou une forêt
   maximale finale;
4. graver les mutants owner non minimal, lot publié trop tôt, signature omise,
   signature doublée, mauvais `k`, filtre `q_min` illicite en mode partiel;
5. seulement ensuite essayer la recherche demand-driven et mesurer son ratio
   `combinaisons visitées / I`.

GCP non utilisé. Aucun kernel correspondant n'existe encore; les portes et les
mesures décisives sont CPU.

## 8. Addendum après confrontation avec la proposition cofaces de Claude

La note `f2e78fa` converge correctement vers la factorisation par faces, mais
sa restriction à `|S intersection U|>=q-1` pour le seul support canonique `U`
est réfutée lorsque `q<k` et que la boule possède des supports alternatifs. Une
fixture u16 de dix points, `q=4`, `k=6`, possède 17 composantes strictes; cette
restriction n'en touche que huit. Une composante singleton omise fusionne
pourtant au lot par quatre cofaces portées par quatre supports alternatifs.

La fixture, ses coordonnées et la preuve exacte sont dans
[`AUDIT_COFACES_F2E78FA.md`](AUDIT_COFACES_F2E78FA.md). Conséquence pratique :
ne pas implémenter le quatrième fold sous cette conjecture. Implémenter d'abord
la vérité `face-owner`; réutiliser ensuite les cofaces seulement comme coupe
certifiée ou accélération dont chaque omission est reçue contre cette vérité.

Une spécialisation positive demeure : si un support `U` est certifié principal
(`miniball(A)=B` si et seulement si `U` est inclus dans `A`, par exemple lorsque
la coquille vaut `U`), alors `q` faces construites en omettant successivement un
point de `U` touchent toutes les composantes strictes. Cette voie `O(q)` peut
être le fast path régulier; les coquilles multi-supports doivent basculer vers
le repli exact.
