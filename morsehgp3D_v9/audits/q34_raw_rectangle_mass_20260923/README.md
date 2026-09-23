# Rectangles q3/q4 de la trame LiDAR brute : deux budgets pour S2a

23 septembre 2026. Mesure **CPU hors chaîne**, sur la trame brute entière
SemanticKITTI 08/000000 à grille 1 mm, 123 389 sites, avec sol, s8 et K5/K10.
Elle complète la [matrice des moitiés, quarts et densités emboîtées](../lidar_raw_k10_sectors_20260923/README.md)
sur les mêmes octets. Une seule trame et un seul profil sont concernés ;
aucune hypothèse d'alignement des points ou des passages n'est utilisée.

Le sidecar [measure.cpp](measure.cpp) rejoue le front `MidpointSamples` avec
masque q3/q4, puis le certificat de rectangle `Affine`. Il classe chaque
rectangle selon `floor(log2(|A|·|B|))` **avant** toute expansion de ses paires.
Un rectangle est « ouvert » si au moins une des deux voies q3/q4 survit ;
sa masse est comptée une seule fois, même si les deux voies survivent.
Les [histogrammes K5](k5.stdout) et [K10](k10.stdout) contiennent toutes
les classes, y compris les rectangles fermés. Le [lecteur](verify.py)
contrôle les sommes, les bornes de classe, les SHA-256 et cinq compteurs
du [reçu v12 K5](../lidar_raw_physical_scaling_20260923/CASES.jsonl) et du
[reçu v12 K10](../lidar_raw_k10_density_20260923/CASES.jsonl) : nombre de
rectangles d'entrée, masse d'entrée, rectangles fermés, visites de nœuds
du filtre et masse développable. Les cinq concordent pour chacun des K.

| K | rectangles du front | rectangles ouverts | paires développables | ouverts ≥16 : rectangles / part des paires | ouverts ≥64 : rectangles / part des paires |
| --- | ---: | ---: | ---: | ---: | ---: |
| 5 | 6 175 011 | 2 548 453 | 22 034 426 | 134 765 / 75,04 % | 42 020 / 62,46 % |
| 10 | 9 875 830 | 4 308 768 | 37 868 819 | 198 169 / 74,62 % | 54 822 / 63,52 % |

Le filtre de rectangles ferme 90,76 % de la masse du front à K5 et
90,93 % à K10 **avant** la recherche par paire. Les colonnes `open_q3`
et `open_q4` des histogrammes comptent des rectangles portant chaque
voie, pas des masses de paires par voie. Avec `dead_core` actif, le reçu
v12 compte ensuite 3 986 433 (K5) et 7 811 827 (K10) paires arrivées
au cœur ; l'histogramme ne mesure pas ce flux aval.

Au K10, les **4 110 599** rectangles ouverts de moins de 16 paires
représentent 95,40 % des rectangles ouverts, mais seulement **9 612 126**
paires (25,38 %). À l'autre extrême, **2 337** rectangles de masse ≥1 024
portent 14 030 989 paires (37,05 %) ; **225** de masse ≥16 384 en portent
8 810 601 (23,27 %). Le plus gros rectangle *du front* porte 187 824
paires ; la plus grande classe *ouverte* est `[65 536, 131 071]`.
Ces deux budgets ne se remplacent donc pas : la grande majorité des
descripteurs est minuscule, mais une faible minorité porte la masse des
paires.

**Décision d'architecture suggérée pour S2a.** Borner séparément
`R_lot` (rectangles émis par le front, avant les fermetures du filtre) et
`P_lot` (paires des rectangles encore ouverts), en plus du scratch du
filtre et de la sortie vivante. Produire le front par lots ordonnés avec
backpressure, filtrer les rectangles, puis empaqueter plusieurs petits
rectangles dans une même unité GPU et découper les rares gros produits
en tuiles de paires. Un produit dépassant `P_lot` doit être scindé, pas
écarté. Garder l'ordinal de présentation, le masque de chaque voie et les
rangs du même index à travers les tuiles ; conserver l'ordre lors de la
réunion des jobs parallèles. Cette étape ne crée aucun nouveau rejet :
elle borne la représentation et vise l'occupation du GPU. La réduction
du travail géométrique reste le problème distinct de S2b.

Le port batch publié matérialise d'abord tous les rectangles dans
`by_job`, puis les recopie vers un vecteur global avant le filtre
(`wspd_q34.cpp:1128–1210`). `WspdRectangle` occupe 24 octets sur la cible
64 bits : les 9 875 830 rectangles K10 demandent **237 019 920 octets
(226 MiB)** pour *un* vecteur dense, avant les capacités transitoires
des jobs, masques et paires. Un plafond portant seulement sur les
37,87 M paires ne limite pas cette accumulation. Les tailles de lots
doivent être réglées sur RSS/HBM, scratch et occupation mesurés, pas sur
une constante déduite de cette seule trame.

Une porte utile est un replay apparié sur la matrice existante : les sept
secteurs par plans capteur (plein, deux demi-scènes en `x` et quatre quarts
après une coupe supplémentaire en `y`) aux densités emboîtées 1/4, 1/2,
entière, à K5 et K10, puis plusieurs scènes. Comparer le même front et
les mêmes ordinals S2a/CPU, les masques
zéro et non zéro, paires et catalogues/FULL, temps CPU/mur de toute la
chaîne, octets H2D/D2H et pics RSS/HBM. Les sommes de morceaux spatiaux
ne remplacent pas le calcul de la trame entière. Cette distribution et
les pentes finies de la matrice ne prouvent aucune borne sous-quadratique.

Relecture du reçu :

```sh
python3 morsehgp3D_v9/audits/q34_raw_rectangle_mass_20260923/verify.py
python3 -O morsehgp3D_v9/audits/q34_raw_rectangle_mass_20260923/verify.py
```

Pour reproduire les histogrammes depuis le checkout constructeur épinglé,
à la racine du dépôt :

```sh
g++ -std=c++20 -O2 -I build/v9-open-worktree/morsehgp3D_v9/src \
  -I build/v9-open-worktree/morsehgp3D_v9/src/gen \
  morsehgp3D_v9/audits/q34_raw_rectangle_mass_20260923/measure.cpp \
  build/v9-open-worktree/build/v9-dev/libmhgp9_gen.a -pthread \
  -o /tmp/mhgp9_raw_s2_hist
/tmp/mhgp9_raw_s2_hist \
  morsehgp3D_v8/receipts/float32_precision_20260921/release_r2/precision_a1drpf9i/scene_00_000000_grid/full.u32le \
  5
/tmp/mhgp9_raw_s2_hist \
  morsehgp3D_v8/receipts/float32_precision_20260921/release_r2/precision_a1drpf9i/scene_00_000000_grid/full.u32le \
  10
```

Capture : source du sidecar SHA-256
`a3b7eb8bb50e28dad97a18cf03641453f2ed4e3a04c932a7362783ccdfb871dc`,
binaire `cc92f64010060b812261aa99455bc76100bd5b70bdc56cca7aaf1e33c72086a6`,
archive `208aabb30a764bad25ab1c99d74885bd405e84a13dcb8375622d66aa6203f70a`,
source de travail `77c27f70649213d19edb9bfed387b34b339d91ac`.
Entrée :
`morsehgp3D_v8/receipts/float32_precision_20260921/release_r2/precision_a1drpf9i/scene_00_000000_grid/full.u32le`,
SHA-256 `233cc4ea8cac6e0b1155ea845af57b32e5236764bf2e119557aeab5bac76c172`.
L'archive locale a été construite avant le dernier checkout ; l'égalité
des cinq compteurs avec les reçus v12 vérifie ce périmètre de mesure,
sans qualifier le port S2 actuel. Les durées de `*.stdout` viennent d'un
hôte partagé sous contention et ne sont **pas** des mesures de débit.
Aucun GPU/G4 ni contrat FULL n'est acquis par cette sonde.
