# Preuves locales MEB et tour FULL par boules — 10 septembre 2026

Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Paquet documentaire portable de captures fermées. Il ne relance aucun binaire et ne qualifie ni la complétude WSPD sur de grands nuages, ni CUDA, ni un contrat de performance. GCP non utilisé pour ces captures. Les fichiers ELF et les dépendances système/Boost ne sont pas distribués ; leurs pins historiques restent dans les reçus. Les captures brutes, commandes, codes de retour et pins existants ne sont pas réécrits.

## Résultats retenus

| Capture | Résultat et portée |
| --- | --- |
| `meb_o2`, `meb_san` | 11 752 contrôles, 605 comparaisons MEB, 300 permutations, 197 cas de frontière supplémentaire. Géométrie locale bornée, pas producteur FULL global. |
| `meb_mutants` | Trois mutants tués avec code 1 : accepter un point extérieur, confondre coquille et support, rejeter toute frontière supplémentaire. |
| `full_gate_r3` | 18 commandes ; O2 et ASan/UBSan/LSan identiques : 130 734 contrôles, 24 variantes, 100 ordres, 2 136 coupes, 35 462 contrôles verticaux. Oracle indépendant Gram/Gamma borné. Quatre mutants tués avec code 1. |
| `mono_o2`, `mono_san`, `mono_extended`, `mono_mutant` | Support régulier directement contrôlé et lots unitaires sans DSU ; même objet géométrique. 212 contrôles de support directs, 32 MEB extra-shell, 210 lots unitaires sans DSU sur l'ancien corpus de 22 variantes. Le mutant de travail reste géométriquement valide mais échoue au compteur physique. |
| `mono_monotone_o2`, `mono_monotone_san`, `mono_monotone_mutant` | Lecture monotone des historiques inférieurs : 512/1 024/2 048 activations de liens contre 32 896/131 328/524 800 parcours sans cache sur peignes de 256/512/1 024 fusions. Même gate finale ; activation des fusions futures tuée par deux juges. |
| `work_o2`, `work_san` | Source active combinée, pins avant/après égaux. 24 variantes ; 270 supports directs, 36 appels MEB, 42 supports MEB, 290 lots unitaires, 120 lots groupés, 356 slots DSU. Les mêmes peignes sont contrôlés. |
| `worker_pure` | Quatre commandes Python normal/`-O`, toutes code 0 ; tests purs de protocole, aucune VM et aucun kernel CUDA exécuté. |
| `baseline_8k_failed` | **Échec fermé**, non `completed` : n=8 et n=400 terminent ; n=8 000 est interrompu, code 143 après environ 575,43 s, sans résultat terminal valide. Tous les processus sont déclarés fermés par le reçu d'origine. |

La baseline FULL est `4929a5420d7401c7b69ab4de004ed3b99c9fceb0eee9b4eb8bc86881af55718c`, le premier delta `01d6dc1666d5e41b04c081f6b205504d3fbe6ab213c50da33fd3ab20f91036a2`, le delta combiné `0b72b4e9cb3858f7026d6b5d2b55f8a7b191903fc37aa24c140dfb8b557657e8`. Les trois patches sont conservés avec leurs pins.

## Provenance et limites conservées

- Le premier essai O2 du juge privé de compteurs a été corrigé après son pin initial mais avant compilation. Ce pin initial est conservé, mais les bytes correspondants ne sont pas disponibles ici et cette exécution n'est **pas** qualifiée à source immuable. La reprise `mono_extended/counter_o2_*`, ainsi que SAN, fournit le contrôle retenu.
- Les premiers recorders mono ont été reconstitués après extension de leur code et vérifiés contre leurs SHA initiaux. Leur présence ne signifie pas capture antérieure à l'exécution.
- Les captures MEB et mono n'avaient pas toutes un hash avant/après de chaque dépendance. Les dépendances communes supplémentaires sont portables grâce aux bytes qui correspondent aux pins de la gate active ; cela n'invente pas un contrôle d'absence de dérive rétrospectif. Les vrais pins avant/après de `work_o2` et `work_san` restent distincts, verbatim.
- Le mutant monotone et les mutants MEB conservent à l'entrée du recorder les pins du nominal. Les fichiers réellement mutés sont conservés séparément, et leur transformation exacte est vérifiée. Ne pas prendre le pin nominal d'entrée pour celui du binaire mutant.
- Les premières itérations de développement `full_ball_tower_gate/run_r1` et `run_r2` ne sont pas utilisées à la place de `run_r3`. Le premier run n'avait pas la même capture portable ; le second portait un corpus antérieur. Les notes originales décrivent cette progression.
- La recherche privée de la fixture d'échange à rayon égal a essayé huit positions sans observer l'échange, puis une rotation de 180 degrés a donné le témoin retenu. Une erreur initiale de chemin d'inclusion dans le script de recherche a été corrigée ; aucun résultat du script erroné n'est revendiqué. Ces essais exploratoires non scellés ne remplacent pas la fixture permanente de `run_r3`.
- Le failed probe baseline ne conservait pas de SHA par stdout/stderr dans `commands.json` : ces flux sont scellés **au moment de ce paquet**, sans prétention de pin préexistant. Le statut original demeure `failed`.
- Les premiers tests du wrapper CUDA sur host stub n'avaient pas de recorder propre. Ils ne sont pas promus dans ce paquet. Aucune compilation NVCC ni validation device ne peut en être déduite.
- La capture de performance `monotone_r1`, encore active lors de la sélection, est exclue entièrement ; elle doit être publiée séparément après fermeture. Le refus de quota GCP et la fermeture ciblée sont également documentés séparément par ROOT. Aucun claim GPU n'est transféré depuis les workers purs.

Les temps des microgates ne sont pas des mesures isolées de performance ; plusieurs travaux locaux pouvaient être concurrents. Aucune borne sous-quadratique de toute la génération WSPD n'est déduite du peigne : sa réduction porte uniquement sur la normalisation inférieure, en fonction des nœuds, liens et requêtes de cette étape. Les contrats de toute la tour 50k K=1..10 (repli K=1..5), 1 s puis 100 ms, et dizaines de millions de points sur G4 demeurent non acquis par ce paquet.

## Vérification et reconstruction

Depuis le répertoire du paquet :

```bash
python3 -B verify.py
python3 -B -O verify.py
python3 -B verify.py --extract mono_monotone /chemin/neuf/pour/la/vue
```

Le vérificateur contrôle l'inventaire exact, tous les hashes, les pins des sources disponibles, les flux et codes des commandes, les mutants, l'absence d'ELF/symlinks et les planchers de non-vacuité. Il ne contient pas de porte `assert`. L'extraction facultative exige un répertoire inexistant et ne compile ni n'exécute de code.

`objects/` contient chaque source unique une seule fois ; son nom est son SHA256. `views.json` décrit les arbres reconstituables par références et petits deltas, notamment baseline/candidate/monotone. `source_origins.json` conserve les chemins d'origine des copies retenues. `captures/` contient les reçus et flux bruts ; `metadata/` les résumés et patches ; `notes/*.md.txt` les notes originales, historiques, sans en promouvoir la portée. Les chemins absolus des commandes sont documentaires : une reconstruction ailleurs doit adapter les racines et fournir son compilateur C++20/Boost, puis enregistrer de nouveaux résultats. Le manifeste n'est pas une signature externe : son pin publié avec le commit est l'ancre d'intégrité.
