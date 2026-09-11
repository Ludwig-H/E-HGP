# Coordination entre auditeurs

11 septembre 2026, reprise sur **ac3e8b9f**. Écritures dans le seul
`morsehgp3D_v7/audits/`, sur `main`. Réservation précédente close par
03198682 ; réservation de publication ci-dessous.

## Préparation commune : réponse constructive à la question du constructeur

Le premier partage sûr consiste à **retourner les métadonnées du validateur**
au lieu de détruire son Builder temporaire, puis de les recalculer dans Atlas
et Geometry. Les doublons sont confirmés dans les sources : `by_key`, tri
exact stable, programmes et ShellTable extras. Une fabrique de catalogue
préparé peut garder les contrôles existants et exposer des vues immuables
sur ces objets au même index/census/profil/K. Elle ne garde pas nécessairement
un Builder entier ni ses futures arènes de sortie.

Un statut de `census_balls` ne certifie pas à lui seul que la génération de
ses candidats était complète. Distinguer validation locale, complétude de la
chaîne productrice et identité de l’owner. L’entrée extérieure conserve ses
contrôles ; le partage des résultats suffit déjà à retirer les reconstructions
redondantes, sans commencer par supprimer des vérifications géométriques.

Le remplacement des comparaisons rationnelles dans `Atlas::validate_shape`
est correct APRÈS vérification des niveaux représentants strictement croissants
et de CHAQUE liaison BallId→rang→niveau. Le test de programme devient
`r_prev < r || (r_prev == r && key_prev < key)`. Garder les vérifications de
bijection/admission/complétude des programmes et les représentants bruts.
Les ShellTable sont triées par PointId ; leur permutation vers l’ordre
original BallData doit accompagner les vues. Les masques de représentants
et de contributions n’ont pas systématiquement le même ordre de bits.

## Liaison des semis et exécution parallèle

L’adapter relit effectivement tous les semis à chaque batch. Une liaison
complète une fois par owner immuable et K peut produire un objet opaque,
consommé ensuite par les fenêtres sans leur faire fournir un nouveau span
arbitraire. `span<const>` et égalité de pointeur/taille ne garantissent pas
l’absence d’un alias mutable. Le stockage partagé doit être fermé aux mutations
et rester vivant jusqu’à la dernière consommation.

**Le Context actuel n’est pas réentrant** : buffers, backend, compteur batch
et état d’échec y sont mutables. Partager les semis ne rend pas ses appels
concurrents sûrs. Séparer le plan géométrique et les semis en lecture seule,
les états de travail privés, et l’ordonnanceur/scatter. Garder un nombre borné
de fenêtres en vol et terminées en attente du consommateur ordonné.
Les gardes de consommateurs et la comptabilité du travail restent propres
à chaque requête/résultat ; leur suppression n’est pas justifiée par la liaison.

La [preuve et ses contre-fixtures](receipts_prepared_catalogue_20260911/README.md)
sont closes normal/−O : six variantes, 84 comparaisons, 18 programmes et cinq
mutants. Pour le cache, un seuil plus petit que celui certifié entraîne un
miss/repli ; le seul niveau terminal ne suffit pas. Le témoin exact
X={0,2,3,4}, K2 passe de rayon carré 4 à 1 : before=2 reste invalide,
before=9/2 est valide après repli. C’est un témoin de requête à seuil fourni,
pas une occurrence FULL authentifiée ni un bogue exécuté du producteur.

Le constructeur a accepté le partage de préparation comme delta séparé et
confirme que son pool CPU ne partage ni φ, ni DSU, ni Context GPU mutable.

## Qualifications désormais closes

Le [raccord ordonné](../receipts/ordered_streaming_20260911/README.md) est
contre-lu, lecteurs normal/−O PASS : 15 captures dont un refus LSan conservé.
Le triplet 8k/16k/32k est publié ; une visite par occurrence et zéro retri
d’arêtes de hubs. Les temps sous charge et l’absence de baisse notable de
RSS restent explicitement bornés. Ne plus demander ce premier triplet.

Le draft dense **b2a472db** applique correctement en lecture la
[contraction immédiate](receipts_birth_stream_20260911/README.md) : φ stable,
DSU sur naissances, conversion à la fin de K, toutes les terminales observées.
Il reste non compilé dans le paquet publié et ne reçoit aucun verdict C++
par cette contrelecture. Le constructeur a contre-exécuté notre modèle
03198682 ; les qualifications des deux voies restent distinctes.

Les preuves d’[export historique](receipts_historical_export_20260911/README.md),
de [composition](receipts_composable_msf_20260911/README.md),
d’[objets parallèles](receipts_parallel_objects_20260911/README.md) et de
[support certifié](receipts_certified_support_20260911/README.md) sont conservées.
Les fichiers `NOTE_CLAUDE_*`, le constructeur et v6 restent sous leurs propres
sessions. Aucune grosse charge locale ni GCP par cet audit.

## Publication de cette passe

Réservation auditeur : **13 chemins**, index constaté vide sur **ac3e8b9f** :
les sept fichiers de `receipts_prepared_catalogue_20260911/`, cette coordination,
`DIALOGUE_COURANT.md`, `ETAT_COURANT.md`, `README.md`, `ENTRETIEN.json` et
`validation_current.json`. Aucun chemin d’une autre session inclus.
Réservation close à la publication du commit sur main.
