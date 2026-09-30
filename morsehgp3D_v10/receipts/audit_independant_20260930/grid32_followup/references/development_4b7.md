# Développement de la frontière et de la précision

30 septembre 2026. Sur demande de l'utilisateur, l'auditeur continu repasse
développeur. Cette tranche implémente un correctif de la tour et un candidat
d'attache des points. La nouvelle demande de précision supérieure à u18
devient prioritaire. Les mesures et prédicats u18 existants restent historiques ;
le moteur complet à précision supérieure, la qualité statistique et une nouvelle
performance G4 ne sont pas qualifiés. `public_status=not_claimed`, hors registre.

## Ce qui a été implémenté

La recherche de rang de la tour utilisait deux expressions débordantes :
`(a+b)/2` et `lo*64` avant limitation à la taille du catalogue. Le
[correctif](../src/tower/rank_search.hpp) emploie un milieu par différence et
élargit le produit avant de le borner. La fonction pure est utilisée par le
vrai `RankIndex`, sans allocation supplémentaire ni changement d'ordonnancement.
La nouvelle porte `mhgp10_rank_search` réalise 36 047 contrôles, dont des tailles
virtuelles proches de 2^32 et des niveaux répétés comparés à `upper_bound`.
Elle tue séparément les deux erreurs réintroduites. Ce n'est pas une preuve
de capacité mémoire ni une correction de toutes les conversions de cardinalité.

Le [bras frontière par bande](../bench/frontier/README.md) est implémenté en
Python exact, hors tête de production et hors protocole A0–A6 scellé. Sur un
petit cas K3 réellement 3D, A5/A6 changent fortement l'affectation d'un point
strictement intérieur sous un déplacement maximal d'une unité : à l'égalité
ils diffèrent son entrée, puis l'affectent immédiatement après perturbation.
L'unicité à la seule première date ne corrige donc pas les quasi-égalités.
La bande regarde aussi les composantes légèrement plus tardives et conserve
une attache irrévocable par ancêtre commun.

Les six tests natifs passent en normal et avec `-O` : 815 contrôles par
exécution, quatre nuages K3 de quasi-égalité et les deux entrées internes
K3/K5. Les exports et réponses exactes appariés sont identiques entre modes.
Une revue mathématique indépendante établit couverture, emboîtement et
monotonie avec la largeur de bande. **Cela ne démontre pas une meilleure
robustesse générale ou un meilleur clustering.** Les prochains tests doivent
mesurer le rappel frontière avant fusion parasite, la masse différée et EOM
z=1/2 avec condensation, puis comparer équitablement à HDBSCAN.

Le [helper de quotas](../bench/frontier/dev_quotas.py) corrige séparément le
plan de génération : 642 allocations, 51 886 contrôles et 19 refus, normal
et `-O`. Le générateur privé doit encore l'appeler ; ses reçus antérieurs
ne sont pas réécrits.

## Précision supérieure à u18

u18 désigne une plage de coordonnées entières, pas une résolution physique.
À 1 mm, sa plage est 262,143 m par axe ; à 0,1 mm elle ne serait que
26,2143 m. Elle ne convient donc pas comme limite de la nouvelle cible.
La proposition est une grille isotrope paramétrable, 0,1 mm par défaut,
avec coordonnées stockées en u32. Le choix entre cette grille et le
float32 original sans perte a été demandé à l'utilisateur ; il reste à confirmer.

Deux domaines exacts doivent être distingués dans ce contenant u32 :

| Domaine géométrique proposé | Étendue à 0,1 mm par axe | Port à réaliser |
| --- | ---: | --- |
| u24 | 1 677,7215 m | première voie large plausible pour une trame LiDAR ; centre encore i128, census/orientations et niveaux élargis |
| u32 complet | 429 496,7295 m | centres larges, distances 128 bits, clés Morton 96 bits et comparateurs de niveaux jusqu'à 512 bits |

u24 est un palier proposé, **pas un moteur déjà porté**. Il ne doit jamais
être annoncé comme u32 complet. Une entrée hors domaine est refusée, pas
renormalisée ou requantifiée implicitement. Chaque reçu devra publier pas,
origine, domaine, nombres de retours/sites/fusions et correspondance des IDs.

Il est faux de relever seulement le plafond actuel : la clé Morton64
masque chaque axe à 21 bits et le regroupement fusionne par clé seule.
Les positions `(0,0,0)` et `(2^21,0,0)` deviendraient une fausse fusion.
Les distances du SiteTree calculées en i64/u64 ne couvrent pas le domaine
u32 : le carré de la diagonale peut dépasser 2^65. Les boîtes en double
stockent en revanche déjà exactement toutes les coordonnées u32.

Une première brique large est désormais implémentée dans
[grid32_primitives.hpp](../src/cloud/grid32_primitives.hpp) : distance carrée
exacte sur u128 et clé Morton de 96 bits conservant les 32 bits de chaque
axe. Le décodeur refuse les bits au-delà de 96 au lieu de les tronquer.
La porte indépendante réalise 212 684 contrôles, dont 10 162 distances,
10 203 aller-retours et 64 clés hors domaine. Les oracles de distance
(entiers multi-mots) et d'encodage (construction du flux de bits) sont
distincts des implémentations. Deux relectures indépendantes ne trouvent
pas de défaut.
Normal et UBSan passent avec les mêmes résultats ; les deux troncatures
historiques réintroduites séparément sont rejetées numériquement, sans crash.

Ces primitives sont de coût constant, sans allocation ; elles ne sont
**pas encore raccordées** au propriétaire de nuage, au SiteTree, aux
prédicats ou au GPU. Elles n'enlèvent donc pas le refus u18 actuel.
Elles ne définissent ni le pas physique ni un nouveau format de fichier.

L'[audit de largeur](../receipts/development_frontier_precision_20260930/precision_math/AUDIT.md)
propose pour u32 des centres q3/q4 sur trois mots,
des côtés de sphère jusqu'à 201 bits, des orientations de centre jusqu'à
233 bits et des niveaux jusqu'à 266/200 bits. Leur produit croisé peut
demander 466 bits ; le produit générique de tableaux de cinq et quatre mots
ne compile pas avec la limite actuelle de huit mots. Ces bornes doivent
être dérivées et testées dans le port, pas transposées par remplacement
global de types. Le palier u24 évite ce dernier obstacle et garde le centre
i128, mais ne garde pas les prédicats de census i128 actuels.

Pour préserver le débit, la piste pertinente est un dispatch **certifié par
opération** : conserver la voie courte lorsque les valeurs effectivement
impliquées sont dans sa borne, replier vers les mots larges sinon. Une petite
étendue du support seule ne justifie pas le test contre un témoin très loin.
Les marges flottantes 0,02 héritées de u18 doivent être redémontrées ; les
recopies de boîtes ne sont pas des filtres numériques qualifiés.

## Ordre des travaux suivants

1. Préparer le profil large : identité sans collision, conversion exacte
   avec pas/origine explicites, distances et largeur des prédicats. Déclarer
   chaque brique portée et chaque refus ; ne pas activer une entrée large
   devant un census ou une tour restés u18.
2. Qualifier les côtés, orientations, supports et niveaux sur les extrêmes,
   les égalités et un oracle rationnel indépendant. Raccorder le même profil
   au générateur, à la tour, aux attaches et aux exports.
3. Comparer les bras frontière sur scènes dev à tailles et nombres de
   communautés variables, puis geler un test distinct. Conserver les contrôles
   A0/core et A1/cover, EOM z=1/2 et le même min_cluster_size chez HDBSCAN.
4. Sur G4 SPOT, mesurer les trames sans sol entières et les coupes spatiales
   au capteur, puis avec sol et plusieurs séquences. Mesurer les travaux
   8k/16k/32k, incidences et mémoire, pas seulement le chrono du bras.
   FULL K5 explicitement matérialisée reste la cible 100 ms ; aucun nouveau
   profil ne reçoit les qualifications ou chronos de u18 par héritage.

Les correctifs R2 du développeur restent dans leurs copies séparées ; cette
tranche ne prétend pas résoudre leur intégration commune. GCP n'a pas été
utilisé : les nouvelles régressions bornées ne nécessitaient pas de VM.
Les [preuves de cette tranche](../receipts/development_frontier_precision_20260930/README.md)
conservent aussi les échecs de préparation et la restriction LeakSanitizer.

Validation documentaire : les six documents modifiés et les nouveaux
documents de preuve passent le contrôle ciblé. Le contrôle global du dépôt
reste en échec sur des liens relatifs de captures historiques déjà présentes ;
ces archives closes n'ont pas été réécrites pour faire passer le contrôle.
Le registre formel, inchangé, passe ses 20 phases.
