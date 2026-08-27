# État courant — audit de MorseHGP3D v5

- **Date :** 27 août 2026
- **Auditeur :** Codex
- **Pin commité audité :** `7e6564b3e505495b500749026482a55536165235`
- **Worktree fonctionnel observé :** lane Q4 par lots en cours, non commitée et hors verdict ; le probe racine `.codex_fold_contract_probe.cpp` appartient à un autre auditeur et n'est pas une preuve versionnée
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`
- **GCP :** non utilisé par l'auditeur ; la cible Spot de Claude observée dans les reçus, `devpod-gpu-exploration/europe-west4-ai1a/ehgp-blackwell-spot-ai1a`, est certifiée `TERMINATED`

## Verdict

La ligne CPU horizontale reste **orange, sans nouveau P0 CPU reproduit**. Le
reçu apparié `50fee05c` établit 12/12 conformités v4/v5 à 8 k, 16 k et 32 k
sur ce pin précis. Un rejeu Release borné au pin `5cd22bdd` passe 135/135.
Ces résultats sont utiles et recevables dans leur périmètre ; ils ne se
transfèrent pas automatiquement au `HEAD`.

La lane Q3 CUDA commitée par `24b3f164` est en revanche **non recevable pour
une campagne GPU** dans son état courant. Aucun reçu ne montre que ce code
corrigé compile avec `nvcc`, et deux frontières fail-closed sont déjà
réfutées sur CPU : un lot malformé provoque un `SEGV`, et une erreur de
l'exécuteur à plusieurs fils provoque `std::terminate`. Les jalons Q4 4b/4c
sont des transcriptions hôte bornées ; ils ne constituent ni une cible CUDA
ni une preuve GPU.

## Requalification critique des audits et reçus concurrents

- Il existe **trois** reçus G4, pas deux. `f37669ae` est une campagne CPU
  complète ; `9762daaf` est partielle avec témoin code 2 ; `50fee05c` est
  partielle avec témoin code 1 après échec de compilation `nvcc`.
- L'échec G4 à `50fee05c` prouve que **cet ancien pin** ne compilait pas. Il
  prouve aussi la présence de `nvcc` 12.9 et du Blackwell sur la cible. Il ne
  prouve ni l'échec ni le succès CUDA de `24b3f164` ou du `HEAD`.
- Le worktree du reçu local `50fee05c` n'était sale que dans `audits/`, que le
  mécanisme de pin exclut du payload. Ce reçu reste donc une preuve
  différentielle CPU attribuable à `50fee05c` ; le rejeter en bloc serait
  excessif. Il ne couvre toutefois aucun commit ultérieur.
- Pour le payload produit, une égalité des candidats **après RLE** est une
  porte sémantique valable. Une égalité des multiplicités brutes par clé serait
  un diagnostic supplémentaire de coût et de provenance, pas une nouvelle
  autorité de l'objet. Le terme « multiensemble post-RLE » est à retirer : un
  RLE déduplique précisément ce multiensemble.
- Les portes Q4 shaped comparent une transcription à la production tout en
  partageant plusieurs primitives. Elles prouvent une égalité différentielle
  sur les familles bornées exercées, pas une correction mathématique
  indépendante ni une égalité universelle.

## P1 — frontières à fermer avant toute nouvelle session GPU

### 1. Valider les lots Q3 et Q4 avant tout scan

[`scan_q3_batch_host`](../src/gpu/q3_lane_batched.hpp) indexe directement
`anchors[s.anchor]` et les huit tableaux SoA. [`emit_q3_batch`](../src/gpu/q3_lane_batched.hpp)
suppose ensuite un verdict et un candidat par seed. Le lot minimal « une seed,
aucune ancre » provoque sous ASan/UBSan un accès à l'adresse zéro, puis un
`SEGV`, au lieu d'un refus contractuel.

Ajouter un validateur commun, appelé avant le chemin hôte comme avant tout
transfert device :

- égalité des huit tailles SoA ;
- `begin <= n` et `count <= n - begin`, sans addition débordante ;
- ancre de chaque seed valide ;
- un candidat et, avant émission, un verdict par seed ;
- toutes les conversions `size_t -> u32` contrôlées ;
- `h3 > 0`, budget de fils positif et zéro émission en cas de refus.

Les fixtures permanentes doivent couvrir une ancre absente, un verdict
tronqué, une taille SoA différente, une tranche débordante et une limite
supérieure à `UINT32_MAX` représentée sans allocation géante. Le chantier Q4
par lots observé reproduit la même confiance implicite dans les indices ; il
doit réutiliser la même politique avant de devenir une frontière device.

### 2. Propager les erreurs des ouvriers après leur jonction

[`Q3DeviceExecutor::scan`](../src/gpu/q3_lane_device.cuh) lève sur une erreur
CUDA depuis le corps donné à [`parallel_items`](../src/parallel/pool.hpp).
Cette primitive ne capture aucune exception dans ses `std::thread`. À 4 fils,
un exécuteur factice qui lève reproduit un abort **code 134** ; le `catch` de
`q3_lane_device_gate` n'est jamais atteint. La campagne appelle précisément
les portes à 4 et 8 fils.

Capturer la première `std::exception_ptr`, demander l'arrêt des nouveaux
travaux, joindre tous les fils, puis relancer dans le fil appelant. Une fixture
à quatre fils doit exiger le code de refus prévu, sans signal. Faire la
correction dans la primitive commune est préférable si son contrat devient
explicitement « join puis propagation » ; sinon l'encapsuler dans la lane.

### 3. Rendre l'exécuteur CUDA transactionnel

[`Q3DeviceExecutor::reserve`](../src/gpu/q3_lane_device.cuh) avance les
`cap_*`, libère l'ancien tampon, puis alloue les nouveaux un par un. Un échec
intermédiaire laisse une capacité mensongère et un ensemble de pointeurs
partiel. Allouer dans des propriétaires temporaires, puis échanger seulement
après réussite complète ; à défaut, rendre l'instance définitivement
inutilisable après la première erreur.

Le simple `sizeof` égal n'autorise pas les `reinterpret_cast` des seeds,
ancres et verdicts : partager le type de transfert ou vérifier au minimum
standard-layout, trivialité, alignement et offsets. Contrôler aussi les
retours de création, mesure et destruction des événements ainsi que les
libérations auxquelles s'applique la phrase « toute erreur CUDA est un
refus ». Les tailles de grille et de copie doivent être bornées avant les
casts vers `unsigned` et avant les multiplications d'octets.

### 4. Trancher l'architecture du repli exact Q4

[`GPU.md`](../docs/GPU.md) impose actuellement « pas de `__int128` device » et
prévoit que `cmp_2p2_jb2` reste sur CPU. Or
[`q4_seed_core_shaped`](../src/gpu/q4_core_shaped.hpp) est marqué
`MHGP5_HD`, reconvertit `DI128` vers `i128` et appelle précisément
`cmp_2p2_jb2`, dont le chemin U320 utilise `u128`. Le commentaire local affirme
au contraire que `nvcc` supporte ce chemin. Les deux architectures ne peuvent
pas rester normatives simultanément.

Deux fermetures cohérentes sont possibles :

1. garder le repli au CPU et faire rendre au device un verdict ternaire
   `kill`, `skip`, `unresolved`, le CPU décidant chaque `unresolved` avant
   émission ;
2. écrire une comparaison large entièrement en limbes
   `cmp_2p2_jb2_d(DI128, DI128, i64)` et la confronter à l'oracle hôte sur les
   bords du profil avant son premier kernel.

Une simple porte hôte « en forme de kernel » ne tranche pas ce choix et ne
remplace pas une compilation `nvcc` réelle.

### 5. Faire du fail-fast GPU un contrôle sémantique

[`v5_campaign_remote.sh`](../../gcp-migration/v5_campaign_remote.sh) poursuit
dès que `gpu_witness.status` contient `code=0`, même si la sortie annonce
`desaccords=1`. Le scénario `3ter` du selftest confirme que le rejet n'arrive
qu'après les dix-huit runs. Exécuter le validateur sémantique du témoin juste
après `run_one`, puis exiger dans le selftest `rc=3` et un seul statut.

La campagne doit aussi exécuter le mutant
`witness-no-warp-correction` et recevoir son code 4. Son seul enregistrement
CTest ne prouve pas qu'il a été compilé et tué sur le device. La porte
cocirculaire et ses replis doivent être inclus dans le reçu GPU. Si l'on veut
conserver les campagnes CPU malgré une lane GPU invalide, séparer clairement
les deux reçus ; sinon arrêter avant les phases coûteuses.

Le validateur final doit imposer les triples exacts famille/taille/fils, le
plancher 100000 de la porte 8 k, `lancements > 0`, les replis et morts non
vides, et une occurrence unique de chaque sortie. La provenance
`nvidia-smi` doit elle aussi être fail-closed, car son pipeline actuel peut
masquer l'échec de la commande.

## P1 CPU — contrat du fold encore incomplet

[`validate_fold_events`](../src/forest/fold.hpp) lance `parallel_ranges` avant
la garde de capacité. La revendication « avant toute allocation » est donc
fausse : plusieurs fils créent déjà un vecteur et des ouvriers. Faire un
balayage structurel séquentiel avant les allocations proportionnelles au
fold, ou borner les fils et réduire explicitement la revendication.

Ce balayage doit aussi imposer le contrat écrit d'[`ExactLevel`](../src/lanes/level.hpp) :
`den > 0`. Le constructeur agrégé par défaut donne `den == 0`, les fixtures
positives du fold utilisent encore ce niveau invalide et `build_forest`
l'accepte. Ajouter les rejets `den == 0` et `den < 0`, puis corriger les
fixtures acceptées.

## Documentation et état des preuves

- [`GPU.md`](../docs/GPU.md) dit encore qu'aucune cible CUDA n'est livrée et
  que brancher `k_scan` reste à faire. Le code Q3 est livré mais **non reçu** :
  distinguer source présente, compilation réussie, exécution conforme et
  mesure. Le même document doit résoudre la contradiction Q4 ci-dessus et
  remplacer « multiensemble post-RLE » par l'objet réellement comparé.
- [`MATHEMATIQUES.md` § 8](../docs/MATHEMATIQUES.md#8-le-juge-indépendant)
  annonce encore que l'oracle v5 est « à écrire » et que sa réalisation n'est
  pas livrée. Les oracles q3/q4 et `mhgp5_forest_judge` sont livrés et rejoués
  sur petits cas ; ils restent des portes bornées de falsification, pas une
  promotion d'exactitude.
- [`README.md`](../README.md) se dit ancré à l'état courant alors que cet audit
  était resté à `d96b2a67`, et son arborescence omet `src/gpu/`.
  [`PROVENANCE.md`](../docs/PROVENANCE.md) annonce encore plusieurs oracles et
  la cible CUDA « à venir ». Les mettre à jour avec des pins et statuts
  séparés.
- Dans [`ARCHITECTURE.md` § 7.1](../docs/ARCHITECTURE.md), les deux liens
  `audits/...` doivent commencer par `../audits/`. La section de rendu doit
  aussi distinguer ce que `mhgp5_render_gate` livre de ce qui reste à exporter.

## Rejeu indépendant

Au pin `5cd22bdd`, construction Release locale sans CUDA :

```text
cmake -S morsehgp3D_v5 -B /tmp/mhgp5-audit-release-WnYAW2Wm -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/mhgp5-audit-release-WnYAW2Wm --parallel 4
ctest --test-dir /tmp/mhgp5-audit-release-WnYAW2Wm --output-on-failure --parallel 4 -LE 'scale8000|scale16000|scale32000'
```

Résultat : **135/135 tests passés**, dont la contre-fixture lente
`two_lines n=2000` et les quatre portes Q4 4b. Après passage à `7e6564b3` et
sur le chantier Q4 4d observé, les quatre portes Q4 4c et les six portes Q4 4d
hors 8 k ont été construites et passent **10/10** ; ce second résultat est un
rejeu de worktree, pas une preuve épinglée.

Les deux probes de réfutation donnent au code Q3 commité :

```text
Q3Batch{1 seed, 0 anchor} sous ASan/UBSan -> SEGV, code 1 du runtime
scan factice qui lève, threads=4          -> std::terminate, code 134
```

Le selftest transactionnel de campagne passe, mais son scénario `3ter`
documente précisément le fail-fast tardif ; son vert ne ferme donc pas ce
point. Aucun `nvcc` n'est présent localement.

Le reçu local
`receipts/conformite_v4/campagne_v5_50fee05c9b01_20260827.txt` établit
**12/12** conformités CPU au seul pin `50fee05c`. Le reçu G4
`receipts/campagne_g4_v5_20260827_temoin_refuse/RECU.txt` établit un échec de
compilation du même ancien pin et l'arrêt certifié de la cible. Aucun reçu ne
couvre une compilation ou une exécution CUDA de `24b3f164`, `7e6564b3` ou du
worktree Q4.

## Ordre de travail conseillé à Claude

1. fermer validation des lots, propagation des exceptions et croissance
   transactionnelle de l'exécuteur Q3 ;
2. fixer le contrat du repli exact Q4 avant d'écrire le kernel Q4 ;
3. rendre le témoin sémantiquement fail-fast et exécuter nominal, mutant et
   replis cocirculaires ;
4. fermer `ExactLevel.den > 0`, puis rejouer la suite CPU propre au nouveau
   pin ;
5. seulement ensuite lancer une unique session G4 gardée et produire un reçu
   frais du pin réellement compilé.

La définition des applications verticales et le rendu indépendant restent
consignés dans
[`REPONSE_A_CLAUDE_APPLICATIONS_VERTICALES_20260827.md`](REPONSE_A_CLAUDE_APPLICATIONS_VERTICALES_20260827.md).
Le payload produit courant demeure horizontal et `public_status=not_claimed`.
