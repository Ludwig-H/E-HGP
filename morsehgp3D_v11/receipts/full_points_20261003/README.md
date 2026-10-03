# FULL → hiérarchie de points : audit et expérience

Cadre : `phase=exploration_v11_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u21_input_only`, `public_status=not_claimed`.
Source documentaire a45daff3a, moteur c40f40798 inchangé. Retour côté auditeur.

La proposition est la fermeture d'équivalence des couvertures FULL ayant au
moins m sites distincts. Elle conserve toutes les incidences dynamiques et
publie les plateaux fermés. m est un seuil de transmission, distinct de la
condensation. [Preuve, optimum limité et concession](qualified_proof/README.md).

Les trois vérifications indépendantes sont déjà closes :

- [Projection des fixtures existantes](projection/README.md) : 3415 contrôles,
  normal/−O identiques ; ledger e13da1ed567c71c731feaa8e47f5e3836c203cfa6b74caca0e2daea78c140107.
- [Équilatéral exact dans un plan de R³](equilateral/README.md) : 618 contrôles,
  transformations et renumérotations ; ledger 6c46b2ba744548c4e260d8514829e73ddf228da65253da1d651d60f76ee8671b.
- [Couverture qualifiée](qualified_proof/README.md) : oracle Gram/Fraction et Γ
  sans import du produit, cinq fixtures, verticalité et minmax ; ledger
  50e78cb49fb3ef541608aae434a2e45249bfe628d521ef6e98e788e346eb0e0a.

Le contrôle d'intervalles `check_intervals.py` ajoute une route géométrique
indépendante sur 121 nuages alignés : 2180 appariements perturbés, invariances
et ultramétrie. Il ne remplace pas une qualification native en dimension3.

L'expérience [préenregistrée](experiment/campaign.json) compare core, première
couverture canonique, LCA de toutes les premières attaches ex æquo et
couverture qualifiée à HDBSCAN sklearn officiel. Douze synthétiques de quatre
familles, graines9331/9332/9333 nouvelles, et cinq scènes Zoltan entières :
67 114 /76 011 /44 339 /126 267 /72 426 sites, quinze objets suivis.
Les paramètres « hard » hérités sont géométriques ; leur nom ne certifie pas
la difficulté. Aucun seuil n'est ajusté après lecture des objets.

Les données réelles restent hors Git. `inventory.json` décrit leurs empreintes
et masques ; le préparateur conserve les IDs de retours originaux et tous les
sites. Les cinq scènes appartiennent à quatre trames de la séquence08.
Tous les producteurs reçoivent les mêmes coordonnées u21 sur grille1mm.
Les void participent à la géométrie/seuil, mais sont exclus du dénominateur
IoU comme dans Zoltan. Les autres objets/background y participent.

La sonde `points_probe.cpp` exporte depuis le moteur FULL inchangé les
populations fortes complètes et leurs propriétaires fermés, plus core/1ère
couverture. Ce n'est pas un module livré ; ses compilations et vérifications
natives doivent passer uniquement sur G4 gardée. Aucun gain de temps ou
contrat points n'est déduit du coût Python/export. La stabilité perturbative
ne certifie pas l'absence de percolation ni une consistance statistique.

Le consommateur binaire/Python passe **10091 gardes sur32cas**, dont les
28fixtures unitaires distinctes de la référence, contre-exemples et équilatéral
exact, avec3334 comparaisons de coupes ; normal/−O identiques (SHA
f41915a29a83c180bdbf604b35ae25c7c416cd08fa9cb882ad6b793452c2a02e).
Les sixfixtures pondérées sont exclues de ce domaine de réussite. Le comparateur
HDBSCAN passe12 contrôles de format/plateaux sans fit local.

**État :** preuves et tests Python bornés obtenus ; une première porte native
a réussi, les campagnes comparatives restent à jouer. Les reçus fermés sont
conservés ci-dessous ; chaque tentative a son certificat d'arrêt.
Le meilleur IoU d'un groupe dans une hiérarchie est un diagnostic oracle, pas
la qualité d'une partition automatiquement choisie. Zoltan est un jeu de
développement sélectionné ; aucune supériorité générale n'est revendiquée.

Politique de score : groupes actifs seulement, singletons inactifs exclus.
HDBSCAN a toutes ses feuilles actives à0. K2/m2=SL/2 concerne les partitions
complétées et hauteurs de réunion hors diagonale, pas les dates d’entrée.
Les 42 contrôles simulés du runner vérifient en plus le raccord d’IDs arbitraires,
les permutations natives, la compaction et la comparaison perturbative.

Supplément exact `check_jitter_boundary.py` :8263 gardes normal/−O identiques,
32 perturbations appariées à±1mm par axe,1344 bornes qualifiées sans violation.
Sur ce cas, ABC/DEF et IoU restent identiques ; les premières attaches peuvent
changer leurs branches. Cela ne prouve pas de stabilité générale d'IoU.
2478 contrôles illustrent les identités m≤k et `(k,k+1)=(k+1,k+1)` ; la preuve
Γ est dans le script et les JSON, et une chaîne réfute l'extension à m>k+1.

La [première session](sessions/points_synth1/README.md) conserve le défaut de
raccord initial : sonde compilée, pas de test natif commencé, sortie par commande
confondue avec stockage partagé. Arrêt certifié. Le raccord corrigé utilise
`{build}` et refuse toute campagne sans reçu de porte réussi au même binaire.
Les50 contrôles simulés r2 vérifient aussi ce refus ; aucune campagne réelle
n'est déclarée obtenue par ce premier essai.

La [deuxième session](sessions/points_synth2/README.md) réussit la porte native :
39 cas, 9379 contrôles contre les oracles A/B, sans changement du moteur c40.
La campagne échoue avant génération, car pip manque sur la VM. Une préparation
d’hôte gardée distincte est prévue ; ces essais ne constituent aucun résultat
comparatif sur les synthétiques ou Zoltan. La porte suivante ajoute un
cuboctaèdre de12sites/K10 : le dk10²=6 est hors du catalogue, de racine β=2.

La [préparation d’hôte](sessions/points_python1/README.md) est maintenant close :
pip prêt, sept phases jointes, DONE0 et arrêt ciblé certifié. Les47 contrôles
simulés de ses gardes passent en normal/−O ; ils ne remplacent pas cette
fermeture G4. Les installations scientifiques des campagnes restent privées
et épinglées par le worker.
