# Échec natif conservé — parallel4

Source `a7cd34ee2a5edbefc6ad98d9854e56e4df278b2e`, session du 2 octobre 2026.
Cette capsule conserve l'échec et sa fermeture ; elle ne qualifie pas le catalogue
parallèle. Aucun produit, natif ou GCP n'a été exécuté pour cette lecture.

La matrice sélectionne **1 779 portes : 1 729 passent, 50 échouent**. Six
configurations ont chacune huit lancements impossibles du même binaire de test.
La construction de `tests/catalogue/parallel.cpp` échoue pour deux raisons :
`num::Wide` n'a pas de méthode `.hex()` et l'alias global `detail` est ambigu avec
`mhgp11::detail`. Les autres messages de variables inconnues en découlent.
Ces erreurs sont dans le harnais ; aucune divergence géométrique n'en découle.

Deux portes de mutations échouent séparément :

- Catalogue : témoin rouge à la construction, **aucun mutant jugé**.
- Tower : **36 mutants tués, un invalide sur 37**. Le mutant
  `cells_deux_passes_non_comparees` laisse `first` et `second` inutilisés sous
  `-Werror`. Il doit consommer ces paramètres sans effectuer la comparaison afin
  de pouvoir être jugé causalement. Ce refus de compilation n'est pas une mort.

Le complément **ASan/UBSan18 passe 107/107**. Clang est absent et facultatif.
Le banc refuse les constructions non qualifiées en 0,028 s : **zéro mesure**,
aucun résultat de performance. Les fichiers capturés restent inchangés après
les corrections du harnais.

Le reçu original confirme la cible
`devpod-gpu-exploration/us-central1-c/ehgp-v7-3b1d496aed430749ea7e049f`
en `TERMINATED`, avec la génération de démarrage attendue, arrêt gardé code0,
clé OSLogin retirée, clé privée détruite et réserve libérée. Les commandes sont
closes sans groupe résiduel ni flux tronqué. Le statut reste `failed_remote`.

`provenance.json` épingle les originaux, l'archive et les extraits. Les membres
résultats utilisés ont été comparés à l'archive au moment de la capture. Aucun
paquet source ni archive de résultats n'est recopié ici. `check.py` lit cette
capsule seule ; normal et `-O` donnent les mêmes comptes. Son `--selftest` refuse
14 corruptions, dont la fausse réussite, une autre génération/cible, une clé ou
réserve non libérée, un mutant invalide transformé en succès et un faux banc.

Corrections minimales proposées : encodeur de test explicite signe/limbes,
alias `cat_detail`, et mutation compilable conservant l'omission visée. Ces
corrections attendent une nouvelle qualification ; elles ne réécrivent pas celle-ci.
