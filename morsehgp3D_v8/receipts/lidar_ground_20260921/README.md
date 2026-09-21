# LiDAR sans sol : masque reproductible et coordonnées conservées

21 septembre 2026. `exploration_v8_hors_registre`, `cpu_reference`,
`public_status=not_claimed`. Qualification distincte du masque Patchwork++
et de la préparation des entrées, **pas de qualité sémantique du sol,
de census HGP, de tour FULL ni de GPU/G4**. Aucun GCP utilisé.
Voir le [protocole](../../docs/LIDAR_SANS_SOL_PROTOCOLE_20260921.md).

## Captures closes

| Capture | Commandes | Portée |
|---|---:|---|
| [Release](release/ground_fq64xq_6/COMPLETION.json) | 44, PASS | Petites fixtures, trois trames × trois états neufs, six préparations et douze lectures |
| [Clang ASan/UBSan/LSan](sanitize/ground_ggg5tyc0/COMPLETION.json) | 17, PASS | Petites fixtures et refus uniquement ; aucune grande trame sous sanitizer |

Chaque capture comprend les vingt tests indépendants de préparation en
Python normal et `-O`, une description des paramètres chargés, quatre
petites exécutions natives et dix refus CLI attendus. Les fixtures natives
couvrent brut vide, retours tous indécis, plan avec obstacles/doublons et
la même géométrie avec réflectances NaN. Les deux derniers masques doivent
être identiques et la classe sol doit être non vide ; ce plancher n'est
pas un score de segmentation. Vide/tout indécis : zéro appel à la bibliothèque.
Le préparateur refuse un brut vide, mais accepte sept résultats retenus vides.

Les refus conservent code1 et diagnostic attendu : fichier absent/tronqué,
XYZ non fini, option non finie/négative, portée incohérente, option dupliquée,
inconnue ou incomplète, sortie déjà présente. Aucun crash ni erreur de
compilation n'est compté comme test positif. Aucun essai en échec dans
ces deux campagnes ; les [préflights](preflight/README.md) et
[builds](builds/README.md) ont leurs propres reçus.

Le [lanceur](../../bench/run_lidar_ground_checks.py) consomme deux builds
déjà clos, sans les reconstruire : `build/v8_ground_patchwork_20260921`
et `build/v8_ground_patchwork_sanitize_20260921`. Leur autorité est vérifiée
avant/après, avec sources, dépendances compilées et exécutable ; elle est
reproduite dans chaque `BUILD_AUTHORITY.json`. Les snapshots tiers et
licences sont liés au commit Patchwork++ `3e6903a1d5537a4cc2ace897b0bbb98a92d6014c`.
Les sources antérieures du moteur, ses builds et CMake restent inchangés.

## Ce que donnent les trois trames

Un seul masque est calculé sur chaque **trame brute entière**, puis réutilisé
pour float32 et grille1mm. Trois processus neufs par trame donnent des
masques identiques octet pour octet et les mêmes paramètres/comptes.
Ce sont trois acquisitions de la séquence08, pas plusieurs séquences.

| Trame | Retours bruts | Sol prédit retiré | Non-sol prédit | Indécis conservés | Sites retenus, deux profils |
|---|---:|---:|---:|---:|---:|
| 000000 | 123389 | 83504 | 39844 | 41 | 39885 |
| 000100 | 124479 | 88928 | 35519 | 32 | 35551 |
| 000200 | 125526 | 79681 | 45811 | 34 | 45845 |

Tous les indécis de ces trames viennent de la portée ; aucun domaine
numérique exclu, sentinel tiers ou ID tiers non assigné n'est observé ici.
Les fixtures exercent le domaine numérique exclu et la sentinelle ; aucune
omission tiers supplémentaire n'est positivement exercée. Seuls les retours
explicitement classés sol sont retirés. Leurs IDs restent publiés ; un site à décisions
mixtes serait conservé dès qu'un de ses retours est conservé.
Ces trois trames n'ont ni doublon XYZ, ni fusion1mm, ni décision mixte ;
les tests synthétiques exercent ces cas, pas les mesures réelles.

Les XYZ float32 retenus restent ceux du brut, hormis normalisation des zéros
signés par le préparateur, nulle ici. La grille1mm et sa translation sont
préparées sur le brut complet avant sélection, jamais recentrées sur le
résidu. Il y a six préparations et42nuages : full, deux moitiés, quatre quarts.
Les correspondances retours/sites d'origine/sites retenus/IDs locaux sont
conservées ; la réflectance reste dans le brut haché. Pas de labels lus.

Les effectifs retenus sont identiques entre profils, mais pas nécessairement
leur partition. Trame200 : un site retenu change de demi-plan x après
quantification ; la moitié négative passe de25731 à25730 et la positive
de20114 à20115. Les manifestes détaillent toutes les coupes et les changements
du brut, qui comprennent aussi des retours ensuite retirés. Ni absence de
fusion, ni nombres identiques ne prouvent l'identité des distances/topologies.
Les fichiers f32/u32 ne sont pas des entrées du moteur historique u16.

## Temps observés, mono-thread local

Trois répétitions, médiane [minimum ; maximum], en millisecondes :

| Trame | Segmentateur | Lecture brute → masque fermé |
|---|---:|---:|
| 000000 | 21,186 [21,164 ; 22,943] | 30,358 [29,837 ; 31,772] |
| 000100 | 21,141 [20,793 ; 22,036] | 29,843 [29,536 ; 31,226] |
| 000200 | 21,271 [20,352 ; 21,366] | 29,927 [29,106 ; 30,300] |

Le segmentateur comprend construction/destruction de l'état neuf et
extraction des listes d'IDs. Le temps `wall` natif comprend lecture du brut,
validation/préparation de sa copie, segmentation, remappage/hashes et
écriture/fermeture du masque ; **pas** démarrage du processus, émission du
JSON, préparation Python des deux profils, ni leurs lectures de preuve.
Les commandes de préparation n'ont pas de chrono subprocess séparé dans
ce reçu : leur coût n'est ni nul, ni inclus dans le tableau ci-dessus.
Les six durées natives de chaque répétition restent publiées intégralement.

Eigen est limité à un thread, sans OpenMP. SAN puis Release ont été lancés
séquentiellement, mais deux processus CPU de l'auditeur travaillaient en
parallèle sur cet hôte partagé. Ce ne sont pas des latences isolées, ni un
gain stable, ni une mesure sur G4. Aucun réglage sur labels n'a été réalisé.
Le filtre de portée emploie une distance double approximative ; le lecteur
Python vérifie la même politique numérique. Une divergence Python/C++ près
d'un bord ferait échouer le reçu, pas retirer silencieusement un point.
Aucune exactitude radiale universelle ni portabilité inter-libm n'est revendiquée.

## Relecture et empreintes

Le lecteur vérifie commandes exactes, codes, flux bruts/base64, FNV du brut
et du masque, statistiques, provenance du descripteur et reconstruction
complète des coordonnées/IDs. Les inventaires incluent les `COMPLETION.json`
imbriqués ; seul celui du conteneur est exclu de sa propre empreinte.
Les mêmes contrôles sont exécutés avant de fermer une capture PASS.
Interruption : le collecteur annule et joint son groupe enfant, puis
conserve sorties partielles et première cause. Aucun plafond de temps de test.
Les quatre lectures finales normal/−O passent, avec sorties octet pour octet
identiques par build et sources/entrées/binaires/clôtures inchangés :
[READBACK.json](READBACK.json) conserve commandes, flux complets et empreintes.

```bash
python3 -B morsehgp3D_v8/bench/run_lidar_ground_checks.py read --path morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6
python3 -B -O morsehgp3D_v8/bench/run_lidar_ground_checks.py read --path morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6
python3 -B morsehgp3D_v8/bench/run_lidar_ground_checks.py read --path morsehgp3D_v8/receipts/lidar_ground_20260921/sanitize/ground_ggg5tyc0
python3 -B -O morsehgp3D_v8/bench/run_lidar_ground_checks.py read --path morsehgp3D_v8/receipts/lidar_ground_20260921/sanitize/ground_ggg5tyc0
```

Clôtures :

- Release : `67d1e6a8428cf92bf892001fd5cd31646677d0627f75ddff9115fd6c7292fa6d`.
- SAN : `8fd805912aeba52bf02f6ab9e7749629e3853e4a58c77cfdd61ec5502c73f7ec`.
- Lanceur : `49d49fe8612cd37960c7b81bc16f74d049e35d04fe944f96990e177201d1f584`.

Les captures gardent250fichiers/25338644octets Release et37fichiers/890908octets
SAN, sans archive binaire supplémentaire. Le lecteur est **LIVE** : ces builds,
sources et bruts doivent rester accessibles ; snapshots/hashes ne remplacent
pas automatiquement une distribution autonome de toutes les dépendances.

La diminution de population ne prouve pas une diminution proportionnelle du
travail HGP : retirer des témoins peut créer de nouveaux candidats admissibles.
La prochaine campagne doit comparer le flux natif global brut/sans sol et
ses coûts complets. La présente qualification prépare ce régime, elle ne
ferme aucun contrat de tour, de sous-quadraticité globale ou de précision
sémantique. Le [plan du raccord natif](../../docs/RACCORD_NATIF_GLOBAL_PLAN_20260921.md)
sépare explicitement ces travaux encore ouverts.
