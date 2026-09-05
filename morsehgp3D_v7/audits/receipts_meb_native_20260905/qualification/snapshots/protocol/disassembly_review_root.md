# Lecture du binaire de micro-coût — 5 septembre 2026, avant mesure

Reçu build `de6de29f55ab55d8edd64f9e3307d4748688635ca7338c36105555da39e0574f` ; binaire `56e022c817d2e726eb2e3b135e78e577bbdf344ebd0ff352d64d1121300fd976` ; désassemblage complet `52392c6a8b9a8a230133113fdad0bfa9ca64b25291600349b9905be9f126c9c4`.

Lecture root du source C++ 5a0fd397 et des instructions suivantes dans le binaire fermé, pas exécution du harnais :

- `invoke_f` 0x5240 : appel de `Builder::miniball` à 0x530c, résultat stocké dans Outcome.
- `invoke_dual` 0x11c50 : appel de `miniball<false,NoObserver>` à 0x11ce9 ; ce n'est pas Trace.
- Sélection des deux adresses à 0x16226/0x16234, pointeur conservé avant l'horloge 0x1625b.
- Initialisation de l'état par tentative dès 0x16320 ; entrée dans les étapes à 0x16420.
- Appel indirect effectif 0x16507, puis `terminal_hash` 0x1651e et mise à jour des captures terminal/Work depuis les résultats.
- Retour de boucle des étapes 0x165dc vers 0x16420 ; répétitions 0x16bcd vers 0x16320 ; parcours des jobs 0x16be9 vers 0x162a2.
- Horloge finale 0x16bef ; comparaison des deux captures à 0x16c52/0x16c62 et appel du garde à 0x16c84.

Les appels et consommations de chaque résultat subsistent entre les horloges. Ce constat n'est pas une preuve d'isolation matérielle ni un coût de helper nu : resets, copies, enveloppes d'appel et captures sont inclus. Les comparaisons scientifiques complètes avant/après restent requises par le harnais et son juge ; aucun temps n'est encore attribué par cette lecture.

La première compilation `247c952c` a échoué : le macro objet main renommait aussi Metrics::main. Ses fichiers restent inchangés dans le dossier v1. La v2 ne change qu'une ligne C++ (macro fonctionnel) et son pin runner ; son build réussi ne réécrit pas cet échec.
