# Schémas de la sonde et du plan après le levier MEB

Statut au 23 septembre 2026 : constat de versionnement sur le produit publié
`458fb0ed` → `78e94b04`, sans défaut géométrique déduit et sans nouveau reçu G4.

**Clôture au pin `f55ea40c` :** le worker et la sonde utilisent désormais
`mhgp9_tower_probe_v11`, le plan `mhgp9_tower_plan_v6`, et le selftest
refuse explicitement les anciens libellés. La contrelecture du pin donne
21/21 selftests Python normal et 21/21 sous `-O`. La rupture v10/v5 décrite
ci-dessous reste le diagnostic historique des deux snapshots, pas un
défaut encore actif. Aucun reçu de calcul G4 ne découle de cette clôture.

Les deux commits annoncent `mhgp9_tower_probe_v10` et
`mhgp9_tower_plan_v5` (`gcp-migration/tower_worker_v9.py:49–51` ; la sonde
écrit encore v10 dans `morsehgp3D_v9/bench/tower_probe.cpp:170`). Pourtant,
`78e94b04` ajoute `tower_meb_proposal` aux cinq leviers et impose quatre
champs entiers dans `tower_work` : `meb_proposals`,
`meb_verified_proposals`, `meb_boundary_canonicalizations` et
`meb_proposal_fallbacks` (`tower_worker_v9.py:99–100,137–141` ;
`tower_probe.cpp:175–176,218–220`).

La rupture est bidirectionnelle. Dans chaque version, `_levers` exige
**l'égalité exacte** des noms (`tower_worker_v9.py:223` dans `458fb0ed`,
`:228` dans `78e94b04`) ; `_tower_work` exige l'égalité exacte des clés
(`:352` puis `:357`). Un plan ou JSON de la version précédente est donc
refusé par le nouveau worker, et réciproquement, malgré les mêmes étiquettes
v5/v10. Le plan est effectivement validé par `_levers` dans `validate_plan` ;
la sortie est validée par `_levers` et `_tower_work` dans `validate_probe`.

Les reçus correctement épinglés restent interprétables : le manifeste vérifie
le hash du worker exécuté (`tower_worker_v9.py:301–305` dans `78e94b04`) et
les sources du snapshot. Le risque est d'utiliser v5/v10 *seuls* comme
identifiants d'un format ou de mêler des outils de ces deux commits.

Avant une nouvelle session G4, donner au contrat étendu les étiquettes
`mhgp9_tower_plan_v6` et `mhgp9_tower_probe_v11` dans worker, sonde et
selftest ; le générateur de plan reprend `worker.PLAN_SCHEMA`. Ajouter deux
mutations qui refusent explicitement un ancien plan v5 à cinq leviers et une
ancienne sortie v10 sans les quatre compteurs, puis rejouer les portes
protocole normal et `-O`. Cette correction de protocole ne requiert pas de
changer l'algorithme MEB ni de requalifier les reçus antérieurs.
