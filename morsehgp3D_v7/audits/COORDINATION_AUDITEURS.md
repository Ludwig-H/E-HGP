# Coordination entre auditeurs

11 septembre 2026, reprise après **e3903b2a**. Écritures dans le seul
`morsehgp3D_v7/audits/`, sur `main`. Réservation d’index pour six fichiers de ce dossier uniquement :
`COORDINATION_AUDITEURS.md`, `DIALOGUE_COURANT.md`, `ETAT_COURANT.md`,
`README.md`, `ENTRETIEN.json`, `validation_current.json`. Aucun autre
fichier à inclure. Réservation close à la publication de ce delta.

## Delta utile : gardes de rang au scatter

Le [dialogue courant](DIALOGUE_COURANT.md) applique la preuve publiée des
rangs aux gardes `first_consumer`, `terminal_admission_strict` et
`seed_not_strict`. Tous les contrôles de domaine/admission restent présents.
À 32k, les captures ordonnées scellées donnent **90 662 398** comparaisons
rationnelles de scatter et **26 901 500** de semis initiaux substituables.
Ce comptage logique ne prédit ni la latence ni le nombre de MEB.
Les gardes des MEB initiales/intermédiaires restent exactes et distinctes.
Le constructeur a accepté ce delta séparé ; le premier reçu dense/parallèle
reste figé. La préparation commune et les contrôles MEB sont conservés.

## Raccord dense et workers : lecture favorable

Source dense **87eb210e**, gate **b2427c24** : capture privée O2 contre-vérifiée
en lecture, 26 commandes closes, 114 census et 456 essais ; les trois fautes
φ=find, pivot oublié et conversion précoce ont chacune un hit effectif.
Le constructeur annonce SAN clos et prépare son paquet ; notre lecture
O2 seule ne reçoit pas automatiquement ces autres résultats.

Pool **4ec1a206**, gate **2c7358a7** : quiescence avant retour/exception,
construction partielle et réemploi traités. Raccord parallèle **f400de79**,
gate **ce2e18b5** : scratch et statistiques privés, leaders disjoints,
scatter puis φ/DSU dans l’ordre original, un pool pour toute la tour et
une fenêtre en cours. Aucun défaut bloquant trouvé en lecture.
Préparation, tri, scatter, φ/DSU et boucle K restent séquentiels ; les
compteurs distinguent fenêtres géométriques et tâches réellement résolues.

Correction de runner proposée : `record_workers.py` appelle subprocess.run
sans timeout. Borner chaque commande et conserver une capture failed si
une fixture à latch reste bloquée après régression. Aucun blocage observé.
Le refus TSan code66 annoncé survient avant les tests ; ce n’est pas une
validation de l’absence de races.

## Acquis et entretien

Le détail du [plan préparé et des semis liés](receipts_prepared_catalogue_20260911/README.md)
reste dans e3903b2a ; le constructeur en a accepté les principes. Les
anciennes recommandations répétées ont été condensées dans les notes
courantes. Preuves, échecs scientifiques et fichiers du second auditeur
restent intacts. L’[entretien](ENTRETIEN.json) conserve les commandes et
empreintes de cette lecture. Aucun C++ exécuté par cet audit. GCP non utilisé.
