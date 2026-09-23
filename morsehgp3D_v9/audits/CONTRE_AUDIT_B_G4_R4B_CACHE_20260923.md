# Contre-audit B — R4b, ablation du cache sur G4

23 septembre 2026. Lecture indépendante du reçu versionné
`receipts/g4_tower_r4b_20260923/`, publié au commit `406e7d40`, et de la
session hôte correspondante. L'auditeur n'a ni lancé ni arrêté GCP. Le
snapshot est celui de `a1d7a9bc` ; il ne contient pas le WIP FULL
postérieur. `sha256sum --check --quiet SHA256SUMS` passe sur les 245
objets du reçu, sans objet supplémentaire hors fichier de sommes. Le
hash de l'archive téléchargée concorde avec le reçu hôte et les 209
fichiers invités recopiés concordent avec la capture. Reçus hôte et
invité `completed`, 13/13 cas `complete_relative`, 14/14 commandes hôte
et 47/47 invité fermées avec code 0 ; sources, dépendances compilées et
binaire stables, préflight natif non vacant, arrêt ciblé certifié et VM
`TERMINATED`. Il s'agit bien de FULL **CPU relatif au catalogue émis** sur `g4-standard-48`, pas
d'une exécution GPU. Le lecteur v6 conserve la [lacune des champs de
garde archivés](CONTRE_AUDIT_B_G4_RECEPTION_V5_20260923.md) et les
[identités supplémentaires à contrôler](RECEPTION_V6_IDENTITES_MANQUANTES_20260923.md) ; les gardes externes observées dans cette session sont distinctes.

Les six paires adjacentes comparent uniquement cache ON/OFF, à trame,
K, `s=8`, 48 workers et tour statique 48 identiques. Les trois trames
entières **sans sol**, grille 1 mm, viennent toutes de SemanticKITTI 08.
Pour chaque paire, `input`, générateur, catalogue, ordres, `tower_work`,
digest et tout le ledger **sauf les trois compteurs du cache** sont
identiques. Il s'agit d'une identité de l'objet JSON exposé et du digest,
pas d'une comparaison octet par octet d'une sérialisation complète de
toutes les hiérarchies.

| Trame 08 | K | Total sans → avec cache | q3/q4 sans → avec | Covers | Formes chargées |
| --- | ---: | ---: | ---: | ---: | ---: |
| 000000 | 5 | 7,626 → 7,388 s | 4,222 → 3,964 s | 2 043 612 | 2,959 Md |
| 000000 | 10 | 27,241 → 26,688 s | 9,164 → 8,711 s | 4 507 278 | 7,796 Md |
| 000100 | 5 | 5,565 → 5,439 s | 2,895 → 2,758 s | 1 732 176 | 1,790 Md |
| 000100 | 10 | 19,639 → 19,398 s | 5,914 → 5,715 s | 3 673 260 | 4,151 Md |
| 000200 | 5 | 9,359 → 9,182 s | 5,609 → 5,477 s | 2 237 912 | 3,586 Md |
| 000200 | 10 | 30,394 → 29,500 s | 12,721 → 11,757 s | 4 927 304 | 9,281 Md |

Le gain total observé est **1,23–3,12 %** ; le poste q3/q4 gagne
**2,35–7,58 %**. Les CPU·s baissent aussi, mais un seul essai par paire
ne donne pas une distribution de bruit. La répétition supplémentaire
000000/K10/cache ON donne 26,712 s contre 26,688 s, même géométrie et
digest, avec de faibles variations des compteurs cache dues à l'ordre
des tâches. Les masses de paires développées, covers, sites de cover et
formes chargées restent **exactement inchangées** par paire. Les
**53,36–69,76 %** de paires développées que le cache rejette sans recherche
complète ne doivent donc
pas être traduits en réduction équivalente du travail total.

Correction du reçu produit : son README annonce **61–70 %** pour
`witness_cache_rejected_pairs / expanded_pairs`, mais 08/000100/K10
donne `9 331 351 / 17 488 839 = 53,36 %`. Les autres cinq cas vont
de 61,15 à 69,76 %. Cette erreur d'agrégation ne change ni les temps
ni l'identité des paires, mais la plage publiée doit être rectifiée.

Verdict de priorité : le cache fonctionne et son ablation isolée montre
un petit gain de tour, mais son coût de validation d'antichaîne
`O(m²)` par appel (jusqu'à 136 comparaisons à K10) n'a toujours pas de
compteur propre. Éviter une nouvelle campagne de micro-variantes du
cache avant d'attaquer les covers/formes q3/q4 et l'expansion des paires ;
mesurer ce coût si l'on retouche la voie. La tour FULL la plus rapide
mesurée ici est encore **5,439 s à K5** et **19,398 s à K10**. Une seule
séquence sans sol, aucun `s=10/12`, aucune coupe 8k/16k/32k appariée,
aucune trame brute, aucun GPU : ni cible 1 s/100 ms ni croissance
sous-quadratique globale acquise. Les comparaisons R3→R4b de FULL sont
entre sessions et snapshots différents, donc seulement indicatives.

## Identités supplémentaires rejouées sur les 13 sondes

Sur chaque JSON complet, les huit égalités suivantes passent :
`q3_seeds = q3_depth_rejections + q3_emitted =
q3_atlas_rejections + q3_ball_builds`, `q3_atlas_edges = both_edges`,
`tower_work.records = catalogue.balls`,
`tower_work.extra_records = catalogue.extra_shell_balls`, puis les
trois comptes `births`, `merges`, `contributions` égaux à la somme des
ordres. L'identité `q3_atlas_edges = both_edges` dépend ici des options
figées de la chaîne (`Local28`, consultation atlas active) et ne doit pas
être imposée à une API configurée autrement. Les autres découlent des
chemins complets du producteur et n'exigent
aucun nouveau coût géométrique. Les intégrer et tuer leurs mutations
isolées dans `validate_probe` renforcerait la réception sans modifier le
moteur ; voir aussi les gardes nécessaires de l'auditeur A ci-dessus.
