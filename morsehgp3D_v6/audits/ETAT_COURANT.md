# État courant v6 — audit coopératif

Date de coupe : 31 août 2026.

HEAD observé : `e171ff3a`. Autorités techniques : `c0a17f2d` pour la réponse
courante, `781fbe27` pour le protocole GCP et `320299df` pour le reçu de
réplication. Les modifications E6 et la campagne de confirmation encore
présentes dans le worktree à cette coupe ne sont pas une autorité. Les
derniers rejeux produit indépendants complets restent rattachés à `88e530c6` ;
toute extension de ce périmètre est explicitée ci-dessous.

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
| callbacks et échecs du fold | ouverts ; la porte nominale contient une data race |
| première campagne d'octaves | `exploratory_complete`, jamais décisionnelle |
| répétition `a30c3a98` | `replication_complete`, reproductibilité seulement |
| confirmation hors échantillon | préenregistrée, exécution `.partial`, aucun verdict |
| sonde E6 `7611418a` | diagnostic utile, causalité et gain non démontrés |
| étage E6 du worktree | non reçu : quatre facteurs changés ensemble, oracle G16 absent |
| protocole GCP | **NO-GO**, audit dédié faisant autorité |

Le checkpoint mathématique reste reçu : coefficient 4 sur les deux covers q4,
contre-fixture causale, digest post-préfiltre séparé et conformité v5↔v6 jugée
sur les forêts et `digest_all`. Aucun résultat de campagne ne promeut
`public_status`.

## Rejeux indépendants disponibles

Au pin `88e530c6` :

- configuration et construction Release canoniques : succès ;
- huit portes ciblées ownership, mutants cap/split, contrats d'échec,
  provenance de campagne et agrégateur : 8/8 en 15,33 s ;
- suite hors label `scale` : 68/68 en 105,83 s, dont deux tests `oracle` ;
- contrôle documentaire : 238 fichiers Markdown actifs validés ;
- contrôle du registre : 20 phases et leurs portes validées.

Le probe `fold-inject-b-exception-k3` termine encore par SIGABRT, code 134.
Le troisième tour GCP a séparément rendu verts ses deux selftests, son test
d'intégration et 81/81 tests de sûreté locaux sur le code de `781fbe27`, mais
ces tests ne suffisent pas à rouvrir une session facturable : plusieurs
mutants passent encore par une cause parasite et le manifeste du reçu est
mécaniquement faux.

Le claim « 68/68 avec le binaire de la sonde E6 » n'est accompagné d'aucun
journal ou hash qui le lie au binaire annoncé. La suite sera donc rejouée sur
le prochain `HEAD` stabilisé avant réception.

## Exact-K, omission unique et ownership WSPD

Le juge exige exactement les forêts K de 1 à `kmax_eff`. La fixture n=2
fournit K1 correct plus K10 surnuméraire et vérifie le refus de code 2.

`781fbe27` ajoute une tentative de rejet de K11, mais sa fixture contient
deux fois `digest_forest_K1`. Le code 2 peut donc être causé par ce doublon et
ne reçoit pas encore causalement la borne globale K≤10. Supprimer la seconde
ligne K1 et contrôler le diagnostic exact.

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

`88e530c6` grave trois observations utiles : entrée invalide sans payload,
invalidation d'un sous-ensemble de champs sur échec census/fold-A, et ordre
croissant des callbacks `on_forest` en nominal. La preuve nominale n'est
toutefois pas recevable : `phase_calls` est un entier non atomique incrémenté
depuis les fils A et B. Le test possède donc un comportement indéfini.

Les autres lacunes restent concrètes :

1. `any_fail` exige un échec sur au moins une des valeurs inflight 1/2/8,
   pas sur chacune.
2. Les injections actuelles n'atteignent pas réellement deux ou huit folds
   simultanés et ne contrôlent pas `peak_fold_inflight`.
3. Les callbacks d'échec acceptent une sous-séquence croissante arbitraire ;
   census devrait exiger le vide et fold-A K2 le préfixe exact K1.
4. `FoldPhase::kPublished` signifie « livré provisoirement », avant le statut
   global. Renommer ce jalon et ajouter un terminal global commit/abort.
5. Une exception B ou issue d'un callback est relancée avant invalidation.
   Le CLI ne la convertit pas en statut terminal.
6. « invalidation totale » surqualifie le nettoyage : génération, expansion,
   temps, workers, RSS et plusieurs diagnostics peuvent rester partiels.

Correction constructive : typer séparément diagnostics partiels et payload
transactionnel, tracer les callbacks sous mutex ou atomiques, puis exercer le
mutant B et les exceptions de callbacks sous inflight 1/2/8 avec statut,
préfixe et payload exacts.

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
La campagne lancée au pin `320299df` est encore publiée sous
`campagne_confirmation_20260831.partial` : aucun verdict, pente ni
`E6_active` ne doit en sortir avant sa clôture terminale. Le mode `auto` lie
son binaire aux sources archivées au pin, et la publication par répertoire
partiel réduit les collisions. Pour ce run, scripts et profil ont aussi été
recoupés ex post avec les blobs du pin. La chaîne générique garde néanmoins
les frontières suivantes :

- les scripts et le profil archivés sont hashés entre eux mais pas comparés
  à leurs blobs Git au pin ;
- un binaire explicite est encore acceptable par le validateur, même pour un
  reçu nommé décisionnel ;
- un code de run non nul peut encore finir `DONE` et être publié, avant refus
  ultérieur de `pentes.py` ;
- l'agrégateur exécuté ne s'authentifie pas contre sa copie, relit les sorties
  après validation (TOCTOU) et écrit `AGREGAT.txt` directement.

Créer le répertoire partiel par opération exclusive, lier les scripts au pin
dans la chaîne elle-même, réserver le mode `auto` au profil confirmatoire et
faire produire au validateur une représentation immuable consommée une fois
par un agrégateur auto-authentifié. Ces défauts limitent l'autorité générique ;
ils n'autorisent ni à interrompre ni à requalifier rétroactivement la capture
privée déjà épinglée.

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

Le worktree postérieur à `c0a17f2d` raccorde déjà une option
`--e6-grille`, mais elle ne doit pas être reçue en l'état. Elle change en une
fois quatre facteurs sur les ancres à cover≥1024 : G8→G16, levée de
`near_m`, levée du ratio seeds/cover et nouvelle politique de seuil. La sonde
n'a testé aucun contrefactuel sur `near_m` ou le ratio ; écrire qu'elle les a
« réfutés » est donc faux. Aucun test G16 ni preuve ON/OFF n'accompagne encore
le diff, et un échec de construction G16 ne retombe pas vers G8.

Recette constructive avant réception : quatre bras factoriels (G16 avec
politique inchangée, `near_m` seul, ratio seul, combinaison), oracle direct
i128 et localisateur rationnel pour G16, mutants non-strict/h−1/epsilon,
digests candidats/forêts/all ON/OFF sur fils 1 et 8 et trois graines, plancher
de kills additionnels, fixture fail-open et compteurs de coût séparés par
résolution. L'option active doit être imprimée même si elle ne construit
aucune grille ; la représentation de la sonde simultanée doit être
versionnée.

## GCP

`AUDIT_GCP_V6_P0_20260831.md` est l'autorité dédiée. Le troisième tour
`781fbe27` ferme l'entrée externe, l'authentification du contenu épinglé, la
création exclusive du registre et le terminal extérieur sur le chemin
nominal. Le NO-GO reste néanmoins actif avant toute session facturable : le
profil canonique n'est pas comparé axe par axe, le validateur n'est pas
idempotent, plusieurs mutants passent par l'inventaire déjà enrichi, et le
`SHA256SUMS` du reçu référence son propre ancien nom temporaire. L'unicité du
répertoire de reçu et la lecture stricte du registre restent aussi à finir.

## Dette d'échelle et ordre utile

La garde u32 des candidats est reçue. Restent les conversions de
`CloudIndex` vers `int/i32/u32`, les offsets, la recherche Karras près de
2^30, les produits signés des générateurs et les additions u64 des monnaies.
Déclarer les plafonds avant allocation et utiliser des additions contrôlées
ou u128 pour les compteurs.

Ordre recommandé à Claude :

1. laisser finir sans perturbation la capture hors échantillon déjà lancée,
   puis la juger par son protocole privé sans toucher aux sorties ;
2. factoriser l'étage E6 et construire ses oracles G16 avant tout claim de
   gain ou de causalité ;
3. fermer callbacks/exceptions et leurs fixtures transactionnelles ;
4. corriger les quatre défauts locaux du reçu/validateur GCP et obtenir un
   nouvel audit statique GO ;
5. reprendre ensuite les plafonds API.

GCP non utilisé par le présent audit.
