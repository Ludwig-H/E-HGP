# Préflight distinct des captures finales

20 septembre2026, tranche25 après77f659e4.

Les nouveaux builds sont `build/v8_q34_pruning_20260920` (GCC13.3,
Release) et `build/v8_q34_pruning_sanitize_20260920` (Clang18,
ASan/UBSan). Aucun build antérieur n'a été recompilé.

Une compilation de la nouvelle porte a été lancée pendant l'écriture de
son fichier par le juge : le fichier encore partiel n'avait pas de main
et trois fonctions inutilisées ont déclenché `-Werror`. Attendre la fin
de l'écriture puis reconstruire a résolu cet échec de coordination, sans
correction mathématique du moteur. La bibliothèque était déjà compilée.

Le premier préflight du runner a trouvé `mhgp8_exact_ball_gate` non encore
compilé. Aucun reçu final n'a été produit par cet essai. Deux invocations
manuelles de la nouvelle porte sans `--selftest` ont renvoyé2 sans sortie,
conformément à son interface ; l'appel correctement formé passe.

Avant gel : nouvelle porte Release puis Clang ASan/UBSan PASS,
5 176 contrôles ; runner Python normal et −O, quatre portes et sondes
des trois régimes/budgets PASS. Ces vérifications exploratoires ne sont
pas comptées comme mesures supplémentaires. Les captures finales ont
leurs propres commandes, sorties brutes et158 empreintes de sources.

Une exploration de sélection par premiers IDs a montré un biais sur le
petit adversaire lors d'une permutation des IDs. Elle n'est pas le moteur
qualifié. Le pool produit échantillonne les positions floor(i*m/C) dans
les plages spatiales ; aucune qualification n'est héritée du prototype.

Contrôle final de format : `git diff --cached --check` relève une ligne
vide supplémentaire à la fin du seul auxiliaire `mutants/run_mutants.py`.
Ce fichier est déjà empreinté par ses deux captures et leurs lectures ;
il est conservé tel quel, sans modification silencieuse après gel.
Le contrôle avec seule cette règle cosmétique désactivée passe. Aucun
avertissement de compilation ni défaut fonctionnel n'est ainsi ignoré.
