# Contrelecture indépendante du reçu G4 R14

23 septembre 2026. Lecture **hors GCP** du paquet publié à `3605cef99`,
produit depuis le commit épinglé `b68b6761`. Cette note complète le
[préflight B](CONTRE_AUDIT_B_PREFLIGHT_G4_R14_V19_20260923.md) par la lecture du
[reçu effectivement publié](../receipts/g4_tower_r14_20260923/README.md).

## Fermeture observée

- Export du reçu par `git archive 3605cef99` dans `/tmp` : les **326 entrées
  distinctes** de `SHA256SUMS` concordent ; le 327e fichier est le manifeste
  SHA lui-même. Les SHA du contrôleur et du worker annoncés dans `PACKAGE.json`
  coïncident avec les objets Git de `b68b6761` et avec `host/receipt.json`.
- Le `validate_received` du contrôleur **épinglé à `b68b6761`**, appliqué à
  `vm/`, `vm/sources_before.json`, aux 18 cas et aux identités/à la garde
  certifiées de `host/receipt.json`, rend `completed`. Les 18 issues sont
  `complete_relative` ; 12 sont des cas GPU achevés, 12 comparaisons entre
  cas portent `equal=true`, aucun jumeau n'est non apparié. Les 13 couples
  d'intention/commande hôte archivés concordent ; les 14 commandes consignées
  ont `exit_code=0` et `group_closed=true`. `oslogin_add` figure dans le reçu
  hôte, mais son journal brut est volontairement absent de l'archive.
- L'arrêt ciblé est marqué certifié. Ces contrôles soutiennent une mesure
  relative de **trois trames sans sol à grille 1 mm de la seule séquence 08**,
  pas une preuve FULL générale ni le contrat de trame brute ou multi-séquence.

## Attribution v19 : données réelles fermées, lecteur encore permissif

Sur les **12 sorties GPU** archivées, le résidu
`device_ms − kernel_ms − transfer_ms` du filtre et des certificats reste
entre −0,001 et +0,001 ms, exactement l'ordre de l'arrondi JSON. La
décomposition est donc numériquement fermée **dans R14**.

Le prédicat gelé `tower_worker_v9.validate_probe` accepte pourtant, sur le
vrai `vm/probe_0.stdout` et son cas, quatre mutations **isolées** : retirer
10 ms à `filter_kernel_ms`, 1 ms à `filter_transfer_ms`, 10 ms à
`certificate_kernel_ms` ou 1 ms à `certificate_transfer_ms` rend encore
`complete_relative` à chaque fois. Ces mutations ont été faites seulement
en mémoire après export du reçu ; elles ne décrivent pas les chronos R14.
Le validateur borne la somme par le total, sans exiger leur fermeture.
Exiger `abs(device_ms − kernel_ms − transfer_ms) ≤ tolérance_d_arrondi`
pour les deux voies et ajouter ces quatre mutations aux selftests fermerait
ce trou de réception.

Les intitulés ont aussi une portée physique distincte. Pour S2,
`tower_chain.cpp` classe `rect+scan+pair+select` comme *kernel*, alors que
`filter_runner.cu` exécute des copies synchrones D2H dans `scan` et
`select`. Pour S3, `certificate_kernel_ms` encadre bien le lancement du
noyau par événements CUDA ; `certificate_transfer_ms=upload+download`
comprend aussi allocations, initialisations et copies. Les paires
entrelacées soutiennent le gain **de chaîne** S3, mais les champs v19 ne
mesurent pas tous « calcul pur » et « copies pures ».

## Préflight disque distinct de la double garde

Au commit de la session, ni `tower_session_v9.py::run_session` avant
`guarded_start`, ni les quatre scripts de protocole transportés, ni le
`start_and_verify.sh` épinglé n'effectuent un contrôle d'espace libre de
**2 Gio sur `/workspaces`**. La marque `double_guard_verified` et l'arrêt
ciblé certifient la durée et le cycle de vie de la VM ; ils ne certifient
pas ce seuil disque. La réserve B antérieure reste donc ouverte pour R14,
sans invalider les chronos ni la fermeture de cette session achevée.
