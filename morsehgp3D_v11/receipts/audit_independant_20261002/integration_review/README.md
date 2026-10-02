# Première intégration core — capture WIP du 2 octobre 2026

49 fichiers figés avant compilation, catalogue dans [SOURCE_BEFORE.json](SOURCE_BEFORE.json).
Copie de travail `/tmp/mhgp11-integration-review-20261002/source`, build neuf hors dépôt.
GCC 13.3.0, C++20, Release, avertissements stricts, B=18, modules=core.
Sources non committées au moment de la capture : ce reçu ne qualifie pas un HEAD.

[RUN.json](RUN.json) conserve les six commandes et les sorties brutes.
Configuration et compilation passent. CTest `-LE long -j2` sélectionne 71 portes :
**70 exécutées passent, une sentinelle LiDAR sautée** faute de dossier ; une
porte longue de campagne mutants est exclue. Les deux portes manifestes jugent
le schéma/motifs, elles ne constituent pas une campagne de mutants tués.
Voir [inventaire](gates.json), [sortie CTest](run_04.stdout), [log détaillé](LastTest.log).

Les preuves restent attachées aux copies : [SOURCE_AFTER.json](SOURCE_AFTER.json)
relève huit fichiers devenus différents pendant le développement, dont les aides
CMake et buffer.hpp. Aucun transfert du résultat au checkout courant.
Pas de Clang, sanitizer, poison, profil 21/24, moteur FULL, LiDAR ou performance.
Le chrono de tests est un coût de contrôle, jamais celui d'une tour HGP.

[Le juge](judge.py) vérifie les 49 hashes, l'inventaire non vide et les
causes pass/skip/exclusion. Normal et `-O` rendent la même sortie.
[JUDGES.json](JUDGES.json) conserve aussi le premier refus du juge :
son parseur exigeait un espace avant `***Skipped`, absent du log CTest.
[Version initiale](judge_initial.py) et erreur sont gardées ; aucun échec produit
masqué par cette correction. [SHA256SUMS](SHA256SUMS) ferme toutes les pièces.
