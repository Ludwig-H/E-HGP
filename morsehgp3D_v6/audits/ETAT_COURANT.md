# État courant v6 — audit coopératif

Date de coupe : 1er septembre 2026.

Coupe source observée : `1069bc20` pour le profil `reduce`; les mises à jour
du seul audit ne changent pas cette coupe. Autorités techniques : `6d755804` pour le
prototype E3/G16, `cd49a390` pour les callbacks, `d98f4729` pour le protocole
et la source réellement exécutée sur G4, `94c74155` pour l'autorisation
mono-session désormais consommée, `df1a3c5f` pour l'archivage de son reçu,
`6e293deb` pour le checkpoint de plafonds, `671ed3cc` pour le premier pool
d'exécuteurs GPU hôte, `4a85c13d` pour sa correction causale, la garde 2E et
le témoin arithmétique hôte, `1069bc20` pour le harnais de profil,
`320299df` pour le reçu de
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
| saturation multi-CPU du fold `1069bc20` | pin construit et suite `gate` 92/92 ; profil, liveness, builds distincts, projection nommée, `join=1` et effacement K2 reçus dans leur portée, mais attribution nulle et fuite `stderr` restent faussement vertes ; aucun reçu de performance |
| wire série C C2, WIP post-`1069bc20` | division hôte et indices `upos` approuvés en principe ; `t1 i64`, transactionnalité, vues typées, golden digest et budget/dataflow doivent être fermés avant réception ; C3--C5 restent non ancrés et sans reçu device |
| pool d'exécuteurs C1 `4a85c13d` | reçu comme brique hôte : confinement fatal côté worker, passage file→actif sous verrou et quatre dents sélectives ; aucun raccord produit/CUDA |
| témoin arithmétique série C `4a85c13d` | reçu comme harnais C++ hôte partiel avec trois dents et contre-fixture composée ; aucun `nvcc`, device, `BallKey::power`, `AxisBounds` ou division plancher C3 |
| protocole GCP `d98f4729` | GO mono-session consommé et clos ; reçu `df1a3c5f` validé non décisionnel, arrêt ciblé certifié |

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
performance.

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
et gardes épinglés. Aucun GO courant n'est ouvert. Elles interdisent de
présenter les documents cités comme une photographie fraîche de toute la v6
avant leur alignement.

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

Une garde est toutefois placée trop tard par rapport à son propre contrat.
Si `E` est la somme exacte des shards, ceux-ci possèdent déjà au moins `E`
emplacements puis `out->reserve(E)` en alloue au moins `E` autres. La garde
logique `2E*sizeof(BallCandidate)` de `run.hpp` n'est consultée qu'après le
retour de `generate_candidates`. La fixture `(b)` provoque donc l'allocation
qu'elle prétend prévenir, puis refuse. Correction courte : propager une borne
de fusion dérivée du budget, la tester au point `exact_fusion` avant
`reserve`, et tuer la variante tardive par un mutant. Cela reste un proxy
logique ; supprimer réellement la coexistence des deux copies demanderait une
fusion segmentée.

Les commentaires de `caps.hpp` et `generate.hpp` doivent suivre cette portée :
la fenêtre de test se calcule sur les cardinalités, pas sur `capacity()`, et
la projection 1,6–3,2 M / `ECHELLE.md § 3` n'a pas d'autorité v6. La porte
CLI juge désormais code et signature dans un seul processus ; son
`EXPECT_LINE` reste une recherche de sous-chaîne et les refus ne gravent pas
stdout vide. Ce sont des durcissements P2, tout comme l'isolation des trois
sites q2/q3/q4 et de la garde de vague initiale.

Ordre recommandé à Claude :

1. fermer causalement le pool C1 côté worker et tuer sa réutilisation
   post-fatale, avant de bâtir C2 dessus ;
2. avancer la garde `2E` avant la fusion globale et corriger ses commentaires,
   sans rouvrir le contrat plus large de RAM résidente ;
3. tuer les deux contre-fixtures de profil du § 5.10 avant d'interpréter la
   matrice locale du fold ou de segmenter `reduce` ;
4. fermer le wire C2 transactionnel et typé selon le § 5.11, puis choisir
   explicitement `census_all` ou compaction stable avant de figer le budget ;
5. corriger `GRAND_LIVRE.md` avant toute interprétation des coûts, puis
   aligner `ARCHITECTURE.md`, `PROVENANCE.md` et `PLAN_DE_TESTS.md` au
   prochain checkpoint stable ;
6. enrichir seulement C1–C4 dans `MATHEMATIQUES.md` et remplacer l'`assert`
   Python opportunément : ce microcorrectif P2 ne bloque aucun résultat ;
7. geler le cœur de confirmation déjà acquis et lier ses dérivés sans
   re-régler la règle ;
8. réparer les supports, compteurs et planchers des bras E3/G16, puis fermer
   l'oracle et la preuve G16 avant toute nouvelle mesure ;
9. fermer callbacks/exceptions et leurs fixtures réellement concurrentes ;
10. ne rouvrir une mesure d'échelle ou G4 qu'avec une question décisionnelle,
   un nouveau pin exact et un budget couvrant les résidences revendiquées.

GCP non utilisé par le présent audit.
