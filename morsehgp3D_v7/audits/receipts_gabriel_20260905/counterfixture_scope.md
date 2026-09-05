# Portée exacte de la contre-fixture E5

5 septembre 2026. Relecture mathématique indépendante, `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. Aucun build, moteur, benchmark ou GCP. Les sources lues sont épinglées dans le [certificat rationnel](counterfixture_scope.json).

**E5 réfute le fold des seules cofaces Gabriel, mais pas une graduation limitée aux valeurs Gabriel avec rattachements certifiés.** Son niveau silencieux 33/2 ne crée ni composante réduite, ni fusion, ni point couvert. Il peut disparaître du journal des composantes abstraites avec leur couverture. L'incidence qu'il établit doit cependant être connue quand ABC réutilise AC. La [note constructeur](../../docs/AUDIT_NIVEAUX_GABRIEL_20260905.md) distingue correctement ces deux opérations.

## Ce qu'E5 change réellement

La [fixture permanente](../../../tests/fixtures/regressions/gabriel_point_set_counterexample.json) et le [calcul transverse, §4](../../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md#4-résolution-exacte-du-cas-à-cinq-points) donnent les états suivants à K=2. Tous les niveaux sont des rayons carrés.

| Niveau fermé | Gamma réduit | Flot Gabriel brut |
| --- | --- | --- |
| 162/25 | Naissance CDE | Même naissance |
| 189/17 | Continuation par ADE, ajout de A | Même continuation |
| 33/2 | ACD et ACE rattachent AC à la composante existante ; aucun point ajouté | AC reste sans incidence retenue |
| 83886/3563 | ABC continue cette composante ; seul B est ajouté | Fausse naissance ABC, distincte de la composante couvrant ACDE |
| 24 | BCE continue l'unique composante ; aucun point ajouté | Fausse fusion des deux composantes |

Le flot brut perd donc les partitions de facettes, le nombre de composantes, leurs couvertures à certaines coupes, et même l'arbre non gradué. Un simple reparamétrage de ses niveaux ne répare pas ses deux naissances et sa fusion. En revanche, le véritable arbre réduit n'a aucun nœud à 33/2 : préserver l'attache de AC permet de contracter ce niveau sans changer cet arbre ni son évolution ponctuelle. La formule historique « delays the Gamma fusion » dans le champ explicatif de la fixture doit être lue avec cette précision : ABC est une **continuation** dans Gamma, et la fusion à 24 n'existe que dans le flot brut. Aucun fichier historique n'est modifié ici.

## Quatre points suffisent pour un niveau silencieux contractible

Retirer B de E5 laisse A=(0,0,7), C=(1,4,0), D=(0,0,1), E=(4,1,2). Il existe exactement quatre cofaces à K=2 : CDE et ADE sont Gabriel ; ACD et ACE ne le sont pas. C'est le cardinal minimal pour qu'une coface de trois points possède un intrus strict extérieur.

Les centres et rayons carrés sont respectivement `(9/5,9/5,1)`, `162/25` pour CDE ; `(24/17,6/17,4)`, `189/17` pour ADE ; `(1/2,2,7/2)`, `33/2` pour ACD et ACE. Le certificat associé vérifie avec des fractions les barycentriques positives du support, les égalités sur celui-ci et l'inclusion des sommets. Il vérifie aussi les puissances extérieures : A vaut 36 pour CDE, C vaut 312/17 pour ADE ; E vaut −1 pour ACD et D vaut −6 pour ACE. Les quatre MEB et leur classification ne reposent donc pas sur un calcul flottant ou un helper produit.

Avant 33/2, l'unique composante non triviale contient AD, AE, CD, CE, DE et couvre déjà ACDE. Après le lot, elle contient aussi AC et couvre exactement les mêmes points. Le journal réduit peut donc se limiter à la naissance CDE et à l'ajout de A. E5 ajoute le cinquième point B pour montrer pourquoi l'incidence supprimée doit tout de même être résolue avant une consommation future. Cette construction prouve une contraction possible ; elle n'est pas un nouveau contre-exemple à la suffisance des valeurs Gabriel.

## Pourquoi aucun plateau silencieux régulier ne crée un nœud caché

Le [lemme 2 et la confluence du §5.2](../../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md#52-théorème-conditionnel-de-rétraction-sur-le-cœur-direct) portent l'argument général. Pour une coface non-Gabriel, remplacer chacun des sommets essentiels par un intrus strict donne des cofaces de niveaux inférieurs. Leurs facettes communes relient tous les bras stricts à une seule ancienne composante non triviale, qui couvre déjà les points de la coface. Deux cofaces silencieuses simultanées partageant une facette stricte ont la même composante antérieure. Si la facette commune est de niveau égal, l'unicité de la MEB puis le remplacement d'un sommet essentiel donnent encore un chemin strict entre leurs bras. Chaque groupe du plateau possède donc un seul parent antérieur et aucun nouveau point.

L'argument exige ses prémisses : supports essentiels uniques, intrusions strictes, traitement atomique des égalités et absence d'extra-shell pertinente. Le [certificat horizontal courant](../CERTIFICAT_HORIZONTAL_COURANT.md) dispose aussi d'une autorité distincte d'inertie des blocs hors fenêtre. Un nouveau constructeur doit certifier le domaine qu'il consomme. E5 ne fournit ni défaut hors Gabriel dans ce domaine, ni justification pour supprimer ces contrôles. L'inertie des plateaux établit la suffisance d'une graduation Gabriel ; elle ne dit pas que la géométrie des portails cesse d'exiger des comparaisons exactes.

## Information suffisante selon l'objet restitué

| Objet à reconstruire | Information à conserver ou certifier |
| --- | --- |
| Composantes abstraites réduites à toute coupe, arbre et couverture | Racines K1, identités distinctes, niveaux exacts, parents pré-lot des naissances et multifusions, deltas ponctuels des continuations utiles, identité d'entrée et complétude terminale |
| Même objet pendant la construction | Résolution exacte des facettes réutilisées vers les composantes pré-lot ; les points seuls ne décident pas cette résolution |
| Tour inter-K | Une ancre inférieure certifiée par vraie naissance source, puis propagation et normalisation dans l'histoire inférieure ; coupes ouvertes et fermées cohérentes |
| Facettes incidentes ou labels complets à toute coupe | Dates et affectations des facettes omises, explicitement ou par une autorité capable de les reconstruire |
| Masse affectée à première incidence, condensation ou vote par feuilles | Univers contributif et scores du contrat pondéré, identités des feuilles et leurs affectations datées ; le quotient topologique seul ne suffit pas |

La suffisance du premier journal se prouve par induction sur les lots conservés : remplacer les parents strictement antérieurs par leur sortie et unir leurs couvertures avec le delta. Entre ces lots, l'objet annoncé reste constant. Les ensembles de points ne remplacent pas les identités : des composantes distinctes peuvent se recouvrir. Les [contre-fixtures verticales](../CONTRAT_VERTICAL_COURANT.md#6-contre-fixtures-et-reçus-bornés) montrent notamment pourquoi choisir un point source ne résout pas une cible d'ordre au moins deux. Elles ne réfutent pas l'ancre par naissance une fois certifiée. Après une fusion inférieure, son ancien token doit être normalisé ; l'union des graduations Gabriel des ordres demandés permet de placer tous ces changements.

Les labels géométriques peuvent servir à produire ou vérifier une ancre sans figurer tous dans la sortie persistante destinée au seul rejeu. Il n'est donc pas nécessaire d'exporter Gamma complet, ni toutes les facettes, pour cet objet. La taille de cette sortie ne borne toutefois pas le travail nécessaire pour la découvrir. Les identités historiques engageant les listes exhaustives de cofaces ou de facettes ne sont pas conservées automatiquement par ce nouveau quotient.

Enfin AC est une facette du catalogue Gabriel global d'E5, puisque ABC est directe. Sous une fonction de poids positive et un univers fixé comme dans le [contrat des masses](../CONTRAT_MASSES_VOTE_COURANT.md), sa masse est positive. Si la politique l'affecte à première incidence Gamma, elle rejoint la composante à 33/2 ; reporter son affectation à ABC change la masse sur l'intervalle intermédiaire. Ce fait ne rétablit pas l'obligation d'un Gamma exhaustif pour les scores : les contributions du catalogue Gabriel et un journal séparé des affectations utiles suffisent au contrat correspondant. Une autre politique est possible, mais doit être déclarée.

Verdict borné : conserver l'arbre HGP réduit avec couverture demande les bons rattachements et les bons changements, pas tous les niveaux Gamma. E5 impose la première exigence et illustre pourquoi la seconde peut être réduite.
