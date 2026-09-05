# Qualification ciblée du producteur FULL Gabriel

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Les **sept CTests nommés** passent sur un build Release neuf puis sur
les mêmes sources dans un build ASan/UBSan neuf, LeakSanitizer actif.
Le [résultat détaillé](qualification.json) porte uniquement le
[producteur horizontal relatif](../../docs/CONTRAT_PRODUCTEUR_FULL_GABRIEL.md)
et son [certificat structurel](../../docs/CONTRAT_CERTIFICAT_FULL.md).
Ce n'est ni la suite F complète, ni une intégration de la CLI ou de la
verticale FULL, ni une promotion industrielle.

## Tentatives conservées

| Exécution | CTest | Temps total rapporté par CTest | Preuves |
| --- | --- | --- | --- |
| Release, build neuf | 7/7, code 0 | 0,27 s | [sortie verbose](release_ctest.txt), [LastTest](release/LastTest.txt), [JUnit](release/junit.xml) |
| SAN, première tentative dans le contexte enfant | 0/7, code 8 | 1,26 s | [échec verbose](sanitized_ctest_attempt1.txt), [LastTest](sanitized_attempt1/LastTest.txt), [JUnit](sanitized_attempt1/junit.xml) |
| SAN, reprise ROOT en exécution normale | 7/7, code 0 | 1,32 s | [sortie verbose](root_sanitized_ctest_attempt2.txt), [LastTest](sanitized_attempt2/LastTest.txt), [JUnit](sanitized_attempt2/junit.xml) |

La première tentative SAN échoue explicitement dans LeakSanitizer, qui
signale l'incompatibilité avec le traçage. Elle reste un échec, sans faux
vert. Une demande d'exécution escaladée a été interrompue sans résultat
d'exécution ; elle n'est pas comptée comme une tentative qualifiée.
ROOT a ensuite relancé les sept portes dans son contexte normal non tracé,
sans override de permission, ni désactivation d'un sanitizer. Les options
`ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` et
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1` restent actives.
La [commande ROOT](root_sanitized_attempt2_command.json), ses
[pins avant exécution](root_sanitized_before.json) et
l'[amendement d'exécution](execution_amendment.json) distinguent ces étapes.
Le JUnit de ROOT a un nom neuf ; le premier JUnit n'a pas été écrasé.

Ces temps sont ceux de petites portes de qualification, pas des mesures
de construction HGP sur 50k points. Les horodatages de réception dans
[commands.json](commands.json) peuvent inclure un délai de restitution :
leurs différences ne sont pas des chronomètres de compilation.

## Portée des contrôles

Les deux configurations produisent les mêmes lignes de verdict :

- FULL nominal : 10 nuages/configurations fixes, 30 catalogues réels,
  136 records, 67 exécutions d'ordres et 1 492 coupes Gamma ouvertes/fermées ;
  43 701 contrôles, zéro échec. Les catalogues sont générés une fois par
  nuage puis confrontés à l'oracle OBig exhaustif borné à huit points.
- FULL rejets : 80 refus, 44 982 contrôles, zéro échec. Le portail rencontre
  réellement une coquille externe dans une fixture déclarée non régulière
  avec catalogue sciemment invalide. Une autre sentinelle conserve les
  minima aigus mais omet ABC : succès relatif, réfuté par Gamma. Le composant
  n'authentifie donc pas sa précondition de complétude des catalogues.
- Mémoire du producteur : 102 allocations observées et 102 pannes persistantes
  balayées, 786 contrôles, zéro exception échappée ; trois positifs avec portail.
- Certificat structurel : 68 contrôles positifs et 218 contrôles du mode
  rejets, incluant 15 pannes d'allocation. Ses feuilles restent des facettes,
  pas une partition imposée des points.
- Les deux arguments inconnus sortent exactement avec le code moteur 2.
  Les cinq autres exécutions attendent le code 0 ; le wrapper CMake épinglé
  vérifie ces codes. Chaque CTest conserve un timeout de 60 secondes.

Les compteurs d'isolés sont des observations répétées aux coupes, pas des
points distincts. Les 392 tests enregistrés apparaissant dans LastTest
n'ont **pas** tous été exécutés : seuls les sept noms de l'inventaire le sont.
Le plancher actuel n'impose pas encore une deuxième itération de portail
nommée ni un état intermédiaire à exactement un intrus.

## Compilation et provenance

Les répertoires `build/v7_full_gabriel_20260905` et
`build/v7_full_gabriel_20260905_san` ont été constatés absents avant création.
Les trois cibles seulement sont compilées, CPU0, `--parallel 2` au maximum.
Release utilise `-O3 -DNDEBUG`. SAN utilise `RelWithDebInfo` explicitement
fixé à `-O1 -g -DNDEBUG`, avec `-fsanitize=address,undefined`,
`-fno-omit-frame-pointer`, `-fno-pie`, et `-no-pie` au lien.
C++20 et `-Wall -Wextra -Wpedantic -Werror` sont présents, sans
`MHGP7_TESTING`. GNU C++ 13.3.0 et CMake/CTest 3.28.3 sont capturés.

[commands.json](commands.json) référence les sorties configure/build/CTest
complètes, sans réutiliser les essais directs O2 antérieurs des agents.
Ces essais restent une antériorité explicitement séparée dans le résultat,
pas une provenance fraîche pour les builds CMake présents.

Les [56 pins sources avant](source_before.json) et
[après](source_after.json) sont identiques, ainsi que les six binaires
avant/après CTest et les trois exécutables de la chaîne d'outils.
Ce jeu de sources est un sur-ensemble, pas une affirmation que tous ses
fichiers ont été compilés. Les [liaisons de compilation](compile_bindings.json)
extraient les dépendances projet exactes des `.o.d` : 9 pour le certificat,
36 pour le juge FULL et 34 pour la porte allocation, dans chaque build.
Les `.o.d`, flags, liens, caches CMake et identification du compilateur sont
archivés intégralement. Les en-têtes système sont listés par le compilateur
mais n'ont pas été individuellement épinglés avant build : ce reçu ne
revendique pas une chaîne de construction hermétique.

Les [28 copies brutes](raw_copy_provenance.json) sont contrôlées octet pour
octet, lignes vides terminales incluses. Les captures ROOT restent intactes.
La [vérification de clôture](verification.json) compare les noms, verdicts,
sorties JUnit/LastTest, footers, dépendances et pins ; elle ne relance aucun
test produit. Le manifeste couvre les fichiers du paquet hors lui-même et
`SHA256SUMS` ; `SHA256SUMS` couvre aussi le manifeste, hors son propre contenu.
Depuis ce répertoire, `sha256sum -c SHA256SUMS` vérifie le sceau.

Aucun contrat de RAM, de 1 seconde ou de 100 ms, aucune tour massive ni
qualification GPU n'est déduit de ces portes. GCP non utilisé.
