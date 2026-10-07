# Grands blocs gardés dans le tas : règle atteinte (mur à chaud 0,905), implémentation à suivre (session claudetas1)

7 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Session G4 `v11.20261007.claudetas1`, cible `us-central1-c / ehgp-v7-3b1d496aed430749ea7e049f`, arrêt `TERMINATED`
certifié (`claudetas1/receipt.json`). Source : `01f10d256`, sans changement du moteur. Trames LiDAR réelles ng00, ng01
et ng02, W48. Aucune mesure ne promeut un statut public.

## Objet

`claudefront1` a relevé pour la première fois sur G4 le coût de restitution des tampons de la passe unique, sur un
seul fil : 11 à 17 ms à K5 en voie GPU, 48 à 58 ms à K10. Les grands blocs sont rendus au système par `munmap`, puis
refaits, page par page, à la passe suivante. Le préréglage `@tas` de `gpu_ab` lance le banc avec
`GLIBC_TUNABLES=glibc.malloc.mmap_threshold=1073741824:glibc.malloc.trim_threshold=4294967296` : les blocs restent
dans le tas de glibc, et la passe suivante les réutilise sans nouvelle faute de page. Le moteur est inchangé. Chaque
mode est comparé à son jumeau `@tas` dans la même source.

## Règle écrite dans le plan avant la session

Exactitude : bancs conformes, vidages identiques entre modes et égaux aux empreintes ; la session relève la version de
glibc. Si la moyenne géométrique des 6 rapports tas/nu du mur à chaud (médiane des passes 2 à 10 d'un processus, K5
CPU feuilles 16 en `278523` et K5 GPU feuilles 24 en `344059:400`, trois trames) est ≤ 0,95, une rétention propre est
implémentée dans le moteur, qualifiée et jugée dans une session suivante sur des prises neuves ; sinon la piste est
notée et fermée.

## Résultat

Les trois bancs sont `conforme`, avec les vidages des empreintes à K5 et à K10. La VM tourne sous glibc 2.35, qui
accepte le seuil de 1 Gio par variable d'environnement : la restitution passe de 8,4–14,8 ms à 0,8–2,3 ms à K5.

| Mode | Trame | Mur à chaud nu → tas (ms) | Rapport | Restitution (ms) | Mur à froid nu → tas (ms) |
| --- | --- | --- | ---: | --- | --- |
| CPU, 16 | ng00 | 321,6 → 294,4 | 0,915 | 10,0 → 2,2 | 321,3 → 324,4 |
| CPU, 16 | ng01 | 272,4 → 239,2 | 0,878 | 8,4 → 1,9 | 273,6 → 275,4 |
| CPU, 16 | ng02 | 318,8 → 312,6 | 0,981 | 9,5 → 2,3 | 344,4 → 345,5 |
| GPU 400 ‰, 24 | ng00 | 274,3 → 247,9 | 0,904 | 14,1 → 0,8 | 357,5 → 326,9 |
| GPU 400 ‰, 24 | ng01 | 229,4 → 196,8 | 0,858 | 14,2 → 0,8 | 308,6 → 290,5 |
| GPU 400 ‰, 24 | ng02 | 268,1 → 240,9 | 0,899 | 14,8 → 0,8 | 356,5 → 331,9 |

**Moyenne géométrique du mur à chaud : 0,905 (seuil 0,95) : la règle est atteinte.** Une rétention propre sera
implémentée dans le moteur, puis qualifiée et jugée sur des prises neuves.

Lecture (descriptive) :
- En voie GPU de référence, le mur à chaud perd 26 à 33 ms ; sur ng01, il passe sous 200 ms (196,8 ms). Le gain
  dépasse la seule restitution : les passes suivantes ne refont plus leurs pages (étages `domain` et forêts).
- À froid, la voie GPU gagne aussi (0,914 à 0,942) ; la voie CPU ne bouge pas (1,003 à 1,010).
- À K10 (voie GPU sans partage), le mur à chaud passe de 1911 / 1428 / 1652 à 1763 / 1331 / 1526 ms (0,92 à 0,93) ; la
  restitution, de 43–53 ms à 1,3–1,5 ms.
- Le préréglage est un réglage de l'allocateur du processus, fixé avant son lancement. Pour que tout appelant en
  profite (CLI, API, banc), la rétention doit vivre dans le moteur : les blocs rendus par les `Buffer` sont gardés
  pour être réutilisés, sans que leurs octets cessent d'être comptés honnêtement.

## Pièces

`claudetas1/` : `plan.json`, `launch.json`, `receipt.json`, `gpu_ab_report_ab_k5_16_cpu.json`,
`gpu_ab_report_ab_k5_24_gpu.json`, `gpu_ab_report_ab_k10_24_gpu.json`. Aucune donnée ni coordonnée LiDAR. `SHA256SUMS`
couvre tous les fichiers sauf lui-même.
