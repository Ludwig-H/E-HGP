# Comparer baseline et x86-64-v3 sur G4

Ce banc compare uniquement la cible compilateur à source, entrées, profils,
options algorithmiques et compilateur identiques. Il construit quatre Release
neufs : baseline/v3 pour u21/u24 ; IPO est explicitement désactivé. Chaque
binaire passe la sélection fonctionnelle complète hors référence Python et
mutants. Un cinquième build exécute les suites de mutants avec v3.
Cette expérience ne remplace pas la qualification ASan/UBSan/TSan préalable.

Le collecteur vérifie les options, le cache CMake, les flags effectifs de la
bibliothèque et de la sonde, les liens et les hashes binaires. Les inventaires
CTest doivent être complets, sans doublon ni porte désactivée, et identiques
entre les quatre variantes. Les sélecteurs sont fixés. Cette garde a été
ajoutée après contre-revue : le premier collecteur aurait accepté une sélection
incomplète commune aux quatre builds malgré son plancher de tests.

La transmission de `-march=x86-64-v3` aux clones mutants est reliée aux
commandes LastTest et aux recettes épinglées. Les flags individuels des clones
supprimés ne sont pas déclarés observés. Les profils u21/u24 particuliers des
mutants restent conservés. Aucun LTO ni `-march=native` n'est activé.

Le calendrier contient24 FULL K1..5 W48 : trois trames sans sol entières,
deux profils, deux variantes et deux répétitions. L'ordre des paires est
alterné. Les caches sémantiques des variantes sont séparés ; chaque réemploi
rehache le dump complet et contrôle les événements courants. Objets canoniques,
entiers bruts au même profil et compteurs logiques doivent concorder. Temps
FULL, préparation, processus et décodage restent séparés. Une unité ratée
n'est pas remplacée ; chaque omission de budget reste publiée.

Le plan gardé déclare1350+750+120=2220 secondes. Les modèles normal/−O
passent :9 campagnes,146 enfants simulés,73 petits décodages réels,
58 corruptions,4 provenances,2 interruptions et562 contrôles. Ils utilisent
le vrai collecteur avec processus simulés, sans exécution native locale.
Aucun résultat G4 ni gain compilateur n'est encore acquis pour ce banc.
