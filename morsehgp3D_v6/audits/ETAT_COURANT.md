# État courant v6 — audit coopératif

Date de coupe : 31 août 2026.

HEAD observé : `95bb2281`. Autorités techniques : `6d755804` pour le
prototype E3/G16, `cd49a390` pour les callbacks, `95bb2281` pour le protocole
GCP, `320299df` pour le reçu de réplication et `8ed2dea6` pour le reçu de
confirmation contre-audité ci-dessous. Les notes Claude ne priment pas sur le
présent verdict.

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
| protocole GCP `95bb2281` | **NO-GO étroit** ; septième tour largement reçu, trois patches locaux avant dépense |

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
il reste volontairement hors CTest. Au pin `95bb2281`, le selftest campagne
passe 64 vérifications et le lifecycle 27 scénarios plus 11 refus de pin. Deux
rejeux indépendants des 83 tests de sûreté/intégration donnent 83/83 en
125,256 s et 129,257 s ; les mutants de frontière, le vrai signal 6 et le
tuple post-arrêt sont reçus dans leur portée. Le NO-GO ne porte plus que sur
le budget du second arrêt, la grâce distante et l'autorité de GNU time.

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
discordante sont couverts et étendus à `95bb2281`. Le GO GCP reste refusé
pour les trois corrections concentrées décrites ci-dessous.

## GCP

`AUDIT_GCP_V6_P0_20260831.md` est l'autorité dédiée. `95bb2281` ferme les
défauts de registre/cible, l'ancien clamp à 60 secondes, les `describe` sans
borne, les exécutables principaux résolus par `PATH`, 124/134, le wrapper
RLIMIT décoratif, les diagnostics mélangés et le tuple post-arrêt incomplet.
La validation est déplacée après une première tentative d'arrêt et les deux
selftests sont verts.

Trois patches locaux restent requis. Un échec du premier arrêt peut lancer
330 secondes de validation puis une seconde garde, bien qu'une seule enveloppe
soit réservée. `GRACE_S` peut dépasser les 60 secondes post-deadline et
`TIME_BIN` peut encore inventer RSS et signal. La correction directe est une
tentative SCP, deux réserves de 900 s avec reprise avant validation, une grâce
fixe de 30 s et `/usr/bin/time` constant. Cette variante conserve 501 s de
marge nominale sur G4.

Deux observations du contre-audit concurrent restent utiles comme
durcissements, sans devenir des portes supplémentaires. Handshake et bundle
consomment la marge de rendement avant le build ; l'échéance absolue les
empêche toutefois d'élargir la facture et le profil est déclaré tronquable.
La relation `guest * 60 + 300 <= maxRunDuration` est déjà vérifiée par
`start_and_verify.sh` avant `instances start` ; la vérifier aussi avant le
`set-scheduling` de l'instance arrêtée éviterait une mutation inutile, sans
changer la sûreté du démarrage.

## Dette d'échelle et ordre utile

La garde u32 des candidats est reçue. Restent les conversions de
`CloudIndex` vers `int/i32/u32`, les offsets, la recherche Karras près de
2^30, les produits signés des générateurs et les additions u64 des monnaies.
Déclarer les plafonds avant allocation et utiliser des additions contrôlées
ou u128 pour les compteurs.

Ordre recommandé à Claude :

1. finir les trois patches GCP ciblés du verdict frais, puis contre-auditer
   le commit propre avant toute session facturable ;
2. geler le cœur de confirmation déjà acquis et lier ses dérivés sans
   re-régler la règle ;
3. réparer les supports, compteurs et planchers des bras E3/G16, puis fermer
   l'oracle et la preuve G16 avant toute nouvelle mesure ;
4. fermer callbacks/exceptions et leurs fixtures réellement concurrentes ;
5. reprendre ensuite les plafonds API.

GCP non utilisé par le présent audit.
