# Documentation MorseHGP3D

Ce dossier est le corpus scientifique actif de E-HGP. Il a été recentré sur la voie 3D : reconstruire une tour hiérarchique K-NN exacte jusqu'à $K_{\max}=10$, avec une implémentation future adaptée au GPU. Les textes décrivent séparément ce qui est démontré, ce qui dépend d'une hypothèse et ce qui reste heuristique.

Deux cas d'usage gouvernent toutes les décisions : p95 `warm_e2e` inférieur à une seconde autour de 50 000 points pour $K_{\max}\leq10$, et traitement transactionnel reprenable de dix millions de points ou davantage. Le chemin produit est GPU-first avec recertification exacte; Geogram ou toute bibliothèque mature équivalente reste une baseline de test versionnée, jamais une raison d'étendre un oracle maison ni une autorité scientifique sans rejeu.

## Parcours recommandé

1. Lire obligatoirement les Parties I et II du [manuscrit de thèse](references/MANUSCRIT_THESE_HAUSEUX.pdf), pages PDF 35 à 134, pour garder la requête hiérarchique — et non la mosaïque d'ordre supérieur — comme objet algorithmique.
2. Lire la [spécification MorseHGP3D](SPECIFICATION_MORSEHGP3D.md) pour fixer l'objet et les deux profils de sortie.
3. Lire la [définition HGP 3D](math/DEFINITION_HGP_3D.md) pour relier les multicovertures aux K-polyèdres du manuscrit.
4. Lire le [catalogue critique 3D](math/CATALOGUE_CRITIQUE_3D.md) pour la caractérisation Morse et l'énumération sans mosaïque matérialisée.
5. Lire l'[audit de la tour globale de boules saturées](math/TOUR_BOULES_SATUREES.md) pour distinguer cette représentation exacte de la voie Morse tronquée et de ses limites pratiques.
6. Consulter les [attaches par miniball](math/ATTACHES_DESCENTE_MINIBALL.md) pour le profil complet et le contrôle croisé.
7. Vérifier le [registre des preuves et heuristiques](math/STATUT_PREUVES_ET_HEURISTIQUES.md).
8. Lire l'[architecture GPU G4](GPU_G4_ARCHITECTURE.md), puis la [feuille de route d'implémentation](ROADMAP_IMPLEMENTATION_MORSEHGP3D.md).
9. Utiliser le [plan de tests](TEST_PLAN_MORSEHGP3D.md) comme contrat de réception et le [registre des phases](implementation_status.toml) comme état opérationnel.
10. Consulter les [contrats de données de phase 0](contracts/README.md) avant de produire ou de consommer un certificat sérialisé.

## Sources fondatrices

Le [manuscrit de thèse](references/MANUSCRIT_THESE_HAUSEUX.pdf) est la source normative pour :

- l'équivalence entre K-polyèdres de Čech et amas discrets de forte densité K-NN;
- la suffisance des adjacences élémentaires;
- la nécessité de Gabriel pour les simplexes séparants;
- la revendication de préservation des K-polyèdres non triviaux par le K-graphe de Gabriel et un K-arbre minimum couvrant, désormais contredite en général par une fixture exacte du dépôt.

La théorie de Morse de la distance K-NN fournit la caractérisation locale des centres critiques et de leur indice. Les articles sur les mosaïques d'ordre supérieur guident l'énumération, mais la structure combinatoire complète n'est pas matérialisée. L'article GPU sur les diagrammes de puissance guide une baseline de proposition pour l'oracle cellulaire borné; il n'est ni un certificat numérique, ni la primitive du chemin produit direct. Le [répertoire des références](references/README.md) distingue les PDF locaux des liens externes et documente les licences.

## Contrats scientifiques

| mot | signification dans ce dépôt |
|---|---|
| `exact` | résultat complet sur le profil annoncé, relativement aux coordonnées d'entrée interprétées exactement |
| `conditional` | événements retournés vérifiés, mais catalogue, attaches ou dégénérescences non fermés |
| `unsupported_degeneracy` | l'entrée sort du domaine exact actuellement implémenté; aucun résultat exact n'est publié |
| `budget_exhausted` | une limite de temps, mémoire ou sortie a interrompu la fermeture |
| objectif | valeur à falsifier expérimentalement, jamais une garantie de complexité |

Le profil `hgp_reduced` conserve toutes les composantes singleton à $k=1$, puis vise les K-polyèdres non triviaux garantis par le manuscrit pour $k\geq2$. Le profil `full_pi0` ajoute les composantes isolées et leurs attaches globales à tous les ordres. Les statuts ne sont jamais convertis silencieusement.

## Hors périmètre actuel

La voie de très grande dimension est différée. La DTM, les lissages entropiques et les index ANN restent documentés comme mécanismes de proposition éventuels, mais ne font pas partie de la définition exacte du backend 3D. Le code historique du manuscrit reste dans [`HGP-old`](../HGP-old/); il n'est ni un oracle numérique ni une dépendance du futur backend.

## Traçabilité

L'[historique condensé](HISTORIQUE.md) explique les suppressions et les changements de direction. Les anciens rapports restent accessibles dans l'historique Git; leurs résultats utiles ont été reformulés ici sous forme de contrats testables.
