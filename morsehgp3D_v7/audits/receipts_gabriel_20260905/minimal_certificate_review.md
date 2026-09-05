# Certificat final minimalement suffisant : portée et précisions

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. Relecture documentaire indépendante ; aucun moteur, build, Git ou GCP. Les [pins](minimal_certificate_pins.json) attribuent les versions lues ; l'[extrait du manuscrit](minimal_certificate_manuscript_pdf122_126.txt) couvre §9.1 et Algorithme 1, PDF 122–126.

**Le certificat proposé au §6 de la [note constructeur](../../docs/AUDIT_NIVEAUX_GABRIEL_20260905.md) suffit pour les composantes abstraites réduites, leur généalogie et leurs couvertures en points.** Il n'est pas nécessaire de conserver toutes les facettes, cofaces ou descentes temporaires pour rejouer cet objet. Cette suffisance est conditionnelle à une autorité géométrique de fidélité et de complétude ; le schéma seul ne la démontre pas. Elle ne constitue pas une preuve d'optimalité en bits.

## Informations et invariants à rendre explicites

| Élément final | Détail nécessaire au rejeu |
| --- | --- |
| Manifeste | Entrée et domaine des PointId, métrique/unité, ordres réellement disponibles, objet réduit déclaré, version et autorité terminale. Distinguer ordre vide et ordre absent. Déclarer une éventuelle borne de filtration ; une sortie complète sans borne doit certifier que le prolongement après son dernier événement est constant. |
| Racines K1 | Un singleton par PointId à `0+`, explicitement ou par une règle normative liée au manifeste. Aucune racine réduite initiale aux ordres supérieurs. Le label isolé terminal de Gamma complet n'est pas ajouté silencieusement au domaine réduit. |
| Niveaux | Valeur exacte commune `β=ρ²` et convention de coupe ouverte/fermée. Des fractions égales doivent appartenir au même lot, même si leurs encodages bruts diffèrent. Une densité normalisée dépendant de K n'est pas une horloge verticale commune. |
| Naissance | Identifiant nouveau, zéro parent, niveau et ensemble de points couverts. Une première apparition dans une vue filtrée ne vaut pas naissance de l'histoire complète. |
| Multifusion | Identifiant nouveau et au moins deux parents distincts, actifs strictement avant le niveau. Les groupes du lot ne peuvent consommer deux fois le même parent. Tous les liens du lot sont contractés atomiquement, sans pseudo-naissance intermédiaire. |
| Continuation utile | Identifiant de la composante persistante, niveau et points ajoutés. Une continuation sans point nouveau peut disparaître de ce certificat. Si l'encodage change néanmoins d'identifiant, il doit conserver un lien de succession. |
| Delta de points | Points du domaine déclaré, sans doublon et nouveaux relativement à l'union des couvertures parentales. Le même PointId peut appartenir à plusieurs composantes : cette répétition entre branches est légitime. |

Le rejeu applique chaque lot à l'état strictement antérieur, retire ses parents consommés, installe ses sorties et prend les unions de couvertures. Les continuations complètent l'ensemble attaché à une composante sans changer son identité abstraite. Cette induction reconstruit les deux côtés de toute coupe, et l'état reste constant entre événements conservés. Il faut donc bien conserver les continuations qui gagnent des points : dans E5, elles portent A puis B, même en l'absence d'une naissance ou d'une multifusion.

Les identifiants de composantes sont distincts des PointId. Ni égalité ni recouvrement des ensembles de points ne décide leur identité. Leur représentation canonique n'est pas une nécessité mathématique ; elle devient un contrat supplémentaire si les identifiants ou digests publics doivent être reproductibles à l'octet.

## Une ancre par naissance suffit pour la tour réduite

Le [contrat vertical](../CONTRAT_VERTICAL_COURANT.md), §§3–5, fournit exactement le complément voulu : une ancre certifiée pour chaque vraie naissance supérieure, vers l'ordre adjacent inférieur à la même coupe fermée. L'identifiant cible peut être historique si une succession permet sa normalisation.

Aux continuations, propager cette ancre ; aux multifusions, normaliser les images des parents **après fermeture du lot inférieur au même niveau**, puis exiger une cible commune. Les lots propres à chaque K ne sont pas une horloge globale. La naturalité permet ensuite toutes les consultations et compositions d'ordres. Il n'est pas nécessaire de sérialiser une carte par coupe, ni une nouvelle ancre à chaque première matérialisation ou gain de point. Les facettes témoins et les chemins peuvent rester des preuves de construction séparées de la sortie de rejeu.

## « Les K hiérarchies » doit nommer l'objet demandé

Ce certificat restitue un arbre réduit décoré par des ensembles de points, et avec les ancres une tour de tels objets. Il ne restitue pas les composantes comme ensembles de facettes identifiées, leurs adjacences, le carrier géométrique marqué, les régions de multicoverture ou toutes les dates d'activation des feuilles. Le §9.1 présente aussi la hiérarchie comme une partition de facettes : demander cette représentation plus riche exige son supplément d'appartenance. Cette réserve ne justifie pas de reconstruire Gamma complet pour le seul arbre avec couverture.

## Masses : respecter l'univers Gabriel et fixer le temps d'affectation

L'Algorithme 1, PDF 126, est explicite pour K2 : ses triangles Gabriel définissent le catalogue C, ses sommets sont **toutes** leurs arêtes-facettes F, les scores sont agrégés à la ligne 5, avant le MST de la ligne 6. Une facette de F n'a pas à être elle-même Gabriel. Garder seulement les contributions retenues par le MST perd des scores ; ajouter les cofaces auxiliaires de descente les change. Généraliser ce profil à chaque K doit déclarer le catalogue Gabriel correspondant, sans lui substituer toutes les cofaces Čech.

Le [contrat pondéré](../CONTRAT_MASSES_VOTE_COURANT.md) distingue à raison ce catalogue, la fonction de poids et les incidences. Pour un profil fixé, scores par facette et incidences point–facette suffisent aux masses et votes ; un histogramme par facette/niveau permet de changer ultérieurement la fonction de poids. Un profil utilisant le rayon ne doit pas être évalué directement sur son carré.

**Précision normative encore nécessaire :** §9.1 et l'Algorithme 1 ne fixent pas explicitement une politique « affecter une feuille à sa première incidence Gamma ». Le pseudocode construit d'abord l'ensemble global F, puis son graphe pondéré/MST. Attribuer cette politique temporelle précise au manuscrit serait excessif ; fixer une réserve de feuilles puis ses transferts est un choix contractuel à déclarer.

Si cette politique de première incidence Gamma est retenue, E5 exige le transfert de la masse positive de AC à `33/2`, même si aucun point ni nœud réduit ne change alors. Le reporter à la directe ABC de niveau `83886/3563` change la courbe de masse. Le seul certificat topologique ne suffit donc pas à promettre une condensation identique. Pour les seules masses à toute coupe, un journal supplémentaire de transferts scalaires datés vers les composantes peut remplacer les feuilles détaillées. Des votes arbitraires demandent en outre l'information de répartition de cette masse par point et par groupe effectivement étiquetable ; une masse scalaire ou une couverture binaire ne la détermine pas.

Le contrat final peut ainsi rester plus petit que Gamma : choisir précisément les observables à reconstruire, puis conserver leurs deltas irréductibles. La découverte certifiée des rattachements et le coût des requêtes demeurent des obligations du constructeur, distinctes de la taille du certificat final.
