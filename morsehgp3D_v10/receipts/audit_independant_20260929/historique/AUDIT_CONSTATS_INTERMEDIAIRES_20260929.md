# Constats déjà reproduits — base 6206d1d11

Rapport intermédiaire indépendant, 29 septembre 2026. Le rapport global suit dans ce dossier. Ces constats ne réfutent pas les objets et mesures publiés sur leurs entrées valides ; ils identifient des frontières qui doivent devenir des portes permanentes.

## 1. Entrée entière : une fin u32le incomplète passe pour un succès

Les trois CLI lisent des triplets par `fread(buf, 4, 3)` et abandonnent toute fin partielle. Quatre points valides suivis d'un octet, d'un mot ou de deux mots donnent neuf succès `status=ok`, code 0, avec `n=4`. Même le mot surnuméraire 262144, hors u18, disparaît avant validation. La lecture n'établit donc pas l'intégrité de la trame reçue.

Preuve : `FRONTIER_CHECKS.json`, `run_frontier_checks.py`. Source : `cli/mhgp10_catalogue.cpp:43`, `cli/mhgp10_tower.cpp:65`, `cli/mhgp10_cluster.cpp:105` (lignes à confirmer par le rapport final).

Correction ciblée : lecteur partagé qui refuse toute taille non multiple de 12 et toute erreur de lecture, puis valide tous les triplets. Régression pour les restes 1 à 11 octets sur les trois CLI. Aucun nouveau sous-échantillonnage implicite.

## 2. Échec mémoire dans un worker : arrêt du processus

`Pool::run_chunks` et `worker_loop` ne protègent pas l'appel de tâche. La sonde `pool_failure_probe.cpp`, deux fils et 64 petites tranches, injecte `std::bad_alloc` dans un worker : SIGABRT, code −6, au lieu d'une fermeture contrôlée. Les tâches produit font réellement des `vector::resize/push_back` et `make_shared`, donc cette exception est une voie d'échec réelle à grande échelle. La garantie de retour par statut et de libération transactionnelle n'est pas couverte par les tests de succès du pool.

Preuve : `pool_failure_worker.json`. Source : `src/sched/pool.cpp:28`, `:51`, `:80`. Une exception du fil appelant saute aussi la fermeture de `current_` et l'attente des utilisateurs ; cette seconde conséquence est établie par lecture, pas par une sonde positive ici.

Correction ciblée : garde RAII pour l'état TLS et la restitution de l'utilisateur, capture d'une exception/cause par job, annulation des nouvelles tranches, fermeture de la capture puis attente de tous les participants avant remontée/conversion en statut. Tester séparément échec caller, worker et construction partielle du pool. Préserver la correction de course `8e3b76245`.

## 3. Validation de dendrogramme insuffisante avant la tête

L'audit tête reproduit deux entrées acceptées par `validate` : un `point_rank` hors de `level` cause un dépassement de tas dans `head.cpp`; une relation parent absente du CSR laisse un point sans cluster. Il faut vérifier les rangs des points, les parents et l'équivalence parent–enfants, ainsi que les niveaux finis. Les fixtures et le diagnostic ASan seront dans `TETE_BANCS_PREUVES.md` et `evidence_*`.

## 4. Portée à clarifier, sans annuler les résultats

- Le lot C archivé est complet selon le recomptage de l'audit : 30720 couples, aucun doublon/manquant/refus. En revanche `decide.py` accepte un lot incomplet ; la porte de décision doit vérifier le manifeste exact, pas seulement les méthodes présentes dans les unités présentes.
- Les trois trames 08/000000, 08/000100 et 08/000200 démontrent les chronos annoncés sur ces trames, pas le contrat de conception sur 30 trames de 10 séquences. La phrase « contrat tenu » du reçu G4 session2 doit être bornée à ce diagnostic.
- Le test des requêtes rationnelles du `SiteTree` sur un centre non bien centré révèle une marge flottante insuffisante. La portée produit reste à vérifier : les requêtes issues de miniboules certifiées ont un centre borné dans l'enveloppe. Ne pas en déduire une erreur de la tour valide avant cette distinction.

GCP non utilisé. Aucun source produit ni reçu existant modifié.
