# État courant — audit de MorseHGP3D v5

- **Date :** 27 août 2026
- **Auditeur :** Codex, avec relecture critique des audits concurrents
- **Tip produit relu statiquement :** `7d94aee9919a054cc8bb6a79e4b8f1afef58820a`
- **Worktree concurrent post-tip :** relu statiquement et hors verdict ; fixtures F1/F3/F5/F6/F7, jeton `AnchorPretests`, sonde/provenance et auto-copie GCP en cours
- **Dernier pin avec sorties locales observées :** `7eb33608180be1ece5444601c951dfb770c418df`
- **Pin du protocole `EXTRA_N` relu statiquement :** `d837adb2a4cad65b4bce51640df8124539bedf56`
- **Pin d'activation initiale relu :** `a9a2f509428bbfebd9543579d16d1579a7591106`
- **Pin documentaire relu :** `fa99b3f127e06aa686a301c084f8311e80d5c554`
- **Reçu G4 le plus récent :** [`campagne_g4_v5_20260827_adaptatif`](../receipts/campagne_g4_v5_20260827_adaptatif/RECU.txt), source `8f95df2effd07ffa7a8aa7cf7fe79be1be9c7b2c`
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`
- **GCP :** non utilisé par l'auditeur ; une session concurrente de Claude est en cours, sans résultat reçu ici et sans aucune mutation de notre part

## Verdict

La v5 reste **orange, avec un progrès fonctionnel réel mais une autorité encore
incomplète**. Les dernières sorties locales observées portent sur le pin propre
`7eb33608` : **165/165** CTests étiquetés `gate`, dont 7 oracles. Elles viennent
du travail concurrent et n'ont pas été rejouées par cet audit. Le tip
`7d94aee9` ajoute une gate qualifiée d'oracle, les comparaisons/planchers des
nouveaux compteurs et un signal d'invariant Q4 ; il est reçu ici statiquement,
pas comme nouveau résultat exécuté.

La suffisance mathématique du compte `W3` et du certificat sectoriel est reçue
statiquement sous le profil u16. F2 et F4 qualifient désormais séparément les
frontières sectorielle et `W3` à `h-1`, et `7eb33608` retire les doubles scans
Q3 hôte et Q4 oversized. Aucun faux rejet nominal n'a été identifié.

Cette réception ne qualifie pas encore l'activation comme autorité produit. Il
manque surtout :

- une comparaison indépendante filtre ON/OFF de tous les supports owner et de
  leur profondeur exacte, avant **et** après RLE, sur de petits cas Q3 et Q4 ;
- une fixture Q4 ciblée prouvant qu'une ancre sectoriellement morte possédait
  bien au moins un seed dans le bras OFF ;
- une garde `J <= 0`, et non seulement `J < 0`, ainsi qu'une identité `P/B`
  calculée sans dépassement sur tout le profil u16 ;
- une sémantique de routage Q4 indépendante du seuil ;
- un reçu de mesure réellement épinglé au binaire exécuté.

Les chiffres sectoriels régénérés par `7eb33608` sont une exploration utile,
pas un reçu : leurs fichiers déclarent `pin_execution=fa99b3f1` alors que ce
pin ne contient ni leur sonde ni le bypass contrefactuel compilé. La mesure G4
appariée proposée par Claude reste donc la bonne prochaine décision avant tout
kernel q3 par rectangle.

### Alerte sur le worktree concurrent non commité

Le correctif courant de `gcp-migration/session_campagne_v5_scale_g4.sh` ne doit
pas être lancé en l'état. Il copie le script dans `/tmp`, l'y ré-exécute, puis
calcule `REPO_ROOT` depuis `BASH_SOURCE[0]`. La racine obtenue est alors `/`,
pas le dépôt, avant toute porte ou mutation GCP. Conserver dans l'environnement
le chemin canonique du script source avant `exec`, dériver `REPO_ROOT` de ce
chemin et ajouter au selftest une exécution depuis une copie. Aucun lancement
GCP n'a été effectué pour établir ce défaut.

## Résultats exécutés

| Périmètre | Résultat local | Portée exacte |
|---|---:|---|
| Tip `7d94aee9` | 18,8 M identités et 0 désaccord annoncés dans le commit | auto-rapporté, sans journal reçu ni rejeu par cet audit |
| Release `7eb33608` | **165/165 gates**, 7 oracles, 105,73 s réelles | build canonique local ; aucun CUDA |
| Archive propre `fa99b3f1` | **165/165 gates**, 7 oracles, 101,58 s réelles | requalifie le code de `a9a2f509` indépendamment du worktree |
| Snapshot pré-commit devenu `7eb33608` | 10/10 fixtures et routes ciblées | F2/F4, non-strict, all-host, mixed et mutants ; subsumé par la suite complète |
| ASan+UBSan Debug historique `6e8a6aba` | 11/11 portes ciblées | aucune généralisation à toute la suite |
| ASan+UBSan RelWithDebInfo historique | échec de compilation dans `cloud_index.hpp:130-131` | warning GCC 13 `array-bounds` sous `-Werror`, pas un diagnostic sanitizer d'exécution |
| G4 source `8f95df2e` | 4 couples CPU/GPU à 50 k, deux digests appariés | égalité bornée ; campagne partielle 24/25, non terminale |

Les 165 gates vertes établissent une non-régression bornée du pin précédent,
pas l'exhaustivité du nouveau certificat. `7d94aee9` compare désormais ON/OFF
sur cinq nuages et impose des compteurs sectoriels Q4 non nuls, mais seulement
après RLE et via les mêmes corps produit ; ce n'est pas encore le juge
indépendant de profondeur demandé.

### Temps historiques à 50 000 points

Le seul reçu G4 disponible reste la campagne partielle source `8f95df2e`. Les
durées murales de ses couples à objet identique sont :

| Famille | CPU | `--gpu` | Surcoût GPU |
|---|---:|---:|---:|
| `uniform` | 78 s | 89 s | +14 % |
| `terrain` | 23 s | 44 s | +91 % |
| `scanline_single_pass` | 38 s | 96 s | +153 % |
| `eight_clusters` | 246 s | 718 s | +192 % |

Ces chiffres ne se transfèrent pas à `7d94aee9`. Ils expliquent le verrou
actuel : le chemin GPU historique accélère des kernels isolés, mais paie la
matérialisation, les transferts, les très nombreux lancements et tout le reste
du pipeline hôte. Une nouvelle campagne appariée doit mesurer le pin réellement
exécuté avant toute conclusion de performance.

## W3 et secteurs — ce qui est reçu

- Pour une ancre non dégénérée, les rayons carrés `D2/12` en Q3 et `D2/8` en
  Q4, la stricte intériorité aux sommets du fan et les largeurs i128 sont
  cohérents sous u16.
- `anchor_universal_kill` est un certificat suffisant : au moins `h` témoins
  universels impliquent qu'aucun seed admissible ne survit.
- `anchor_sector_kill` est également suffisant si le fan couvre le disque des
  centres et si chacun de ses secteurs possède au moins `h` témoins stricts.
- Le radial break à partir de la classe 11 est sûr pour un cover trié selon le
  contrat courant : aucun point de la boule diamétrale ouverte ne se trouve
  après cette classe.
- F2 contient un vrai seed aigu de profondeur 8 pour `h3=9` et qualifie la
  frontière sectorielle. F4, ajouté par `7eb33608`, contient un vrai seed,
  `W3=8`, `wmin=0` et qualifie séparément la frontière `W3`.
- Le « K8 octogone » est en général un parallélogramme dont quatre côtés sont
  subdivisés par des sommets colinéaires. L'appeler **fan parallélogramme à huit
  secteurs** évite de lui attribuer une convexité stricte ou une monotonie K4
  inexistante.

## W3 et secteurs — fermetures encore requises

### Fixtures et oracle

Au tip, F1 et F3 établissent l'incomparabilité des deux **prédicats**, mais leur
effet produit reste vacu. Le worktree ajoute `x=(1000,1200,0)` aux deux cas :
la construction est statiquement cohérente, avec neuf intérieurs pour F1 et
neuf témoins `W3` pour F3, mais elle reste hors verdict tant qu'elle n'est pas
commise et reçue.

Le même défaut touche l'exemple 2.4 publié. Le worktree en grave une version F5
avec le même seed et treize intérieurs, ce qui rend l'idée non vacue. Corriger
toutefois « 28 sites » : le tableau construit six quadruplets et une paire,
soit **26** sites sur la sphère diamétrale. L'analyse canonique doit ensuite
être alignée sur la fixture réellement retenue.

Le worktree corrige le commentaire 25/37 et ajoute F6 pour isoler une égalité
de demi-plan strictement à l'intérieur de la boule diamétrale. Cette séparation
est mathématiquement cohérente ; le mutant reste à recevoir sur un pin propre.

Il n'existe encore aucune fixture SectorKill Q4 **non vacue côté produit**. F7
dans le worktree réemploie F1, dont tous les points sont coplanaires : elle peut
faire monter `wmin` Q4, mais aucune complétion Q4 n'existe dans ce nuage. Une
construction minimale proposée à Claude utilise l'ancre
`(0,0,0)`–`(2000,0,0)`, le support
`x=(1000,1000,1000)`, `y=(1000,1000,-1000)` et sept paires
`(1000+e,+-900,0)`, `e=0..6` : nominalement `W4=0`, `wmin=7` et le candidat
Q4 survit ; le mutant `h-1` le perd par secteurs. Ajouter aussi les cas de
borne équilatérale Q3, tétraèdre régulier Q4 et extrêmes u16.

La porte d'autorité doit énumérer indépendamment tous les supports owner et
leur profondeur exacte, comparer filtre ON/OFF avant et après RLE, puis imposer
des morts `W3` et sectorielles non nulles. Elle peut éviter le juge continu en
`O(m^2 log m)` ; elle ne doit pas réutiliser le prédicat d'ancre qu'elle juge.

La nouvelle `mhgp5_anchor_tests_oracle` de `7d94aee9` est une bonne porte
différentielle, mais son nom sur-promet encore quatre propriétés :

- elle appelle directement `scan_anchor_q3` et `process_anchor_q4` des deux
  côtés ; elle ne calcule ni supports owner ni profondeurs par une autorité
  indépendante ;
- elle applique `rle_candidates()` avant la seule comparaison et peut donc
  masquer un écart de multiplicité brute ;
- ses compteurs sectoriels agrégés ne prouvent pas que les ancres tuées avaient
  un seed OFF, condition de non-vacuité de l'optimisation ;
- elle annonce `J > 0` mais ne rejette que `J < 0`. L'identité `P/B` multiplie
  en i128 des termes dont la borne u16 approche 156 bits, puis ne visite que
  douze sites par complétion malgré le commentaire « tout site » ;
- son enregistrement CMake porte seulement le label `oracle`, contrairement aux
  autres juges `oracle;gate` : la commande canonique `-L gate` ne l'exécute pas.

Conserver cette porte comme différentiel borné, puis employer une arithmétique
large signée pour `P(z)B(y)-P(y)B(z)`, refuser `J <= 0`, comparer les sorties
brutes et ajouter un petit oracle structurel réellement indépendant.

### Intégration et compteurs

Le booléen de `7eb33608` est remplacé dans le worktree par le jeton typé
`AnchorPretests`. C'est un progrès de lisibilité, mais les valeurs
`kAlreadyApplied` et `kCounterfactual` ont le même effet et restent publiquement
sélectionnables dans le header produit. Un corps interne
`*_after_anchor_tests`, et une exposition contrefactuelle limitée aux builds de
test, fermeraient réellement la précondition.

`7d94aee9` compare maintenant `anchors_killed_w3` et
`anchors_killed_sectors` entre exécuteurs. Les planchers sont toutefois imposés
sur chaque invocation, y compris les variantes qui visent une autre propriété ;
des options `--min-anchor-*` à zéro par défaut et des portes dédiées seraient
plus robustes. En Q4, une ancre sous le seuil est envoyée à
`process_anchor_q4`, peut être comptée morte W4/secteurs puis aussi
`anchors_host`, tandis que la même ancre au-dessus du seuil meurt avant routage.
Le ledger dépend donc encore du seuil. Appliquer W4+secteurs avant toute décision
de route et verrouiller une décomposition commune, ainsi que
`seeds = seeds_host + seeds_device`.

Le nouveau compteur `invariant_jneg` améliore le statut de `run_pipeline`, mais
le corps continue de traiter `J < 0` comme une mort avant le refus terminal ;
les appels directs à `generate_candidates` et aux APIs batched/device peuvent
donc encore rendre un objet amputé sans statut. Propager l'erreur à la source,
inclure le cas `J == 0`, comparer ce compteur dans les gates Q4 et ajouter une
porte de statut/code 3 sans callback.

Les huit compteurs sectoriels sont des `u32` incrémentés jusqu'à la taille du
cover alors que seul `h <= 10` importe. Les saturer à `h` retire un domaine
d'overflow. Les additions `size() + ajout` Q3/Q4 et plusieurs index cumulés
`u32` restent à borner avant addition et avant cast. Les portes oversized
doivent imposer un compteur de repli non nul, séparément pour sites, seeds et
paires.

## Sonde et provenance des mesures

`7eb33608` corrige la circularité principale de la sonde : le corps
contrefactuel s'exécute avec les tests d'ancre désactivés, puis le certificat de
production est évalué séparément. Les nouvelles sorties permettent donc une
exploration des populations. Elles ne sont toutefois pas un reçu :

- les anciens fichiers de `mesures_secteurs_20260827` ont été écrasés au lieu
  de créer un dossier distinct pour la nouvelle variante ;
- chaque brut annonce `pin_execution=fa99b3f1`, mais le binaire inclut le diff
  devenu `7eb33608` ; `fa99b3f1` ne peut donc pas le reconstruire ;
- `git rev-parse HEAD` est capturé à la **configuration** CMake, sans dépendance
  à `.git/HEAD`, hash du diff ou hash binaire ; le worktree ajoute un marqueur
  dirty configure-time utile mais toujours non reconstructible ;
- `LISEZMOI.txt` conserve en tête `HEAD 312034ce + sonde`, contradictoire avec
  les bruts ;
- la cible emploie `mhgp5_executable` et compile donc `MHGP5_TESTING=1`, pas le
  mode produit ;
- au tip, `wrong` fusionne K4, K8 et le cumul puis le programme rend toujours 0,
  et le timer mélange sonde et production. Le worktree rend désormais tout
  `wrong` bloquant et sépare trois timers ; il reste à distinguer les familles
  de contradiction dans le reçu ;
- commande, toolchain, configuration complète, RSS, code de sortie, hash
  binaire et manifeste d'entrée ne sont pas conservés.

Le nouveau dossier devra être construit depuis un commit source propre, avec
commande et toolchain, `git status --porcelain` vide, SHA-256 du binaire et des
entrées, sorties, codes et manifeste. Le pin imprimé dans le binaire peut
rester un contrôle secondaire, jamais l'autorité de provenance.

La documentation doit aussi distinguer secteurs seuls et cumul. Les nouveaux
bruts `eight_clusters` Q3 annoncent pour le cumul 64,5 % / 95,0 % à n=2000 et
67,8 % / 97,1 % à n=4000, alors que `GPU.md` attribue encore 54–60 % / 92–94 %
au cumul. Les temps locaux `33,0 -> 13,7 s` restent des
`mesure_locale_non_recue`.

## Autres P1 encore ouverts au pin courant

- **Validateur Q4 :** gardes nulles incomplètes, flux vide cohérent encore
  acceptable, option `emit_eq=false` utilisable comme bypass et recherche
  `O(n_emits * lens_count)`.
- **CLI et capacités :** parsing permissif par `std::atoll`, additions avant
  bornes et casts `u32` avant validation exhaustive.
- **CUDA :** la porte `route-ignore-threshold` reste enregistrée alors que la
  source CUDA Q3 ne parse pas ce mutant. `nvcc` est absent de la machine ; aucun
  résultat CUDA courant n'est revendiqué.
- **Autorité des overrides :** le statut terminal ne distingue toujours pas
  `cpu_reference` de `experimental_override` dans les callbacks et reçus.
- **Campagne G4 :** `d837adb2` juge les extras qu'il découvre, mais le
  validateur ne reçoit pas le plan `EXTRA_N/EXTRA_FAMILIES` demandé. Un extra
  omis peut donc laisser `complete`, et un reçu 50 k recopié sous un nom 100 k
  passe faute de liaison `famille/n` entre nom et corps. Le scénario négatif
  fait déjà échouer le contrat obligatoire 50 k de la famille et ne qualifie
  pas l'extra seul. Valider/normaliser les entrées avant SSH, refuser doublons
  et `N=50000`, graver l'argv et le plan, puis les transmettre au validateur.
  Enfin, placer les extras après les phases obligatoires ou leur donner un
  budget global : quatre timeouts de 7200 s précèdent actuellement la phase GPU
  dans une session bornée à 14400 s.
- **Documentation :** corriger `GPU.md`, versionner le schéma des compteurs et
  rendre le pin différentiel v4 reproductible avant d'augmenter son autorité.

## Réponse à Claude et ordre de fermeture

La réponse détaillée V7–V14 est conservée dans
[`QUESTION_CLAUDE_TESTS_D_ANCRE_20260827.md`](QUESTION_CLAUDE_TESTS_D_ANCRE_20260827.md).
Ordre conseillé :

1. Recevoir la campagne G4 appariée CPU/`--gpu` au même pin, sur les quatre
   familles, avant tout kernel q3 par rectangle. Mesurer murs par phase,
   variance, RSS/VRAM et deux digests ; ne pas décider sur le seul seuil 20 s.
   Les extras optionnels ne doivent ni masquer ni affamer ces phases.
2. Transformer la gate ON/OFF de `7d94aee9` en porte d'autorité : comparaison
   brute et post-RLE, profondeur indépendante, `J <= 0`, produit large `P/B`,
   F1/F3 non vacus et fixture Q4 avec seed OFF.
3. Uniformiser le ledger Q4, remplacer le booléen de bypass et fermer les
   bornes `size_t/u32` ainsi que les replis oversized.
4. Publier un nouveau reçu sectoriel depuis un pin propre, sans écraser
   l'historique, puis corriger `GPU.md`.
5. Fermer ensuite le validateur Q4, le mutant CUDA, l'autorité des overrides et
   le protocole de campagne.

Le test cellulaire reste une dérivation expérimentale, pas une lane ouverte.
Une grille de cellules et un fan sectoriel à plus de huit secteurs sont deux
certificats différents ; ne pas présenter l'un comme le raffinement automatique
de l'autre. Le facteur 2,33 du mou histogramme est structurellement plausible,
mais demeure une observation de famille/taille/pin, jamais un invariant.

## Reproduction et limites

```text
sorties concurrentes au pin 7eb33608 : 165/165 gates, 7 oracles, 105,73 s
archive propre fa99b3f1 : 165/165 gates, 7 oracles, 101,58 s
tip 7d94aee9 : non exécuté par cet audit
```

Aucun test n'a été lancé pour cette passe, conformément à la demande. Les
journaux cités restent locaux et non versionnés. Le probe racine
`.codex_fold_contract_probe.cpp` appartient à un autre auditeur ; il n'a été ni
ouvert, ni modifié, ni inclus. GCP n'a pas été utilisé par l'auditeur ; la
session concurrente de Claude n'a été ni interrogée ni mutée.
