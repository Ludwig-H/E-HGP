# Artefact GitHub Actions 10704262200, rapatrié

Preuve d'audit du 22 septembre 2026 qui n'existait que hors dépôt, avec une
rétention limitée : l'artefact expire le **6 octobre 2026** (15:36:59 UTC).
Il a été téléchargé en lecture seule le 22 septembre par le développeur
sortant de la v8, pendant l'ouverture de la v9, puis copié ici sans les
octets de données KITTI. GCP non utilisé.

| champ | valeur |
| --- | --- |
| workflow | `V8 LiDAR rectangle audit` (`.github/workflows/morsehgp3d-v8-lidar-audit.yml`, introduit par `562d090c`) |
| exécution | run 35748470640, créée le 22 septembre 2026 à 15:35:52 UTC, conclusion `success` |
| exécution antérieure | run 35747981707 sur `562d090c`, conclusion `success`, sans artefact rapatrié |
| commit jugé | `ca73e96b` (`REVISIONS.txt`) ; arbre `morsehgp3D_v8/src` = `54a6d581`, identique au dépôt |
| sources de la sonde | `probe.cpp` et `run.py` de `morsehgp3D_v8/audits/lidar_rectangles_20260922/` : sha256 identiques à `ca73e96b` (`SHA256SUMS.txt`) |
| archive téléchargée | 1 558 796 octets, sha256 `c1507232e6d3681beb4846ed65d797ea7a8251ac47eade650ba77b81a4f0cd32` (égal au `digest` publié par l'API) |
| machine | exécuteur GitHub, AMD EPYC 9V74, 4 vCPU (`CPU.txt`), g++ 13.3.0 (`COMPILER.txt`) |
| autotest | `SELFTEST.json` : `pass`, 70 672 comparaisons de voies de paires |

## Contenu

- `measurements/` : les six mesures (scènes 0, 1, 2 ; K = 5 et 10) et
  `SUMMARY.json`, copiés tels quels.
- `replay/native_sources.tar.gz` : les sources v8 compilées par l'exécuteur,
  copiées telles quelles.
- `REPLAY_SHA256SUMS.txt` : empreintes publiées par le workflow.
- `SHA256SUMS` : empreintes de tous les fichiers de ce dossier.
- `EXCLUDED_SHA256SUMS.txt` : empreintes recalculées des fichiers **non
  copiés** (`libmhgp8_p0.a`, binaire reconstructible, et les trois
  `scene_0X.u32le`, coordonnées de trames KITTI à 1 mm). Elles sont égales à
  celles de `REPLAY_SHA256SUMS.txt` et aux entrées versionnées en v8 sous
  `morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6/`.

## Ce que l'artefact établit, et ce qu'il n'établit pas

Portée déclarée par le workflow : « full-cloud native front, sampled
rectangles filter only; no HGP atlas or FULL timing ». Le front natif est
parcouru sur la trame sans sol entière (2,2 à 5,6 s CPU par trame sur
l'exécuteur). Le filtre de rectangles n'est chronométré que sur 24 rectangles
tirés par classe de masse, en trois répétitions.

Lecture du développeur sortant, à rejuger par les auditeurs v9 :

- sur les classes de masse 64–1 023 et 1 024–65 536 paires, le mode 3
  (réemploi des singletons et plans factoriels sur tous les rectangles non
  singletons) divise le temps médian du filtre échantillonné par 1,3 à 4,0
  par rapport au mode 0 ;
- les paires survivantes sont identiques dans les quatre modes, pour chaque
  classe, scène et valeur de K : la cascade accélère le filtre sans rejeter
  davantage ;
- la classe au-delà de 65 536 paires n'est pas mesurée ; elle porte 6 à 23 %
  de la masse de paires selon la scène et K ;
- rien ici ne mesure l'atlas q4, le census ou une tour : aucun gain de bout en
  bout ne s'en déduit. Le plafond de quelques pour cent retenu par la
  synthèse (le filtre pèse environ 7 % du profil gprof) reste la lecture
  prudente.

Les archives `MorseHGP_LiDAR_cascade_2026-09-22.zip` et
`MorseHGP_LiDAR_natif_resultats_2026-09-22.zip`, citées par les audits du
22 septembre comme jointes à une conversation, ne sont pas dans le dépôt et
n'ont pas pu être rapatriées.
