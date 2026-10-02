# Diagnostics du catalogue parallèle — revue 18

Pin publié **a7cd34ee2a5edbefc6ad98d9854e56e4df278b2e** : 37 fichiers LIVE copiés et hachés avant lecture, tous identiques à Git ; un document supplémentaire est extrait du même Git avant sa lecture. Cette revue ne touche pas les descentes, les forêts ou la capsule17. Aucun natif, build, GCP, import produit ou allocation massive exécuté. Les échecs de capacité G4 mentionnés dans DEVELOPMENT ne sont pas des chronos du catalogue ; la prochaine campagne reste à jouer.

Les diagnostics nouveaux permettent une attribution **par phase** et une première mesure du déséquilibre. Deux points concrets empêchent toutefois un diagnostic complet : l’essai W8 est supprimé par un premier échec W48, et le lecteur ne garde pas encore la borne de concurrence des durées par tâche. Aucun mauvais compteur natif ni régression W48 n’est constaté ici.

## Le protocole peut supprimer précisément le bras diagnostique utile

`bench/catalogue_parallel.py:123–134,153–154` mémorise toute première issue W48/K5 non `ok` par trame/profil, puis omet ses répétitions, **son W8/K5 et son W48/K10**. Les portes factices vérifient cette politique ; elle est intentionnelle, pas un défaut de conservation des captures.

Un échec W48 ne prouve pourtant aucune impossibilité à W8 : contention, désynchronisation, coût de session, mémoire ou serialization peuvent changer l’issue. Le modèle de calendrier conserve36 demandes ; un premier échec supprime quatre essais dont l’unique W8 correspondant. Il n’y a pas de W1 dans cette campagne, et les trois répétitions portent seulement sur W48/K5.

Conseil : conserver au moins **un W8 indépendant de l’issue W48**, sous son propre budget. Pour séparer surcharge du chemin Pool et scaling, ajouter un W1 apparié aux mêmes octets/IDs/paramètres/profil, si le budget autorise ce diagnostic ; une paire W0/W1 distingue également le préambule parallèle du mono. Le mono historique reste historique. Ne pas remplacer cette paire par un rapport de chronos de sources ou régimes différents. Les omissions de budget restent explicites.

Le timeout15s couvre le **processus**, donc lecture, Cloud, Pool, API et serialization. `catalogue_probe.cpp:153–174` écrit/flush le résultat API **avant** serialization. Un processus interrompu après ce flush peut conserver un intervalle API complet sans sortie canonique close. Le protocole garde ses événements bruts, mais `check_parallel:61–62` ne dérive rien si l’essai n’est pas `ok`. Il serait utile de présenter cette télémétrie complète séparément sur échec, tout en gardant l’échec du processus/artefact et le statut non conforme ; ne jamais promouvoir un timeout à réussite API complète du livrable.

## Un garde-fou manquant pour les métriques

Les nouveaux contrôles sont positifs : champs exacts, entiers u64 sans bool, somme des murs disjoints≤mur API, max≤somme≤J·max et max≤mur de phase ; borne du tri par tas. Les durées restent hors des octets canoniques. On/off est optionnel : `std::optional<Stopwatch>` évite les lectures d’horloge internes quand diagnostics=null (`parallel.cpp:58–61,111–112`, `assemble.cpp:79–82`). Un brouillon n’est rendu qu’au succès (`parallel.cpp:200–202`) ; les portes présentes préservent le diagnostic ancien sur refus, non exécutées ici.

Il manque à `check_timings:25–27` la relation **sum_task≤min(W,J)·phase_wall**. Les intervalles execute_task de chaque worker sont disjoints et inclus dans le mur de sa phase ; leur somme, y compris après conversion en nanosecondes, ne dépasse donc pas ce mur par worker. Cela mesure des fenêtres murales, sans supposer une durée CPU.

Témoin scalaire du lecteur de métriques seulement : W8,J256,count_wall=100 ms,count_max=100 ms,count_sum=900 ms, mur API200 ms, autres murs/sommes0, deux boules et une comparaison de tri. Toutes les relations actuellement écrites dans check_timings sont satisfaites, mais la concurrence borne la somme à800 ms. Ce témoin ne prétend pas passer le décodeur géométrique complet ni être une sortie native. Ajouter la relation par phase, plus ce mutant, aiderait à empêcher une mauvaise interprétation d’occupation.

## Ce qui devient observable, et ce que cela ne prouve pas

| Champ | Attribution justifiée | Limite |
| --- | --- | --- |
| prefix/replay | Deux préambules sur le pilote | Inclut filtres, scans, listes/allocation ; pas une mesure isolée G1 |
| count/fill | Murs des deux appels Pool, sans overlap entre phases | Comprend orchestration, suffixes, vérifications et rebasage fill |
| task_sum/task_max | Intervalles execute_task seulement | Temps mural avec attentes/descheduling, **pas CPU** ; hors vérifications/rebasage après retour |
| sort_ns/sort_comparisons | Tri par tas exact séquentiel et ses comparaisons | Ne chronomètre pas séparément chaque type de comparaison |
| level_scan/assembly | Scan exact puis ranks/CSR/copies, séquentiels | L’assemblage refait aussi des comparaisons exactes voisines |
| allocation_ns | Trois régions d’admission/planification/allocation sur le pilote | Les allocations DFS sont dans count/fill ; les listes préambule dans prefix/replay ; ce n’est pas le temps de toutes les allocations |
| peak_reserved/reserved_after | Pic global et état final des Buffer, Cloud déjà vivant | Ni RSS ni piles/threads OS ; ne localise pas le pic par phase |
| cpu_seconds | std::clock autour de l’appel, CPU processus agrégé | Distinct des sommes de tâches ; création Pool, Cloud et IO hors intervalle |

Les murs instrumentés sont disjoints mais **non exhaustifs** : validation, réductions count/fill, temps entre étapes et destructions peuvent rester dans `API_wall−Σstage_walls`. Afficher ce résidu, plutôt que forcer une somme égale au temps API. Un ratio sum_task/phase_wall donne une concurrence effective moyenne des fenêtres mesurées ; divisé par min(W,J), il donne leur présence relative, jamais l’utilisation CPU.

Pourquoi W48 **pourrait** régresser, sans cause déjà mesurée :

- La frontière reste géométrique fixe, J≤256, sans subdivision dynamique d’un suffixe lourd. Un seul long ordinal peut borner count ou fill ; plus de workers n’accélère pas cet ordinal. Le max par tâche rend ce cas visible, mais ne dit pas seul si cette durée vient du travail géométrique ou d’attentes.
- Prefix/replay/tri/scan/assemblage restent séquentiels. Leur somme est un plancher séquentiel **de l’essai instrumenté**, pas une prédiction de tous les W. Le tri exact global préserve les plateaux ; ne pas revenir aux bandes flottantes pour gagner du temps.
- min(W,J) workspaces et les DFS actifs coexistent ; le coût d’allocation, le budget atomique partagé, l’allocateur et la bande mémoire peuvent augmenter. Ce sont des hypothèses à départager, pas des causes acquises. Le banc passe max_nodes=0 : **NodeQuota ne fait alors aucun CAS**, donc ne pas lui attribuer une contention sans autre profil. Les CAS du Pool et du compte Buffer restent présents.

Conseil de diagnostic à faible coût : publier quelques **ordinaux** responsables des maxima avec leurs count/profondeur, capacité de liste et compteurs de tâche (préfixes/census/q4/boules). TaskCounts.ledger et Frontier.task sont déjà disponibles, J est borné ; aucun nouveau scan du Cloud ni copie d’index n’est nécessaire. Comparer ces mêmes ordinaux à W1/W8/W48 pour distinguer charge intrinsèque et durée gonflée. Garder la télémétrie non canonique, privée jusqu’au succès, et mesurer son propre surcoût.

## Mémoire : attribuer les coexistences, sans réinitialiser le pic public

La revue17 conserve les formules complètes. Ici, le point utile est de publier les quantités **réelles déjà connues** : F=4Σ(capacités Frontier), S=scratch total min(W,J), T=sizeof(Emission)·boules+4·incidences, et les bornes H de DFS/V de replay explicitement étiquetées **bornes**. F/S sont vivants pendant count ; F/S/T pendant replay/fill ; ils sont rendus avant Assembly::finish, où T coexiste avec la sortie finale. Une même valeur de peak_reserved à W8/W48 ne prouve pas que leur mémoire parallèle est identique : l’assemblage peut dominer les deux pics.

Des échantillons used() aux barrières localisent les états persistants, sans mesurer les pics transitoires. **Ne pas appeler restart_peak à chaque étape sans préserver le maximum public** : le banc le remet déjà au début de l’API et attend le pic de toute l’opération. Un compteur de phase séparé serait nécessaire pour conserver simultanément pic public et pics par phase. Aucun nouveau pic ni débit mémoire n’est mesuré dans cette capsule.

## Vérification

`scalar_model.py` est autonome, normal/−O identiques : contraintes temporelles sur petits intervalles exacts, témoin900>800, calendrier36/omission W8 et ratios/residu. Il ne charge aucune fonction du produit, ne simule aucun pthread/atomique et ne constitue aucun chrono. `SOURCE_AFTER.json` consigne séparément le LIVE final ; les conclusions portent sur les copies a7cd avant lecture. Toute dérive ultérieure demande sa propre lecture/qualification.
