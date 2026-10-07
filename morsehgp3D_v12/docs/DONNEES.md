# Données de morsehgp3D_v12 : rapport de préparation

> **Versement dans le dépôt (7 octobre 2026).** Les scripts de préparation sont dans [`../bench/data/`](../bench/data/)
> (empreintes dans `SHA256SUMS.txt`, chemins relatifs à ce dossier) ; les données restent hors dépôt et se rejouent par
> `bash bench/data/replay_all.sh`. Les chemins `scripts/` et `/tmp/...` ci-dessous sont ceux de la préparation.


7 octobre 2026 (heures UTC lues par `date -u`). Dossier hors dépôt :
`/tmp/claude-1000/-workspaces-E-HGP/6acaaf62-b44a-41e7-b5d6-81e4c7b6b7f1/scratchpad/v12_donnees/` (volatil : rejouable
par `scripts/replay_all.sh`). Rien n'a été écrit sous `/workspaces/E-HGP`.

```text
phase=exploration_v12 (preparation des donnees, decision D7)
backend=aucun (preparation seule ; aucune mesure du moteur)
profile=entrees u32 a 1 mm (profil minimal publie par scene : u21 ou u24)
public_status=not_claimed
GCP non utilise
```

## 0. Résumé

- **Recensement** de 43 jeux (dont 4 non-LiDAR signalés comme tels) : licence, accès testé ce jour, points par
  scène, format et grain, étendue et bits, doublons (§ 1).
- **Quatre jeux multi-millions retenus**, quatre capteurs, tous téléchargés automatiquement sans compte ni formulaire,
  empreintes épinglées : **IGN LiDAR HD** (aérien, Licence Ouverte), **ETH3D** (terrestre, CC BY-NC-SA 4.0),
  **FOR-instance** (drone, CC BY 4.0) et **Boreas** (véhicule, CC BY 4.0). 24 scènes entières de 0,15 à
  48,4 millions de retours (14 de 2 à 10 M, 7 de plus de 10 M, 3 de moins de 2 M), avec et sans sol (sauf ETH3D,
  sans classification), et 69 découpes emboîtées de 1, 2, 4 et 8 millions de sites (§ 3) ; 2,9 Go téléchargés.
- **SemanticKITTI sans sol** : 275 trames de 6 séquences (00, 02, 05, 06, 08, 10) préparées localement avec la
  sonde Patchwork++ épinglée, dont 164 au-delà de 60 000 sites (33 179 à 99 099, médiane 65 384) ; sélection
  stratifiée `v12set` de 37 trames ; la chaîne rejoue **à l'octet** ng00, ng01, ng02 du contrat v11 (§ 4).
- **Petits nuages** : 159 nuages de 100 à 10 000 sites — synthétiques (dont deux familles dégénérées déclarées),
  objets réels SemanticKITTI, boules des k plus proches, et 51 bouts de scène de la v11 couvrant 10 séquences (§ 5).
- **Mesures** : doublons au mm massifs en TLS (0,8 à 3,7 %, multiplicité jusqu'à 26) et dans une placette de drone
  (17,6 %), négligeables ailleurs ; u21 suffit partout sauf l'itinéraire Boreas (22 bits, seule entrée réelle de
  qualification u24) (§ 6).
- **G4** : paquets plats conformes au lanceur gardé (≤ 512 fichiers, ≤ 8 Gio), vérifiables sur la VM en Python 3.10
  nu ; ou rejeu complet sur la VM avec un Python portable (§ 7).

## 1. Recensement des benchmarks LiDAR réels de plusieurs millions de points

Méthode : recherche web (un agent de recensement, 209 requêtes), puis **requêtes réelles** le 7 octobre 2026 (HEAD,
lectures partielles par plage, listages de seaux publics, index WFS ou STAC) ; aucune donnée téléchargée derrière un
formulaire, un compte, un mot de passe ou une acceptation de licence. Bits à 1 mm : `ceil(log2(étendue en mm + 1))`
sur l'axe le plus long (1 km → 20 bits ; 1,25 à 2 km → 21 ; 5 à 6,25 km → 23). **[V]** vérifié par une requête ce
jour, **[L]** lu dans une documentation, **[S]** supposé ; **[M]** mesuré sur les données préparées ici. Pièces du
recensement (index WFS de l'IGN, en-têtes HTTP, en-têtes LAS lus par plage) dans `survey/`. Incident : une dalle de
l'Environment Agency (171 Mo) a été téléchargée entière une fois (le serveur ignore `Range`), puis supprimée.

| Jeu | Capteur | Licence | Accès | Points par scène | Format, grain des coordonnées | Étendue → bits | Doublons au mm | Verdict |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| **IGN LiDAR HD** (France) | ALS (RIEGL VQ-780 II-S, Leica TerrainMapper) | Licence Ouverte 2.0 [V] | direct, HTTP 200 ; index WFS avec URL et nombre de points [V] | 4,0 à 45,3 M par dalle de 1 km² [V] | COPC LAZ 1.4, format 6, échelle 0,01 (cm), Lambert-93 [V] | 1 km → 20 | 0,005 à 0,022 % [M] | **retenu** (aérien) |
| AHN4 (Pays-Bas) | ALS (RIEGL VQ-1560 II) | Public Domain Mark 1.0 [V] | direct, `basisdata.nl` → 307 → stockage objet, 200 [V] | 1,1 à 1,7 milliard par tuile de 5 × 6,25 km (8,6 Go) ; tuiles partielles 24 à 167 M [V] | LAZ 1.4, format 1, mm, RD New [V] | 5 km → 23 | inconnu | non : tuiles trop grosses sans découpe préalable |
| AHN5 | ALS | ouverte, mention de source obligatoire [L] | direct, 200 [V] | 21,6 à 62,6 M (tuiles partielles) [V] | LAZ 1.4, format 6, mm [V] | 23 | inconnu | réserve |
| GeoTiles (sous-tuiles AHN, TU Delft) | ALS | selon AHN [L] | **injoignable** (délai TLS) [V] | 40 à 70 M [S] | LAZ | 21 | — | non (service hors ligne) |
| USGS 3DEP, LAZ par projet | ALS | domaine public [L] | API TNM répond, mais l'hôte des fichiers `rockyweb` ne répond pas et `prd-tnm` rend 404 [V] | 30,4 M par carré de 500 m (San Francisco 2023) [V] | LAZ 1.4 | 19–21 | inconnu | non (inaccessible ce jour) |
| USGS 3DEP, flux EPT `usgs-lidar-public` | ALS | domaine public | S3 public, 200 [V] | 13,1 milliards (San Francisco) [V] | arbre EPT de LAZ 1.2, **reprojeté en EPSG:3857** (plan dilaté d'environ 1/cos φ) [V] | selon emprise | inconnu | non (exige PDAL et une reprojection) |
| swissSURFACE3D (Suisse) | ALS | conditions OGD swisstopo : libre, attribution [L] | direct + API STAC, 200 [V] | 11,5 à 25,7 M par km² en ville [V] | COPC LAZ 1.4 format 6 ou LAS zippé, cm, LV95 [V] | 20 | inconnu | réserve (alternative aérienne) |
| NRW 3D-Messdaten (Allemagne) | ALS | dl-de/zero-2-0 [L] | direct, index de 35 861 dalles, 200 [V] | 9,3 à 77,5 M par km² [V] | LAZ 1.2 format 1, cm, UTM32 [V] | 20 | inconnu | réserve (alternative aérienne) |
| Environment Agency NLP (Angleterre) | ALS | OGL v3 [L] | API catalogue (POST) ; HEAD 405 ; `Range` ignoré [V] | archive de 171 Mo par tuile de 5 km [V] | zip de LAZ, OSGB | 5 km → 23 | inconnu | réserve (pas de lecture partielle) |
| Dublin 2015 (NYU) | ALS très dense (hélicoptère) | CC BY 4.0 [L] | direct, 200 [V] | 38,4 M par ligne de vol [V] | zip de LAZ 1.2, mm [V] | 21 | une bande par fichier [S] | réserve (ALS très dense) |
| OpenTopography, dépôt brut | ALS et TLS | propre à chaque jeu [S] | S3 listable, 200 [V] | 8,8 à 49,9 M (TLS), 37 M (ALS) [V] | LAZ 1.2, cm [V] | 18–21 | inconnu | réserve (licence par DOI) |
| NLS (Finlande) | ALS | CC BY 4.0 (produit 0,5 p) [L] | clé d'API (compte) [L] | — | LAZ | — | — | non (compte) |
| PNOA / CNIG (Espagne) | ALS | CC BY 4.0 [L] | panier web, non testé [L] | 2 à 20 M par bloc de 2 km [S] | LAZ | 21 | — | non testé |
| Dataforsyningen (Danemark) | ALS | conditions KDS [L] | connexion et clé d'API [L] | — | LAZ | — | — | non (compte) |
| DALES | ALS (RIEGL Q1560) | non commerciale [L] | **formulaire** [L] | ~12 M par tuile, 40 tuiles [L] | PLY ou LAS | 500 m → 19 | — | non (formulaire) |
| ECLAIR | ALS | CC BY-NC-SA 4.0 [V] | **formulaire** Google [V] | ~600 M sur 10 km² [L] | LAZ | — | — | non (formulaire) |
| **ETH3D**, scans de vérité terrain | TLS (FARO Focus X 330) | CC BY-NC-SA 4.0 [L, page d'accueil] | direct, HTTP 200, 1 920 731 473 o [V] | 6,25 à 28 M par station ; 13 scènes, 1 à 4 stations [M, L] | 7z de PLY binaires float32 + matrices `.mlp` [M] | 30–70 m → 15–17 | **0,8 à 3,7 %**, multiplicité ≤ 26 [M] | **retenu** (terrestre) |
| Semantic3D | TLS | CC BY-NC-SA 3.0 [S] | **HTTP 401** (redirection vers `phd-apache.ethz.ch`) [V] | jusqu'à ~400 M par station [L] | 7z de texte | 19–20 [S] | probables [S] | non (authentification) |
| Würzburg / 3DTK (Bremen City, wue_city, Randersacker) | TLS (RIEGL VZ-400) | « utilisation libre » avec attribution [L] | direct, 200 [V] | 14 à 18 M par scan [L] | texte xyz + réflectance + poses [L] | 19–20 [S] | probables [S] | réserve (licence peu formelle) |
| HPDBSCAN Bremen (B2SHARE) | TLS (11 stations) | **inconnue** (champ de droits vide) [V] | direct, 200 [V] | 81,4 M (`bremen`), 3,0 M (`bremenSmall`) [V] | HDF5 float32 [V] | ~1 km [S] | inconnu | non (licence), mais c'est le banc officiel de HPDBSCAN |
| WHU-TLS | TLS | — | **formulaire** [L] | ~15 M par scan [S] | — | — | — | non (formulaire) |
| Tanks and Temples (vérité terrain) | TLS FARO, **sous-échantillonné par voxels** | non commerciale propre [L] | Google Drive sans connexion (page de confirmation) [V] | 12,7 M (Barn) [V] | PLY double [V] | dizaines de m | faibles (voxels) [L] | non (sous-échantillonné) |
| Newer College | carte TLS (Leica BLK360) + Ouster | CC BY-NC-SA 4.0 [L] | **formulaire** [L] | carte de 290 M [L] | — | — | — | non (formulaire) |
| **FOR-instance** | ULS (drone, RIEGL), forêts | CC BY 4.0 [V, Zenodo] | direct (Zenodo), `Range` accepté [V] | 1,48 à 7,87 M par placette [V] | zip de LAS 1.2 non compressés, mm [V] | 28–86 m → 15–17 | **0,0002 à 17,6 %** [M] | **retenu** (drone) |
| Hessigheim (H3D) | ULS | non publiée [L] | inscription [L] | — | — | — | — | non (compte) |
| **Boreas** (UTIAS) | MLS véhicule, Velodyne Alpha Prime 128 | **CC BY 4.0** [L, registre AWS] | seau public `s3://boreas`, listage et téléchargement anonymes [V] | 215 000 par trame ; 9 984 trames par séquence [M] | `.bin` float32 ×6 + poses Applanix CSV [M] | fenêtre 0,4 km → 19 ; itinéraire 2,5 km → 22 [M] | < 0,005 % [M] | **retenu** (véhicule) |
| KITTI-360 | MLS (HDL-64E + SICK) | CC BY-NC-SA 3.0 [L] | **connexion requise** [L] | ~3 M par fenêtre accumulée, 300 fenêtres [L] | PLY float32 | ~200 m → 18 [S] | inconnu | non (compte) |
| Toronto-3D | MLS (Teledyne Optech Maverick) | CC BY-NC 4.0 [V, README] | OneDrive : 301 puis **403**, API **401** ; Baidu ; plus de lien Google Drive [V] | 78,3 M en 4 tuiles [L] | PLY, UTM (décalage conseillé) [L] | ~1 km → 20 | inconnu | non (pas de téléchargement automatique) |
| Paris-Lille-3D (NPM3D) | MLS (Velodyne HDL-32E) | CC BY-NC-ND 3.0 [L] | partage Nextcloud protégé par un mot de passe publié sur la page, non utilisé [V] | 143 M en 3 zones (71,3 / 26,8 / 45,7 M) [L] | PLY | 19–20 [S] | inconnu | non (mot de passe ; licence ND) |
| Paris-CARLA-3D | MLS réel + synthétique | CC BY-NC-ND 3.0 [L] | idem [V] | 60 M réels [L] | PLY | — | — | non |
| Paris-rue-Madame | MLS | CC BY-NC-ND 3.0 [L] | **lien mort** [V] | 2 × 10 M [L] | PLY | 160 m → 18 | — | non |
| HelixNet | MLS (HDL-64E), trames | CC BY 4.0 [V] | direct (Zenodo), 200 [V] | ~130 000 par trame ; 236 Go [V] | trames `.bin` [V] | séquences ~1 km [S] | si accumulées [S] | réserve (trames de véhicule, gros volumes) |
| TUM-MLS-2016 | MLS (2 × HDL-64E) | CC BY-NC-SA 4.0 [L] | identifiants envoyés par e-mail [L] | 1,7 milliard au total [L] | PCD | — | — | non (compte) |
| NCLT | MLS (HDL-32E) | ODbL [V] | direct (S3), 200 [V] | ~70 000 par balayage [S] | binaire | campus → 21 [S] | — | non (trames petites) |
| Argoverse 2 | MLS | CC BY-NC-SA 4.0 [L] | S3 public, 200 [V] | ~100 000 par balayage | Feather (Arrow) | — | — | non (format, trames) |
| nuScenes | MLS (32 faisceaux) | CC BY-NC-SA 4.0 [L] | compte [L] | ~34 000 par balayage | — | — | — | non |
| Waymo Open | MLS | non commerciale Waymo [L] | connexion Google (302) [V] | ~177 000 par trame [S] | tfrecord / parquet | — | — | non |
| Oakland 3D | MLS (SICK) | — | direct, 200 [V] | 1,6 M au total [V] | texte | — | — | non (trop petit) |
| Sydney Urban Objects | MLS (HDL-64E), objets isolés | — | direct, 200 [V] | petits objets | — | — | — | non (petits objets seulement) |
| SensatUrban | **photogrammétrie, pas du LiDAR** | à vérifier | formulaire Google [V] | ~3 milliards [L] | PLY | — | — | non |
| STPLS3D | **photogrammétrie et synthétique** | CC BY-NC-SA 3.0 [L] | formulaire ou Drive [L] | — | PLY | — | — | non |
| Swiss3DCities | **photogrammétrie** | CC BY-NC-SA 4.0 [V] | direct (Zenodo), 42,6 Go [V] | — | — | — | — | non |
| Stanford 3D Scanning Repository | **triangulation laser, pas du LiDAR** | recherche seulement [V] | direct, 200 [V] | 5 à 14 M par objet [S] | PLY | objets → ~10 | — | non |

**Points saillants du recensement.** (1) Grain des coordonnées : au centimètre pour l'IGN, NRW, swisstopo, l'EPT USGS
de San Francisco et OpenTopography (sur la grille de 1 mm, toutes les coordonnées sont des multiples de 10) ; au
millimètre pour AHN, Dublin, FOR-instance ; en flottant pour ETH3D, KITTI, Boreas, Tanks and Temples. (2) Une dalle de
1 km tient en 20 bits ; une tuile AHN ou Environment Agency de 5 km en demande 23 (u21 insuffisant sans découpe).
(3) Les jeux annotés de MLS les plus cités (KITTI-360, Toronto-3D, Paris-Lille-3D, TUM-MLS) sont tous derrière un
compte, un mot de passe ou un partage non automatisable ; d'où le choix de Boreas, accumulé ici.

## 2. Jeux retenus et pourquoi

Critères, dans l'ordre : (1) LiDAR réel ; (2) téléchargement **automatique sans compte, formulaire ni mot de passe**
(vérifié par une requête réelle) ; (3) licence compatible avec un usage de recherche hors dépôt ; (4) capteurs
différents ; (5) au moins une scène de 2 à 10 millions et une de plus de 10 millions de points ; (6) octets bruts
épinglables (SHA-256) et conversion déterministe à l'octet.

| Rôle | Jeu retenu | Pourquoi | Écartés pour ce rôle (cause vérifiée) |
| --- | --- | --- | --- |
| aérien (ALS) | **IGN LiDAR HD** : dalles de Marseille 0891_6248 (6,7 M), Paris 0651_6863 (14,6 M), Lyon 0842_6521 (32,4 M) | Licence Ouverte (avec CC0 et le domaine public, la plus libre), URL directes et nombres de points donnés par l'index WFS officiel, COPC LAZ classé (sol = 2), deux capteurs (Leica TerrainMapper, RIEGL VQ-780 II-S), centres-villes denses ; 1 km² = 20 bits | AHN4 : tuile de 8,6 Go (1,1 à 1,7 milliard de points, 23 bits) ; GeoTiles hors ligne ; USGS 3DEP : hôte des LAZ injoignable ce jour, flux EPT en EPSG:3857 ; DALES, ECLAIR : formulaires. Alternatives directes équivalentes, gardées en réserve : NRW (dl-de/zero), swissSURFACE3D, Dublin 2015 (CC BY) |
| terrestre (TLS) | **ETH3D**, scans laser de vérité terrain (FARO Focus X 330) : stations seules meadow (6,25 M) et courtyard (17,0 M), scènes assemblées courtyard (34,5 M, 2 stations) et electro (48,4 M, 4 stations) | archive directe de 1,92 Go (HTTP 200), CC BY-NC-SA 4.0, densité extrême (sous-millimétrique près des stations) : doublons au mm massifs (0,8 à 3,7 %, multiplicité jusqu'à 26), le vrai test de la décision D8 | Semantic3D : **HTTP 401** (authentification) ; WHU-TLS, Newer College : formulaires ; HPDBSCAN Bremen : licence inconnue ; Tanks and Temples : nuage sous-échantillonné par voxels ; Würzburg/3DTK : licence informelle (réserve) |
| drone (ULS) | **FOR-instance** : placettes forestières SCION plot 61 (3,6 M), TUWIEN train (7,6 M), NIBIO plot 12 (7,9 M) | Zenodo direct, **CC BY 4.0**, extraction des seuls membres utiles par `Range` (environ 215 Mo lus au lieu de 1,64 Go) ; LAS **non compressé** lu en numpy pur (aucune dépendance) ; végétation et troncs, géométrie absente des autres jeux ; doublons de 0 à 17,6 % (TUWIEN) | H3D : inscription |
| véhicule (MLS) | **Boreas** (UTIAS), Velodyne Alpha Prime 128 faisceaux, poses Applanix post-traitées : fenêtres emboîtées de 1, 10, 50 trames consécutives (0,22 / 2,15 / 10,8 M) et itinéraire de 44 trames sur 2,5 km (9,2 M, **22 bits**) | seau public AWS Open Data (listage et téléchargement anonymes), **CC BY 4.0**, poses lidar de vérité terrain fournies, au centimètre (donc accumulation sans étalonnage supplémentaire) ; l'analogue sans compte des fenêtres accumulées de KITTI-360 ; seule donnée réelle au-delà de 21 bits à taille raisonnable (qualification u24 de la décision D6) | KITTI-360, TUM-MLS-2016 : compte ; Toronto-3D : OneDrive **403**, API **401** ; Paris-Lille-3D : mot de passe et licence ND ; HelixNet : trames seules, 236 Go (réserve) ; accumulation de SemanticKITTI : `calib.txt` absent du cache local (archive KITTI soumise à enregistrement) |
| régime principal | **SemanticKITTI** sans sol, 275 trames de 6 séquences déjà en cache local | chaîne de la v11 rejouée à l'octet (ng00–ng02) | séquences 01, 03, 04, 07, 09 : trames absentes du cache (seuls les « bouts » de la v11 en viennent, repris en famille c) |

## 3. Scènes multi-millions préparées (famille b)

Chaque scène existe en variante `tout` et, quand le producteur classe le sol ou qu'un filtre épinglé s'applique,
`sans_sol` ; chaque variante est ensuite découpée en carrés horizontaux concentriques de **1, 2, 4 et 8 millions** de
sites distincts (emboîtés, toute la hauteur, jamais de sous-échantillonnage), plus la scène entière. Colonnes :
retours écrits (`<nom>.u32le`, ordre lexicographique), positions distinctes (`<nom>.distinct.u32le` quand il y a des
doublons), étendue, bits, et les 16 premiers caractères des SHA-256 (empreintes complètes dans les manifestes). Les
découpes sont écrites en sites distincts (`.mult.u32le` = retours couverts).

### 3.1 IGN LiDAR HD (aérien)

| Scène | Retours | Positions distinctes | Doublons au mm | Étendue (m) | Bits | SHA-256 `<nom>.u32le` (16) | SHA-256 `.distinct` (16) |
| --- | ---: | ---: | ---: | --- | ---: | --- | --- |
| `ign_lyon_0842_6521` | 32 415 138 | 32 412 887 | 2 251 | 1000.0 × 1000.0 × 123.2 | 20 | `c3c709c83d918b25` | `7c52658ef04b1631` |
| `ign_lyon_0842_6521_sans_sol` | 24 018 428 | 24 016 862 | 1 566 | 1000.0 × 1000.0 × 123.2 | 20 | `df0da3f51331eb0d` | `93daa17d414a698c` |
| `ign_marseille_0891_6248` | 6 710 535 | 6 709 045 | 1 490 | 996.7 × 1000.0 × 83.3 | 20 | `dba35878fe49fa19` | `5beaeae410a97224` |
| `ign_marseille_0891_6248_sans_sol` | 2 465 588 | 2 465 285 | 303 | 996.5 × 1000.0 × 83.1 | 20 | `27ca3bf629d0f909` | `46c7597d6a31577b` |
| `ign_paris_0651_6863` | 14 552 516 | 14 551 520 | 996 | 1000.0 × 1000.0 × 112.1 | 20 | `1eb625c14b8e1cca` | `8d89af8dcc4fd55f` |
| `ign_paris_0651_6863_sans_sol` | 9 111 862 | 9 111 422 | 440 | 1000.0 × 1000.0 × 112.1 | 20 | `4c8ee30f1cc964c6` | `7ec5de4be28c4cc8` |

| Découpe | Sites distincts | Retours couverts | Côté du carré (m) | Bits | SHA-256 (16) |
| --- | ---: | ---: | ---: | ---: | --- |
| `ign_lyon_0842_6521_c1M` | 1 000 000 | 1 000 099 | 163.2 | 18 | `e773b4edbf03b09c` |
| `ign_lyon_0842_6521_c2M` | 2 000 000 | 2 000 202 | 230.7 | 18 | `5d3a7704e72d2a56` |
| `ign_lyon_0842_6521_c4M` | 4 000 000 | 4 000 377 | 324.8 | 19 | `81681a1b64e30c10` |
| `ign_lyon_0842_6521_c8M` | 8 000 000 | 8 000 688 | 463.6 | 19 | `1c6699ce5e313887` |
| `ign_lyon_0842_6521_sans_sol_c1M` | 1 000 000 | 1 000 101 | 173.4 | 18 | `2b44dfb423a33218` |
| `ign_lyon_0842_6521_sans_sol_c2M` | 2 000 000 | 2 000 201 | 246.3 | 18 | `488689ff8d96e188` |
| `ign_lyon_0842_6521_sans_sol_c4M` | 4 000 000 | 4 000 373 | 358.0 | 19 | `9934c9fdaccf3d61` |
| `ign_lyon_0842_6521_sans_sol_c8M` | 8 000 000 | 8 000 680 | 529.3 | 20 | `5e0881adfa4af4d3` |
| `ign_marseille_0891_6248_c1M` | 1 000 000 | 1 000 203 | 550.2 | 20 | `e0e2d1395c772046` |
| `ign_marseille_0891_6248_c2M` | 2 000 000 | 2 000 500 | 663.3 | 20 | `412a35a5e453ed0c` |
| `ign_marseille_0891_6248_c4M` | 4 000 000 | 4 001 009 | 820.2 | 20 | `691bb739552c51f0` |
| `ign_marseille_0891_6248_sans_sol_c1M` | 1 000 000 | 1 000 119 | 791.5 | 20 | `4ef6dfe702dc879b` |
| `ign_marseille_0891_6248_sans_sol_c2M` | 2 000 000 | 2 000 232 | 953.5 | 20 | `49566ed5a3162d71` |
| `ign_paris_0651_6863_c1M` | 1 000 000 | 1 000 082 | 268.4 | 19 | `aa7f80f6f75959a2` |
| `ign_paris_0651_6863_c2M` | 2 000 000 | 2 000 154 | 370.3 | 19 | `e003c71a4343d882` |
| `ign_paris_0651_6863_c4M` | 4 000 000 | 4 000 280 | 535.1 | 20 | `1041d013cb865c61` |
| `ign_paris_0651_6863_c8M` | 8 000 000 | 8 000 538 | 750.1 | 20 | `81ce2aca6470cdcd` |
| `ign_paris_0651_6863_sans_sol_c1M` | 1 000 000 | 1 000 034 | 364.5 | 19 | `6b3eeba5860be97e` |
| `ign_paris_0651_6863_sans_sol_c2M` | 2 000 000 | 2 000 076 | 490.5 | 19 | `bfce6ea1c4f35670` |
| `ign_paris_0651_6863_sans_sol_c4M` | 4 000 000 | 4 000 171 | 690.8 | 20 | `0203772bcbedf2f4` |
| `ign_paris_0651_6863_sans_sol_c8M` | 8 000 000 | 8 000 393 | 938.2 | 20 | `7d9b619953f03d59` |


### 3.2 ETH3D (terrestre)

| Scène | Retours | Positions distinctes | Doublons au mm | Étendue (m) | Bits | SHA-256 `<nom>.u32le` (16) | SHA-256 `.distinct` (16) |
| --- | ---: | ---: | ---: | --- | ---: | --- | --- |
| `eth3d_courtyard` | 34 454 474 | 34 104 732 | 349 742 | 31.7 × 23.5 × 18.3 | 15 | `5de335e1539e8572` | `24ed5839cd8ac219` |
| `eth3d_courtyard_scan1` | 16 964 565 | 16 828 368 | 136 197 | 30.9 × 25.6 × 18.3 | 15 | `4b23f202b5ca084d` | `2f08e182f38fa770` |
| `eth3d_electro` | 48 385 601 | 46 600 012 | 1 785 589 | 57.9 × 66.0 × 47.2 | 17 | `0fe8ec458e99d06a` | `d1bb59b385c0ddaa` |
| `eth3d_meadow_scan1` | 6 250 029 | 6 181 091 | 68 938 | 55.1 × 56.0 × 8.2 | 16 | `00bb5ed330c343a9` | `54ca49a3880bd9a9` |

| Découpe | Sites distincts | Retours couverts | Côté du carré (m) | Bits | SHA-256 (16) |
| --- | ---: | ---: | ---: | ---: | --- |
| `eth3d_courtyard_c1M` | 1 000 000 | 1 001 683 | 11.2 | 14 | `a04ad7655158b0ef` |
| `eth3d_courtyard_c2M` | 2 000 000 | 2 004 552 | 12.8 | 14 | `3da3d6640ce05862` |
| `eth3d_courtyard_c4M` | 4 000 000 | 4 014 324 | 14.4 | 14 | `d89571eacbc35c77` |
| `eth3d_courtyard_c8M` | 8 000 000 | 8 134 486 | 16.2 | 15 | `f8bc65d25121ce5d` |
| `eth3d_courtyard_scan1_c1M` | 1 000 000 | 1 002 315 | 11.8 | 14 | `50dc335bf20a3f1c` |
| `eth3d_courtyard_scan1_c2M` | 2 000 000 | 2 032 749 | 13.3 | 14 | `290cabd8376933c7` |
| `eth3d_courtyard_scan1_c4M` | 4 000 000 | 4 106 307 | 15.6 | 15 | `14a4bb2cc10c72e1` |
| `eth3d_courtyard_scan1_c8M` | 8 000 000 | 8 136 184 | 19.0 | 15 | `d293700808b0c226` |
| `eth3d_electro_c1M` | 1 000 000 | 1 006 171 | 24.7 | 15 | `d10c915dc7a804a5` |
| `eth3d_electro_c2M` | 2 000 000 | 2 031 074 | 26.5 | 15 | `3672bb91415879e2` |
| `eth3d_electro_c4M` | 4 000 000 | 4 121 890 | 28.4 | 15 | `cb44807ed9d6a978` |
| `eth3d_electro_c8M` | 8 000 000 | 8 374 231 | 30.5 | 16 | `af9a329cc1f71295` |
| `eth3d_meadow_scan1_c1M` | 1 000 000 | 1 000 000 | 40.6 | 16 | `743b190cac2edc62` |
| `eth3d_meadow_scan1_c2M` | 2 000 000 | 2 000 000 | 41.8 | 16 | `fbeb561d56215fc4` |
| `eth3d_meadow_scan1_c4M` | 4 000 000 | 4 000 000 | 44.1 | 16 | `e6c0d9f489bdbb9c` |


### 3.3 FOR-instance (drone)

| Scène | Retours | Positions distinctes | Doublons au mm | Étendue (m) | Bits | SHA-256 `<nom>.u32le` (16) | SHA-256 `.distinct` (16) |
| --- | ---: | ---: | ---: | --- | ---: | --- | --- |
| `forinst_nibio_plot12` | 7 866 181 | 7 825 857 | 40 324 | 27.8 × 27.8 × 33.2 | 16 | `2f9b4206474f7f6d` | `c91bd51fea13a7ed` |
| `forinst_nibio_plot12_sans_sol` | 7 833 878 | 7 793 680 | 40 198 | 27.8 × 27.8 × 33.1 | 16 | `5e7c054e053eac88` | `62f85b9dc81fba45` |
| `forinst_scion_plot61` | 3 589 254 | 3 589 247 | 7 | 32.2 × 32.2 × 34.6 | 16 | `559abc898b5119b3` | `28e55ca582dee6d0` |
| `forinst_scion_plot61_sans_sol` | 3 439 378 | 3 439 371 | 7 | 32.2 × 32.2 × 34.5 | 16 | `38a5430149a39850` | `d1d416e9d6bab7fa` |
| `forinst_tuwien_train` | 7 568 844 | 6 236 167 | 1 332 677 | 85.9 × 61.0 × 41.3 | 17 | `03c190b87900ae21` | `fcac43bb8fc9f8a2` |
| `forinst_tuwien_train_sans_sol` | 6 277 880 | 5 199 758 | 1 078 122 | 85.9 × 61.0 × 40.7 | 17 | `d91761768355332c` | `984280b0b1ba7b79` |

| Découpe | Sites distincts | Retours couverts | Côté du carré (m) | Bits | SHA-256 (16) |
| --- | ---: | ---: | ---: | ---: | --- |
| `forinst_nibio_plot12_c1M` | 1 000 000 | 1 004 840 | 8.2 | 15 | `86812d39fe33e895` |
| `forinst_nibio_plot12_c2M` | 2 000 000 | 2 009 659 | 12.0 | 15 | `aea517c7a17c4688` |
| `forinst_nibio_plot12_c4M` | 4 000 000 | 4 019 782 | 16.8 | 15 | `ac579365fe46eacb` |
| `forinst_nibio_plot12_sans_sol_c1M` | 1 000 000 | 1 004 844 | 8.2 | 15 | `e578fd436d2fe7e2` |
| `forinst_nibio_plot12_sans_sol_c2M` | 2 000 000 | 2 009 666 | 12.0 | 15 | `d7c5a65c469a64eb` |
| `forinst_nibio_plot12_sans_sol_c4M` | 4 000 000 | 4 019 789 | 16.8 | 15 | `c53050d6b4227d14` |
| `forinst_scion_plot61_c1M` | 1 000 000 | 1 000 002 | 12.6 | 16 | `e38f4f487829c625` |
| `forinst_scion_plot61_c2M` | 2 000 000 | 2 000 003 | 18.8 | 16 | `bf25f3a47b7151a0` |
| `forinst_scion_plot61_sans_sol_c1M` | 1 000 000 | 1 000 002 | 12.9 | 16 | `b078b74feba5a33d` |
| `forinst_scion_plot61_sans_sol_c2M` | 2 000 000 | 2 000 003 | 19.2 | 16 | `5327af5968c51638` |
| `forinst_tuwien_train_c1M` | 1 000 000 | 1 222 955 | 24.2 | 16 | `ff165c466239b6d1` |
| `forinst_tuwien_train_c2M` | 2 000 000 | 2 420 415 | 34.8 | 16 | `bc8aa51000866b62` |
| `forinst_tuwien_train_c4M` | 4 000 000 | 4 883 021 | 51.0 | 16 | `b93f8811efce8393` |
| `forinst_tuwien_train_sans_sol_c1M` | 1 000 000 | 1 210 564 | 26.4 | 16 | `05da0ff75bd57ee9` |
| `forinst_tuwien_train_sans_sol_c2M` | 2 000 000 | 2 403 692 | 38.1 | 16 | `90f534f973273419` |
| `forinst_tuwien_train_sans_sol_c4M` | 4 000 000 | 4 841 606 | 58.8 | 16 | `c23e3152c3c9f4df` |


### 3.4 Boreas (véhicule)

| Scène | Retours | Positions distinctes | Doublons au mm | Étendue (m) | Bits | SHA-256 `<nom>.u32le` (16) | SHA-256 `.distinct` (16) |
| --- | ---: | ---: | ---: | --- | ---: | --- | --- |
| `boreas_202011261358_f4500_n1` | 215 665 | 215 665 | 0 | 356.2 × 403.8 × 49.4 | 19 | `46fae285d9c0d9d6` | — |
| `boreas_202011261358_f4500_n10` | 2 153 373 | 2 153 342 | 31 | 363.1 × 425.1 × 57.8 | 19 | `0919dcd96ee73bd8` | `f304c8e6f1001632` |
| `boreas_202011261358_f4500_n10_sans_sol` | 1 513 511 | 1 513 483 | 28 | 363.1 × 425.1 × 57.8 | 19 | `658f6cf1b0ddadc8` | `548e3bba27b09813` |
| `boreas_202011261358_f4500_n1_sans_sol` | 146 316 | 146 316 | 0 | 356.2 × 403.8 × 49.4 | 19 | `395b1a449602178a` | — |
| `boreas_202011261358_f4500_n50` | 10 767 359 | 10 766 998 | 361 | 363.9 × 477.5 × 79.3 | 19 | `a46270d3ea10ce6b` | `333c6d2da2a4c035` |
| `boreas_202011261358_f4500_n50_sans_sol` | 7 857 594 | 7 857 268 | 326 | 363.9 × 477.5 × 79.3 | 19 | `9b945413c4c30a59` | `c71fd814db695162` |
| `boreas_202011261358_route_s200` | 9 236 006 | 9 235 702 | 304 | 1748.0 × 2503.3 × 71.9 | 22 | `b7c5a26638f1feee` | `4c5e4eac20e3a4c5` |
| `boreas_202011261358_route_s200_sans_sol` | 6 455 630 | 6 455 329 | 301 | 1748.0 × 2503.3 × 60.4 | 22 | `e1aed252f94ce2ac` | `cacc4d5d25c3a70a` |

| Découpe | Sites distincts | Retours couverts | Côté du carré (m) | Bits | SHA-256 (16) |
| --- | ---: | ---: | ---: | ---: | --- |
| `boreas_202011261358_f4500_n10_c1M` | 1 000 000 | 1 000 024 | 37.6 | 16 | `e19914c386f5c600` |
| `boreas_202011261358_f4500_n10_c2M` | 2 000 000 | 2 000 031 | 98.3 | 17 | `799cd10e7c1377f8` |
| `boreas_202011261358_f4500_n10_sans_sol_c1M` | 1 000 000 | 1 000 025 | 56.3 | 16 | `52362a1e30381fe0` |
| `boreas_202011261358_f4500_n50_c1M` | 1 000 000 | 1 000 025 | 17.3 | 15 | `b237d21d22d0ee1f` |
| `boreas_202011261358_f4500_n50_c2M` | 2 000 000 | 2 000 050 | 29.5 | 15 | `5ba0a2df58ba7c74` |
| `boreas_202011261358_f4500_n50_c4M` | 4 000 000 | 4 000 158 | 44.1 | 16 | `986ea17433c219d7` |
| `boreas_202011261358_f4500_n50_c8M` | 8 000 000 | 8 000 308 | 73.4 | 17 | `ca8a9aa88b05b022` |
| `boreas_202011261358_f4500_n50_sans_sol_c1M` | 1 000 000 | 1 000 032 | 20.1 | 15 | `189ca6c81efe01f9` |
| `boreas_202011261358_f4500_n50_sans_sol_c2M` | 2 000 000 | 2 000 074 | 38.2 | 16 | `b831607ce1bb9b7f` |
| `boreas_202011261358_f4500_n50_sans_sol_c4M` | 4 000 000 | 4 000 214 | 55.6 | 16 | `c2a8287f9eef5187` |
| `boreas_202011261358_route_s200_c1M` | 1 000 000 | 1 000 000 | 430.4 | 19 | `ea2d6d4e4580a4a6` |
| `boreas_202011261358_route_s200_c2M` | 2 000 000 | 2 000 000 | 705.5 | 20 | `de7e7b58b52e62a9` |
| `boreas_202011261358_route_s200_c4M` | 4 000 000 | 4 000 001 | 1275.5 | 21 | `5e1bbd25648fa8c8` |
| `boreas_202011261358_route_s200_c8M` | 8 000 000 | 8 000 304 | 2010.5 | 21 | `7b661e2db942e6e0` |
| `boreas_202011261358_route_s200_sans_sol_c1M` | 1 000 000 | 1 000 000 | 539.3 | 20 | `bf0f6e5e432db57e` |
| `boreas_202011261358_route_s200_sans_sol_c2M` | 2 000 000 | 2 000 000 | 1153.7 | 21 | `a93c74c5e8dde98b` |
| `boreas_202011261358_route_s200_sans_sol_c4M` | 4 000 000 | 4 000 002 | 1585.0 | 21 | `5f726b0ae7ae8e6e` |

## 4. Trames SemanticKITTI sans sol (famille a)

**Inventaire local (lecture seule).** Trames brutes `velodyne/*.bin` (+ `.label`) dans `build/v11-persist/kitti_cache`
(201 trames : 00 × 20, 02 × 5, 05 × 3, 06 × 11, 08 × 160, 10 × 2, plus `data_odometry_labels.zip` avec les
`poses.txt` des séquences 00–21), `data_points3` (74 trames de 08 + archives de la sonde), `p08/frames` et
`e1_p08/s6|s7/data` (séquence 08), `build/v10-lidar-demos/_cache`, `build/v11-videos-20261004/Zoltan/demos/_cache`, et
`morsehgp3D_v8/audits/lidar08_20260914/data` (08/000000–000004, 000100, 000200) : **275 trames distinctes de
6 séquences** (00, 02, 05, 06, 08, 10), toutes les copies d'une même trame ayant la même empreinte (contrôlé).
`e1_scenes` ne contient que des scènes synthétiques de l'expérience E1 (8 000 à 32 000 sites) ; `bouts/data` contient
304 « bouts de scène » de la v11 (2 ou 3 objets proches, sans sol, 107 à 17 593 sites) tirés de **10 séquences**
(00, 02, 03, 04, 05, 06, 07, 08, 09, 10), repris en famille (c). Aucune trame des séquences 01, 03, 04, 07, 09
n'est en cache.

**Chaîne de retrait du sol.** Celle de la v11 (`morsehgp3D_v11/bench/points_lidar_prepare.py`) : sonde Patchwork++
v8 (amont `3e6903a1d5537a4cc2ace897b0bbb98a92d6014c`) compilée par g++ depuis `patchwork_sources.tar.gz` (sonde + six
fichiers amont, empreintes épinglées vérifiées) et `eigen3.tar.gz` (en-têtes Eigen), tous deux dans
`build/v11-persist/data_points3/` (les sources dépaquetées sont aussi dans `build/v11-persist/pw_src/`), drapeaux du
constructeur v8 (`-O3 -ffp-contract=off -fno-fast-math -DEIGEN_DONT_PARALLELIZE`). Compilée ici par GCC 13.3, la sonde
est **identique à l'octet** au binaire du reçu v8 (`4aae71381763e274…`) ; masque de 08/000000 = `9db3fe5c…` (contrôle
obligatoire). Politique du masque : seuls les retours de sol (1) sont retirés, inconnus (0) et hors sol (2) gardés.

**Contrôle bout à bout.** Avec la convention de la v11 (translation au minimum de la trame brute entière), la chaîne
reproduit **à l'octet** les coordonnées et les identifiants des trois entrées du contrat, ng00 (`0baa4de1…`,
`c73a4196…`), ng01 (`ba15adc6…`, `bb699c25…`) et ng02 (`a4bbc86d…`, `121e76f3…`), à chaque lancement.

**Préparation v12.** Les 275 trames sont préparées (2 minutes), plus une sélection stratifiée `v12set` de 37 trames
(par séquence, au plus 8 trames à des rangs de taille régulièrement espacés, minimum et maximum compris, plus les
trois trames du contrat). Aucun doublon au millimètre dans aucune trame (ni brute, ni sans sol). Étendue : 17 ou
18 bits (au plus 160 m). Étiquettes SemanticKITTI écrites à part (`.labels.u32le`, étiquette du premier retour du
site), jamais utilisées par la préparation.


- toutes les trames locales : 275 trames, séquences 00, 02, 05, 06, 08, 10 (00 : 20, 02 : 5, 05 : 3, 06 : 11, 08 : 234, 10 : 2) ; sites : min 33 179, médiane 65 384, max 99 099 ; 164 trames au-dessus de 60 000 sites, 0 sous 30 000
- sélection v12set : 37 trames, séquences 00, 02, 05, 06, 08, 10 (00 : 8, 02 : 5, 05 : 3, 06 : 8, 08 : 11, 10 : 2) ; sites : min 33 179, médiane 64 740, max 99 099 ; 21 trames au-dessus de 60 000 sites, 0 sous 30 000

| Trame | Retours bruts | Sans sol (retours) | Sites | Bits | SHA-256 `.u32le` (16) | SHA-256 ids (16) |
| --- | ---: | ---: | ---: | ---: | --- | --- |
| `kitti_ng_00_000648` | 119 920 | 77 965 | 77 965 | 18 | `84252ef8640fa364` | `d373a51bbf196f32` |
| `kitti_ng_00_000660` | 118 704 | 71 483 | 71 483 | 18 | `e7d19d9d965f371a` | `331be79cb5b7e84a` |
| `kitti_ng_00_001464` | 116 080 | 74 736 | 74 736 | 18 | `a04649d54d13e864` | `bff9fa282163cfef` |
| `kitti_ng_00_001502` | 111 112 | 64 995 | 64 995 | 18 | `f9ebb8e488c677d6` | `a897cd66c0819a9b` |
| `kitti_ng_00_001880` | 122 903 | 80 607 | 80 607 | 18 | `b8e52f95c0d493a1` | `55fa4fd014054187` |
| `kitti_ng_00_001896` | 128 146 | 95 586 | 95 586 | 18 | `ed29e345a09e27eb` | `fef39d4bd0da891a` |
| `kitti_ng_00_003612` | 118 073 | 70 537 | 70 537 | 18 | `ff14b47771202d0b` | `83a9a0213337b2d2` |
| `kitti_ng_00_003624` | 116 901 | 72 836 | 72 836 | 18 | `79900d14c8019e1d` | `a8d0da1514e926ea` |
| `kitti_ng_02_000620` | 124 270 | 52 403 | 52 403 | 18 | `caf79fd025ff7032` | `4ec4b86a1d2f3692` |
| `kitti_ng_02_001600` | 119 640 | 67 585 | 67 585 | 18 | `fbb858ca8b216638` | `9504349b3c3974d8` |
| `kitti_ng_02_001604` | 122 974 | 67 805 | 67 805 | 18 | `ca382e83ac862c62` | `97c1cdc39acb0074` |
| `kitti_ng_02_001606` | 122 856 | 64 740 | 64 740 | 18 | `ca6616849a9a1b23` | `0a170d384396f837` |
| `kitti_ng_02_001610` | 120 268 | 67 864 | 67 864 | 18 | `340750422479001d` | `53611f88aa2b05fd` |
| `kitti_ng_05_002060` | 125 601 | 66 456 | 66 456 | 18 | `dd07da75cf117f38` | `6b1dfd5067efaac6` |
| `kitti_ng_05_002064` | 125 684 | 62 859 | 62 859 | 18 | `77bfa35096524e2b` | `0f23f46c7dc7ed00` |
| `kitti_ng_05_002748` | 119 404 | 50 136 | 50 136 | 18 | `eb15b2add60d822e` | `42be3bf97efb61dc` |
| `kitti_ng_06_000770` | 121 301 | 46 561 | 46 561 | 18 | `c06e1a4389d2adbd` | `632cf3eb01a23fac` |
| `kitti_ng_06_000772` | 119 023 | 49 668 | 49 668 | 18 | `cc02619b1d854da0` | `a28e0e753a2e6528` |
| `kitti_ng_06_000774` | 119 073 | 53 754 | 53 754 | 18 | `b8b6ee6b06bda7be` | `1a38af731392c38c` |
| `kitti_ng_06_000780` | 113 256 | 49 628 | 49 628 | 18 | `60aa7e7befe50eed` | `4aa1c92306ac1fd6` |
| `kitti_ng_06_000798` | 123 392 | 37 543 | 37 543 | 18 | `982396e7f2f142d0` | `6bf42b3327aa8f28` |
| `kitti_ng_06_000800` | 122 323 | 38 341 | 38 341 | 18 | `79d11bd8a5fb7436` | `f751ca7952bca446` |
| `kitti_ng_06_000804` | 118 033 | 47 658 | 47 658 | 18 | `a67602fd4d32f711` | `8878b1b79240c7e6` |
| `kitti_ng_06_000806` | 116 482 | 50 884 | 50 884 | 18 | `5189e0ec8b093d30` | `8f37fb1d16a39fc2` |
| `kitti_ng_08_000000` | 123 389 | 39 885 | 39 885 | 18 | `24970b7fbe346811` | `c73a41965f6f2e10` |
| `kitti_ng_08_000100` | 124 479 | 35 551 | 35 551 | 18 | `3cdf4e2fc2691db4` | `bb699c2511a87618` |
| `kitti_ng_08_000200` | 125 526 | 45 845 | 45 845 | 18 | `a4bbc86d00f92627` | `121e76f3ef1fcedb` |
| `kitti_ng_08_000246` | 124 597 | 41 691 | 41 691 | 18 | `a0190ad5f14d167b` | `b3116ac926c0dd8a` |
| `kitti_ng_08_000691` | 126 961 | 61 198 | 61 198 | 18 | `ad81aa8e3b1f709e` | `d9ab5da1be73e01d` |
| `kitti_ng_08_001176` | 126 267 | 67 114 | 67 114 | 18 | `6bf52ade97402062` | `e084c1f597668155` |
| `kitti_ng_08_001193` | 115 605 | 80 433 | 80 433 | 18 | `584d5fec466785c9` | `68914fb47fd8cc96` |
| `kitti_ng_08_001302` | 126 098 | 48 925 | 48 925 | 18 | `a7ad0c44fb09bb9f` | `3adf3671337de2f8` |
| `kitti_ng_08_001847` | 122 627 | 33 179 | 33 179 | 18 | `7734fdc3a76f8c74` | `d816cedb33b78bc0` |
| `kitti_ng_08_002119` | 127 245 | 99 099 | 99 099 | 18 | `6bfdd7302cc00509` | `98fabac040503e7a` |
| `kitti_ng_08_002554` | 116 809 | 72 426 | 72 426 | 18 | `9db193d20ce544de` | `4aae1cc1519cb972` |
| `kitti_ng_10_000424` | 125 173 | 69 106 | 69 106 | 18 | `a6080f07046895f6` | `e83d2ef5a1e33e53` |
| `kitti_ng_10_000430` | 126 640 | 73 576 | 73 576 | 18 | `8213a6348ec4c619` | `9115c67391151f15` |

## 5. Petits nuages (famille c, 100 à 10 000 sites)

`prepare_small.py`, reproductible à l'octet (SplitMix64 entier, aucune loi de `numpy.random`, aucune libm ; les seules
opérations flottantes sont `+ - * / sqrt`, correctement arrondies en IEEE). Tailles de `MESURE.md` § 2 : 100, 300,
1 000, 3 000 et 10 000 sites.

- **Synthétiques** (`synth_*`) : cube uniforme sur 18 bits (comme les uniformes du contrat) ; huit amas gaussiens
  (σ = 2 m, Irwin–Hall entier) ; coquille sphérique de rayon 10 m ; dalle mince de 50 m × 50 m × 5 cm (presque
  coplanaire) ; et deux familles **dégénérées déclarées** : réseau au pas de 1 m (boîtes de 5 × 5 × 4 à 25 × 20 × 20
  nœuds : cosphéricités massives) et points alignés (100 et 1 000). Le moteur doit les traiter exactement ou les
  refuser explicitement, jamais les « jitter ».
- **Objets réels** (`kobj_*`) et **objets avec contexte** (`kctx_*`, boîte englobante + 1,5 m, tous les sites sans
  sol de la trame) : instances SemanticKITTI (voiture, vélo, cycliste, moto, piéton, camion, autre véhicule) d'au
  moins 100 sites dans la sélection `v12set`, jusqu'à 4 par classe, tailles réparties ; les identifiants restent ceux
  des retours bruts de la trame.
- **Boules des k plus proches** (`knn_*`) : 100, 300, 1 000, 3 000, 10 000 sites autour d'un site tiré par
  SplitMix64, une trame par séquence ; sélection exacte (distance carrée entière, égalités par indice).
- **Bouts de scène de la v11** (`bout_*`) : 2 ou 3 objets proches (voitures, vélos, piétons), sans sol ni fond, de
  **10 séquences** (dont 03, 04, 07, 09) ; empreintes du `bouts.json` de la v11 vérifiées ; au plus 6 par séquence.


| Sous-famille | Nuages | Sites (min - max) | Bits (min - max) |
| --- | ---: | --- | --- |
| boule_knn | 30 | 100 - 10 000 | 9 - 17 |
| bout_v11 | 51 | 107 - 9 957 | 11 - 14 |
| objet_reel | 28 | 102 - 9 628 | 11 - 13 |
| objet_reel_contexte | 23 | 103 - 4 522 | 11 - 13 |
| synthetique | 27 | 100 - 10 000 | 12 - 21 |

## 6. Mesures transverses : doublons au millimètre et bits

| Jeu | Doublons au mm (retours en trop / retours) | Multiplicité max | Bits nécessaires (x, y, z) | Remarque |
| --- | --- | ---: | --- | --- |
| SemanticKITTI (275 trames) | 0 dans toutes les trames, brutes et sans sol | 1 | 17–18 | portée ≤ 80 m de part et d'autre du capteur |
| IGN LiDAR HD | 0,005 % à 0,022 % (Lyon 2 251, Paris 996, Marseille 1 490 ; sans sol 1 566, 440, 303) | 2 | 20, 20, 17 (dalle de 1 km²) ; 18–20 (découpes) | source au centimètre : doublons au cm |
| ETH3D (TLS) | **0,8 % à 3,7 %** (meadow 1,10 %, courtyard 0,80 % et 1,02 %, electro 3,69 %) | 4 à **26** | 14–17 | densité sous-millimétrique près des stations : le vrai cas de la décision D8 |
| FOR-instance (ULS) | SCION 0,0002 %, NIBIO 0,51 %, **TUWIEN 17,6 %** (sans sol : 17,2 %) | 2 à 4 | 16–17 | placettes de 28 à 86 m, coordonnées au mm ; TUWIEN : recouvrement de bandes très dense (supposé) |
| Boreas (MLS) | 0 à 0,005 % (n50 : 361 ; itinéraire : 304) | 3 | 19, 19, 16–17 (fenêtres) ; **21, 22, 17** (itinéraire, 1 748 m × 2 503 m) | seule donnée réelle au-delà de u21 |
| Petits nuages | 0 (par construction ou par la source) | 1 | 9–21 | — |

Conséquences pour la v12 : (1) u21 suffit pour SemanticKITTI, l'IGN (dalle de 1 km²), ETH3D, FOR-instance et les
fenêtres Boreas ; l'itinéraire Boreas (22 bits) est l'entrée réelle de qualification u24 ; (2) la variante
`.distinct` est indispensable pour ETH3D et la placette TUWIEN (le moteur refuse les doublons par défaut) et ne retire
ailleurs que quelques centaines à quelques milliers de retours ; (3) les multiplicités publiées permettront de juger
plus tard un modèle « par copies » sans repréparer les données.

## 7. Rejouer sur la VM G4

Deux voies, la première recommandée. Dans les deux cas : aucune donnée dans le dépôt, et le lanceur v11 n'accepte que
`./mhgp11*`, `ctest` et `python3 {src}/morsehgp3D_v11/<script>.py` — la lignée v12 doit avoir son propre lanceur
(`LECONS_ET_PIEGES.md` § 3), qui devra admettre `python3 {src}/morsehgp3D_v12/bench/data/<script>.py` si l'on choisit
la voie B.

**Voie A — préparer ici, envoyer des paquets plats, vérifier sur la VM (Python 3.10 nu suffit).**

1. Sur le codespace (ou toute machine avec Internet, numpy, laspy + lazrs, py7zr, g++) :
   `bash scripts/replay_all.sh` (12 min 27 s mesurées, téléchargements en cache ; idempotent ; sorties identiques
   à l'octet, empreintes aux §§ 3–5). `/tmp` étant vidé à chaque redémarrage et `/workspaces` plein, rejouer juste
   avant la session.
2. Les paquets `bundles/g4_kitti_v12set` (37 trames, avec étiquettes, 45 Mo), `g4_small` (159 petits nuages,
   6 Mo), `g4_ign_lidarhd` (3,2 Go), `g4_eth3d` (3,1 Go), `g4_forinstance` (1,4 Go) et `g4_boreas` (1,7 Go) —
   scènes entières en variante distincte et toutes les découpes — respectent les contraintes de `--data` du lanceur
   gardé (dossier plat, noms `[A-Za-z0-9][A-Za-z0-9._-]*`, au plus 512 fichiers et 8 Gio, aucun fichier vide,
   `.u32le` multiple de 4 ; `SHA256SUMS.txt` et non `SHA256SUMS`, nom réservé au lanceur). Un paquet par session, ou
   plusieurs petits réunis par `make_g4_bundle.py`, qui accepte plusieurs manifestes.
3. Sur la VM, avant toute mesure : `python3 -S verify_inputs.py <dossier de données>/bundle_manifest.json`
   (`$MHGP11_DATA_DIR` dans le lanceur v11 ; bibliothèque standard seule : tailles et SHA-256 de chaque fichier
   contre le manifeste ; code 0 conforme, 1 écart, 2 manifeste illisible). Avec numpy (Python portable), `--measure`
   recompte n, positions distinctes, étendue et bits.
4. Les scènes avec doublons voyagent en variante `.distinct` (`--prefer-distinct`) : le moteur, qui refuse les
   doublons par défaut (D8), les lit sans option ; le manifeste du paquet déclare `bundled: distinct` et l'objet
   calculé (tour des positions distinctes).

**Voie B — préparer sur la VM.** Les scripts (environ 140 Ko, sous-dossiers compris) voyagent dans le paquet source
de la session, donc versionnés dans le dépôt (par exemple `morsehgp3D_v12/bench/data/`). Le dossier `--data` étant
plat, y envoyer : un Python portable ≥ 3.10 avec numpy, laspy, lazrs et py7zr, en archive (FOR-instance et
SemanticKITTI n'exigent que numpy) ; les archives de la sonde (`patchwork_sources.tar.gz`, `eigen3.tar.gz`) ; pour
SemanticKITTI, les trames brutes `.bin` + `.label` (environ 2,4 Mo par trame : 666 Mo pour les 275 trames, environ
90 Mo pour les 37 de `v12set` ; elles ne se retéléchargent pas sans l'outil de cache du projet) et les trois trames de
contrôle 08/000000, 000100, 000200 ; pour les petits nuages, `bouts.json` et les fichiers des bouts de la v11
(`build/v11-persist/bouts`, 36 Mo ; `--bouts` accepte un dossier plat). Puis
`PY=<python portable> PATCHWORK_ARCHIVES=... KITTI_FRAMES=... KITTI_CONTROL=... BOUTS=... bash scripts/replay_all.sh`.
La VM télécharge elle-même IGN, ETH3D, FOR-instance et Boreas (si elle a un accès sortant) ; les empreintes
épinglées refusent tout octet différent. La sonde Patchwork++ y est compilée par GCC 11.4 (GCC 13 ici) : le contrôle
du masque de 08/000000 (`9db3fe5c…`) et le contrôle bout à bout ng00–ng02 décident si la chaîne est la même (code 3
sinon). Les sorties doivent reproduire à l'octet les empreintes des §§ 3–5 : c'est le contrôle de la voie B.

## 8. Limites, écarts déclarés, suites

1. **Volatilité.** Tout est sous `/tmp` (vidé au redémarrage du codespace ; le dernier a eu lieu à 07 h 03 UTC) et
   `/workspaces` est plein (2,1 Go libres) : seuls les scripts méritent d'être versionnés (par exemple sous
   `morsehgp3D_v12/bench/data/`, environ 140 Ko, aucun octet de donnée) ; les données se rejouent en un quart
   d'heure environ.
2. **Translation des trames KITTI.** La v12 translate au minimum des sites **gardés** ; les entrées du contrat v11
   (ng00–ng02) translatent au minimum de la trame brute entière. Le contrôle bout à bout rejoue la convention v11 et
   retrouve les octets de ng00–ng02 ; les fichiers v12 des mêmes trames en diffèrent par la seule translation
   (identique pour 08/000200, dont les extrêmes ne sont pas du sol). Les niveaux de la tour sont invariants par
   translation (D6).
3. **Sol.** IGN : classe 2 du producteur ; FOR-instance : classe 2 « terrain » de l'annotation du producteur ;
   SemanticKITTI et Boreas : Patchwork++ épinglé (approximatif, inconnus gardés) avec, pour Boreas, une hauteur de
   capteur de 2,2 m mesurée sur la première trame (aucun réglage supplémentaire) ; ETH3D : **pas de variante sans
   sol** (aucune classification ; on n'ajoute pas d'heuristique).
4. **Points virtuels de l'IGN** (classe 66, ajoutés sous les ponts) retirés de toutes les variantes : ce ne sont pas
   des retours LiDAR. L'index WFS annonce 0,005 à 0,04 % de points de plus que les fichiers (Paris 14 553 289 contre
   14 552 516 lus, Marseille 6 712 268 contre 6 710 565, Lyon 32 429 318 contre 32 416 199 ; autre édition de
   l'index) : publié dans le manifeste, non bloquant.
5. **Résolution source.** IGN : entiers LAS au centimètre (échelle 0,01) — la grille de 1 mm ne porte que des
   multiples de 10 mm, et les doublons au mm sont des doublons au cm. FOR-instance : entiers LAS au millimètre (la
   grille est exactement celle de la source). ETH3D et KITTI : float32 ; Boreas : float32 transformé en float64.
   FOR-instance est épinglé par le SHA-256 du LAS **extrait** (le zip entier n'est jamais lu ; le CRC32 du membre est
   vérifié par `zipfile`).
6. **Boreas** : accumulation faite ici (le jeu ne publie pas de nuage accumulé) ; compensation du mouvement au
   premier ordre par point (plus fine que les 21 paquets du devkit) ; les objets mobiles laissent des traînées
   (donnée réelle, non filtrée). Convention de pose contrôlée à chaque lancement (`--check` : recouvrement de voxels
   de 5 cm entre trames voisines 0,248 contre 0,211 sans compensation, 0,179 avec le signe opposé et 0,008 avec la
   rotation transposée).
7. **SemanticKITTI** : 6 séquences locales (00, 02, 05, 06, 08, 10) ; 01, 03, 04, 07, 09 exigeraient de nouvelles
   trames, que seul l'outil de cache partiel du projet (`Zoltan/demos/tools/kitti.py`, archive officielle soumise à
   enregistrement sur le site KITTI) sait obtenir : décision laissée à l'utilisateur. Les « bouts » de la v11 couvrent
   déjà 10 séquences en petits nuages.
8. **Taille maximale.** Plus grande scène préparée : 48,4 M de retours (ETH3D electro) ; pic mémoire de la
   préparation 6,8 Go. Au-delà (dalles AHN de 8,6 Go, mosaïques IGN), prévoir une lecture par blocs.
9. **u24 / u32.** Seule la scène Boreas « itinéraire » dépasse 21 bits (22 bits ; ses découpes de 4 et 8 M en
   ont 21). Exercer 23 ou 24 bits sur du réel demande 4,2 à 16,8 km d'étendue (itinéraire Boreas sur plusieurs
   séquences, ou dalles IGN éloignées dans le même repère Lambert-93, concaténées avant la translation : non fait
   ici) ; au-delà de 24 bits, seules des données synthétiques (ou l'union déclarée de deux scènes réelles placées
   loin l'une de l'autre) exercent u32 ; une translation seule ne change rien, puisque tout est translaté au
   minimum.
10. **Empreintes du code.** Chaque manifeste enregistre le SHA-256 du script et de chaque fichier de la bibliothèque
   `v12data` au moment où il a été écrit. Le rejeu complet final (12 min 27 s, zéro écart) a été suivi d'un dernier
   correctif (noms des classes du producteur dans les histogrammes des manifestes, sans effet sur les octets des
   données) : IGN et FOR-instance ont été régénérés avec, les autres jeux portent l'empreinte précédente de
   `lasconv.py`. `scripts/SHA256SUMS.txt` donne l'état final des scripts.

## 9. Suites proposées au développeur

1. Verser `scripts/` dans le dépôt (par exemple `morsehgp3D_v12/bench/data/`, avec `scripts/SHA256SUMS.txt`) et ce
   rapport comme `morsehgp3D_v12/docs/DONNEES.md` (que `MESURE.md` § 2 annonce) ; aucune donnée ni coordonnée.
2. Première session G4 : envoyer `g4_kitti_v12set` + `g4_small` (50 Mo), vérifier par `verify_inputs.py`, puis
   mesurer la v11 gelée (`MES-E`, `MES-P` du plan) sur les 37 trames : médiane **et** maximum (D7).
3. Sessions suivantes : un paquet multi-millions par session (3,2 Go IGN, 3,1 Go ETH3D, 1,4 Go FOR-instance,
   1,7 Go Boreas), découpes 1, 2, 4, 8 M d'abord (exposant d'échelle), scènes entières ensuite.
4. À décider par l'utilisateur : étendre SemanticKITTI aux séquences 01, 03, 04, 07, 09 avec l'outil de cache du
   projet ; préparer une entrée réelle de 23–24 bits (itinéraire Boreas multi-séquences).
