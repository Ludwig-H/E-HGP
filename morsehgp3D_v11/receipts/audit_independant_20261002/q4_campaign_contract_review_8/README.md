# Contre-audit du protocole q4 différé — 2 octobre 2026

Source produit épinglée : `ffc2ff95f0ae7296bdc522df81df34c58c3fdf47`. Cette capsule contrôle les bancs, plans et lecteurs en Python ; elle ne qualifie aucune exécution native, G4, FULL ou GPU. Aucune compilation, aucun appel cloud et aucun test produit natif n'a été lancé.

## État de preuve

Les [sources avant](sources_before.json) et [sources après](sources_after.json) recensent 24 fichiers : 16 blobs Git épinglés et huit lecteurs ou fixtures copiés avec leur chaîne d'import. Les outils et lecteurs n'ont pas changé. Seul `docs/DEVELOPPEMENT.md` a changé en LIVE ; sa copie reste celle du commit épinglé. Au dernier inventaire, le 2 octobre à 15:41:36 UTC, le répertoire q4 ne contient que le lecteur, son selftest et son JSON ; aucun reçu de campagne ni archive close n'est présent. La [documentation figée](sources/morsehgp3D_v11/docs/DEVELOPPEMENT.md), ligne 231, annonce correctement une qualification G4 à venir.

## Constats utiles

Le [plan actif](sources/morsehgp3D_v11/bench/plans/q4_levels_g4.json) prévoit trois commandes : matrice (400 s), complément ASan18 (100 s), puis profils (1 100 s), avec liens explicites vers leurs sorties distinctes. Le budget natif théorique est 36 × 30 = 1 080 s ; les 20 s restants dans la commande profils ne garantissent pas l'achèvement du traitement des sorties. Les interruptions, refus et omissions restent déclarés par le collecteur.

Le [pilote](sources/morsehgp3D_v11/bench/catalogue_profiles.py), lignes 174–193, compare les nouveaux compteurs q4 séparément des signatures sémantiques et des neuf compteurs historiques. Deux profils survivants différents suffisent à produire `different` même si le troisième expire. `q4_levels` égale le nombre de boules q_min=4 émises par passe et ne dépasse pas les candidats ; le chrono API couvre deux passes. Ajouter [qmin_counts](sources/morsehgp3D_v11/bench/catalogue_semantic.py) au diagnostic laisse le digest sémantique inchangé.

Le [lecteur q4 figé](reader_copies/morsehgp3D_v11/receipts/catalogue_q4_20261002/check.py) exige la source ffc2ff95f, une matrice conforme puis le complément ASan18, des commandes closes et une archive cohérente avant le banc. Ses attentes candidate=831, power_paths=207 et Fraction=11 838 sont des critères préparés ; elles ne constituent pas des portes natives déjà jouées dans cette capsule.

L'[ancien plan de profils](sources/morsehgp3D_v11/bench/plans/catalogue_profiles_g4.json), historique de la campagne 9df, omet le nouvel argument obligatoire `--supplement`. Le rejeu de son argumentaire s'arrête au parseur avec le code 2, avant toute E/S ou exécution native : [stderr conservé](legacy_plan_parser.stderr). Le plan q4 actif contient cet argument.

Le pilote accepte un entier JSON artificiel de 2^64 tandis que le lecteur final exige une valeur u64. Cette asymétrie hors domaine de sortie native ne démontre pas un défaut du contrat C++ : ses compteurs natifs sont u64. La première sonde appliquait à tort la borne du lecteur au pilote ; son [code initial](review_pre_bound_assumption.py), son [exécution échouée](own_review_runs_pre_bound_assumption.json) et ses sorties sont conservés. La sonde corrigée enregistre explicitement cette limite sans la présenter comme une panne native.

## Rejeux autonomes

Les [sept commandes initiales](review_runs.json) comprennent les modes normal et −O du décodeur, du collecteur avec sous-processus factices et du selftest du lecteur ; leurs sorties sont identiques entre modes. Elles passent : décodeur 8 fixtures/17 corruptions, collecteur 17 essais/12 corruptions/7 schedules/1 interruption/10 corruptions du complément, lecteur 12 fixtures/41 corruptions. Le septième contrôle est le refus attendu du plan historique (code 2).

La [sonde indépendante](review.py) passe aussi en normal et −O avec sorties identiques : [commandes](own_review_runs.json), [verdict](review_normal.stdout). Elle vérifie une petite boule témoin (6 arêtes, 4 faces, 1 tétraèdre), le digest, les comparaisons partielles q4, les types booléens/négatifs refusés, la limite u64 du lecteur et les budgets/liens des plans. Toutes ces preuves portent uniquement sur des données encodées ou des appels factices, avec zéro exécution native.

## Fermeture

`LEDGER.json` donne les hashes et tailles de tous les payloads. `SHA256SUMS` couvre ces payloads et le ledger. Les captures historiques déjà fermées n'ont pas été modifiées. La qualification native q4 reste en attente d'une campagne G4 fermée et de sa recoupe indépendante.
