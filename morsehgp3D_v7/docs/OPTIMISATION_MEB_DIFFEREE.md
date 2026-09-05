# Matérialisation différée des MEB locales

`phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

État de cette note : **avant mesure C/D**. Le delta a été relu et porté
sur autorisation ; aucun gain pipeline ou SLO n'est encore attribué ici.
La [revue archivée](../receipts/meb_lazy_review_20260904/README.md) est
horodatée avant la mesure. Elle conserve aussi les qualifications locales,
distinctes de la qualification intégrée.

## Objet exact du changement

Dans `silent_detail::Builder::miniball`
([source](../src/forest/silent_incidence.hpp)), les supports q2, q3 puis q4
sont parcourus dans leur ordre existant. Auparavant, chaque candidat
strictement positif q3/q4 construisait sa clé primitive et son niveau
avant de vérifier si sa boule contenait tous les sites locaux.

Le delta vérifie d'abord cette contenance par les helpers entiers existants
q3_power/q4_power. Un site de puissance **strictement positive** rejette le
candidat avant construction. Sinon la même paire clé/niveau est construite
et le même accept est appelé. Le contrôle final de la coquille demeure.

q2, l'ordre des supports, les gardes de positivité et les charges de budget
ne changent pas. Aucune proposition flottante, permutation spéculative,
coface globale ou mosaïque n'est ajoutée. Une MEB reste calculée sur au
plus 11 sites, avec au plus 550 supports de tailles 2 à 4. Le nombre global
de ces calculs et les longueurs des chaînes restent leurs coûts propres.

## Théorème local de conservation

Préconditions : sites distincts d'un index admis quantized_u16, coordonnées
dans [0,65535], cardinal local entre 2 et 11 ; primitives non mutées et formes
réellement construites depuis leurs supports. Cette preuve ne valide pas
des Q3Form/Q4Form arbitraires ni des tableaux hors domaine.

Pour q3, G>0 après le garde existant. Le développement de la puissance
relative donne exactement la forme brute utilisée pour la clé :

$G\lVert z-a\rVert^2-W\cdot(z-a)=G\lVert z\rVert^2-(2Ga+W)\cdot z+G\lVert a\rVert^2+W\cdot a$.

Pour q4, q4_form canonise det>0 avant le test de bien-centrage. De même :

$\det\lVert z-a\rVert^2-2N'\cdot(z-a)=\det\lVert z\rVert^2-2(\det a+N')\cdot z+\det\lVert a\rVert^2+2N'\cdot a$.

ball_key_reduce divise tous les coefficients par un PGCD g>0, au plus
le coefficient A>0. La puissance brute vaut donc g fois la puissance
primitive : leurs signes et leurs zéros sont identiques. Références :
[q3](../src/lanes/q3.hpp), [q4](../src/lanes/q4.hpp) et
[réduction des clés](../src/lanes/keys.hpp).

Par induction sur le même parcours, tout candidat rejeté par le prétest
aurait été rejeté par l'ancien accept, avant toute écriture dans LocalBall.
Tout candidat contenant reçoit les mêmes constructions et le même accept.
Le premier support accepté, ou l'absence de support accepté, reste identique.
Cet argument établit la substitution locale à C ; il ne suppose ni ne
démontre que toute la chaîne HGP de C est exacte.

## Bornes de chaque calcul ajouté

Avec M=65535, les différences de coordonnées sont dans [-M,M] et leur norme
carrée est au plus 3M². Les bornes du
[grand-livre arithmétique](ARITHMETIQUE_PRIMITIVES.md) s'appliquent aux
intermédiaires effectivement écrits, pas seulement au résultat final :

- q3 : chaque terme Wv est au plus 36M⁶ en module ; les sommes partielles
  sont au plus 72M⁶ puis 108M⁶ ; G|v|² est au plus 27M⁶. La soustraction est
  au plus 135M⁶ < 2^104.
- q4 : det|v|² est au plus 144M⁵ ; la somme des N'v est au plus 216M⁵,
  son double au plus 432M⁵ ; la soustraction est au plus 576M⁵ < 2^90.

Tous restent dans i128. Le PGCD positif est au plus A et son cast signé
reste défini. Aucune nouvelle opération large, conversion flottante ou
division de rang nul n'entre dans le delta. Les tests extrêmes confortent
ces obligations ; ils ne remplacent pas leur preuve.

## Contrats conservés et portée de la composition

Les mêmes supports sont chargés avant examen : meb_calls, meb_supports et
la frontière silent_meb_support_budget restent inchangés. Le booléen,
statut, raison et état de LocalBall lors d'un refus restent identiques.

La conservation porte sur les champs de BallKey, q et l'ordre du support,
ainsi que sur la **représentation** du niveau. Le niveau q4 reste non réduit ;
une simple égalité rationnelle ne suffirait pas à ce contrat.

Le test final shell est conservé. Les supports non essentiels gardent
leur refus scientifique ; aucun zéro de coquille n'est supprimé.
Les requêtes d'intrus, descentes, événements, allocations de tas du parcours
et logique de purge ne sont pas modifiés. La paire est un objet local ;
aucune identité de consommation de pile ou de temps machine n'est revendiquée.

La brique attend toujours un catalogue direct régulier complet fourni
par son appelant. La preuve locale ne ferme ni cette hypothèse, ni les
obligations horizontales globales, les applications verticales ou la
publication transactionnelle. Le contrôle local events.empty est vide
pour cette dernière obligation : les portes Gamma/API/refus/archive
doivent être requalifiées après intégration.

## Constructions logiques et mesure de coût

Une seule closure par lane construit la paire en test comme en production.
Seuls les compteurs privés TLS et l'appel mutant eager sont compilés sous
MHGP7_TESTING ; les statistiques et plafonds publics ne changent pas.
Le test permanent active ses mutants par le registre réel, sans adaptateur
d'overlay.

Les deux faux négatifs >0 vers >=0 ont des fixtures minimales distinctes
q3 et q4. Le mutant eager conserve les objets mais rétablit les constructions
sur rejet ; des compteurs non vides dans les deux lanes le réfutent.
Ils mesurent des **appels logiques**, pas les instructions éliminables
par le compilateur ni des cycles.

Le prétest ajoute aussi une passe sur les sites du candidat accepté.
Le gain n'est donc pas garanti pour toute entrée. Aucun temps d'overlay
n'est transféré au pipeline, et aucune ventilation des anciens cumuls G4
n'est supposée. La comparaison C/D devra conserver entrée, ordres K1..10,
digests, représentations, compteurs et caps, puis distinguer processus,
étages et MEB. Une paire exploratoire ne qualifiera pas les objectifs
50 000 points sous 1 s puis 100 ms, ni plusieurs dizaines de millions de points.
