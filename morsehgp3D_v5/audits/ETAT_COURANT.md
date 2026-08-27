# État courant — audit de MorseHGP3D v5

- **Date :** 27 août 2026
- **Auditeur :** Codex, avec relecture critique des audits concurrents
- **Pin fonctionnel de référence relu statiquement :** `635951d654f466cfa7fe1e2297c19b9acb5393a9`
- **Tip d'implémentation relu statiquement :** `2b2bb4483103099ef539964bee15fe981acd2910`
- **Dernier audit concurrent critiqué :** `4c06d045ed1ee0584e3c036d1cf6a3ece5ad4942`
- **Reçu G4 relu :** `e11ad8c7b66617d91c1cdcd74909a4970b5362b0`
- **Worktree concurrent post-tip :** instrumentation `q4_stage_probe` et politique de prétests par requête avant cover en cours dans CMake et `src/`, hors verdict ; le probe de phase et le probe racine sont non suivis
- **Dernier pin avec sorties locales de sonde observées :** `635951d654f466cfa7fe1e2297c19b9acb5393a9`, reçu commité par `259fe21e`
- **Dernière sonde Q4 et sorties suivies :** `f8f5b4ffe803c0d40657b1946892487d5c498bd1`, provenance d'exécution non fermée
- **Dernier pin fonctionnel avec suite locale concurrente :** `635951d654f466cfa7fe1e2297c19b9acb5393a9`
- **Pin du protocole `EXTRA_N` relu statiquement :** `d837adb2a4cad65b4bce51640df8124539bedf56`
- **Pin d'activation initiale relu :** `a9a2f509428bbfebd9543579d16d1579a7591106`
- **Pin documentaire précédent :** `fa99b3f127e06aa686a301c084f8311e80d5c554`
- **Reçu G4 le plus récent :** [`campagne_g4_v5_20260827_tests_ancre`](../receipts/campagne_g4_v5_20260827_tests_ancre/RECU.txt), source `fa99b3f127e06aa686a301c084f8311e80d5c554`
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`
- **GCP :** non utilisé par l'auditeur ; après un premier échec fermé, la récupération gardée concurrente a copié 75 artefacts avec `scp_rc=0`, puis sa cible exacte `ehgp-blackwell-spot-ai1a` a été certifiée `TERMINATED`, sans aucune mutation de notre part

## Verdict

La v5 reste **orange, avec un progrès fonctionnel réel mais une autorité encore
incomplète**. Le pin fonctionnel `635951d6` intègre les fixtures non vacues
F1/F3, F5/F6, le jeton typé `AnchorPretests`, les comptes sectoriels détaillés
et une sonde plus honnête. Dans le build Release partagé, dont le contenu
fonctionnel était statiquement équivalent à ce pin, des sorties concurrentes
rapportent **166/166** CTests étiquetés `gate`, puis **8/8** tests étiquetés
`oracle` ; cette passe statique ne les a pas relancés. Les journaux CTest ont
depuis été écrasés : ces deux succès et leurs durées restent rapportés, pas
durablement reçus.
Sept de ces oracles appartiennent aussi à `gate` ;
`mhgp5_anchor_tests_oracle` reste absent de la commande canonique `-L gate`.
Les commits `259fe21e..e11ad8c7` ne changent ensuite que reçus et documentation ;
`ef5abbd5` corrige ensuite la racine de l'auto-copie G4 sans toucher au moteur,
`f8f5b4ff` ajoute la sonde Q4 et trois sorties exploratoires, puis `2b2bb448`
active la corde dans le produit CPU, le lot hôte et le kernel CUDA.
Ce dernier commit annonce 171/171 portes CPU, un stub CUDA vert, l'objet
inchangé et un ratio local 22,3 vers 18,8 s. Aucun journal, reçu de conformité
au pin, brut de mesure ou compilation NVCC de ce pin n'est suivi : ces résultats
restent des sorties rapportées, pas une autorité. Le worktree postérieur ajoute
une sonde de profilage encore non suivie, également hors verdict.
L'annonce de commit « 170/170 portes CPU » ne correspond donc pas à
l'enregistrement CTest courant et reste sans journal qualifié.

La suffisance mathématique du compte `W3` et du certificat sectoriel est reçue
statiquement sous le profil u16. F2 et F4 qualifient désormais séparément les
frontières sectorielle et `W3` à `h-1`, et `7eb33608` retire les doubles scans
Q3 hôte et Q4 oversized. Aucun faux rejet nominal n'a été identifié.

Cette réception ne qualifie pas encore l'activation comme autorité produit. Il
manque surtout :

- terminer la sûreté du lanceur G4 après la correction de racine : trap avant
  démarrage, cleanup inconditionnel, scripts de sécurité épinglés et marqueurs
  d'auto-copie authentifiés ;
- une comparaison indépendante filtre ON/OFF de tous les supports owner et de
  leur profondeur exacte, avant **et** après RLE, sur de petits cas Q3 et Q4 ;
- une fixture Q4 ciblée prouvant qu'une ancre sectoriellement morte possédait
  bien au moins un seed dans le bras OFF ;
- une garde `J <= 0`, et non seulement `J < 0`, ainsi qu'une identité `P/B`
  calculée sans dépassement sur tout le profil u16 ;
- une sémantique de routage Q4 indépendante du seuil ;
- une activation de la corde Q4 isolée des prétests d'ancre, avec oracle brut
  indépendant, parité device reçue et coût de bout en bout ON/OFF ;
- un reçu de mesure réellement épinglé au binaire exécuté.

Le dossier séparé `mesures_secteurs_635951d6_20260827/`, commité par
`259fe21e`, répare le faux pin principal : ses douze bruts affichent bien
`635951d6`, `worktree_src_modifie=non`, trois timers séparés et zéro
contradiction observée. Il reste une **mesure locale exploratoire**, pas un reçu
reproductible : pin et état sont capturés à la configuration, sans hash du
binaire/diff/entrées, commande, toolchain, RSS ni codes de sortie.

### Lanceur G4 — racine fermée, sûreté encore incomplète

`ef5abbd5` ferme le blocage immédiat : la première invocation calcule la racine
avant `exec` et la transmet à la copie. Cette correction est cohérente sur le
chemin nominal, mais le selftest n'exécute toujours pas l'orchestrateur local et
quatre garde-fous restent à fermer avant un nouveau lancement :

- installer le trap d'arrêt **avant** `start_and_verify.sh`. Aujourd'hui, une
  erreur de pipeline ou de lecture du handoff entre le démarrage et la ligne
  `trap cleanup EXIT` peut laisser la VM sans arrêt immédiat de l'orchestrateur ;
- rendre le cleanup indépendant du journal : son premier `echo | tee` reste
  soumis à `set -euo pipefail` et peut sortir avant `stop_and_verify.sh` si le
  log devient indisponible ;
- épingler et appeler par chemin absolu les scripts de durée, démarrage et
  arrêt. Ils ne figurent pas dans `PROTOCOL_PATHS` et restent lus depuis le
  worktree vivant, donc copier le seul orchestrateur ne ferme pas l'incident
  d'édition concurrente.

Refuser aussi une valeur ambiante arbitraire de `MHGP5_SESSION_SELF_COPY` : elle
désactive actuellement la copie et permet aussi d'injecter une racine arbitraire
sans contrôle. Vérifier avant toute mutation que la copie exécutée est l'octet
du script hashé par le manifeste, puis supprimer la copie temporaire. Aucun
lancement GCP n'a été effectué par l'auditeur pour établir ces défauts.

## Résultats exécutés

| Périmètre | Résultat local | Portée exacte |
|---|---:|---|
| Build Release partagé, contenu fonctionnel `635951d6` | **166/166 gates**, 8/8 oracles, 254,07 s et 13,76 s rapportées | sorties concurrentes sans journal durable ; l'oracle d'ancre est hors label `gate` ; aucun CUDA |
| Commit `635951d6` | **170/170 portes CPU**, registre 54, stub CUDA vert annoncés | auto-rapporté ; cardinal non reproduit par CTest, aucun journal CUDA reçu |
| Tip `7d94aee9` | 18,8 M identités et 0 désaccord annoncés dans le commit | auto-rapporté, sans journal reçu ni rejeu par cet audit |
| Release `7eb33608` | **165/165 gates**, 7 oracles, 105,73 s réelles | build canonique local ; aucun CUDA |
| Archive propre `fa99b3f1` | **165/165 gates**, 7 oracles, 101,58 s réelles | requalifie le code de `a9a2f509` indépendamment du worktree |
| Snapshot pré-commit devenu `7eb33608` | 10/10 fixtures et routes ciblées | F2/F4, non-strict, all-host, mixed et mutants ; subsumé par la suite complète |
| ASan+UBSan Debug historique `6e8a6aba` | 11/11 portes ciblées | aucune généralisation à toute la suite |
| ASan+UBSan RelWithDebInfo historique | échec de compilation dans `cloud_index.hpp:130-131` | warning GCC 13 `array-bounds` sous `-Werror`, pas un diagnostic sanitizer d'exécution |
| G4 source `fa99b3f1` | **25/25 runs acceptés** par le validateur exact du pin rejoué avec `0 0`, deux digests CPU/GPU égaux | contenu reproductiblement complet ; RC de récupération injectés au rejeu mais absents du reçu, donc transaction non autoportante |
| Sonde de corde Q4 suivie par `f8f5b4ff` | `wrong=0`, morts `K=2/4/8` agrégées monotones sur 3 familles à n=2000 | bruts sans pin, RC, temps ni RSS ; « complétions » = tentatives `y`, aucun terrain |
| Activation corde `2b2bb448` | 171/171 CPU, stub CUDA, 23,8 M vers 5,1 M et 22,3 vers 18,8 s annoncés par le commit | aucun journal ni brut au pin ; aucun reçu NVCC ; conformité invoquée mais non reçue au pin |
| G4 source `8f95df2e` | 4 couples CPU/GPU à 50 k, deux digests appariés | égalité bornée ; campagne partielle 24/25, non terminale |

La sortie concurrente 166/166 rapporte une non-régression bornée du contenu
fonctionnel, pas une reconstruction propre du pin ni l'exhaustivité du nouveau
certificat. `7d94aee9` compare désormais ON/OFF
sur cinq nuages et impose des compteurs sectoriels Q4 non nuls, mais seulement
après RLE et via les mêmes corps produit ; ce n'est pas encore le juge
indépendant de profondeur demandé. Le huitième oracle a été rapporté vert et le
cardinal statique des labels vaut bien huit, mais son mauvais label permet
toujours à la porte canonique de l'omettre.

### Mesures à 50 000 points

La session concurrente la plus récente a exécuté les 25 runs prévus sur le
bundle SHA-256 `efe87b9b...`, dont les fichiers produit sont identiques au pin
`fa99b3f1` (donc à l'activation `a9a2f509`). Son stdout donne :

| Famille | CPU | `--gpu` | Surcoût GPU brut |
|---|---:|---:|---:|
| `uniform` | 78 s | 81 s | +4 % |
| `terrain` | 21 s | 25 s | +19 % |
| `scanline_single_pass` | 23 s | 24 s | +4 % |
| `eight_clusters` | 162 s | 174 s | +7 % |

Les mesures et digests sont désormais suivis par `e11ad8c7`. La session initiale
a atteint la ligne finale du runner puis fini en `session_rc=2` avant de graver
`remote_campaign_rc`. Une mini-session gardée a localement copié 75 fichiers
avec `scp_rc=0`, puis certifié la cible exacte `TERMINATED`. Le commit ne suit
toutefois que les 25 `.status`, les 25 `.txt` et le résumé : les deux logs et
les 25 `.status.time` restent ignorés. Les statuts suivis ont bien 24 codes 0,
le code 4 attendu du mutant, `finished=1`, un triplet de pins commun et des RSS
positives ; les deux digests CPU/GPU concordent pour chaque famille et pour les
deux bras adaptatifs.

Le validateur exact extrait du pin `fa99b3f1` a été rejoué sur ce bundle avec
les arguments de récupération `0 0` et rend
`campaign_status=complete (25 runs valides)`. Le **contenu** de campagne est
donc reproductiblement complet ; le statut transactionnel n'est toutefois pas
autoportant dans le commit, car ces deux RC ont dû être injectés au rejeu.
`RECU.txt` annonce « voir ci-dessous » sans conserver commande, arguments,
`validator_rc` ni sortie ; le RC distant exact est inconnu, et le `scp_rc=0`
n'existe que dans un log local ignoré. Corriger aussi « 24 runs, tous code 0 »
en « 24 nominaux à code 0 plus le mutant attendu à code 4 ». Ne pas recalculer
les 25 runs : conserver le transcript du rejeu et qualifier explicitement le
RC distant d'inféré suffit à fermer honnêtement cette provenance.

`GPU.md` dispose donc maintenant de la pièce qu'il anticipait. Les écarts
CPU/`--gpu` observés vont cependant de +4 % à +19 % : « aucun gain net sur ce
run » est reçu, tandis que **parité** exige une tolérance ou des répétitions.
Ni l'attribution du rapprochement aux seuls tests d'ancre ni le diagnostic « le
résidu q3 est la construction des covers » ne résultent de cette campagne, qui
n'a pas d'ablation au même pin ni de temps cover/scan séparés.

Le reçu G4 précédent, source `8f95df2e`, donnait les durées suivantes :

| Famille | CPU | `--gpu` | Surcoût GPU |
|---|---:|---:|---:|
| `uniform` | 78 s | 89 s | +14 % |
| `terrain` | 23 s | 44 s | +91 % |
| `scanline_single_pass` | 38 s | 96 s | +153 % |
| `eight_clusters` | 246 s | 718 s | +192 % |

Ces chiffres historiques ne se transfèrent pas à `fa99b3f1`. La nouvelle
campagne ferme la décision V8 au niveau exploratoire : aucun kernel q3 par
rectangle n'a de gain de bout en bout démontré. Elle ne mesure ni variance ni
ablation causale, et ne justifie donc pas une généralisation de performance.

### Sonde de corde Q4 `f8f5b4ff` — signal reçu, mesure non recevable

Le critère de `bench/q4_chord_probe.cpp` est mathématiquement prometteur. Les
centres des complétions acceptables sont **contenus dans** la corde fermée
`|mu| <= sqrt(J/2)` ; ils ne « forment » pas tout ce continuum. Sur chaque
sous-intervalle d'une sur-corde, compter `h4` sites strictement intérieurs aux
deux extrémités est une condition suffisante de mort du seed ; les témoins
peuvent changer d'un morceau à l'autre. Les facteurs `J/2` et `P-mu*B` ainsi
que la stricte comparaison `< 0` sont corrects. L'entier courant
`isqrt(J/2)+1` sur-approxime toujours la corde et reste donc fail-open, mais il
dépasse inutilement d'une unité les carrés parfaits et peut être très pessimiste
pour les petits `J`. Employer le plus petit entier `r` tel que `2*r*r >= J`, ou
publier le mou de sur-couverture. Le code est maintenant suivi, mais reste une
sonde et ne doit pas encore devenir une lane ni une preuve :

- CMake ne lui transmet ni pin ni état dirty, et la sortie ne conserve ni
  commande, toolchain, temps, RSS, hash du binaire ou code de sortie. Les bruts
  ont été générés **avant** le commit qui ajoute la sonde, puis placés dans
  `mesures_secteurs_635951d6_20260827/` avec la mention « même pin », alors que
  `q4_chord_probe.cpp` n'existe pas à `635951d6` ; les octets peuvent
  correspondre au futur `f8f5b4ff`, mais rien dans le reçu ne le prouve ;
- `wrong` est une alarme géométrique relative utile : toute `BallKey` produite
  que le seed déclaré mort peut engendrer contredit bien « toutes ses sphères
  admissibles sont profondes ». Elle n'est toutefois ni indépendante du corps
  produit ni une preuve d'invariance de l'objet ; une omission produit peut
  masquer un défaut, et une cosphère émise par un autre carrier peut produire
  un signal conservateur. Restreindre l'alarme additionnelle à `!dead1`, la
  ventiler par `K`, puis ajouter une comparaison ON/OFF brute et post-RLE si le
  certificat entre dans le produit ;
- le `std::set<BallKey>` masque les multiplicités et `wrong` peut compter le
  même seed pour `K=2/4/8`. Imposer par seed `dead2 => dead4 => dead8`, faire de
  toute violation un code non nul et ajouter une petite fixture indépendante
  de bord avant d'en faire une gate ;
- « complétions évitées » compte actuellement tous les `y` de la lentille,
  avant owner, canonicalisation et préfiltres. C'est un nombre d'itérations de
  boucle évitées, pas un nombre de scans de profondeur ni un temps économisé.
  Ventiler les rejets produit et mesurer aussi le coût propre des `K` scans du
  cover ;
- aucune non-vacuité n'est bloquante : zéro ancre, seed, mort, complétion ou
  émission rendrait encore le code 0. Ajouter des planchers avant toute lecture
  de ratio, refuser les CLI permissives (`--n=abc` rend actuellement un run
  vide à code 0), vérifier le cardinal réellement généré et refuser `J <= 0`,
  conformément au théorème du seed aigu ;
- la campagne locale a écrit directement dans les `.txt` et enchaîné les probes
  sans bloquer le commit sur chaque RC : des fichiers à zéro octet ont été
  observés pendant les calculs, puis remplacés par des bruts complets, mais les
  RC affichés au terminal ne sont pas suivis. Écrire vers un temporaire,
  vérifier le RC, renommer atomiquement puis construire le résumé depuis les
  bruts ;
- le cas `eight_clusters n=4000` a été abandonné après plus de dix minutes et
  son brut vide supprimé. Cette observation de coût n'est pas reçue non plus,
  mais confirme que le coût propre du certificat doit être isolé avant toute
  décision de lane.

Les trois bruts suivis à n=2000 donnent :

| Famille | Seeds vivants tués K=4 | Tentatives `y` comptées évitées K=4 | `wrong` |
|---|---:|---:|---:|
| `eight_clusters` | 61,7 % | 82,5 % | 0 |
| `uniform` | 57,9 % | 74,8 % | 0 |
| `scanline_single_pass` | 34,9 % | 60,7 % | 0 |

La non-vacuité est forte et le signal justifie de poursuivre le probe. Il ne
prouve aucun gain : le coût des `K` scans n'est pas séparé, le compteur précède
owner/canonicalisation/préfiltres, et terrain n'a pas été exécuté. La bonne
prochaine forme est un reçu distinct au pin de la sonde, comparaison exacte par
complétion, `wrong_K=0`, planchers, temps du certificat et scans réellement
évités.

Une passe concurrente rapporte une compilation Release ciblée et cinq runs
bornés (`n=200/400`, quatre familles) avec `wrong=0` et des morts agrégées
strictement monotones `K=2/4/8`. Ils falsifient utilement le brouillon, sans
constituer un reçu ni une mesure de performance.

### Intégration produit de la corde `2b2bb448` — bonne direction, activation non qualifiée

La transcription en cours conserve deux choix d'architecture utiles. Elle
réutilise le scan du cover déjà payé par le cœur au lieu d'ajouter quatre scans
complets, et le test strict possède un filtre flottant avec repli i128. Sous le
profil u16, les largeurs annoncées pour `J`, `mu_hat`, `B` et
`L-c*mu_hat*B` tiennent dans i128. La logique de préfixe du kernel CUDA paraît
aussi reproduire statiquement l'ordre scalaire : première lane où le cœur ou
les quatre morceaux atteignent `h4`, priorité au cœur en cas d'égalité, puis
masquage des compteurs après cette lane.

Cela ne suffit pas pour qualifier l'activation déjà commise par défaut :

- dans `q4_kernels.cuh`, chaque lane construit d'abord un `ChordPieces` inutilisé,
  puis chaque site reconstruit un objet local et recalcule
  `isqrt128_floor(J/2)`. Le CPU calcule cette racine une fois par seed. Sur GPU,
  ce `sqrt` suivi de corrections i128 par site, les quatre ballots et la pression
  de registres peuvent coûter davantage que les complétions retirées. Calculer
  `mu_hat` une seule fois par seed — idéalement lors de la construction du lot —
  puis exposer un helper pur qui rend le masque des morceaux d'un site ;
- `generate.hpp`, `q4_core_shaped.hpp` et le kernel ignorent la corde dès que
  `L > 0` est certifié. C'est fail-open, donc sans faux rejet, mais le commentaire
  « jamais témoin d'aucun morceau » est faux : avec `L=4`, `mu_hat=2` et `B=2`,
  les deux extrémités du morceau positif extrême donnent des valeurs strictement
  négatives. La sonde `f8f5b4ff` testait tous les `P` ; ses ratios 61,7 % / 82,5 %
  ne se transfèrent donc pas au prédicat produit. Retirer ce raccourci ou refaire
  la mesure avec exactement le prédicat intégré ;
- `seeds_killed_chord` ne signifie pas « vivant après le cœur complet ». Le scan
  s'arrête quand la corde gagne la course avant le `h4`-ième témoin du cœur ; un
  seed ainsi compté pourrait encore être tué par le cœur plus loin dans le
  cover. Renommer ce compteur en cause du **premier arrêt** et réserver un
  diagnostic hors produit aux morts additionnelles après cœur complet ;
- `AnchorPretests::kCounterfactual`, documenté comme jeton des seuls tests
  d'ancre, désactive maintenant aussi la corde. Cela change silencieusement le
  sens de `rect_probe` et fait de `anchor_tests_oracle` une comparaison combinée
  ancre+corde. Ajouter un réglage de corde séparé, explicite et par défaut
  désactivé tant que la porte d'intégration n'est pas fermée ;
- F8 tue utilement le mutant sur une frontière d'une primitive à un site, mais
  ne construit aucun seed réel vivant après K1 et mort par K4. L'oracle ON/OFF
  rappelle les mêmes corps produit puis applique RLE : une émission manquante
  ou une multiplicité peut être masquée. Il faut une petite fixture indépendante
  qui énumère les complétions et leurs profondeurs avant RLE, puis une porte
  scalaire/lot/device où la mort par corde est non vide sur la route device ;
- le mutant `chord-nonstrict` n'applique `<= 0` que lors du repli exact. Si une
  égalité est certifiée positive, par exemple avec `E=0`, elle n'est pas comptée
  par le mutant. Ce défaut P2 ne touche pas le nominal strict, mais F8 force le
  repli avec `E=1` et ne qualifie donc pas toute la sémantique annoncée ;
- les commentaires et impressions des gates Q4 annoncent encore 22 compteurs et
  16 planchers, sans imprimer le nouveau compteur. Les CTests device restent
  conditionnels CUDA, étiquetés `gpu`, et aucun mutant de routage Q4 device
  n'est enregistré.

Le texte de preuve commité doit aussi être resserré sans changer l'algorithme.
Les centres des complétions admissibles sont **contenus dans** la corde, ils ne
forment pas tout son continuum. Pour une forme affine, c'est son **maximum** qui
doit être strictement négatif sur le morceau ; vérifier les deux extrémités le
prouve, tandis que « minimum aux extrémités » n'est pas l'argument requis. Enfin,
les bornes écrites dans `chord_kill.hpp`, prises littéralement, donnent une somme
potentiellement inférieure à `2^111`, pas à `2^110` ; les bornes u16 plus serrées
gardent largement i128 sûr, mais elles doivent être celles de la preuve publiée.

Le claim « objet inchangé » du commit est prématuré. Les conformités v4 suivies
sont antérieures à `2b2bb448`; l'oracle ON/OFF combine prétests d'ancre et corde,
réutilise le corps produit puis compare après RLE; F8 n'exerce qu'une primitive
à un site. En outre, aucun reçu CUDA ne couvre la nouvelle primitive HD : les
risques de compilation `std::sqrt`/lambda device et son coût ne sont pas encore
fermés. `README.md`, dont le dernier changement est `db757af4`, ne décrit pas
l'activation ; avec `ETAT_COURANT.md` en cours et ce README non frais, le cadre
du dépôt interdit de transformer ces annonces en claim.

La sonde `q4_stage_probe` du worktree va dans la bonne direction pour isoler les
phases, mais ne ferme pas encore l'A/B : elle n'a ni pin/dirty, mode corde OFF,
commande, RC ou RSS, et appelle l'horloge autour de chaque seed et seed survivant.
Des millions d'appels peuvent déformer les temps absolus ; `prof_q4_cover_ns`
est déclaré mais jamais alimenté, et l'énumération lentille/acuité/J reste dans
un résidu non nommé. Chronométrer des blocs plus grossiers ou calibrer ce coût,
nommer le résidu et comparer ON/OFF dans le même exécutable reçu.

Le même worktree ouvre un second levier : pour les rectangles dont les handles
portent au moins 512 points, une requête d'arbre de coefficient 1 applique Wq et
les secteurs avant de construire le cover complet. Aucun faux rejet nominal
n'est visible statiquement : tous les témoins possibles vérifient
`|2z-(a+b)|^2 < D2`, et `cover_query(..., 1)` les contient. La révision
courante évite désormais le tri et transmet correctement
`radially_sorted=false`, de sorte que les sorties de classe deviennent des
`continue` et non des `break`. Mais l'activation par défaut est, elle aussi, en
avance sur son autorité :

- la policy est dupliquée dans `GenerateOptions` et `BatchLimits`; changer l'une
  peut rendre les chemins scalaire et lot/device causalement différents, et
  `RunOptions` ne la transporte pas ;
- aucune porte n'apparie encore `pretest_query_min_points=0` et `SIZE_MAX` sur
  candidats bruts, RLE, compteurs et routes ;
- `cover_query` traverse l'arbre et matérialise encore toute la boule par ancre.
  Un seuil fondé seulement sur le nombre de points des handles ignore le nombre
  d'ancres, la sélectivité et la taille de sortie ; il peut déplacer le coût
  plutôt que le supprimer ;
- les morts précoces par requête se produisent hors du chronométrage
  `prof_q4_anchor_ns` et, côté lot, avant les compteurs de route hôte/device :
  ces dénominateurs ne sont pas comparables entre policies sans ventilation.

Conserver ce chemin expérimental jusqu'à la porte appariée. Pour le rendre
réellement léger, préférer une traversée spécialisée qui accumule directement
les huit comptes et s'arrête dès que le verdict est acquis, sans matérialiser
tout le cover de coefficient 1 ; choisir ensuite le seuil sur une courbe
par famille, pas sur un seul ratio local.

Chemin court conseillé à Claude : séparer d'abord le mode corde des prétests
d'ancre et revenir à un défaut expérimental OFF ; factoriser `mu_hat`/le masque par site ;
fermer ensuite une fixture réelle K1-vivant/K4-mort et la parité de route ; enfin
recevoir au même pin les murs CPU/GPU ON/OFF, les digests, les compteurs de
premier arrêt et le nombre de scans de profondeur réellement supprimés. Les
mesures G4 à 50 k sont toutes antérieures à cette intégration et restent sa
baseline, pas son résultat.

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

`635951d6` ajoute `x=(1000,1200,0)` à F1 et F3. La géométrie est statiquement
cohérente : F1 possède un seed aigu de profondeur 9, F3 un seed aigu et neuf
témoins `W3`. Cela ferme la vacuité géométrique des prédicats, mais les
assertions n'appellent pas directement le bras OFF et ne vérifient ni nombre de
seeds ni profondeur par une autorité indépendante.

F5 ne grave pas encore l'exemple 2.4 publié. Le commentaire, le commit et
`PLAN_DE_TESTS.md` annoncent 28 sites, mais le tableau construit six
quadruplets et une paire, soit **26** sites ; les deux directions
`(y,z)=(0,+-1000)` manquent. Avec seulement treize paires antipodales, une
direction orthogonale à une paire peut n'avoir que douze intérieurs : la
propriété publiée « tout centre non nul a profondeur au moins 13 » ne suit
plus. Le seed choisi `x` conserve bien treize intérieurs, donc la fixture porte
un contre-exemple plus étroit. Ajouter les deux axes manquants et asserter
explicitement seed aigu, seed OFF et profondeur plutôt que d'abaisser la
documentation canonique à 26.

F6 isole correctement une égalité de demi-plan strictement à l'intérieur de la
boule diamétrale. Elle n'asserte toutefois que les secteurs 6 et 7, alors que
le message de commit revendique aussi 4 et 5 ; verrouiller tous les comptes
annoncés si cette portée est conservée.

F7 établit seulement le premier de trois niveaux de non-vacuité : le prédicat
sectoriel Q4 s'active. F1 contient bien un seed aigu, donc un travail évité est
plausible, mais F7 ne lance pas le bras OFF et ne l'asserte pas. Tous ses points
sont coplanaires : aucune complétion ou candidate Q4 n'existe. Ajouter un cas
non coplanaire et exiger, pour la même ancre tuée, `seeds OFF > 0`,
`q4_completions OFF > 0` et l'égalité d'objet ON/OFF. Ajouter aussi les cas de
borne équilatérale Q3, tétraèdre régulier Q4 et extrêmes u16.

Une fermeture minimale a été vérifiée hors dépôt avec l'ancre
`(0,0,0)`–`(2000,0,0)`, les supports `x=(1000,1000,1000)` et
`y=(1000,1000,-1000)`, puis les paires `(1000+e,+-900,0)`. Pour `e=0..6`,
`W4=0`, `wmin=7`, ON=OFF conserve un candidat et le bras OFF compte 2 seeds et
30 complétions. Pour `e=0..7`, `wmin=8` tue nominalement l'ancre ; le bras OFF
compte 2 seeds, 34 complétions, une mort en profondeur et zéro candidat. Graver
les deux cas qualifie séparément la frontière `h4-1` et la non-vacuité produit.

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

Le booléen de `7eb33608` est remplacé au pin `635951d6` par le jeton typé
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

`259fe21e` sépare enfin la variante finale dans son propre dossier. Les douze
bruts sont complets ; noms, familles, tailles, lanes et pin concordent, les
identités de compteurs se ferment, K8 domine K4 sur cet échantillon et aucune
contradiction n'est rapportée. La sonde bloque désormais sur `wrong` et sépare
les trois timers. C'est une bonne mesure exploratoire de prévalence.

Elle ne démontre cependant ni un gain temporel produit ni une provenance de
reçu complet :

- le pin et le bit dirty sont capturés à la **configuration** CMake ; une
  édition après configuration peut laisser `worktree_src_modifie=non`, et un
  échec de `git status` est aussi interprété comme propre ;
- commande exacte, toolchain, configuration de build, hash binaire/entrées,
  code de sortie, RSS et manifeste ne sont pas conservés ; la cible est en
  outre compilée avec `MHGP5_TESTING=1` ;
- la génération a été observée avec des `.txt` temporairement à zéro octet :
  écrire vers un temporaire puis renommer éviterait qu'un collecteur lise un
  reçu partiel ;
- `wrong` peut compter jusqu'à trois fois la même ancre (production, K4, K8) :
  le code de sortie est désormais bon, mais le cardinal imprimé n'est pas un
  nombre d'ancres distinctes ;
- en Q4, W4 est préfiltré avant le contrefactuel et hors des trois timers. La
  ligne « production Wq exact + secteurs cumulés » mesure donc les secteurs
  **après W4**, pas le coût total des prétests Q4 ; aucun bras ON de bout en
  bout n'est chronométré ;
- les ratios « candidat de rectangle / somme des covers » du `LISEZMOI` sont
  ceux de Q3 seulement sans le préciser. Les ratios Q4 sont très différents ;
- l'ancien dossier a retrouvé les fichiers communs à `a9a2f509`, mais conserve
  trois sorties ajoutées après ce pin. Dire « contenu historique exact de
  a9a2f509 » reste donc littéralement faux.

Les bruts `eight_clusters` Q3 annoncent pour le cumul 64,5 % / 95,0 % à n=2000
et 67,8 % / 97,1 % à n=4000. Ce sont des fractions de travail contrefactuel,
pas un accéléré mesuré. `GPU.md` doit distinguer secteurs seuls, cumul et lane,
et les temps locaux restent des `mesure_locale_non_recue`.

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
- **Documentation :** `fabd75bd` décrit F5 comme 28 sites alors que la fixture
  en construit 26, présente F7 sans distinguer prédicat/seed/complétion et range
  `mhgp5_anchor_tests_oracle` parmi les oracles bien qu'il réutilise les corps
  produit. Le reçu `e11ad8c7` ferme le rapatriement anticipé par `77e143b2`,
  mais `GPU.md` attribue encore le résidu q3 aux covers sans mesure cover/scan
  séparée. `MATHEMATIQUES.md` décrit encore F1/F3 sans leur seed ajouté.
  Corriger ces portées, le schéma des compteurs et le pin différentiel v4 avant
  d'augmenter leur autorité.

## Réponse à Claude et ordre de fermeture

La réponse détaillée V7–V14 est conservée dans
[`QUESTION_CLAUDE_TESTS_D_ANCRE_20260827.md`](QUESTION_CLAUDE_TESTS_D_ANCRE_20260827.md).
Ordre conseillé :

1. Compléter d'abord le lanceur G4 après la correction de racine : scripts de
   sécurité épinglés, trap posé avant démarrage, cleanup qui tente toujours
   l'arrêt, environnement contrôlé et selftest de l'orchestrateur local. Aucun
   nouveau démarrage avant cette fermeture.
2. Rendre le reçu G4 autoportant : logs et `.status.time`, transcript/RC du
   validateur, `scp_rc` durable et `remote_campaign_rc` explicitement inféré ;
   ne pas recalculer les 25 runs. La décision V8 garde le kernel q3 par rectangle
   fermé faute de gain net. Toute conclusion causale exige une ablation au même
   pin et des temps cover/scan séparés.
3. Recevoir la sonde de corde Q4 avant toute intégration : provenance complète,
   non-vacuité bloquante, verdict exact par support et par `K`, monotonie et
   temps du certificat comparé aux scans réellement évités.
4. Transformer la gate ON/OFF de `7d94aee9` en porte d'autorité : comparaison
   brute et post-RLE, profondeur indépendante, `J <= 0`, produit large `P/B`,
   F1/F3 avec bras OFF et fixture Q4 avec complétion OFF.
5. Compléter F5 à 28 sites, qualifier les trois niveaux de F7, uniformiser le
   ledger Q4 et fermer les bornes `size_t/u32` ainsi que les replis oversized.
6. Compléter la provenance du reçu sectoriel, corriger ses libellés et les
   documents, puis fermer le validateur Q4, le mutant CUDA, l'autorité des
   overrides et le protocole `EXTRA_N`.

Le test cellulaire reste une dérivation expérimentale, pas une lane ouverte.
Une grille de cellules et un fan sectoriel à plus de huit secteurs sont deux
certificats différents ; ne pas présenter l'un comme le raffinement automatique
de l'autre. Le facteur 2,33 du mou histogramme est structurellement plausible,
mais demeure une observation de famille/taille/pin, jamais un invariant.

## Reproduction et limites

```text
build partagé, contenu fonctionnel 635951d6 : 166/166 gates, 254,07 s ; 8/8 oracles, 13,76 s — sorties rapportées, journaux écrasés
selftest concurrent : violations=0, mais le wrapper de session n'est pas exécuté
G4 fa99b3f1 : validateur exact rejoué avec 0 0, 25 runs valides, deux digests appariés ; RC injectés/transcript absents du reçu
sorties historiques 7eb33608 : 165/165 gates, 7 oracles, 105,73 s
archive propre fa99b3f1 : 165/165 gates, 7 oracles, 101,58 s
reçus/docs 259fe21e..e11ad8c7 et lanceur ef5abbd5 : relus statiquement
```

Rejeu réellement exécuté depuis la racine :

```text
python <(git show fa99b3f1:gcp-migration/validate_v5_campaign.py) morsehgp3D_v5/receipts/campagne_g4_v5_20260827_tests_ancre/out fa99b3f127e06aa686a301c084f8311e80d5c554 efe87b9b7e3a65e218997c9bce43a2ec95f2fa38e0f4efcffd5cf2f9cdb149e1 d5d6c71efc44365c2bab311016315966ee7674d48923e1cf2bc2a9923806fdc8 0 0
campaign_status=complete (25 runs valides, source_commit=fa99b3f127e0)
=== CAMPAGNE COMPLETE ===
```

Cet audit n'a lancé ni CTest ni probe. Une passe concurrente rapporte la
compilation de la cible Q4 ensuite suivie par `f8f5b4ff` et cinq probes bornés ; d'autres sorties du
build partagé rapportent les portes CPU, les oracles et le selftest. Aucun
journal CTest durable ni CUDA local n'est disponible. Les sorties G4
`.txt/.status` sont suivies, contrairement aux logs de session et fichiers
`.status.time`. Le probe racine
`.codex_fold_contract_probe.cpp` appartient à un autre auditeur ; il n'a été ni
modifié ni inclus. GCP n'a pas été utilisé par l'auditeur ; la récupération
concurrente de Claude n'a été ni pilotée ni mutée. Après un premier échec fermé,
elle a transféré les artefacts puis certifié sa cible exacte `TERMINATED`.
