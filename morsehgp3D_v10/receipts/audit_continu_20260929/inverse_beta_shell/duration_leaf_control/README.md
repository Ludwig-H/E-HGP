# Contrôle de conception : durée couverte des feuilles de naissance

30 septembre 2026. Ce paquet secondaire n'est pas la preuve principale
inverseβ. Il teste seulement une piste sur quatre nuages de quatre sites.
Pas de moteur, compilation native, GCP, benchmark statistique ou chrono FULL.

À K2, le script énumère Γ exact avec le référentiel Fraction et traite les
plateaux en une seule fois. Les branches sans composante antérieure sont
les feuilles de naissance ; aucune branche ancêtre n'est un nouvel atome.
Pour chaque incidence point x→feuille v, il pose une masse fixe

`w_xv = φ(β_firstcover(x,v)) − φ(β_death(v))`.

Deux fonctions sont testées : `φ=1/r=1/sqrt(β)` et `φ=1/β`.
La première utilise des intervalles dyadiques exacts de 200 bits pour les
racines et vérifie chaque majorité stricte par une inégalité certifiée,
jamais par un arrondi double. La masse de la racine après sa mort à
l'infini serait zéro. Les poids nuls et les rayons nuls ne sont pas couverts
par ce petit contrôle ; le script refuse un cas incertain au lieu d'inventer
un propriétaire.

Les points suivent l'ascendance FULL de leur feuille active. La majorité
stricte et le dénominateur fixe préservent l'emboîtement sur chaque arbre.
Le constructeur Γ conserve toutes les unions de trois points : les
événements de fusion ne sont pas limités au filtre fort de couverture.

## Observations

Les deux tétraèdres du paquet principal, à M=2048, donnent chacun trois
feuilles et cinq nœuds, avec six incidences point→feuille. Pour **les deux
φ**, C et A sont réunis à la naissance de CA : `β=16777216` dans le nuage
rectangle original et `β=67092483/4` après jitter. Le saut parasite de la
normalisation inverseβ par boules n'apparaît pas dans ce petit contrôle.

Les deux paires sur la ligne `{0,1,100,101}`, puis leur variante tétraédrique
`(0,0,0),(1,0,0),(100,1,1),(101,1,2)`, sont aussi récupérées chacune à leur
naissance. La branche du pont a une durée faible et ne pèse pas autant que
la branche locale persistante. Cela corrige ici le retard de la majorité
à poids uniformes. Les traces complètes sont dans les reçus.

Normal et `python3 -O` terminent avec code 0 et les mêmes résultats
sémantiques. Les sources et référence sont hachées avant/après. La portée
du code 0 est huit contrôles nuage×φ réussis, **pas** une qualification
générale de cette projection ou de ses scores.

## Pourquoi ce n'est pas une solution complète

- Certains points peuvent être couverts seulement par une branche interne,
  jamais par une feuille avant sa mort. L'univers proposé n'est alors pas
  complet ; le contrôle refuse de leur attribuer une masse fictive.
- Une branche fantôme qui fusionne dans une branche longue peut couper
  celle-ci en segments. Ignorer tous les ancêtres peut retirer une partie
  longue, même si la durée de la branche fantôme tend vers zéro. Le choix
  feuilles/antichaînes ne bénéficie pas gratuitement d'une stabilité de
  diagramme de persistance.
- Ajouter tous les ancêtres futurs répare cette perte de segments mais peut
  retarder la frontière via le dénominateur. Il faut définir l'unité de
  branche et les incidences une fois, sans doublonner une continuation.
- Un vote dur reste sensible à un seuil de majorité presque égal, même
  avec des poids continus. La preuve de laminarité sur un arbre ne prouve
  ni stabilité sous perturbation ni fidélité au modèle générateur.

La piste à conserver est **la durée effectivement couverte**, indépendante
du nombre de boules, comme diagnostic fractionnaire avant condensation.
Il faut mesurer rappel frontière avant fusion, masse tardive, ghost branches,
cas à couverture seulement interne et coût des incidences. Ce contrôle ne
justifie aucun vaste port ni promesse de battre HDBSCAN.

Rejeu : `python3 -B check_duration_leaf.py ../frozen_reference.py`.
