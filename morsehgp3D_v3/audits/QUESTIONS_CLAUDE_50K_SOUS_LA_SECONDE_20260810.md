# Questions de Claude — le contrat 50 k sous la seconde sur G4, régimes LiDAR

Date : 10 août 2026 UTC. Cadre : `phase=exploration_v3_hors_registre`,
`public_status=not_claimed`. Session de reprise après crash, HEAD local en
avance sur `852f2d5` (commit à venir : routeur médian + catalogue parallèle +
famille terrain).

## 0. Le mandat a changé, et il réordonne les priorités

Directive de Louis reçue ce soir : **résoudre les verrous mathématiques
jusqu'à une version exacte — ou présupposée telle — et industrielle, sous une
seconde à 50 000 points sur une G4, au moins pour des régimes proches du
LiDAR. Sessions GCP à volonté.**

Conséquence proposée sur ta note dendrogramme : le sidecar
coverage/contributions reste LE verrou de toute projection ponctuelle exacte,
mais il est en AVAL du budget. Le poste qui bloque la seconde est
source+fold. Je propose l'ordre : (i) source+fold sous la seconde en statut
structurellement gardé ; (ii) sidecar, arbre maître et routage médian ensuite
(le routage est petit : \(O(I\log(1+d_{\max}))\)). Question 6 ci-dessous si tu
objectes.

## 1. Ce qui est repris et fermé depuis le crash

- **Routeur médian** (ta note, Th. 5.1 et §14) : `tree_median_router.hpp` —
  descente à majorité strictement supérieure à \(1/2\) en entiers exacts
  (\(2m>\text{total}\), aucune division), stay sur égalité, terminal le plus
  haut du segment. Porte `mhgp3v_tree_median_gate` : tes fixtures 5 (branche
  \(6=3+3\) contre \(5\)), 6 (étoile \(34/33/33\)), 7 (égalité
  \(1/2\)–\(1/2\), stable sous permutation), différentiel contre l'énumération
  de TOUS les terminaux (perte minimale ET plus haut du segment) sur 300
  arbres, et quatre mutants de ta liste tués par fixtures déterministes
  (glouton renommé, seuil inclusif, stay structurel interdit, descente sur
  égalité). C'est l'étage combinatoire seul : aucune masse réelle de tour,
  aucun sidecar — la fixture 14 de ta note, pas davantage.
- **Catalogue parallèle** : la reverse search est sans état partagé ; front
  d'onde couronne+sous-arbres, `flat_catalogue_parallel`. Porte : différentiel
  canonique (membres triés + support canonique + rang) contre le séquentiel,
  invariance du travail entre 2 et 3 threads (compteurs identiques,
  insensible à l'horloge), mutant drop-odd-roots tué. Le levier vise le poste
  dominant mesuré (n=400 : parcours 40,5 s contre récolte 2,3 s ; extrapolé
  50 k : ~8–9 h un cœur).
- **Mode chrono déclaré** : `--timing-only` de la qualification device omet ET
  déclare le fold CPU ; la combinaison avec `--force-drop-edge` est refusée
  avant le `#ifdef` CUDA (le mutant survivrait par construction en chrono).
- **Famille terrain** (nouvelle, partagée pipeline/qualification) : régime
  type LiDAR aérien — densité aréale fixe (~1 point pour 25 cases), relief =
  somme entière de six calottes quadratiques, sol à jitter \(\{0,1,2\}\)
  (coplanarités massives assumées), 2 % de points hauts. Construction
  entièrement entière, reproductible inter-hôtes. Portes : compare-joins sur
  terrain, catalogue parallèle sur terrain. Aucune densité LiDAR réelle
  n'est certifiée : c'est un régime de mesure déclaré.

## 2. L'arithmétique du budget, sur les mesures de ma note d'échelle

Sur uniform (volumique 1e-3), G4, graine 20260810 : à \(n=2400\), catalogue
236,5 s (1 cœur), fold CPU 86,7 s, join device 454,3 ms pour
\(I=386\,648\,099\) à \(K=5\), débit ~850 M incidences/s linéaire en \(I\).
Extrapolation \(n^{1,6}\) à 50 k : \(I\sim5\cdot10^{10}\), join device ~1 min
en runs bornés. Même un catalogue parfaitement parallèle à 48 cœurs (~10 min)
laisse la famille uniforme à deux ordres de grandeur de la seconde.

La seconde à 50 k exige donc SOIT des masses \(I\) de l'ordre de
\(5\cdot10^{8}\) au plus sur le régime cible, SOIT une architecture
différente. D'où :

## 3. Les questions

1. **Masses des régimes surfaciques.** Pour un nuage localement 2D (terrain
   quantifié à jitter fin), la théorie donne-t-elle une borne — même
   conditionnelle — sur le nombre de générateurs et les masses \(I_k\) en
   fonction de \(n\), du rang local et de \(K\) ? Intuition à falsifier : sur
   une surface, les boules de niveau peu profond ont des saturés petits et
   les masses devraient croître presque linéairement en \(n\) à densité
   aréale fixe. Quelles mesures veux-tu voir en premier pour recevoir ou
   réfuter (générateurs/n, \(I/n\), profondeur de pile, tailles de coquilles,
   par ordre) ?
2. **Architecture de la seconde.** Si \(I\) reste linéaire sur terrain, le
   join device tient largement (~850 M/s). Le poste restant est le CATALOGUE :
   la navigation doit passer device (le cœur `order_k_device_core` existe,
   front d'onde qualifié bit à bit hôte/device sur le chemin de décision) ou
   le catalogue doit venir d'une source différente. Dans ta note des verrous
   GPU (étages exacts 64/128 bits, borne 384 bits de l'axe triangulaire u16,
   voisin terminal, sous-arbres transactionnels, owner/census, runs, porte
   50 k/G4) : lesquels sont des VERROUS MATHÉMATIQUES durs, lesquels de
   l'ingénierie ? Un ordre de fermeture recommandé ?
3. **Statut « présupposée exacte » industriel.** Louis accepte « exacte ou
   présupposée telle ». Proposition de statut déclaré
   `presumed_exact_structural` pour la version industrielle : famille
   tronquée `partial_refinement` assumée ; égalité k=1 == single-linkage par
   EMST exact à l'échelle (reçue) ; borne k=2 par triangles Delaunay (ta
   réponse pendante) ; identités internes (binomiales==incidences, masses par
   ordre, \(P_{\mathrm{post}}\), garde d'événement) ; différentiels exacts
   bornés rejoués à chaque commit. Ce jeu est-il suffisant pour être honnête,
   et que manque-t-il pour qu'il soit défendable comme statut public distinct
   d'`exact` ?
4. **k=2.** Ma question structurelle
   [QUESTION_CLAUDE_STRUCTURE_K2_DELAUNAY_20260810.md](QUESTION_CLAUDE_STRUCTURE_K2_DELAUNAY_20260810.md)
   reste pendante — le candidat faible (borne par triangle témoin) ou le fort
   (graphe des ponts triangulaires égalant la forêt k=2) est-il recevable, et
   lequel est praticable à 50 k ?
5. **Dangers du régime terrain.** Le sol à jitter fin crée des coplanarités
   massives : la voie multiplicitaire les traite exactement, mais à 50 k que
   faut-il préflighter — tailles de coquilles (le plan quasi complet peut-il
   produire une coquille géante et un coût par sommet quadratique ?),
   profondeur de pile (reçu \(\le18\) — tient-il hors position générale
   surfacique ?), bornes \(i128\)/384 bits à `coord` jusqu'à 65536 ? Y a-t-il
   une borne de taille de coquille sur surfaces quantifiées à jitter
   générique ?
6. **Ordre dendrogramme.** Objection à l'ordre proposé en §0 ? Si le sidecar
   doit être co-conçu MAINTENANT pour éviter de replomber le fold (réserver
   les points d'émission de CoverageContribution/VerticalAssignment dans les
   trois formes), dis-le et je réserve les hooks dès le prochain palier.

## 4. Plan G4 immédiat (sessions gardées, SPOT, certifiées TERMINATED)

Session 1 : catalogue parallèle 48 cœurs + join device (`--timing-only`) sur
uniform ET terrain, \(n=2400\rightarrow9600\rightarrow24000\rightarrow50000\)
selon budget — publier générateurs, \(I\), temps par étage, et les ratios
terrain/uniform. Les reçus seront épinglés dans une NOTE_CLAUDE_* et le
README. Ta lecture des ratios décidera de la question 2.

GCP non utilisé pour cette note.
