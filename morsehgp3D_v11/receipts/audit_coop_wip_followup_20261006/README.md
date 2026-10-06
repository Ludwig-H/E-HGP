# Premier noyau coopératif : corrections proposées avant G4

6 octobre 2026, WIP développeur `v11-impl-l3`, base Git `3b76a3fcf`.
Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Les empreintes distinguent ce WIP de la base publiée. Aucun code développeur
modifié, aucun build, test natif, benchmark ni appel GCP par les auditeurs.

Deux patches sont prêts à intégrer puis qualifier :

1. [Garde CUDA](fix_cuda_guard.patch) : lire `s.unresolved` dans un booléen
   privé puis synchroniser le warp avant les écritures atomiques de la boucle.
   Le garde ordinaire actuel peut être encore lu lorsqu'une autre lane écrit.
   [Sources et modèle d'ordonnancement](cuda/README.md), normal/−O conformes.
   Aucun diagnostic CUDA, crash ou résultat FULL incorrect prétendu.
2. [Portes et fixtures](gates/suggested_targets.patch) : demander explicitement
   les unités `tower` et `catalogue`, utiliser les six noms CTest réellement
   enregistrés, ajouter une boîte exerçant la dominance et adapter le refus
   `near_max` au profil. Le mutant de publication partielle force u21.
   [Résolution des cibles](gates/README.md), normal/−O conformes.

`git apply --check` passe pour chacun sur le WIP relu ; les patches ne sont
pas appliqués dans son worktree. Ils touchent des fichiers distincts et
peuvent être intégrés ensemble. Leur compilation et leurs verdicts natifs
restent à obtenir sur G4.

Le [témoin exact de cinq sites](mathematics/README.md) fait passer J2 de3 à4
quand le masque initial est omis. Les fixtures précédentes avaient toutes
des dominances nulles. Le même reçu prouve que les trois cas `near_max` ne
peuvent refuser en u18. Quarante contrôles bornés, normal/−O identiques ;
aucun oracle complet de l'exécution CUDA n'en découle.

Les conseils antérieurs sur le layout, la préparation séquentielle i<j et
le mutant des descendants sont intégrés en source. Le layout partagé vaut
8 040 octets sous l'ABI déclarée, à confirmer au build. Suffixes, préfixes et
refus avant publication sont favorables en lecture, hors le garde signalé.
Le banc nouvellement préparé couvre deux synthétiques u21 et six passages
Compute Sanitizer ; aucune mesure ni qualification G4 n'est encore affirmée.

Les manifestes des trois sous-dossiers sont conservés. Le manifeste racine
ferme cette compilation et le patch CUDA additionnel. Aucun octet LiDAR
n'est inclus ; les notes actives indiquent l'état ultérieur des corrections.
