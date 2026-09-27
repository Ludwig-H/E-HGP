# Mesurer les constructeurs A sur de vrais catalogues

27 septembre 2026. Harnais isolé, CPU uniquement, sans modification du moteur.
Le but est de comparer les deux constructeurs événementiel R1 et min-label
sur les **mêmes vrais manifestes A**, pas d'annoncer une accélération FULL.
R1 est close : qualification Release/sanitizers, puis quatre grandes mesures
avec comparaisons exactes et lecteurs LIVE normal/−O. Les
[résultats et périmètres](RESULTATS.md) donnent les volumes, temps, capacités
et la croissance uniforme8k/16k/32k. Moteur et prototypes antérieurs inchangés.

Cadre : `phase=exploration_v9_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u18_input_only`, `mode=real_catalogue_serial_A_comparison`,
`public_status=not_claimed`. GCP non utilisé.

## Filiation exacte

`probe.cpp` porte explicitement l'orchestration et la lecture u32LE de
`b_full_real_drafts_20260927/probe.cpp`, SHA256
`2c5c2265506b1649761ceb54d21e4541eab8ddbf4910f9227cc4e1987128bfdc`.
Il utilise les sources qualifiées de manifeste/capture, event R1 et min-label
sans les modifier, et inclut une seule fois `tower_chain.cpp` et le stub GPU.
Il n'y a ni substitution du Builder natif, ni seconde libchain, ni nouveau
code géométrique, ni fabrique de sceau de catalogue.

Le générateur natif produit le catalogue avec
`run_tower=false, keep_catalogue=true`. La **copie intégrale** de BallData
demandée par keep_catalogue reste payée dans le mur de cet appel ; sa durée
isolée n'est pas observable sans changer la chaîne, et n'est pas inventée.
Le helper natif `build_tower_index` reconstruit l'index depuis les mêmes
points possédés, immuables et dans le même ordre. PointId reste le rang
d'entrée ; le tri déterministe (Morton, PointId) redonne les mêmes rangs
géométriques que le catalogue. Tous les IDs/rangs et positions sont vérifiés.

Le Builder observé qualifié consomme cet index et ce catalogue **sans sceau**.
Il paie donc sa validation complète et construit encore toute la tour native
(A, B, C, banque, encodage). Tous les threads doivent avoir fini et les hooks
de tous les ordres être complets avant admission globale de la capture.
Ses Input/Output sont possédés, sans span ou banque empruntés survivants.
La tour native, le catalogue et l'index peuvent alors être libérés.

Un seul manifeste K est créé à la fois. Chaque candidat est comparé champ
à champ à la sortie A native capturée : draft, niveaux représentés, parents,
contributions datées, ancres, next/runs/birth_ball, racines par occurrence
avant déduplication et compteurs sémantiques. Les hashes sont des identités
supplémentaires, jamais les juges de correction. `replay_a`, qui suit les
histoires sans compression, est appelé **uniquement dans la petite gate**,
jamais sur les trames ou les mesures 8k/16k/32k.

## Temps et mémoire : périmètres non interchangeables

Chaque K exécute trois paires dans l'ordre M/E, E/M, M/E. M et E sont
séquentiels ; leurs sorties sont comparées puis détruites entre appels.
Le même manifeste et la même référence native restent vivants. Chaque appel
paie ses allocations, tris, validation et destruction interne des temporaires.
La destruction de son résultat est mesurée à part. Les phases internes et
le mur de l'appel sont publiés, ainsi que copies de capture, construction et
validation du manifeste, comparaison, destruction du manifeste et du slot.
Les compteurs Work publiés par candidat proviennent du premier appel ; ils
ne sont pas multipliés par les trois répétitions chronométrées.

Les temps A natifs sont **instrumentés** et peuvent se recouvrir entre K.
`capture_copy_sum` est une somme de temps de hooks, déjà incluse dans le mur
de la construction native ; la soustraire de ce mur serait incorrect. Ce
harnais ne permet pas de revendiquer un facteur natif→candidat FULL.
L'appariement interprétable concerne seulement event/min-label sur un même
manifeste. Une mesure favorable d'A ne qualifie pas B/C, le générateur ou G4.

`chain_wall` englobe keep_catalogue, digests et tout l'appel amont. Le champ
interne `chain_reported` exclut les digests comme le moteur : il n'est pas
substitué au mur. `external` couvre la préparation de la chaîne, reconstruction,
capture, admission, toutes les comparaisons/candidats K et libérations de
ces objets. `process` ajoute lecture/génération d'entrée et libération des
points. Tous deux excluent la sérialisation JSON et la destruction du petit
Report restant. La segmentation sans sol et la préparation 1 mm sont des
entrées figées **hors de ces mesures**.

Les captures recopient encore domaine et métadonnées de tout le catalogue
pour chaque K. Ces copies restent payées et leurs capacités publiées ; aucun
partage opportuniste n'est ajouté ici. Capacités index/catalogue/capture,
manifeste par K, temporaires+sortie de chaque candidat et pic RSS du processus
ont des périmètres différents : ne pas les additionner en un faux pic global.
Les maxima candidats excluent notamment l'intérieur des réallocations,
allocateur, pile et manifeste d'entrée. Les trois domaines P_event/P_draft/
P_forest sont publiés séparément, ainsi que V/E/G/C et les historiques.

## Préflight et qualification close

Préflight mutable : `build/v9-a-real-preflight-20260927`. GEN est recompilé
depuis ses 25 sources natives, plus une TU sonde ; aucune ancienne archive
opaque n'est réutilisée. Première configuration/build passés, handle 96259
joint code0. Première gate passed : six petits cas (carré, ABCZ, uniforme12,
ordre d'entrée direct/inversé), 26 ordres, 156 comparaisons candidates,
35 positions non identitaires, deux continuations et 16 blocs non réguliers.
Les tableaux FULL,
populations/verticales et trois digests sont comparés à une chaîne complète
témoin ; deux index reconstruits sont comparés champ à champ. Cette chaîne
FULL témoin n'est jamais exécutée en grande mesure.

Smoke uniforme64 seed3 passé : hash `14940886961791931765`, 2 282 boules,
V par K = 228/475/764/1061/1354 et E = 328/767/1275/1823/2348.
Ces préflights ne sont pas des reçus de qualification. La reconstruction
du seul probe après ajout de ce compteur a aussi passé (handle 78249 joint).

La qualification fraîche épingle les sources, configure les deux profils
avec `EXPORT_COMPILE_COMMANDS`, puis extrait les dépendances par `-M` des
26 **vraies commandes** de compilation de chaque profil : même compilateur,
ordre des options et définitions, includes, répertoire et fichiers de réponse
éventuels expansés/hachés. Seuls `-c` et l'argument `-o` sont retirés.
Les dépendances sont épinglées **avant** les builds Release/Clang ASan/UBSan/LSan ; elle ferme
aussi les objets, dépendances réellement compilées, archive GEN, flags et
binaires. Gate et smoke sont suivis de refus CLI précis. Les lecteurs LIVE
normal/−O ne recompilent ni ne relancent la géométrie.

R1 close après gel : 69 commandes, 489 dépendances pré-épinglées,
deux gates identiques, deux smokes et quatre refus CLI précis. Builds neufs
`build/v9-a-real-20260927-r1_release` et `_sanitize`, capture
[`receipts/r1/qualification`](receipts/r1/qualification/summary.json).
La reprise a rejugé les lecteurs LIVE normal/−O et les 35 corruptions du
lecteur dans chacun des deux modes, sans reconstruire ni rejouer la géométrie.
Ces builds, leurs objets et les sources du harnais sont maintenant figés.

Mesures exécutées séquentiellement après qualification : ng00 sans sol entier
(39 885 sites, hash `9245360528374966039`, masque figé, grille 1 mm), puis
uniforme8k/16k/32k seed3. K5/s8/W4, CPU local partagé. Aucune trame artificielle
n'est nommée LiDAR, aucun sous-échantillonnage, aucun nouveau plafond de
recherche. Les 120 comparaisons candidates de ces quatre mesures passent ;
leurs reçus normal/−O ferment les mêmes sources, objets, binaires et entrées.
Aucun GCP dans ce ticket.

Depuis le checkout dont les chemins absolus sont épinglés dans les reçus :

```sh
python3 -B morsehgp3D_v9/audits/b_full_a_real_20260927/run.py --readback morsehgp3D_v9/audits/b_full_a_real_20260927/receipts/r1/qualification
python3 -B -O morsehgp3D_v9/audits/b_full_a_real_20260927/run.py --readback morsehgp3D_v9/audits/b_full_a_real_20260927/receipts/r1/qualification
python3 -B morsehgp3D_v9/audits/b_full_a_real_20260927/selftest.py morsehgp3D_v9/audits/b_full_a_real_20260927/receipts/r1/qualification
python3 -B -O morsehgp3D_v9/audits/b_full_a_real_20260927/selftest.py morsehgp3D_v9/audits/b_full_a_real_20260927/receipts/r1/qualification
```

Pour chaque mesure, remplacer `qualification` après `--readback` par
`ng00`, `uniform_8000`, `uniform_16000` ou `uniform_32000`. Les lecteurs ne
relancent aucun binaire. Ils exigent les builds, dépendances système et
entrées locales épinglés ; les reçus seuls ne constituent pas une archive
d'exécution autonome. Une nouvelle mesure utilise toujours un dossier neuf.
