# Contrelecture sémantique du certificat FULL C++

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**Aucun défaut concret identifié dans le contrat structurel déclaré.** Cette conclusion résulte d'une lecture du code et de ses invariants, sans compilation ni exécution du composant. Elle ne qualifie pas un producteur géométrique. Les sources lues sont identiques à celles du commit `f4c0734c53a18d1e2de477ca09584c8f15c938f9` ; les empreintes, comparaisons au commit et exemples analytiques sont conservés dans [semantic_review.json](semantic_review.json).

Sources principales : [composant](../../src/forest/full_certificate.hpp), [contrat](../../docs/CONTRAT_CERTIFICAT_FULL.md), [porte C++](../../tests/full_certificate_gate.cpp). Dans cette note, les numéros de lignes renvoient aux octets épinglés, et non à une future version de ces fichiers.

## 1. Invariant structurel et lots simultanés

Le constructeur valide d'abord les niveaux strictement croissants, les clés de feuilles, les listes de parents et leurs ordres canoniques (composant, lignes 117–167). Pendant la construction privée, `prior_count` est fixé avant le lot. Chaque parent doit vérifier `p < prior_count` et être encore actif ; il est consommé avant l'installation de toute nouvelle feuille ou fusion (179–198).

Par induction sur les lots, cela donne les propriétés suivantes :

- Chaque nouveau nœud de fusion possède au moins deux racines provenant de niveaux strictement antérieurs. Les cycles et les parents créés dans le même lot sont impossibles.
- Deux fusions simultanées ne peuvent partager un parent. Un ancêtre déjà absorbé ne peut être repris à un niveau ultérieur.
- Chaque ancien nœud possède au plus un successeur. Les ensembles de feuilles descendantes de deux racines actives sont donc disjoints ; la structure est une forêt sur les minima.
- Le tri global temporaire interdit de représenter deux fois le même label minimum, y compris à des niveaux différents (201–206).

Les fusions successives à une même valeur ne sont pas acceptées comme une chaîne arbitraire : le producteur doit fournir les groupes déjà quotientés en multifusions maximales. C'est exactement la frontière décrite par le contrat, lignes 56–61. Le module ne prétend pas reconstruire ce quotient depuis des cofaces.

Le rejeu traite physiquement les nœuds d'un lot dans l'ordre des identifiants. Cela reste atomique à la coupe : les nœuds du lot portent tous la même fraction, le test ouvert/fermé inclut ou exclut tout le lot, et les groupes consomment des ensembles disjoints d'anciens parents. Installer les feuilles avant les fusions ne change donc pas l'état final du lot. Aucune sortie intermédiaire du rejeu n'est exposée (235–245).

### Couvertures égales et différentes

Les identités restent les `FullNodeId`, jamais les ensembles de PointId. Le code ne fusionne ni ne déduplique deux racines parce que leurs couvertures sont égales. La déduplication de points est locale à une requête `coverage`, après la visite des feuilles (298–299).

La porte existante vérifie deux fusions indépendantes au même niveau avec couvertures différentes, puis leur fusion, ainsi qu'une naissance et une fusion simultanées (tests, 228–242). Elle ne contient pas le cas plus discriminant de deux sorties distinctes à couvertures identiques. Voici une fixture analytique structurelle à ajouter au futur pont d'audit, sans allégation de réalisabilité géométrique :

| Niveau | Entrée canonique | Identifiants créés |
| --- | --- | --- |
| 1 | minima `01`, `02`, `13`, `23` | `0,1,2,3` |
| 2 | parents `[0,3]` et `[1,2]` | `4,5` |
| 3 | parents `[4,5]` | `6` |

À `2` fermé, les deux racines `4` et `5` couvrent chacune `{0,1,2,3}`, mais portent des ensembles de feuilles différents. À `3` fermé, la seule racine est `6`. Supprimer cette dernière fusion au motif qu'elle n'ajoute aucun point serait une faute. Par inspection, le composant conserve les trois événements. Les plafonds exacts sont trois lots, sept nœuds et six références de parents ; la couverture de `6` exige sept visites et huit références ponctuelles avant déduplication. Cet exemple est proposé, pas exécuté par cette contrelecture.

## 2. Niveaux exacts et frontière zéro

Les dénominateurs de chaque lot sont contrôlés positifs avant comparaison ; celui de la coupe l'est également. Les numérateurs sont non signés sur 192 bits. L'ordre repose sur `compare_exact_level`, pas sur l'ordre de représentation ni sur `ExactLevel::operator==` (composant, 126–128 et 228–239 ; [level.hpp](../../src/lanes/level.hpp), 36–58).

Cette validation suffit même pour les grandes fractions arbitraires acceptées par le contrat structurel. Pour $0\leq N<2^{192}$ et $0<D<2^{127}$, chaque produit croisé vérifie $ND<2^{319}$ : il tient dans les 320 bits effectivement calculés. Dans [wide.hpp](../../src/core/wide.hpp), lignes 44–60, chaque produit élémentaire est inférieur à $2^{128}$ ; les sommes intermédiaires de mots de 64 bits et de retenues sont inférieures à $2^{67}$, donc ne débordent pas `u128`. Le dernier mot ne perd aucune retenue, puisque le produit entier tient sur cinq mots. Cette preuve vise le chemin nominal, sans activation du mutant `level-trunc-hi`.

Les égalités simultanées de représentations différentes sont donc reconnues. Deux lots `9/1` et `18/2` sont refusés ; une coupe `18/2` voit correctement le lot `9/1`. Les fractions restent volontairement non réduites : l'équivalence sémantique ne promet pas l'identité des octets d'un futur export.

À `K=1`, le premier lot contient exactement tous les points à zéro, sans fusion ; toute naissance ultérieure est refusée. Le rejeu ouvert à zéro donne le vide, le rejeu fermé donne les points. Pour `K>1`, les lots de niveau zéro sont refusés. Ces règles sont explicites et exercées par les fixtures de la porte ; aucun transfert implicite depuis la convention initiale du profil réduit n'est nécessaire.

La porte utilise des numérateurs U192 hauts, mais ses exemples structurels de grands niveaux n'atteignent pas à eux seuls le sommet de la largeur des produits croisés. La borne précédente ferme l'argument statique ; ce n'est pas un débordement identifié ni une nouvelle demande de qualification des primitives déjà couvertes ailleurs.

## 3. Comptages, conversions et plafonds

La fonction `add` vérifie `total <= cap` puis `count <= cap-total` avant tout ajout (92–95). Avec des plafonds `u64`, cela borne les sommes sans addition débordante. Les totaux de nœuds et de références sont ensuite contrôlés contre `size_t::max` avant conversion ; le nombre de minima est au plus celui des nœuds (130–169). Les tailles non représentables par les vectors passent par `length_error`, intercepté sans résultat partiel.

Les offsets CSR et les identifiants sont issus des tailles déjà bornées. Pour chaque nœud interne, `first + parent_count` est au plus la taille finale du tableau des parents. Dans les boucles de lecture, `first+j` reste donc un index valide et ne déborde pas `u64`. La validité découle de la construction privée ; les lecteurs ne sont pas des validateurs d'arènes arbitraires désérialisées.

Pour `coverage`, soit $v$ le nombre de visites effectuées et $s$ la taille de la pile restante. L'invariant est $v+s\leq C$, où $C$ est `max_nodes` :

- L'initialisation ne programme la racine que si `C>0`.
- Dépiler puis incrémenter `visited` conserve $v+s$.
- Un nœud interne ne programme ses $q$ parents que si $q\leq C-v-s$.

Les deux soustractions de la ligne 288 ne peuvent donc sous-déborder. Les descendants ne sont jamais visités deux fois dans cette forêt, mais le plafond reste valide directement par l'invariant de programmation. Les références ponctuelles sont chargées de `f.k` avant insertion, puis dédupliquées seulement à la fin. Aucun calcul du produit `K * nombre_de_feuilles` susceptible de déborder n'est utilisé pour autoriser une allocation.

`roots_at` exige un plafond couvrant **tous** les nœuds stockés, même à une coupe vide ; le contrat le dit explicitement. Les plafonds ne sont ni des octets ni une limite RSS : entrée détenue par l'appelant, sortie en construction, indicateur `live`, copie triée des minima et capacités de vectors peuvent coexister. Le contrôle initial de l'ordre des points précède le refus du budget des lots, et les recherches de membership ne disposent pas d'un compteur de travail séparé. Aucun budget de temps CPU n'est revendiqué.

Les branches défensives exigeant des vectors matériellement irréalisables ne sont pas présentées comme exécutées. Leur absence du corpus n'établit pas une défaillance ; les petits seuils exacts/insuffisants et la preuve des additions traitent le comportement contractuel disponible.

## 4. Publication transactionnelle, possession et déplacements

Les trois arènes sont privées. Les entrées sont recopiées ; aucun `span`, pointeur de naissance ou référence à une liste de parents n'est conservé. Modifier ou détruire les lots de l'appelant après un succès ne peut donc altérer la forêt. Les accesseurs ordinaires ne donnent que des références constantes aux arènes.

Une panne après plusieurs nœuds construits, après consommation de parents privés ou pendant le tri des minima détruit l'état provisoire. Chaque refus de construction renvoie un nouvel objet avec ordre zéro et arènes vides (105–116, 171–214). Une panne de lecture efface tous les éléments de résultat sans modifier le certificat. Les interceptions ne promettent pas de conserver la capacité allouée à zéro : elles garantissent l'absence de valeurs publiables, comme annoncé.

La copie est supprimée. Le déplacement du certificat passe uniquement par échanges d'arènes, ne demande aucune allocation et invalide explicitement la source (49–79). L'affectation remplace également une destination déjà non vide ; l'auto-déplacement est sans effet. La porte vérifie ces propriétés sous panne persistante et rejette ensuite la lecture des sources déplacées (251–278).

Un `FullBuildResult` dont on a déplacé le champ `value` peut garder son ancien champ public `status=kOk` : le déplacement porte sur la valeur, pas sur le statut historique de l'appel. Cela n'ouvre pas un certificat incohérent, car les lecteurs contrôlent `order()==0`. L'intégration devra consulter la valeur qu'elle possède effectivement, conformément au contrat de déplacement.

Le modèle de possession ordinaire n'inclut pas l'altération volontaire par `const_cast`, la corruption mémoire ni une mutation concurrente sans synchronisation. Ce ne sont pas des entrées sérialisées que cette API prétend valider. Un futur import devra reconstruire par la porte validante, ou posséder sa propre validation des arènes.

## 5. Ordre terminal et permutations

Le cas `K=n` est plus fortement contraint que la seule fixture positive. Sur un domaine trié de `n` points, une clé valide de cardinal `n` est nécessairement ce domaine entier. L'interdiction des minima répétés autorise donc au plus une feuille. Comme il en faut au moins une et que toute fusion exige au moins deux racines antérieures, toute construction réussie à `K=n` contient exactement une feuille et aucune fusion. Cette déduction inclut `n=1`, dont le niveau est nécessairement zéro ; pour `n>1`, le niveau doit être positif mais son exactitude géométrique reste externe.

La borne de représentation est `1 <= K <= 10`. L'ordre terminal `K=n` est donc accepté lorsque `n<=10` ; le module ne promet pas de stocker le terminal d'un nuage de plus de dix points. Pour `K<n`, plusieurs racines terminales restent autorisées : le composant n'impose ni une fusion finale arbitraire ni la complétude d'une filtration jusqu'à l'infini.

Les permutations arbitraires des listes physiques d'entrée sont refusées lorsqu'elles rompent l'ordre contractuel. Pour un renommage bijectif des PointId, l'équivalence attendue est une isomorphie de forêts : renommer et trier les points et chaque label, retrier les feuilles du lot, réadresser les références d'anciens nœuds puis retrier les groupes. Les identifiants denses peuvent changer. Cette opération transporte exactement les partitions des feuilles et leurs unions de points ; elle ne doit pas être jugée par égalité brute des tableaux de NodeId.

La porte actuelle teste les rejets d'ordres non canoniques et les reconstructions identiques, mais pas cette covariance sous un renommage non monotone complet. C'est une vérification ciblée utile au pont d'audit, sans modifier le contrat d'entrée du composant.

## 6. Horizon, scellement et portée réelle d'un rejeu

Le contrat, lignes 35–40 et 99–103, place expressément l'identité de l'entrée, la métrique, l'unité des niveaux, les ordres, l'horizon, la complétude et les ancres dans un futur manifeste extérieur. Le certificat ne conserve même pas le domaine PointId complet reçu, seulement les labels des minima. Il n'existe pas de marqueur de fin géométrique ou d'identité de payload dans cette API.

Il est donc correct qu'un préfixe structurel valide réussisse et que `roots_at` le prolonge après son dernier niveau enregistré. Par exemple, les deux premiers lots de la fixture du §1 donnent deux racines après le niveau 3 si le troisième lot est absent ; les trois lots donnent une racine. Les deux forêts encodées sont structurellement valides. Aucun lecteur de ces seuls octets ne peut déduire si la fusion omise devait exister. Ce constat justifie l'autorité `structural_only` et ne constitue pas un défaut de ce composant.

Pour un futur résultat FULL géométriquement qualifié, le manifeste devra lier la source, les ordres et le payload au domaine de coupes effectivement certifié, avec le côté ouvert/fermé et un statut terminal de complétude. Un ID de nœud doit rester rattaché à son certificat : réutiliser le même entier dans la forêt d'un autre ordre ne prouve aucun lien vertical. De même, `coverage` accepte volontairement tout nœud historique valide ; elle ne vérifie pas que ce nœud est une racine à une coupe donnée. Le demandeur d'une couverture de composante courante doit d'abord utiliser les racines de cette coupe.

Le rapprochement prévu avec les événements FULL scellés et leurs états de référence peut qualifier le raccord des lots, des identités et des coupes pour ce corpus. Il ne transforme pas la structure en générateur Gabriel et ne requiert pas de catalogue Gamma global dans le chemin produit.

## 7. Décision et limites de cette contrelecture

La sémantique lue satisfait la forêt FULL déjà décidée : parents stricts, lots atomiques, feuilles distinctes, conservation des identités abstraites, coupes exactes, couverture par descendants et refus transactionnels. Les deux ajouts ciblés utiles au pont préparé sont la fixture de couvertures égales du §1 et le renommage non monotone du §5. Aucun résultat compilé n'est attribué à ces propositions dans cette note.

Les [reçus du constructeur](../../receipts/full_certificate_20260905/README.md) rapportent séparément leurs portes Release et ASan/UBSan, leurs planchers et leurs limites de provenance. Ils ont été lus pour interpréter la couverture annoncée, sans rejeu ni nouvelle certification ici. La qualification compilée indépendante, si elle est produite, doit être liée par les entrées de ce dossier plutôt que rétroattribuée à cette lecture.

Écritures limitées à cette note et son JSON sous `audits/`. Aucun build, benchmark, modification du produit, mutation Git ou appel GCP.
