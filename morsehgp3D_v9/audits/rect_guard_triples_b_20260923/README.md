# Shadow B — triples de gardes bornés sur rectangles LiDAR

23 septembre 2026. Expérience **audit-only**, hors produit et hors registre,
sur la trame brute entière SemanticKITTI **08/000000**, grille commune 1 mm/u18,
123 389 sites, K5/s8. Elle reprend exactement les rectangles, la palette et le
matching de paires du [shadow B](../rect_pair_shadow_b_20260923/README.md),
puis mesure le [certificat de groupes](../CERTIFICAT_B_GROUPES_GARDES_Q34_20260923.md)
avec des triples. Le résultat limite **cette sélection bornée de triples** ; il
ne réfute pas le certificat mathématique ni une sélection plus intelligente.

## Verdict et coût payé

Parmi les **1 747 grands rectangles** ouverts du front réel (produit ≥ 1 024),
**299** portent au moins une arête S2. Ce dernier nombre est une jointure
*diagnostique après S2* ; la sélection des gardes et les tests géométriques ont
été exécutés sur les **1 747**, sans lire le nombre d'arêtes ni `F` pour choisir.
Les 24 petits rectangles de contrôle du shadow B restent dans le reçu, mais
leurs coûts ne figurent pas dans les colonnes « grands » ci-dessous.

| Certificat sur grands rectangles | Rectangles positifs S2 fermés | Arêtes S2 | `F` potentiellement évitable |
| --- | ---: | ---: | ---: |
| Paires, matching maximum exact (baseline) | 72 | 2 175 | 5 059 809 |
| Triples seuls, glouton borné | 1, déjà fermé par paires | 12 | 15 658 |
| Mix paires + triples glouton, seul | 71 | 2 066 | 4 194 061 |
| **OU logique : paires maximum, puis mix si échec** | **74** | **2 219** | **5 113 332** |

Les deux fermetures réellement supplémentaires portent **44 arêtes** et
`F=53 523`, soit **0,00956 %** du `F=559 661 741` du cœur q3/q4 de cette
trame. Une fermeture du rectangle n'est qu'un potentiel de saut du cœur :
aucun gain net du pipeline n'a été mesuré. Le mix glouton isolé perd certaines
fermetures de l'appariement maximum ; seul le **OU** préserve la baseline.

Le sélecteur utilise la même palette de 32 sites au plus, choisie par les
quatre recherches indexées du shadow B à partir du premier couple de rangs
spatiaux du rectangle. Il **énumère tous les triples** de cette palette
(≤ 4 960), calcule au couple représentant les marges entières q3/q4, conserve
les 64 meilleurs triples strictement positifs par voie, puis déduplique
(≤ 128 propositions). Chaque proposition retenue subit les 64 coins virtuels
jusqu'au premier échec ; les groupes positifs sont assemblés sans réutiliser
de site, en essayant paires-avant-triples et triples-avant-paires. Un échec du
budget ou de la preuve conserve le repli exact. Ni `F` ni le statut S2 positif
n'interviennent dans l'énumération, le classement ou le test.

| Travail supplémentaire des triples | Tous les 1 747 grands | 696 grands non fermés par paires |
| --- | ---: | ---: |
| Triples énumérés et classés | 7 150 671 | 2 546 513 |
| Triples soumis aux coins | 149 897 | 61 477 |
| Couples de coins effectivement testés | 8 271 994 | 2 712 129 |
| Sélection, somme d'intervalles CPU locaux | 930,11 ms | 290,45 ms |
| Classification et glouton, somme d'intervalles CPU locaux | 205,86 ms | 66,06 ms |

La colonne des 696 est une **somme conditionnelle des lignes de cette même
exécution**, utile pour estimer le chemin « paires d'abord » ; elle n'est pas
un second chrono autonome avec sa propre localité mémoire. Le coût du
sélecteur de base des paires, payé avant les triples, est 79,44 ms pour les
1 747 grands, et leur classification + matching 662,51 ms dans ce run. Le
programme complet, y compris la reconstruction du front, la jointure des
traces et 24 contrôles, a pris 43,90 s mur, 43,55 CPU utilisateur et
277 504 KiB RSS sur l'hôte partagé. Ces durées locales ne prédisent pas un
port GPU/G4.

Les [quatre lignes `PROOF`](RUN.stdout) donnent les groupes et coordonnées
pour les deux rectangles supplémentaires, q3 et q4 séparément. Le
[lecteur indépendant](verify.py) refait **1 216 tests stricts de coins** avec
entiers Python et vérifie la disjonction des IDs, les agrégats et le
[résumé](RESULTS.json). Les quatre témoins passent ; les rectangles concernés
sont les ordinaux front 1 790 368 (`36` arêtes, `F=44 046`) et 2 938 279
(`8` arêtes, `F=9 477`). Pour le second, q4 est fermé par trois triples ; la
voie q3 est fermée par des paires. Ce sont des preuves positives réelles de
pouvoir supplémentaire **sur la relaxation des boîtes**, mais à masse faible.

## Provenance et rejeu

Le [source](probe.cpp) est une copie instrumentée du `probe.cpp` B à SHA-256
`7f4f2c4e50d0b9e4e3261288ad582abaffa3d9e971466d30a9878bb56c6c589c`
dans `main` (`6d447bb3949b1ff6774b290be8eeb7b034a2343c`). Il reproduit
la baseline B **72 / 2 175 / 5 059 809** sur les grands rectangles. Les
[empreintes](SHA256SUMS) couvrent les fichiers archivés ; `DEPENDENCIES.json`
épingle les deux entrées, les huit traces S2, la bibliothèque et le binaire
local. Ces dépendances `/tmp` et `build` restent **LIVE non versionnées** : le
reçu versionné permet la contrelecture statique complète, mais pas le rejeu
géométrique autonome sans elles. Le lecteur ne certifie pas à lui seul que les
boîtes archivées sont celles du front réel ; le source et le rejeu LIVE
assurent cette jointure.

```sh
sha256sum -c morsehgp3D_v9/audits/rect_guard_triples_b_20260923/SHA256SUMS
python3 -B morsehgp3D_v9/audits/rect_guard_triples_b_20260923/verify.py
python3 -B -O morsehgp3D_v9/audits/rect_guard_triples_b_20260923/verify.py
```

Avec les dépendances épinglées présentes :

```sh
g++ -std=c++20 -O2 -Wall -Wextra -Werror \
  -I build/v9-open-worktree/morsehgp3D_v9/src \
  -I build/v9-open-worktree/morsehgp3D_v9/src/gen \
  morsehgp3D_v9/audits/rect_guard_triples_b_20260923/probe.cpp \
  build/v9-open-worktree/build/v9-dev/libmhgp9_gen.a -pthread \
  -o /tmp/mhgp9-rect-guard-triples-b
timeout 180s /tmp/mhgp9-rect-guard-triples-b \
  /tmp/mhgp9-s2-scaling-20260923-inputs/s00_full_full.u32le \
  /tmp/mhgp9-s2-scaling-20260923-inputs/s00_full_full.raw_return_ids.u32le \
  /tmp/mhgp9-edge-core-audit-20260923/full/trace \
  > /tmp/mhgp9-rect-guard-triples-b.stdout
```

La suite constructive est d'améliorer le **choix** des groupes ou de traiter
des tuiles d'arêtes, puis de mesurer tout le chemin avec repli sur les
demi-scènes/quarts physiques et les densités emboîtées 1/2 et 1/4, brut et
sans sol. Les 1 747 rectangles d'une trame K5/s8 ne prouvent aucune croissance
sous-quadratique, ni le contrat FULL/G4.
