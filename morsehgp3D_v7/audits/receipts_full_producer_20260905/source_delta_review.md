# Delta documentaire et source du prochain probe FULL

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Revue bornée des octets capturés dans [source_delta_review.json](source_delta_review.json), avec `HEAD` observé `b63203b5897181d857cf87b98fa5cf96dfde3da7`. Les cinq documents, le probe et CMake sont conservés intégralement dans ce JSON. Il s'agit du worktree effectivement lu, pas de sept fichiers prétendument publiés dans ce commit. Aucune commande Git, compilation, exécution du probe ou campagne de mesure n'a été lancée. Le constructeur peut poursuivre ses modifications : cette clôture n'attend pas sa dernière version.

## 1. Portée des changements documentaires

Les [README](../../README.md) et [PASSATION](../../PASSATION.md) introduisent le producteur horizontal FULL après le lecteur structurel, en gardant séparés le moteur F, ses résultats historiques et la future tour intégrée. Ils attribuent les sept portes ciblées et leurs reprises sanitizer aux reçus du constructeur ; cette revue documentaire ne les réexécute pas. Les facettes isolées, K=n et l'autorité extérieure des catalogues sont explicites.

Le [contrat du certificat](../../docs/CONTRAT_CERTIFICAT_FULL.md) relie désormais le producteur relatif qui fournit ses lots. Son autorité propre reste `structural_only`, sans géométrie authentifiée par la seule validation des arènes. Le [contrat de performance](../../docs/CONTRAT_PERFORMANCE.md) rappelle qu'exécuter horizontalement 1..K ne qualifie pas encore la tour intégrée, sa verticale, son autorité terminale et sa publication. Les objectifs 50k/1 seconde puis 100 ms ne sont pas présentés comme atteints.

Le [contrat du producteur](../../docs/CONTRAT_PRODUCTEUR_FULL_GABRIEL.md) conserve les contrôles locaux et budgets, et rend précises les limites du corpus constructeur : 67 exécutions correspondant à 40 couples nuage/ordre et 27 répétitions, oracle borné à n≤8, pas de qualification dynamique de K9/K10 par ce corpus. Son témoin K10 structurel reste attribué à une autre porte. Il conserve aussi les limites de régularité globale du corpus et l'absence de plancher nommé sur un deuxième pas de descente. Aucun résultat du futur probe n'est déduit de ces chiffres.

Ces textes respectent la séparation des autorités. Leur actualisation ne transforme ni les anciennes qualifications F/G ni une comparaison de couvertures terminales en qualification géométrique nouvelle.

## 2. Comparaison exacte à G145

La variante observée dans [validation_current.json](../validation_current.json) est `G_full_structural`, contenant 145 pins. **144 correspondent au worktree capturé, dont les 48 fichiers sous `src/`.** La liste complète, attendus et empreintes observées, est dans le JSON de cette revue.

| Seul pin différent de G145 | Attendu G145 | Worktree capturé |
| --- | --- | --- |
| `CMakeLists.txt` | `3dc94a0cbb3fec76880567888c8340c47d4c6d5d6f42136d6cf9dbe693a1e4a6` | `7274e9be3ba42f391c06d66d5ab0caa451849fac36685630a43af0f32b5edf95` |

Le nouveau bloc CMake déclare deux exécutables de test du producteur et cinq CTests : positif, rejets, argument inconnu, allocations, argument inconnu de la porte allocations. Le retrait exact de ce bloc restitue le SHA256 CMake de G145. Les anciennes règles, options et cibles sont donc inchangées au niveau des octets, sans que leur construction ait été rejouée ici.

Ces nouveaux exécutables utilisent `mhgp7_product_executable` : aucun `MHGP7_TESTING` n'est ajouté par cette fonction. Les options C++20 et avertissements stricts restent les options générales du projet. Cette lecture d'une déclaration CMake ne qualifie ni sa configuration, ni les nouveaux binaires. La qualification G145 reste attachée à son ancien fichier CMake complet.

Le nouveau `bench/full_gabriel_probe.cpp` n'appartient pas aux 145 pins et **aucune cible CMake de ce probe n'est présente dans la capture**. Les nouveaux fichiers du producteur ne deviennent pas des sources G qualifiées du seul fait que les 144 anciens pins correspondent. Le contrat de performance porte par ailleurs un pin commun historique distinct de ceux de la variante G ; son nouveau texte est capturé séparément, sans modification du manifeste par cette revue.

## 3. Lecture bornée du probe non exécuté

Le [probe capturé](../../bench/full_gabriel_probe.cpp), SHA256 `f3de0d3ca850611f328cb41b251ec66c914afe473eed8e55f89eb889898f1849`, est un programme autonome. Il fabrique un nuage uniforme fixé, graine 3, domaine 65536, puis appelle index, génération, RLE, préfiltre, census, expansion et producteur FULL avec `threads=1`. Il réutilise le catalogue direct de l'ordre précédent comme minima de l'ordre suivant ; l'ordre effectif est tronqué à n, en conservant le minimum K=n sans connexion supplémentaire.

La lecture retrouve les gardes de refus de génération avant le contrôle du grand livre, la vérification de régularité pertinente de la fenêtre et des gardes de payload. Les plafonds FULL sont fixes par ordre et non surchargeables par les arguments. Le proxy nommé de payload de 8 GiB n'est pas présenté comme une limite RSS ; `RLIMIT_AS` réduit séparément la limite souple d'espace d'adressage à au plus 26 GiB, sans la relever. Cette distinction de sources ne vaut pas vérification d'un futur processus exécuté.

Après chaque ordre, le probe exige une racine terminale et une couverture égale aux points générés. Ces sentinelles ne vérifient ni les niveaux intermédiaires, ni les partitions de minima, ni l'égalité des forêts entre s=8/10/12. Les sorties déclarent donc explicitement l'absence de digest d'entrée et de certificat, et limitent cette comparaison aux coûts et volumes. Les forêts sont libérées après ces lectures ; aucune archive, verticale ou masse n'est exportée.

Les lignes par ordre sont marquées provisoires. Le statut terminal vérifie que tous les ordres effectifs ont été accomplis et garde la portée horizontale relative. Le temps de référence commence après la configuration, couvre la création de l'entrée, la construction, les lectures, les sorties provisoires et la destruction avant le terminal. La soustraction du temps de sortie est nommée diagnostic, pas chronométrage indépendant. Les compteurs de phases ne doivent pas être interprétés comme une décomposition exhaustive du temps du processus ou une mesure de tour intégrée.

Deux limites opérationnelles sont visibles dans les octets : l'admission initiale « n=8 seulement » est une consigne pour le runner, car le parseur accepte aussi 8000/16000/32000 ; et l'arrêt sur durée murale est délégué au runner externe. Avant un futur essai, ce runner doit donc porter l'admission choisie, le délai, le code de sortie et le terminal effectivement reçu. La simple présence du code ou d'une ligne provisoire n'est pas un essai réussi. Aucun essai de ce type n'est attribué ici.

## 4. Clôture de cette lecture

Les évolutions documentaires et le probe annoncé gardent une portée compatible avec les preuves déjà acquises. Les points concrets à préserver pour la prochaine étape sont l'enregistrement explicite du probe s'il doit être construit par CMake, l'admission et le délai externes, et l'absence d'attribution de G145 à un nouveau CMake ou à une nouvelle mesure.

Le JSON conserve la capture initiale ainsi qu'une observation de fraîcheur à la clôture. Si un fichier a changé entre les deux, sa nouvelle empreinte est signalée sans lui attribuer la lecture de l'ancienne version. Cette revue ne modifie aucun manifeste, reçu historique ou source constructeur. GCP non utilisé.
