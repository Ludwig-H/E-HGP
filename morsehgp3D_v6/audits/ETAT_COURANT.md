# État courant v6 — audit coopératif

Date de coupe : 2 septembre 2026.

Coupe source observée : `1069bc20` pour le profil `reduce`, `cd606257` pour
la série C C2--C5, `b97f20ea` pour la première exécution G4, `13669280` pour
le durcissement post-session et `c8f69673` pour la reprise persistante ainsi
que la seconde exécution G4. Le reçu courant est archivé par `e66cd978`, lu
par Claude et son validateur à `d5d0bdd4`. Le lot local `2aaa4a53` ferme les
frontières runtime et versionne le canon tests v2 ; la seconde sonde locale
est archivée par `b79e29a5`. Le pin `fa9b2633` durcit la reprise et le
revalidateur, puis `1cb60655` ferme les cinq faux positifs historiques du
harnais de sonde. `fc8e28b1` ferme ensuite ses coutures de publication et
`32da1550` ajoute la réagrégation après scellement. `4ef96717` livre le § 5.22
de reprise/revalidation, repris par `c2d2ac69` sur ses quatre mutants
résiduels. `8afd1057` épingle ensuite le palier sémantique KeyCSR. Le moteur
effectivement exécuté lors de la seconde session G4 reste celui de
`c8f69673`, identique à la première série C `b97f20ea` hors protocoles,
profils, audits et reçus ; le moteur courant `8afd1057` n'hérite d'aucun
résultat G4.
Autorités techniques : `6d755804` pour le
prototype E3/G16, `cd49a390` pour les callbacks, `d98f4729` pour le protocole
et la source réellement exécutée sur G4, `94c74155` pour l'autorisation
mono-session désormais consommée, `df1a3c5f` pour l'archivage de son reçu,
`6e293deb` pour le checkpoint de plafonds, `671ed3cc` pour le premier pool
d'exécuteurs GPU hôte, `4a85c13d` pour sa correction causale, la garde 2E et
le témoin arithmétique hôte, `1069bc20` pour le harnais de profil,
`cd606257` pour le pin hôte de la série C, `d5ed0fb3` pour le pilote ABBA et
le profil de session initial, `5d886db1` pour la fermeture technique des
coutures 5.13--5.15, `b97f20ea` pour la marge invitée corrigée et
`c6ddf715` pour son accusé de SHA, `852ca703` pour le reçu terminal intègre,
`74ab8e7c` pour sa lecture source par Claude, `62cd2e28` pour la matrice locale
directionnelle, `33a12ccc` pour le lot post-session intermédiaire et
`13669280` pour ses cinq fermetures techniques reçues, `1395c4f2` pour le
profil tests K10/K5, `c8f69673` pour la reprise et le pin réellement exécuté,
`1756d4ba` pour l'accusé consommé, `e66cd978` pour le reçu terminal courant,
`d5d0bdd4` pour sa lecture et ses fixtures post hoc, `81623528` pour la sonde
locale destructive, `1bea4bc4` pour son alerte causale et de revalidation,
`b9ea4659` pour le contre-audit de reprise, `2aaa4a53` pour les fermetures
locales reçues avec réserves, `b79e29a5` pour la sonde équilibrée,
`fa9b2633` pour le durcissement de reprise/revalidation, `1cb60655` pour le
premier harnais de sonde, `fc8e28b1` puis `32da1550` pour son scellement et sa
réagrégation renforcés,
`4ef96717` puis `c2d2ac69` pour le lifecycle § 5.22, `38281dc7` pour la réponse
architecturale KeyCSR, `53610911` pour sa pré-inscription, `c5b52c9c` pour son
durcissement, `d6888093` pour son générateur de plan déterministe et
`8afd1057` pour son pin sémantique, `17b6dbea` pour la question de conception
C6, `fec58e1f` pour la réponse au préflight d'échelle, `9243d69f` pour la
capture moteur de `bad_alloc` et `28d02459` pour son correctif de texte,
provision et portée — la question C6 et `fec58e1f` sont documentaires,
`9c5517c9` pour la demande G4 historique, `e8289d9a` pour son profil
historique, `320299df` pour le reçu de
réplication et `8ed2dea6` pour le reçu de confirmation contre-audité
ci-dessous. Les notes Claude ne priment pas sur le présent verdict.

    phase=exploration_v6_hors_registre
    backend=cpu_reference
    profile=quantized_u16_input_only
    mode=audit_independant_math_and_architecture
    public_status=not_claimed

## Verdict

| Sujet | État audité |
|---|---|
| noyau mathématique `381ba60b` | reçu dans sa portée bornée |
| exact-K, omission WSPD et ownership | reçus dans leurs fixtures |
| callbacks et échecs du fold | course de la porte corrigée ; concurrence réelle et exceptions encore ouvertes |
| première campagne d'octaves | `exploratory_complete`, jamais décisionnelle |
| répétition `a30c3a98` | `replication_complete`, reproductibilité seulement |
| confirmation hors échantillon | cœur reçu ; `confirmation_candidate`, déclencheur E6 non confirmé |
| sonde E6 `7611418a` | diagnostic utile, causalité et gain non démontrés |
| prototype E3/G16 `6d755804` | oracle de primitives reçu ; cinq bras et attribution économique non reçus |
| plafonds et budget mémoire `4a85c13d` | cap coopératif et refus transactionnels reçus ; garde logique `2E` avant la réserve de fusion reçue, proxy de payload nommé seulement, sans claim RSS/OOM |
| saturation multi-CPU du fold `1069bc20` | pin construit et suite `gate` 92/92 ; profil, liveness, builds distincts, projection nommée, `join=1` et effacement K2 reçus dans leur portée ; `d5ed0fb3` recalcule la somme, mais ses tolérances acceptent encore la contre-fixture à neuf composantes nulles ; matrice locale terminée mais non décisionnelle |
| série C C2--C5 `cd606257` | reçue dans sa portée CPU/stub : six candidats `u32`, refus transactionnels, sentinelles/validateur, digest gravé, porte de readback device enregistrée mais jamais exécutée et multi-lots causal ; export propre Release 113/113 en 232,11 s ; aucun `nvcc` ni device |
| préparation G4 `b97f20ea` | quatre coutures du § 5.15 reçues sur le chemin épinglé : canon, identité plan/device, architecture 120, fail-fast et manifeste lifecycle à 13 fichiers ; selftests campagne/lifecycle verts et portes G4 v6 117/117 ; deux sous-liaisons du validateur restent à durcir (argv d'attribution seulement inclusif, `arch_compilees` parsé mais non comparé) ; leurs contrôles manuels sont conformes pour le reçu `852ca703` |
| reçu G4 série C `852ca703` | paquet intègre : 202/202 hashes, 58/58 statuts code 0, 16/16 portes GPU et quatre pilotes paritaires ; validateur épinglé `verifie_non_decisionnel`, contrôles manuels argv/architecture conformes ; résultats bornés au § 5.17, jamais un claim produit |
| durcissement local `2aaa4a53` | frontières invitées 600/601 et 480/481, clamp absolu, rc SSH/horloge, provenance legacy, exactitude K5/K10 et canon prospectif `g4_tests_v2` reçus |
| revalidateur `c2d2ac69` | validateur canonique, juge muet, `SHA256SUMS` imbriqué, noms NUL et résumés absents ou différents fermés ; 22/22 avec snapshot stable, reçu dans cette portée |
| reprise persistante `c2d2ac69` / audit `b9ea4659` | D8=67, D11 causal, garde non fatale, promotion liée à la tentative et quatre mutants de `4ef96717` corrigés ; une conversion `GEN_EPOCH` reste faillible entre connaissance de la génération et armement du funnel ; aucun nouveau GO GCP |
| reçu G4 tests K10/K5 `e66cd978` | paquet intègre : 278/278 hashes, 84/84 statuts code 0 et terminés, pins reconstruits, sept résumés identiques, arrêt exact à la première tentative ; égalités de préfixe observées sur cardinalités/digests ; profil et campagne strictement non décisionnels, accusé consommé |
| porte de préfixe `2aaa4a53` | listes K exactes, appariement par clé complète et jumeau K10 obligatoire reçus ; les pins K5 de v1 restent post hoc, tandis que v2 porte ses huit fixtures avant toute future exécution |
| sonde équilibrée `b79e29a5` / harnais `8afd1057` | reçu concret historique intègre et recalculable ; le harnais courant ferme la commande critique héritée du `PATH`, atteste outils/topologie et passe 23 scènes normales/optimisées ; `liveness`, layout, inflight/pics, coordonnée, clés inconnues, `lscpu` et identité stricte restent permissifs ; restaurer aussi l'option terminale `--` de `sha256sum` en corrigeant le faux test ; seule une nouvelle mesure réutilisable attend ces dents |
| KeyCSR `8afd1057` / pré-inscription durcie `c5b52c9c` / générateur `d6888093` | pin sémantique reçu : deux routes sans repli, arènes possédées, comparateur tiers et rejeu ; rvalue refusée à la compilation, kind construit et vrais offsets signés ; matrice 112 cellules, 818 069 deltas et 6 395 137 clés sans désaccord, 185/185 portes rapportées, sonde finale et sanitizers ciblés verts ; aucun GO performance ; générateur reçu comme calendrier seulement, sa grammaire/adjacence, sa variante CTest `-O`, les identités et les frontières `reduce_v3` restent pré-campagne |
| profil d'échelle G4 / moteur `28d02459` / WIP postérieur | **NO START** : le moteur corrigé et ses 4/4 portes sont reçus ; le WIP recoupe désormais cause/étage/RSS et désarme correctement Q2. Restent le trajet lifecycle de `FRONTIER_LAYOUT`, la porte CLI→validateur, le secours sans tas, les créations partielles de pools, la politique v2 du code 134, la normalisation `:11`, la grammaire/binaire du plan, la portée et le budget |
| conception C6 `17b6dbea` / réponse `489972a5` corrigée | GO de conception borné, sans code ni GO G4 : premier jalon à deux IN + deux OUT hôte aux leases séparés, un flux et un jeu device ; fill hôte et rebuild séquentiel, validation lot-local transactionnelle versionnée, chronos de travail non additifs et modèle différé C6 séparé de `cuda_stub.hpp` |
| pool d'exécuteurs C1 `4a85c13d` | reçu comme brique hôte : confinement fatal côté worker, passage file→actif sous verrou et quatre dents sélectives ; aucun raccord produit/CUDA |
| témoin arithmétique série C `4a85c13d` | reçu comme harnais C++ hôte partiel avec trois dents et contre-fixture composée ; aucun `nvcc`, device, `BallKey::power`, `AxisBounds` ou division plancher C3 |
| protocole GCP série C | départ refusé fail-closed puis arrêté ; perte de superviseur arrêtée sans reçu ; deux relances distinctes reçues et arrêtées sur leurs générations exactes, la plus récente `2026-09-01T18:34:33.420-07:00` ; aucun GO courant |

Le reçu terminal courant contient 279 fichiers et couvre exactement les 278
autres par SHA-256 ; son inventaire distant ferme aussi ses 263 artefacts. Les
84 runs sont valides, terminés et liés aux mêmes source, payload et manifeste.
Le journal atteste `remote_campaign_rc=0`, `scp_rc=0`, `stop_rc=0`, l'arrêt
ciblé à la première tentative et l'état `TERMINATED` de la génération exacte.
La campagne demeure explicitement `verifie_non_decisionnel`. Le reçu série C
précédent `852ca703` conserve séparément ses 202/202 hashes et 58/58 runs ;
aucun résultat ni GO ne se transfère entre les deux sessions.

La lecture utile reste bornée. Sur cette topologie et `uniform`, 48 fils,
`inflight=2`, `join=0` est le réglage de débit ; `join=1` coûte environ 35 %
de mur mais économise environ 22 % de RSS à 50k et reste donc un mode mémoire.
Le reçu tests observe un rapport K10/K5 d'environ 3,1--5,3 selon famille et
taille ; ses écarts de pente restent empiriques sur quatre tailles, une graine
et deux passages. `materialisation_tri_copie` vaut 34,6--34,9 % du cumul
`reduce` à 32k sur deux familles : une cible de sonde, pas une part du mur ni
un goulot exclusif. La sonde destructive courante est donc une reconnaissance
à conserver ; un palier `CompactDelta` ne sera choisi qu'après une mesure
causale et une égalité complète de l'objet. Côté device, le gain de
l'étage vaut 1,37--1,89× mais le
gain bout en bout seulement 1,029--1,116× ; `wire+rebuild` occupe
83,7--87,8 % de l'étage. Le levier est la représentation hôte, pas le kernel
seul. Le plafond descriptif obtenu en supprimant entièrement cet étage reste
1,12--1,31× selon la fixture : le facteur global `1,7–2×` exige donc aussi
des progrès ailleurs dans le pipeline. Ces mesures n'altèrent aucun statut.

La contre-lecture détaillée des § 5.18--5.22 reçoit les frontières runtime,
le clamp, la provenance legacy, la porte de préfixe et le canon v2 de
`2aaa4a53`, puis les fermetures structurelles de `fa9b2633`. Au pin
`4ef96717`, D8 rend 67, D11 devient causal, D12 couvre une panne post-SCP et le
reçu minimal est borné. `c2d2ac69` ferme ensuite les quatre mutants plus
étroits : garde non fatale avant STOP, promotion liée à la tentative courante,
allowlist NUL et résumé reproduit différent fatal. Le revalidateur passe 22
scènes avec snapshot stable. Une fenêtre locale subsiste dans la reprise :
`GEN_EPOCH` est calculé par `python3` après connaissance de `GENERATION`, mais
avant l'armement du funnel. Sur la copie exacte, une panne 42 limitée à cet
appel rend 42 avec zéro STOP, registre encore `targeted_running` et aucun reçu.
Le mutant historique de `tee` est en revanche bien fermé : STOP unique,
registre `targeted_stopped`, reçu et témoin présents. Déplacer le calcul
`GEN_EPOCH` sous le trap, puis graver les issues 74/70 avec exactement une
tentative. Le reçu G4 déjà arrêté n'est pas affecté et aucun GO GCP n'est
ouvert.

Au pin `8afd1057`, la sonde résout ses outils critiques hors du `PATH`
ordinaire, les invoque par chemins absolus, les hache et les relit avant
publication. L'incident exact du faux `sha256sum` est fermé ; la porte finale
passe 23 scènes et ne laisse aucun parasite. La famille, l'argv, les CPU,
threads, seed, `s/smax`, inflight et join sont liés. La promesse « champ à
champ » reste néanmoins trop large : l'agrégateur accepte encore la disparition
de `liveness`, du layout, de l'inflight et des pics, des pics nuls, une
coordonnée absente, une clé de régime inconnue, `lscpu` absent de l'ensemble
d'outils et une identité suffixée. Fermer ce schéma avant une nouvelle mesure.
Le retrait de `--` dans l'appel unitaire à `sha256sum` doit être annulé ; c'est
le faux test qui doit reconnaître l'appel exact au manifeste et rester confiné
dans son temporaire. Cela ne remet pas en cause le reçu historique.

Le pin sémantique KeyCSR `8afd1057` est reçu. Il porte deux routes signées sans
repli, arènes possédées, vue reconstruite, comparateur tiers, rejeu et refus
transactionnels. `for_each_delta` est maintenant ref-qualifié avec rejet de
compilation sur rvalue. Le kind construit et les vrais derniers offsets sont
signés ; `csr_capacity_growths` est correctement déclaré unilatéral et les
octets classiques restent explicitement une borne inférieure. La matrice
pipeline et les sanitizers ciblés sont verts ; Claude rapporte 185/185 portes
et a rejoué la sonde finale sur son harnais exact. Code, architecture et plan
de tests sont alignés : les deux arènes nommées dans l'architecture sont bien
les vecteurs de `FacetKey` appendus dans `csr_emit`, et le plan de tests nomme
le mutant exact. La lecture concurrente qui étendait cette promesse aux
réserves méta/offsets hors `try` est rejetée ; préciser le site dans la phrase
serait seulement éditorial. Le helper `csr_payload_empty` et le nettoyage
pourraient encore inclure `r.deltas` comme dent P2 contre une future pollution
croisée ; aucun chemin nominal courant n'est démontré fautif.

La pré-inscription `53610911` reçoit l'unanimité inclusive à `0,55`, les deux
tailles décisionnelles et les six blocs AB/BA sans remplacement. Le profil
doit toutefois compter les métadonnées communes symétriquement, publier la
destruction complète hors de `delta_payload_build_total`, drainer le sink du
callback sans I/O sous verrou et décider depuis des nanosecondes entières. Les
strates digest off/on et leurs A/A sont séparées. `loadavg > 2,0` devient un
diagnostic, pas une invalidation : les deux reçus antérieurs comptent 35/36
puis 47/48 départs au-dessus du seuil, soit 82/84, et la valeur dépend du bras
précédent. La graine externe est
`0xa2ffb4db2884ddc4`. Un verdict de garde explicite ferme le cas où le ratio
payload passe mais le reduce ou les octets régressent. Seul un échec terminal
attribuable au CSR, avec entrée et classique valides, est un
`NO-GO_IMPLEMENTATION` ; signal, timeout, absence de statut terminal ou échec
classique/bilatéral non attribué restent inconclusifs. Les capacités
classiques sont cumulées symétriquement ou mesurées hors de tout mur
décisionnel. L'instrumentation peut avancer ; aucune campagne ne part avant
le rejeu sémantique sur son commit source exact, le scellement des deux
exécutables et la fermeture de la liaison résiduelle de la sonde.

Le générateur `d6888093` ferme correctement le calendrier déterministe de
cette pré-inscription : graine et PRNG rejoués, flux unique, cinq strates par
cellule, deux échauffements et six blocs équilibrés ; les plans sans et avec
extension comptent 210 et 280 runs. Sa porte directe passe normalement et
sous `python3 -O`. Il est reçu comme générateur d'ordonnancement seulement.
`8afd1057` inscrit sa variante normale dans CTest ; la variante `-O` reste à
enregistrer. Avant de qualifier la porte de canonique, le juge doit refuser
clés dupliquées/inconnues, vérifier l'orientation `AB` des échauffements et
l'adjacence brute des deux bras. Le plan ou son manifeste doit aussi nommer le
rôle décisionnel de chaque cellule et l'agrégateur doit l'exiger. Avant
campagne, un manifeste scellé doit encore lier le plan aux copies exactes, à
la chaîne de construction, aux commandes complètes, au callback résolu, aux
coordonnées et à l'affinité attestée. Aucun résultat de performance n'existe à
ce stade.

Le profil `g4_echelle_v1` reste **NO START**. `28d02459` reçoit le correctif
moteur : texte compatible, réservation sous garde, portée des callbacks et
4/4 portes ciblées. Le WIP protocolaire recoupe maintenant cause, étage et RSS,
mais il lui manque une porte CLI→runner→validateur et une politique versionnée :
le plan v2 doit refuser ou censurer un abort 134 si la nouvelle doctrine dit
qu'il n'est plus une donnée, sans casser le rejeu historique v1. Un vrai échec
de la réservation peut empêcher l'allocation du message de secours ; cause et
étage doivent donc survivre sans tas. Une création partielle des pools
`std::thread` peut encore dérouler des fils joignables et appeler
`std::terminate` : armer stop+join avant leur construction ferme mieux la
frontière qu'un simple `system_error` laissé hors capture.

Dans le reste du WIP, `smax` et les 17 entrées d'inventaire progressent, mais
le layout canonique n'est pas transporté par le lifecycle. Q2 est maintenant
correctement désarmée dans ce profil et reportée à une session distincte ; le
protocole générique doit encore refuser un pilote sans inventaire de portes. La
grille ne justifie pas quatre tailles par famille et par K. Après réduction à
cinq heures, l'estimateur WIP vaut 10 390 s, l'enveloppe de plafonds avec
overhead 10 890 s et la fenêtre calculée 13 195 s. Le commentaire donne la
bonne fenêtre mais appelle à tort 10 890 s l'estimateur. Normaliser
`fam:n:11` avant doublons et émission : les deux formes ensemble écrasent
aujourd'hui le même artefact. Fermer aussi les clés/tokens du plan et lier sa
commande au `BIN_MATRICE` canonique. Les gardes SPOT/STOP restent exigées,
mais aucune cible externe n'a été interrogée ou certifiée par ce préflight ;
aucun GO GCP n'est ouvert.

La question C6 de `17b6dbea` est reçue comme conception prometteuse. La
contre-lecture corrige l'audit initial : deux slots à lease unique sont trop
contraints, mais deux IN et deux OUT hôte aux leases séparés permettent déjà
`pack(k+1)`, device(k) et `rebuild(k−1)` avec un seul flux et un seul jeu
device. Deux flux ne deviennent un facteur qu'après mesure. Le fill device et
la reconstruction parallèle restent eux aussi des facteurs séparés ; les
durées de travail chevauchées ne ferment pas le mur par addition. Le modèle
différé C6 reste séparé du `cuda_stub.hpp` séquentiel, et la porte device doit
forcer plusieurs rotations. La frontière validation/reconstruction est à
versionner par lot avec une corruption tardive et publication nulle. C6 peut
modifier la résidence ; la mesurer avant de le classer comme gain de débit.

Le checkpoint mathématique reste reçu : coefficient 4 sur les deux covers q4,
contre-fixture causale, digest post-préfiltre séparé et différentiel historique
v5→v6 jugé sur les forêts et `digest_all`. La v5 ne fait pas autorité sur la
v6, et aucun résultat de campagne ne promeut `public_status`.

## Rejeux indépendants disponibles

Au pin `6d755804` :

- configuration et construction Release canoniques : succès ;
- nouvelles portes E3/G16 ciblées : 5/5 en 140,32 s (`e6_grille_objet`,
  oracle G8/G16 et trois mutants) ;
- reste de la suite hors label `scale`, sans répéter ces cinq portes : 69/69
  en 27,58 s ; total combiné 74/74 ;
- oracle nominal direct : zéro désaccord sur 4 799 488 cellules G8 et
  19 197 952 cellules G16, zéro violation du localisateur sur 324 171 points
  par résolution.

Le probe `fold-inject-b-exception-k3` termine encore par SIGABRT, code 134 ;
il reste volontairement hors CTest. Au pin `d98f4729`, le selftest campagne
passe 71 vérifications et le lifecycle 35 scénarios plus 11 refus de pin
(51 vérifications). Le rejeu indépendant des 83 tests de sûreté/intégration
donne 83/83 en 387,998 s sur la machine partagée. L'admission SCP à deux
arrêts, la seconde tentative sur les sorties précoces, la grâce fixe et le
refus d'une instrumentation G4 non canonique sont reçus dans leur portée. Le
rejeu v6 hors labels d'échelle donne 74/74 en 196,14 s ; aucune suite complète
d'échelle n'est revendiquée par ce contre-audit.

Au pin propre `6e293deb`, une reconstruction Release suivie de la suite v6
hors labels `scale8000|scale16000|scale32000` donne 78/78 en 265,96 s réels.
Elle inclut les trois portes caps et la porte CLI monoprocessus qui exige dans
la même exécution le code 0 et la signature
`memory_budget_scope=partial_named_payload_proxy_v1`. `git diff --check` est
propre, `tools/check_docs.py` valide 245 fichiers Markdown actifs et
`tools/check_implementation_status.py` valide 20 phases. Ce rejeu est local,
CPU et hors échelle ; il ne mesure pas le comportement G4 du nouveau code.

Le lot `4a85c13d` ferme la contre-fixture historique de `671ed3cc` : la
fermeture fatale part du worker avant notification, le passage file→actif est
indivisible sous `mu_`, et les quatre mutants possèdent chacun une scène
sélective. Une reconstruction Release indépendante depuis
`git archive 4a85c13d` passe 15/15 portes ciblées en 45,93 s : cinq portes
pool, cinq portes caps/CLI et cinq portes du témoin hôte. Le même export passe
ensuite 89/89 tests hors labels `scale8000|scale16000|scale32000` en 194,02 s.

Les campagnes CTest concurrentes lancées auparavant dans le build partagé ne
sont pas reprises comme reçu : elles ont écrasé leur `LastTest.log` mutuel et
leurs durées reflètent la contention. En particulier, 89 est le nombre de
tests **hors échelle**, jamais une suite « échelle comprise ». Aucun rejeu
indépendant des 15 tests d'échelle n'est revendiqué à ce pin.

Depuis un `git archive 1069bc20`, les cibles normales, profilées, vivacité et
contrat ont été construites en Release avec warnings fatals. Les trois portes
ciblées passent 3/3 en 9,32 s ; la suite locale du label `gate` passe 92/92 en
264,93 s réelles et 796,36 s cumulées par CTest. La substitution du binaire
profilé au normal rend bien 1. Deux enveloppes jetables rendent toutefois
encore 0 : neuf composantes et `somme` mises à zéro avec
`residuel=mur_reduce_interne`, puis une ligne `profil_*` ajoutée sur stderr.
Le pin est donc reçu comme harnais fonctionnel, pas encore comme porte fermée
d'attribution et jamais comme performance.

`git diff --check` est propre, `tools/check_docs.py` valide 247 Markdown actifs
et `tools/check_implementation_status.py` valide 20 phases. Aucun `nvcc` ni
device n'a été exercé. C1 reste une brique hôte non raccordée à `run.hpp` ; sa
file ne borne ni les captures des producteurs, ni les buffers d'exécuteurs,
ni la VRAM. Le témoin ne couvre pas encore `BallKey::power`, `AxisBounds` ou
la division plancher. La garde 2E reste un proxy logique de tailles, sans
promesse RSS/OOM. Les écarts de worktree postérieurs à `1069bc20` concernent
le nouveau WIP C2/wire ; ils sont exclus de ce reçu de profil et de tout GO de
performance. Cette phrase décrit l'état du worktree au moment du rejeu.

Le pin `cd606257` rejette désormais les lignes `profil_*` sur stderr et
exige `pf.somme() > 0` dans la porte compilée. La contre-fixture d'attribution
n'est toutefois que partiellement fermée : la porte Python croit encore le
champ imprimé `somme` au lieu de le recalculer depuis les neuf composantes. Les
trois portes de profil passent dans le lot local 24/24 décrit ci-dessous. Ce
durcissement est épinglé, mais ne remplace pas le reçu isolé de `1069bc20` et
ne rend pas encore l'attribution décisionnelle.

Au pin `cd606257`, un export Git indépendant construit toute la v6 en Release
avec warnings fatals et passe 113/113 portes `gate` en 232,11 s réelles
(1 168,42 s cumulées). Les 24 portes ciblées série C/profil avaient déjà passé
24/24 en 188,28 s dans le build partagé. Cette réception est strictement
CPU/stub : aucune compilation `nvcc`, exécution device ou mesure G4.

## Exact-K, omission unique et ownership WSPD

Le juge exige exactement les forêts K de 1 à `kmax_eff`. La fixture n=2
fournit K1 correct plus K10 surnuméraire et vérifie le refus de code 2.

`cd49a390` retire le doublon K1 de la fixture K11. Ses lignes K1–K10 et
`digest_all` correspondent désormais à la référence valide : K11 est
l'unique défaut syntaxique et le code 2 devient causal. Le chargeur ne lit
toutefois la référence qu'après le calcul complet du pipeline ; c'est un
rejet exact, pas un rejet précoce.

Le mutant `wspd-drop-rect` retire une sortie après la fusion ordonnée. La
porte vérifie le compteur test-only `mutant_dropped_rects == 1` et la
fermeture du ledger augmenté de cette masse. Elle ne constitue pas une mesure
indépendante du delta de cardinalité nominal-mutant ; ne pas la présenter
ainsi.

Pour `uniform` et `eight_clusters`, n=300, s=8, h infini et deux fils, la
porte d'ownership vérifie :

- la partition exacte de toutes les paires non ordonnées de positions
  uniques ;
- la séparation de chaque rectangle émis ;
- le degré littéral u−1 de chaque position ;
- masse émise égale masse attendue, masse tuée nulle ;
- les nombres de rectangles gravés pour les deux fixtures.

Les mutants `wspd-cap-terminal` et `wspd-split-heaviest` meurent
causalement. Cette preuve reste bornée : h fini, lanes mortes, descentes
singleton, déterminisme 1/2/8 fils et tailles supérieures à 300 ne sont pas
encore couverts par cet oracle.

## Contrat d'échec et callbacks

`cd49a390` rend la porte simple recevable : `phase_calls` est atomique, la
trace `on_forest` est protégée, chaque valeur inflight doit échouer, census
exige le préfixe vide et fold-A K2 exige exactement K1. Les digests, cartes et
totaux désignés comme payload sont invalidés après les retours à statut.

Cela ne teste pas encore la promesse concurrente. Census échoue avant tout
fold et fold-A K2 avant la création du slot K2 : un seul K peut être actif,
même avec `fold_inflight=2` ou 8. Aucun test ne synchronise plusieurs folds
ni ne contrôle `peak_fold_inflight`. Les mutants exigent seulement un statut
non complet et la forme du préfixe, sans statut, message ou phases causales
exacts ; une panne étrangère de même forme peut donc les rendre verts.

Les exceptions B et celles de `on_fold_phase`/`on_forest` sont toujours
relancées. Elles ne produisent ni `RunResult` terminal, ni code 3, ni
invalidation observable ; le probe B reste volontairement hors CTest car il
termine par signal. `FoldPhase::kPublished` demeure une livraison provisoire
sans événement global commit/abort. Enfin, l'API appelle `on_fold_phase`
depuis plusieurs fils sans documenter l'obligation de thread-safety, et
« invalidation totale » surqualifie toujours les diagnostics laissés
partiels.

Correction constructive : injecter un échec B tardif avec barrière
déterministe, prouver un pic 2 puis 8 et les suffixes calculés mais non
publiés, exiger statut/message/phases exacts, puis convertir les exceptions en
terminal observable. Documenter la concurrence et fournir `on_terminal` ou
un sink transactionnel commit/abort.

## Définitions du grand-livre à aligner

Les vecteurs d'octaves et leurs identités sont utiles, mais cinq libellés
restent trop larges :

1. `H_scan` compte des visites de points dans les ranges des handles, pas des
   nœuds d'index.
2. `P_factor` crédite nA(nA−1)+nB(nB−1), les diagonales étant sautées, et
   non les deux carrés complets.
3. `V_wspd` ne publie pas séparément le nombre d'appels témoins initiaux et
   terminaux annoncé comme population.
4. L'octave 15 est la classe saturée 15+, pas le logarithme exact au-delà de
   65 535 points.
5. `pentes.py` ferme les identités des vecteurs d'octaves, pas « chaque
   monnaie câblée ».

Aligner documentation et sortie avant d'utiliser ces monnaies comme autorité
de coût.

## Campagnes CPU

`campagne_decision_20260831` reste invalide comme matrice : 32 sorties
proviennent de l'ancien schéma et quatre du nouveau. Son
`STATUT_TERMINAL.txt` prime sur le `DONE` historique.

`campagne_sonde_octaves_20260831` est mécaniquement complète pour le binaire
archivé : 36 codes 0, `DONE`, 36 hashes avant/après homogènes au SHA
`4bbb257c...3359`, 36 hashes de sorties recoupés, 36 stderr vides et
`PENTES.txt` reproduit octet pour octet. Un build Release indépendant des
entrées produit de `cca9a2d5` reproduit aussi ce binaire.

Sa portée doit rester `exploratory_complete` :

- `META.txt` déclare le pin d'exécution `cca9a2d5`, tandis que
  `RECU.md` le remplace à tort par `cfaf6b41` ; les entrées produit sont
  équivalentes entre ces pins, mais les arbres Git ne le sont pas ;
- le script scratch exécuté n'est pas archivé ;
- le mode 555 et les hashes avant/après prouvent une stabilité d'octets, pas
  une immutabilité absolue ;
- « machine au repos » contredit les charges gravées ;
- passe 2 représente exactement 0,0425–0,3411 % des seeds lourdes, et non la
  plage arrondie annoncée ; surtout, `W2/W1` global atteint 19,8–48,7 %,
  donc la rareté des seeds ne prouve pas un coût aval marginal ;
- les fractions d'issues de seeds ne permettent pas d'attribuer le coût W1 à
  ces issues.

La capture lancée sous `a30c3a98` réemploie le même binaire, la même matrice
et les mêmes graines. Le reçu publié en `320299df` ferme exactement ses 36
tuples : 36 codes 0, matrice et hashes bijectifs, stderr vides, binaire privé
stable, et 36/36 `digest_all` ainsi que toutes les lignes déterministes
identiques à l'exploration. Le statut exact est donc
`replication_complete`, jamais confirmation. Les stdout complets ne sont pas
identiques à cause des temps et RSS ; seule la partie déterministe est
bit-reproductible.

`99bf6723` préenregistre utilement un profil hors échantillon
`locale_confirmation_v1` : tailles 10k/20k/40k et graines 6/7/8, disjointes
des tailles 8k/16k/32k et graines 3/4/5 qui ont servi à dériver l'hypothèse.
La campagne au pin `320299df` ferme ensuite ses 36 tuples : matrice exacte,
36 codes 0, `DONE`, hashes bijectifs, stderr vides. Les scripts et le profil
archivés égalent leurs blobs du pin ; un rebuild indépendant depuis
`git archive` reproduit octet pour octet le binaire privé
`f74a8759...67ae07e`. Le cœur brut de 81 fichiers audité juste après
publication porte le manifeste
`a05b9b72ea0150859a6c612fab98a4c7e266ce3a812a7430be6f19dc751f7b93`.

Sur le pas réel 20k→40k, les médianes
`W_sweep1/M_anchor[q4]/T_lourde` valent terrain 1,220/1,325/1,308 et
scanline 1,160/1,056/1,773. Aucun terme n'atteint 2 et aucune émergence
n'apparaît : **le déclencheur E6 préenregistré n'est pas confirmé**. Les
gardes bornées uniform et eight clusters ne déclenchent pas non plus.

Le statut reste `confirmation_candidate`, pas confirmation formelle. Les
autorités écrites fixent encore 8k/16k/32k, graines 3/4/5 et le pas
16k→32k, alors que l'exécutable a jugé 20k→40k. La porte complète n'est pas
verte : médiane W1 terrain 2,03 sur le premier pas, et médiane
`P_factor_q2` eight clusters 2,26 sur le second. Enfin, `PENTES.txt`, un
`__pycache__`, puis `AGREGAT.txt` ont été ajoutés après le terminal : le
défaut de snapshot est réalisé, même si l'agrégat ajouté reproduit celui de
la copie auditée (`400533d9...a0895`).

Le rapport de campagne dédié fixe les détails. Ne pas re-régler la règle sur
ces données. Pour fermer la candidate sans refaire les mesures, aligner les
autorités sur le profil effectivement préenregistré, lier les dérivés au
manifeste du cœur et les publier à côté d'un snapshot réellement gelé.

## Sonde E6

`7611418a` ajoute une mesure opt-in par worker, sans nouvelle race évidente.
Les identités publiées ferment :

- terrain : 942 631 seeds sondées, dont 566 924 à min ≥ h/2 (60,143 %), puis
  3 447 439 sans grille ; total 4 390 070 `seeds_core_tues` ;
- scanline : 608 273 sondées, dont 469 319 (77,156 %), puis 6 501 330 sans
  grille ; total 7 109 603.

Après normalisation des fils, les anciens compteurs et cardinalités coïncident
avec la capture précédente. Les deux runs E6 n'activent toutefois pas
`--digest` et le reçu ne porte ni statut, stderr, manifeste de sorties ni pin
hexadécimal. L'invariance d'objet n'est donc pas reçue.

Surtout, la lecture « convertible en kills de cellules » dépasse la sonde :

- `segment_min_count` mesure la boîte rectangulaire conservatrice des deux
  extrémités, pas seulement les cellules traversées par la corde ;
- le théorème 10.5 prouve la sûreté d'un kill obtenu, pas que G=16 ou un
  second niveau le produira ;
- min=0 ne borne pas l'échec de tout raffinement, et min=h−1 ne garantit pas
  le succès du raffinement proposé ;
- les comptes de cellule agrègent tous les sites, sans identité avec les
  témoins du cœur ;
- les ancres exclues par `near_m` n'ont précisément aucune grille : le veto
  n'est pas testé par ce run ;
- 31–54 % est une fraction de seeds lourdes tuées par cœur, pas une fraction
  de `W_sweep1`. La part W1 lourde observée varie plutôt de 19 à 67 % et
  n'est pas ventilée par issue ou raison.

Conserver donc ces 60–77 % comme population candidate à un contrefactuel.
Avant E6 : oracle de `segment_min_count`, contradiction zéro→mort après
subdivision, identité ON/OFF par digests, fils 1/6/8, au moins trois graines,
et ventilation octave × raison × issue de seeds, W1, ancres, covers et
cellules examinées. Le coût du probe lui-même doit être compté : sa boîte peut
parcourir jusqu'à 4 225 cases par kill cœur.

`261d412a` livre maintenant l'option. La revue statique ne trouve pas de
fausse mort intrinsèque à G16 : le certificat affine est générique en G et
le localisateur paraît conserver une marge suffisante. Cette appréciation ne
remplace pas une preuve : la borne publiée était chiffrée G8 et le template
n'est pas borné. Restreindre au moins ses instanciations à G8/G16 et écrire
explicitement le cas G16.

Les fichiers versionnés permettent de recouper cinq faits utiles : sur les
cinq paires ON/OFF, `digest_candidates_v5_compat`, le post-préfiltre, les dix
forêts et `digest_all` coïncident ; les baisses de W1 sont exactes ; les
pentes mono-graine valent scanline 2,407256→1,893616 et terrain
2,081451→1,763386. Ce signal reçoit le prototype comme **sonde W1 opt-in**,
jamais comme preuve universelle ou décision d'activation. La règle du
grand-livre interdit précisément une conclusion mono-graine et exige le coût
de chaque terme payé.

Le reçu `e6_grille_appariee_20260831` reste documentaire : son `pin` est un
placeholder, il n'a ni manifeste de hashes, ni commandes, codes de sortie,
stderr, terminal `DONE`, ni binaire archivé. Le hash annoncé correspond au
`build/v6/mhgp6` local encore présent, sans chaîne durable vers les sources.
Contrairement à la note et au META, aucun `digest_raw_candidates` ne figure
dans les dix sorties ; seule la petite porte n=2000, graine 3, deux fils le
compare. Le « 70/70 » n'a pas de journal versionné et n'a pas été rejoué
indépendamment pendant la campagne CPU.

La porte ON/OFF est donc utile mais insuffisante : `digest_balls` omis, une
seule graine et un seul réglage de fils, aucun mutant ni frontière ; son
plancher exige 100 constructions G16, sans kill additionnel ni baisse stricte
de W1. Une grille construite puis inutilisée peut rester verte.

L'attribution est ouverte. Le chemin couple G8→G16, seuil fixe 1024 qui
court-circuite `--cell-min-sites`, levée de `near_m`, levée du ratio,
remplacement sans repli G8 et suppression du scan de politique du cover. Sur
les cas stationnaires, 79–87 % des G16 remplacent une G8 déjà éligible ; les
G16 nouvellement éligibles ne sont que 1 596–5 067. Sur `eight_clusters`, au
contraire, 139 606 des 148 426 G16 sont nouvelles : les grilles q4 passent de
13 897 à 153 503 pour −12,179 % de W1. Le mur indicatif y monte de 5,472 % et
celui de terrain 16k de 26,272 %. La machine partagée interdit une conclusion
de temps, mais ces deux témoins suffisent à refuser l'assimilation
W1↓ ⇒ coût total↓.

À `261d412a`, le coût G16 n'était pas compté : ni masse des covers construits,
ni comparaisons, ni cellules consultées. La boîte de corde reste
rectangulaire et sa borne brute passe de 4 225 cases en G8 à 16 641 en G16
par seed.

`6d755804` apporte un progrès substantiel : le prototype est enfin nommé
E3/G16, cinq options de bras sont exposées, la porte compare désormais
raw/balls/post-préfiltre/forêts/all, et un oracle v6 exerce G8 et G16 avec
évaluation directe i128, localisateur rationnel, extrêmes u16 et trois
mutants. Les cinq CTests passent ; le nominal ne trouve aucun désaccord sur
4 799 488 cellules G8 et 19 197 952 cellules G16. Aucun faux kill évident
n'a été trouvé par revue statique. Cela reçoit une **porte de primitives et
de régression bornée**, pas encore une certification universelle G16.

L'indépendance de l'oracle est partielle : `cell_needed` vient du produit,
le juge rationnel réutilise `mul_128x128_192`, et les méthodes finales
`cell_dead`, `range_dead`, `point_dead` et `segment_dead` ne sont jamais
appelées. F9 et F11 n'imposent plus leurs verdicts fonctionnels ; `pairs`
compte des couples grille×cellule, pas site×cellule. Chaque mutant est jugé
sur l'agrégat G8+G16, de sorte qu'une seule résolution pourrait suffire à le
tuer. Enfin, aucune porte croisée ne prouve la monotonie de raffinement.

La partie discrète de cette monotonie est démontrable : chaque cellule G8 a
quatre filles G16, tout témoin affine du parent témoigne pour ses filles, et
toute fille nécessaire a un parent nécessaire ; donc compte fille ≥ compte
parent et `all_dead8 => all_dead16`. Les implications point/segment exigent
en plus l'inclusion, après projection par `floor(k/2)`, des boîtes du
localisateur G16 dans celles de G8. L'échantillon actuel ne remplace pas cette
preuve. Écrire aussi dans la v6 la borne d'arrondi G16 et restreindre
`CellGridT` par `static_assert` à G∈{8,16} ; l'oracle traite aujourd'hui tout
G différent de 8 comme 16.

Les cinq bras ne sont pas encore factoriels. `g16_politique` emploie G16 sur
toute ancre q4 admise, tandis que les bras partiels ne l'emploient que pour
cover≥1024. Les bras forcés court-circuitent aussi `cell_min_sites` et le
scan de politique. Après un échec forcé, le compteur crédite pourtant ce
scan comme sauté, puis le code refait la politique et retente la même grille
avant G8 ; les causes de refus actuelles sont indépendantes de G.

Les nouvelles monnaies ne sont pas fermées. `cells_consulted` n'est remis à
zéro ni par `build` ni sur toutes les sorties, n'est jamais récolté en q3 et
inclut la sonde diagnostique : il peut fuiter entre lanes/ancres ou être
perdu. L'instrumentation incrémente aussi sur le baseline `off`, dont elle
change le coût sans imprimer la ligne. Aucun compteur ne pondère la
construction par la taille du cover ou ses comparaisons, qui est précisément
le surcoût G16 attendu.

Enfin, quatre bras sur cinq n'ont aucun plancher d'activité. Le plancher du
seul `g16_leve` somme seulement les deltas positifs de seeds, sans delta net
ni kills d'ancres. Une graine, n=2000 et deux fils ne reçoivent ni causalité
des vetos/seuils, ni déterminisme des compteurs, ni coût à l'échelle.

Correction constructive : donner le même support lourd aux contrastes ou
ajouter leurs contrôles explicites ; compter tentatives, échecs, sites de
build et consultations par résolution avec une garde de récolte sur chaque
sortie ; isoler la sonde ; exiger activité et identité fermante par bras.
Compléter ensuite l'oracle indépendant, les mutants séparés G8/G16 et la
preuve de monotonie avant toute campagne appariée trois graines, fils 1/6/8.
Le résultat confirmatoire négatif ci-dessus interdit de présenter ce chantier
comme activé par la porte E6 ; il peut rester une expérimentation E3/G16
opt-in jugée sur son coût total.

## Réponse au bilan `65757693`

Le bilan reçoit correctement deux résultats bornés : les 74 portes CPU
observées sont vertes, et la règle confirmatoire exécutable rend
`E6_active=non` sur le second pas hors échantillon. Il dépasse en revanche
ces preuves sur trois points :

- les cinq options existent, mais leurs supports, vetos, replis, compteurs et
  planchers diffèrent encore ; elles ne forment pas un plan factoriel reçu ;
- l'oracle ferme une régression de primitives G8/G16, pas encore la preuve
  G16 ni toutes les méthodes finales ;
- deux termes de la porte complète restent au-dessus de 2, donc « moteur
  courant sous-quadratique en médiane » n'est pas un verdict disponible.

Les chemins état post-démarrage perdu avec handoff valide,
`targeted_stopped` d'une autre génération, état illisible et cible
discordante restent couverts à `d98f4729`. Le GO GCP borné avait été accordé
dans les limites ci-dessous ; il est maintenant consommé.

## GCP

`AUDIT_GCP_V6_P0_20260831.md` est l'autorité dédiée. À `d98f4729`, les trois
surfaces du dernier NO-GO sont fermées causalement : pire cas SCP identique au
budget post-campagne avec deux arrêts, deux appels totaux réellement
disponibles sur les sorties précoces, et `TIME_BIN=/usr/bin/time` transmis et
exigé intégralement pour G4. Trois revues indépendantes ne trouvent plus de
défaut matériel temporel ou de cycle de vie.

Le premier GO a été consommé par un refus fail-closed rc 76 avant campagne :
les portes réelles sont vertes (v5 288/288, v6 74/74), mais CTest 4.4.3 a
retiré `, 0 tests failed` de son résumé. L'arrêt de la génération exacte est
certifié `TERMINATED` après environ cinq minutes. `7e346926` accepte les deux
formats sans relâcher le code de sortie, les deux blocs ou les planchers.
`d98f4729` ajoute le `session.log` initialement ignoré et rend le reçu
auto-vérifiable ; aucune mesure scientifique n'est issue de cette tentative.

Le GO mono-session `94c74155` a ensuite été consommé par
`session_g4_20260901_d98f47296d67_1788245493`, exclusivement sur la source
`d98f47296d675d3cbdb1b53019dcc1a8b3b292b4`. Le profil canonique et effectif
est `g4_mesure_v1`, `remote_campaign_rc=0`, `scp_rc=0` et le validateur rend
0. Le reçu archivé par `df1a3c5f` porte 81 issues reconnues par le profil :
79 codes 0, le mutant GPU attendu au code 4 et la frontière uniform 800k au
code 134, typée `bad_alloc` sous `RLIMIT_AS=183500800` KiB. Ce sont 81 issues
valides, pas 81 succès.

Le reçu certifie `issue=arret_certifie_par_le_garde`, `rc=0`, `stop_rc=0` et
`etat_cycle_vie=targeted_stopped` pour la cible exacte
`devpod-gpu-exploration/europe-west4-ai1a/ehgp-blackwell-spot-ai1a`. Les
SHA-256 du reçu se revérifient sans divergence. Le statut demeure
`verifie_non_decisionnel` : les deux répétitions v5/v6 à 50k et les sweeps
CPU sont descriptifs, le contrôle GPU appartient à la lignée v5 historique,
et le refus 800k est un mur virtuel sous limite, pas un mur RAM natif. Aucun
résultat GPU v6 ni avantage de performance n'est reçu.

Ce GO est clos et aucune autorisation de lancement courante n'en découle. Un
futur lancement du code caps demanderait un nouveau pin et une nouvelle
autorité ; cela ne réécrit ni ne révoque le reçu historique `d98f4729`.

Limites non bloquantes : le validateur écrit ses `*_resume.txt` avant le contrôle
final G4. Avec un faux instrument de test, ces fichiers peuvent exister puis
être invalidés par rc 1. Ils sont alors diagnostiques et non recevables ; le
claim plus fort « jamais écrits » est refusé. Déplacer le contrôle en amont
serait un durcissement P1 utile, pas une condition de ce lancement officiel qui
impose `/usr/bin/time`. Le parseur CTest n'est pas ancré à la ligne entière ;
sur ce pin, le code non nul de CTest précède le parsing et aucune chaîne
parasite ne correspond, mais des marqueurs de suite seraient un P1 utile.
Enfin, le bootstrap exécute encore `pip install --upgrade cmake || true` : la
version effective de CMake/CTest n'est pas épinglée. Un nouveau changement de
format échouerait fermé, sans faux vert, mais pourrait consommer une autre
session courte. Fixer et vérifier la version avant tout re-pin ultérieur ; un
marqueur machine peut durcir le parseur, mais ne remplace pas ce pin de
toolchain. Ne pas appeler le présent protocole une toolchain reproductible.

## Fraîcheur documentaire et porte Python

Les verts de `tools/check_docs.py` ne détectent pas les décalages sémantiques
suivants :

- `docs/ARCHITECTURE.md` omet les cinq bras E3/G16 et nomme encore la cible
  inexistante `mhgp6_fused_descent_gate` ; les garanties transactionnelles y
  dépassent aussi le contrat d'exceptions/callbacks encore ouvert ;
- `docs/PROVENANCE.md` classe `linked_arcs_u16` comme prévu alors qu'il est
  livré, et `docs/PLAN_DE_TESTS.md` laisse prévus des portes déjà dans CMake
  tout en omettant la portée de l'oracle G8/G16 ;
- `docs/GRAND_LIVRE.md` conserve les définitions trop larges de `P_factor`,
  `H_scan` et des octaves déjà rectifiées dans le présent état ;
- `docs/MATHEMATIQUES.md` pourrait associer C1–C4 aux pins, oracles et
  portées bornées déjà disponibles ; C5–C7 restent réellement dérivés ou
  planifiés et ne doivent pas être promus artificiellement ;
- `tests/agregateur_gate.py` porte encore sa garde de domaine par
  `assert 0 <= k <= base`. Le nominal actuel passe aussi sous `python3 -O`,
  donc ce n'est pas un mutant causal, mais cette porte viole la règle du dépôt
  et doit employer un refus explicite.

Ces dettes n'ont pas bloqué la session G4 historique, qui a exécuté les blobs
et gardes épinglés ; son GO est consommé. Au protocole `e8289d9a`, aucun droit
de démarrage n'est ouvert : seul subsiste le GO de design conditionnel du
§ 5.12, à convertir en accusé du futur pin exact après ses selftests. Ces
dettes interdisent de présenter les documents cités comme une photographie
fraîche de toute la v6 avant leur alignement.

## Dette d'échelle et ordre utile

`6e293deb` ferme le cap brut borné demandé. Pour un cap `H` et `T` ouvriers,
la publication par blocs prouve
`H < emitted_at_refus <= H + 4096*T` ; le refus intervient avant fusion
globale et tri, sans quota atomique par candidat. Les vagues et rectangles
vivants sont gardés prospectivement avant insertion dans les vecteurs
globaux ; les shards locaux, explicitement bornés par la vague courante,
peuvent néanmoins être matérialisés. Les refus restent transactionnels avant
callbacks et folds.

`memory_budget_bytes` est volontairement un
`partial_named_payload_proxy_v1` calculé sur les cardinalités logiques des
payloads nommés. La capacité d'allocateur n'est qu'un diagnostic. Ce contrat
ne majore ni le RSS, ni toutes les structures résidentes, ni les allocations
locales ; il ne promet donc jamais un refus global avant OOM. La session G4
historique a précisément observé un `bad_alloc` à 800k sous `RLIMIT_AS`, mais
elle exécutait `d98f4729`, pas ce checkpoint. Restent à exercer les frontières
extrêmes de `CloudIndex`/Karras et à contrôler les additions u64 des monnaies.

La garde logique `2E*sizeof(BallCandidate)` n'est plus placée après la
génération. Depuis `4a85c13d`, `generate.hpp` la teste sur `exact_fusion` avant
`out->reserve`, libère les shards au refus et tue
`caps-skip-prefusion-budget` par une fenêtre dédiée. Cette fermeture reste un
proxy de tailles du payload nommé : elle ne majore ni les capacités
géométriques, ni le RSS, et supprimer réellement la coexistence des deux
copies demanderait toujours une fusion segmentée.

Les commentaires de `caps.hpp` et `generate.hpp` doivent suivre cette portée :
la fenêtre de test se calcule sur les cardinalités, pas sur `capacity()`, et
la projection 1,6–3,2 M / `ECHELLE.md § 3` n'a pas d'autorité v6. La porte
CLI juge désormais code et signature dans un seul processus ; son
`EXPECT_LINE` reste une recherche de sous-chaîne et les refus ne gravent pas
stdout vide. Ce sont des durcissements P2, tout comme l'isolation des trois
sites q2/q3/q4 et de la garde de vague initiale.

Ordre recommandé à Claude :

1. fermer la fenêtre `GEN_EPOCH` de `c2d2ac69`, puis rejouer le lifecycle
   complet sur un snapshot stable, entièrement en local ;
2. fermer le schéma exact du harnais (argv/META, `liveness`, layout,
   inflight/pics, coordonnée, `lscpu`, identité) et restaurer
   `sha256sum --` avec une contre-fixture hermétique avant sa prochaine mesure ;
3. **achevé à `8afd1057`** : recevoir le pin sémantique KeyCSR ; ajouter la
   dent P2 `r.deltas` au prochain petit lot, sans rouvrir l'architecture ;
4. fermer le trajet canonique du layout, la sous-classe d'allocation, les pools
   et la politique v2 du code 134 ; normaliser `:11`, fermer plan/binaire,
   corriger portée/budget et rejouer avant tout nouveau départ G4 ;
5. pour C6, livrer d'abord le pack à offsets fixes, le modèle différé séparé et
   deux paires IN/OUT aux leases séparés avec un flux ; n'ouvrir rebuild
   parallèle, second flux et fill device qu'en facteurs isolés après l'échelle ;
6. corriger `GRAND_LIVRE.md`, puis aligner `ARCHITECTURE.md`, `PROVENANCE.md`
   et `PLAN_DE_TESTS.md` au prochain checkpoint stable ;
7. enrichir seulement C1–C4 dans `MATHEMATIQUES.md` et remplacer l'`assert`
   Python opportunément : ce microcorrectif P2 ne bloque aucun résultat ;
8. geler le cœur de confirmation déjà acquis et lier ses dérivés sans
   re-régler la règle ;
9. réparer les supports, compteurs et planchers des bras E3/G16, puis fermer
   l'oracle et la preuve G16 avant toute nouvelle mesure ;
10. fermer callbacks/exceptions et leurs fixtures réellement concurrentes ;
11. ne rouvrir une mesure d'échelle ou G4 qu'avec une question décisionnelle,
   un nouveau pin exact et un budget couvrant les résidences revendiquées.

GCP non utilisé par le présent audit.
