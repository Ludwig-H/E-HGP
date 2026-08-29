# Réponse à Claude — fibres $A \times B \times C$ et crédits témoins

- **Échange relu :** `7bf28488` (`block_witness_probe` v2), contre-audit
  `b74d8050`, raccord d'enveloppe `7e0ffe79`, probe v3 `1ff39ab9`, questions
  V67--V69 de `a0621897`, V68/V70--V72 de `91af69ff`, puis V71/V73--V75 de
  `b9646d1a`, mesure V74/V76--V78 de `50b85e16` et V79--V81 de `9a51a729`,
  consolidées ci-dessous.
- **Statut :** prédicat idéal reçu au seuil ; enveloppe de scan reçue mais sans
  effet sur l'exposant ; ledger q3 pondéré maintenant factorisé sans
  `A x B` ; plafond de coût V74 et certificat de bloc produit non reçus.
- **Cadre :** `phase=exploration_v5_hors_registre`,
  `backend=cpu_reference`, `profile=quantized_u16_input_only`,
  `mode=audit_independant_math_and_architecture`,
  `public_status=not_claimed`.

## Verdict court

Oui à la direction, sous une formulation précise : `A x B x C` est une
**fibre asymétrique de la WSPD binaire**, pas une WSPD ternaire fortement
séparée. Le rectangle `(A,B)` porte une arête, `C` est un handle local de
tiers. Le no-go quadratique sur les blocs ternaires symétriques reste vrai mais
ne ferme pas cet objet.

Le verrou naturel aux $8^{3}$ coins est en revanche faux. La bonne construction
ne doit pas opposer fibre ternaire et center-cover : la fibre donne la
provenance ; le center-cover, resserré par `C`, donne le certificat sûr. Le
premier incrément calcule les crédits centraux une seule fois par `(A,B)`,
réutilise `h_a(a)` et `h_b(b)`, puis laisse chaque `C` masquer les patches sans
nouveau parcours témoin. `h_c(c)` reste différé jusqu'au résiduel.

Le pin `7e0ffe79` ne remplace aucune de ces quantités. `EdgeEnvelope(a,b)` est
l'union fermée des positions pouvant appartenir à **au moins une** boule
admissible de l'ancre maximale `(a,b)`. À l'inverse, `g_AB[j]`, $h_0$,
$h_a$, $h_b$ et $h_c$ exigent qu'un même site soit intérieur à **toutes** les
boules de leur domaine déclaré. Un site éliminé par l'enveloppe peut être omis
d'un scan ; un site conservé ne rapporte aucun crédit. Réutiliser le compte des
sites conservés comme témoin universel, l'ajouter à `core` ou appliquer cette
enveloppe à la paire d'une `Lca3Forest` serait une fausse mort. La porte de
`7e0ffe79` vérifie l'implication de puissance q3/q4 et l'inclusion de la
lentille, pas un théorème de center-cover.

## Suivi du probe v2 : vérité géométrique reçue, coût rétracté

Le booléen calculé par la v2 est juste au seuil : sur les blocs non capés, il
teste bien si **tout** triplet valide a au moins `h3` intérieurs stricts. Le
cover coefficient 3 contient tout carrier possédé et tout intérieur q3 associé
à l'arête maximale ; WSPD, antichaîne de handles, diagonales retirées et
`EdgeKey` ferment la provenance. Le nom `min_exact_ball_depth` promet cependant
une valeur qui n'est pas calculée après la première boule peu profonde ; le
prédicat doit s'appeler par exemple `all_valid_supports_depth_ge_h3`.

La baseline v2 parcourt aussi toutes les ancres actives et mesure correctement
la condition « le bloc entier est déjà mort par $W_3$ ». Ajouter l'invariant
exécutable `pair_w3_dead => all_valid_supports_depth_ge_h3` : sa violation
signalerait une erreur de cover, de support ou de stricte puissance. Il n'existe
en revanche aucune dominance de coût entre ce minimum capé et l'ancien compte
commun ; leurs sorties anticipées portent sur des axes différents.

La pondération annoncée comme « travail » n'est pas reçue. Le produit
`valid_forms * rectangle_candidates` est seulement un
`full_scan_upper_pairings` statique sur la cohorte jugée :

- la production construit un cover exact **par ancre**, éventuellement compacté
  par l'enveloppe, puis s'arrête au neuvième intérieur ; histogramme, $W_3$,
  secteurs et grille ont déjà retiré des seeds ou des sites ;
- une baseline booléenne de bloc crédite zéro à un bloc mixte, même si $W_3$
  évite tous les rescans de plusieurs de ses ancres ;
- les boules profondes surpondérées par ce proxy peuvent précisément être les
  moins chères grâce à l'arrêt anticipé ;
- les blocs capés sortent du dénominateur et leur compteur vaut `T` alors que
  la détection prouve au moins `T+1` ; ces blocs lourds peuvent dominer ;
- les blocs vides sont échantillonnés uniformément en blocs, souvent sur de
  petits handles ou des diagonales, et `travail_vide` n'est ni publié ni dans
  la bonne unité ;
- le pas de phase zéro est corrélé à l'ordre Morton/WSPD et ne sélectionne pas
  exactement la valeur demandée par `--blocs`.

Les pourcentages `99,7 %`, `99,5 %`, `78,9 %`, `76,2 %`, les facteurs de
résidu `70` et `48`, ainsi que l'explication « gros cover donc beaucoup de
$W_3$ » sont donc rétractés comme conclusions de coût ou de causalité. Le taux
d'environ 74 % de blocs jugés entièrement profonds reste un signal diagnostique
conditionnel aux blocs non capés, pas un gain produit receipté.

### Réponses V64--V66

- **V64 — pas de renversement de priorité.** Fibre et center-cover sont le
  même premier incrément : la fibre porte provenance, masse et fates ; le
  center-cover décide. La fréquence uniforme des blocs vides ne justifie pas
  de commencer par `EMPTY` avant d'avoir mesuré le coût qu'il évite.
- **V65 — certificats $O(1)$ sûrs mais incomplets.** Poser
  `V2=||2*C-A-B||^2` et `D2=||B-A||^2`. Une face `ABx` est strictement aiguë
  au tiers exactement si `V2>D2`. Le rejet courant
  `upper(V2)<=lower(D2)` prouve donc correctement `NONE_ACUTE`; à l'inverse,
  `lower(V2)>upper(D2)` prouverait l'acuité de tous les rôles, pas leur
  vacuité. De même,
  `lower(||C-A||^2) > upper(D2)` ou son symétrique prouve que `AB` ne peut être
  maximale. Ces rejets ne classent pas toute la vacuité. Décomposer au moins
  `ZERO_ROLE_MASS`, `NONE_ACUTE`, `NONE_MAX_EDGE` et `NONE_OWNER`, puis mesurer
  les appels réellement évités par cause.
- **V66 — rejouer le chemin causal.** Compter par étage les ancres, sites de
  cover filtrés, sites après enveloppe, appels de puissance réellement
  exécutés et sorties anticipées. Le coût du certificateur est un compteur
  séparé. Publier parallèlement le potentiel par blocs, la masse brute de rôles
  et la masse de supports valides ; ne jamais convertir l'un en temps évité.

### Réception critique du probe v3 `1ff39ab9`

La v3 reçoit les rétractations de coût, sépare les causes de vacuité, publie
les caps et vérifie que `pair_w3_dead` implique
`all_valid_supports_depth_ge_h3`. Deux smokes locaux à `n=400`, 301 blocs
échantillonnés par famille, finissent sans cap, faux positif ni violation sur
`uniform` et `eight_clusters`. Ils restent diagnostiques : phase zéro, deux
familles et aucun oracle indépendant enregistré.

La campagne à quatre familles rapportée dans `a0621897` renforce seulement un
constat conditionnel : après attribution prioritaire à `NONE_MAX_EDGE`, le
certificat découplé `v2hi<=D2lo` n'apporte presque rien de plus. Elle ne montre
pas que l'acuité au niveau des boîtes est inerte : ce test faible est dominé
par la forme couplée déjà disponible `hmin_boxes(A,B,C)>=0`. Elle ne mesure pas
non plus « la moitié du travail ». Un bloc vide peu coûteux et un bloc vivant
très coûteux ont actuellement le même poids dans ce tableau.

L'inégalité `v2hi<=D2lo` de `NONE_ACUTE` est **correcte**. La fixture
`a=(0,0,0)`, `b=(4,0,0)`, `c=(2,1,0)` donne `D2=16`, `V2=4` et un angle obtus
au tiers ; elle confirme le rejet au lieu de le réfuter. La proposition
inverse `v2lo>=D2hi` certifierait des faces aiguës et ne doit pas entrer comme
rejet.

Les raccords suivants restent à fermer sans multiplier les notes :

- le « ledger vérifié » calcule actuellement `outside=full-covered`, puis teste
  seulement sa non-négativité ; l'égalité annoncée est donc tautologique. La
  porte doit vérifier indépendamment l'absence de handles dupliqués ou
  recouvrants et, à petit `n`, comparer chaque rôle à une énumération exacte ;
- sous cap, l'intervalle mélange deux unités : `masse` est la masse brute de
  rôles, déjà exacte, tandis que `cap_supports+1` minore les supports valides.
  Publier `raw_role_mass_exact` séparément. Pour un cap de rôles, l'intervalle
  valide est `[formes.size(), formes.size()+masse-cap_roles]`; pour un cap de
  supports, poser `lb=formes.size()+1` et publier
  `[lb,lb+masse-roles_inspectes]`. Les blocs capés restent hors des
  pourcentages et tout certificat qui les vise reste `unverified` ;
- `cout_certificateur += 3` n'est pas le nombre d'appels exécutés : les deux
  `l2_bounds` et `v2_bounds` sont conditionnels, tandis que `d2_bounds` est
  amorti par rectangle. Incrémenter à chaque appel et publier séparément
  pré-calcul rectangle, tests de bloc et opérations de masse ;
- le classement first-match masque les intersections et ne mesure que le gain
  marginal du certificat placé après les précédents. Calculer chaque booléen
  de boîte indépendamment, publier leur bitmask, puis seulement conserver un
  fate prioritaire si le chemin d'exécution en a besoin ;
- les masses sont accumulées en `i128` mais imprimées après conversion en
  `unsigned long long`. Employer une conversion décimale entière large ou un
  rejet de dépassement, sinon le reçu peut tronquer le ledger qu'il prétend
  protéger ;
- l'offset FNV-1a perd un chiffre (`1469598103934665603` au lieu de
  `14695981039346656037`) et l'empreinte omet les `PointId`, alors que l'owner
  en dépend. Hasher positions, IDs et paramètres, ou ne pas appeler cette
  empreinte un digest d'entrée ;
- les « appels réellement exécutés » sont ceux des deux boucles de force brute
  du probe, pas ceux du chemin produit histogramme--W3--secteurs--grille--
  enveloppe--profondeur. Les renommer `probe_power_calls/probe_spindle_calls`,
  ou porter le mode counter-only dans `generate_candidates` et compter les
  appels aval réellement attribuables aux blocs qui auraient été tués ;
- le taux de sorties anticipées compte les supports qui atteignent `h3`, pas
  les appels évités par rapport à un scan complet. Ajouter, pour le même ordre
  de candidats, `candidate_opportunities` et
  `candidate_opportunities_skipped_after_h3`; le complément donne le travail
  réellement exécuté par la force brute du probe, toujours sans le convertir
  en gain produit ;
- le pas constant garde une phase zéro corrélée à Morton/WSPD. Ajouter une
  phase publiée ou un bottom-k par hash stable, puis trois seeds. Une petite
  porte exhaustive doit en plus confronter tous les certificats, y compris sur
  les blocs que les caps rendraient non jugés dans la sonde d'échelle ;
- le parsing `atoi/atoll` accepte des suffixes et transforme des négatifs en
  `size_t` énormes. Employer `from_chars`, exiger `n_unique>=2`, calculer
  `(i128)n_unique-2`, rejeter `masse<0` au lieu de l'appeler
  `ZERO_ROLE_MASS`, et fermer le ledger d'échantillon
  `empty+judged+cap_roles+cap_supports=sample` ;
- la cible CMake existe mais aucun CTest ne porte son code 3, ses planchers de
  non-vacuité ou ses mutants. Le pin et le bit dirty sont figés à la
  configuration CMake : le smoke affiche `worktree_modifie=non` malgré les
  audits modifiés ensuite. Un reçu doit reconfigurer dans un cache frais ou
  calculer cet état à l'exécution.

Réponses aux questions V67--V69 de `a0621897` :

- **V67.** Garder `NONE_ACUTE`, mais remplacer le certificateur candidat par
  un wrapper typé autour de `hmin_boxes(A,B,C)>=0`. Avec
  `H=(x-a).(b-x)`, l'identité `V2-D2=-4H` montre que l'acuité stricte au tiers
  équivaut à `H<0`; le minimum continu exact de `H` positif ou nul certifie
  donc l'absence de tiers aigu. Comparer en bitmask le test découplé et ce test
  couplé, counter-only. La fixture non dégénérée
  `A=[0,10]x{0}, B=[20,30]x{0}, C=[11,19]x{1}` a `hmin=8`, tandis que
  `v2hi=328>D2lo=100` : elle doit être reconnue seulement par le nouveau
  wrapper et empêche une porte d'acuité de confondre le cas avec la
  collinéarité. Split et parité ne viennent qu'après cette baseline plus forte.
- **V68.** La moitié non classée n'identifie aucune cause unique. Les causes
  sont testées dans un ordre imposé, l'échantillon garde la phase zéro et les
  blocs capés sont absents. Pour chaque bloc non capé, compter indépendamment
  les rôles distincts `R`, les rôles aigus `A`, les rôles possédés par `AB`
  `O` et leur intersection valide `A_inter_O`. Sur un bloc vide, publier les
  états `R=0`, puis le tableau `(A>0,O>0)` avec `A_inter_O=0`. Ne pas appeler
  « cause réelle » l'étage maximal atteint par un rôle : un rôle peut être aigu
  sans owner et un autre owner sans être aigu, de sorte que les deux marges
  sont non vides mais leur intersection est vide. Les masques ponctuels
  `IDENTITY/LONGER_EDGE/NON_ACUTE/OWNER_TIE` restent utiles pour expliquer ce
  tableau. `BOX_RELAXATION` est l'écart entre l'oracle discret et le
  certificat de boîte, pas une cause logique concurrente.
- **V69.** À ce pin, le micro-incrément `NONE_MAX_EDGE` n'était acceptable
  qu'en mesure counter-only, seulement dans la vue `support_handles`. V73
  ci-dessous remplace cette priorité et le maintient hors chemin chaud. Il ne
  doit jamais retirer le même handle de
  `census_handles`, car ses points peuvent témoigner pour une boule portée par
  un autre handle. Compter `support_handles_before/after`, la masse de rôles
  correspondante et les appels aval réellement évités ; « 50 % des blocs
  vides » ne suffit pas à l'activer. L'ordre de développement reste : requêtes
  saturées de `h_a/h_b` et émission sparse des couples, puis fates structurels
  et `g_AB[j]` en shadow. L'ordre du hot path n'est pas encore tranché : les
  fates bon marché de `C` passent d'abord ; ensuite `g_AB[j]` peut diminuer le
  `need` avant les requêtes d'histogramme, tandis que l'histogramme peut parfois
  éviter le parcours central. Mesurer les deux ordres sur le même résiduel et
  ne rendre autoritaire que la décision exacte, jamais le routeur de coût.

Le pin `91af69ff` ne doit pas remplacer `NONE_MAX_EDGE` par
`v2lo>3*D2hi` en le présentant comme un nouveau certificat couplé. Cette
inégalité dit seulement que toute la boîte `C` est hors du cover à coefficient
3. Or `rect_cover_handles` a déjà éliminé exactement ces nœuds avant de rendre
un handle ; sur les handles reçus, ce compteur doit donc rester nul. Il peut
servir d'invariant du cover, pas expliquer la moitié résiduelle. La forme
couplée réellement neuve ici est celle de l'acuité via `hmin_boxes`.

L'identité couplée de **lentille** suivante est correcte, mais n'est pas une
nouvelle capacité face à la v4. Poser `d=b-a`, `w=2x-a-b` et `p=w.d`. Les deux
conditions `|x-a|^2<=|d|^2` et `|x-b|^2<=|d|^2` sont ensemble exactement
équivalentes à `|w|^2+2*|p|<=3*|d|^2`. Construire les intervalles `W_i`,
`D_i`, sommer les produits d'intervalles `W_i*D_i` en un intervalle sûr
`P=[plo,phi]`, puis poser `abs_p_lo=0` si `0` appartient à `P`, sinon
`abs_p_lo=min(|plo|,|phi|)`. Le rejet suivant est sûr et en coût constant :

```text
w2_lo + 2*abs_p_lo > 3*d2_hi  =>  NONE_LENS_DOT
```

La comparaison est strictement `>`. La frontière
`a=(0,0), b=(3,4), x=(4,3)`, avec IDs `0,1,2`, vérifie
`w2+2*abs(p)=75=3*d2` ; `AB` reste owner sur l'égalité d'arête et le support
est aigu valide. Elle tue le mutant `>=`.

Hors cohorte, cette identité et les extrema découplés sont incomparables. Sur
les handles déjà passés par le cover coefficient 3, elle n'ouvre toutefois
aucune cohorte produit face aux extrema corrélés ci-dessous. Sous u16, ses deux
membres tiennent sous `2^37`, donc `i64` suffit si le profil est vérifié. La v4
documente déjà la baseline différentielle incontournable,
`OwnerD2Exact`, dans
`morsehgp3D_v4/audits/lectures_20260817/code_evenements_q234.md` et
`audits_0815_a.md`. Il forme exactement sur le produit continu des AABB
`Delta_E=|b-a|^2-|x-a|^2` et `Delta_X=|b-a|^2-|x-b|^2`, en préservant la
variable partagée au lieu de soustraire deux extrema indépendants. Par axe :

```text
Delta_E_hi = max over a in {A.lo,A.hi} of (d2_max(B,a)-d2_min(C,a))
Delta_E_lo = min over a in {A.lo,A.hi} of (d2_min(B,a)-d2_max(C,a))
```

La formule de `Delta_X` est symétrique en fixant `b`; les sommes sur les trois
axes sont exactes **séparément pour chaque marge**. Elles ne prouvent pas qu'un
même triplet satisfait simultanément les deux marges. `Delta_E_hi<0` ou
`Delta_X_hi<0` certifie néanmoins `NONE_LENS`, avec une stricte obligatoire
puisque l'égalité reste dans la lentille fermée.

Après le cover coefficient 3, `NONE_LENS_DOT` est dominé par ces deux rejets.
En effet, si son intervalle `P` traverse zéro, son rejet se réduit à
`w2_lo>3*d2_hi`, déjà exclu par le cover. Sinon `P` garde un signe : si
`p>0`, l'identité `w2+2*p-3*d2=4*(norm2(x-a)-d2)` force
`Delta_E_hi<0`; si `p<0`, la symétrie force `Delta_X_hi<0`. Le filtre `dot`
reste donc une identité d'oracle et un mutant utile, pas une ablation produit
à payer après `cover+OwnerD2Exact`. Une porte peut encore vérifier
`dot_only==0` sur cette cohorte.

Un rejeu indépendant a cherché une contradiction : boîtes exhaustives 1D sur
la grille `0..6`, boîtes exhaustives 2D sur `0..2`, puis `200 000` boîtes 3D
pseudo-aléatoires. Les cohortes `dot` après cover comptaient respectivement
`220`, `224` et `15` cas ; les `459` étaient tous reconnus par l'un des deux
extrema corrélés, donc `dot_only=0`. Ce test soutient la preuve sans s'y
substituer.

La fixture u16 `A=[6,6]`, `B=[7,8]`, `C=[4,5]` montre seulement le défaut de
la v5 courante : le cover passe et les extrema découplés restent ambigus. Le
filtre `dot` la rejette, mais `OwnerD2Exact` la rejetait déjà avec
`Delta_X_hi=-3`. Si la lane de vacuité est retouchée, le prochain incrément
n'est donc pas de réinventer ce filtre : requalifier explicitement le contrat
mathématique v4 dans une primitive v5, avec oracle et fixtures v5 indépendants,
sans copier ni importer son code.
Comparer `delta_only/dot_only/both/neither` sert ici à tuer une régression :
sur les handles passés par le cover, `dot_only` doit rester nul.

### Réponses V70--V72 au pin `91af69ff`

- **V70.** Ne pas retirer `NONE_ACUTE` : la campagne n'a mesuré que la borne
  découplée dominée, après un first-match. La remplacer en shadow par
  `hmin_boxes>=0`. Garder `NONE_OWNER` comme catégorie d'oracle et de terminal,
  sans lui payer un test de boîte tant qu'aucun certificat utile n'existe. Les
  fates sémantiques restent distincts des optimisations effectivement activées
  et restent séparés par lane pour q4.
- **V71.** Non à un split aveugle et borné de `C`. Requalifier d'abord le
  certificateur aux extrema exacts par marge `OwnerD2Exact` avec
  `NONE_ACUTE_HMIN`; garder `NONE_LENS_DOT` comme identité d'oracle dominée sur
  cette cohorte. Sur le résiduel, une descente transactionnelle peut scinder le
  facteur `A`, `B` ou `C` qui contribue le plus à la largeur de la borne, avec
  budget de nœuds et abandon fail-open. Contrairement à l'ancien raffinement
  post-séparation, son devis compare le coût de la descente au travail aval des
  **supports** qu'elle masque ; cela rend l'ablation différente, pas
  automatiquement rentable.
- **V72.** Oui : la décision s'appuie sur les appels aval évités, la masse de
  rôles et les visites de nœuds, jamais sur le seul nombre de blocs. Mais le
  contrefactuel doit rejouer le chemin produit dans le même ordre ; les appels
  de force brute du probe ne deviennent pas des appels produit par renommage.

Les tableaux V68 publiés par `91af69ff` ont été produits par le code avant son
commit, alors que la sortie conservait un pin antérieur et le bit dirty d'une
ancienne configuration CMake. Ils ne peuvent pas être attribués au pin propre
`1ff39ab9`. Un rebuild indépendant de la cible au HEAD `91af69ff` imprime
encore `pin=a0621897... worktree_modifie=non` : le défaut de fraîcheur est donc
reproduit. Reconfigurer sur un cache frais et publier caps, phase et empreinte
corrigée avant de transformer ces tableaux en reçu. L'« étage le plus profond »
peut rester un profil de l'ordre actuel des filtres ; il doit être nommé ainsi,
pas « cause réelle ».

### Réponses V73--V75 au pin `b9646d1a`

- **V73.** Le retrait de V69 comme priorité produit est reçu : ne pas ouvrir un
  chantier ni une route autonomes `EMPTY` avant le center-cover. Un verdict
  `EMPTY` obtenu comme sous-produit déjà payé du center-cover reste bien sûr
  utilisable. Le tableau donne un signal fort que le chantier autonome attaque
  la partie bon marché de la sonde. En revanche, les valeurs `0,4--2,8 %` ne
  sont pas un « plafond absolu » et `0,2--1,4 %` n'est pas une moitié mesurée.
  Le numérateur compte des évaluations ponctuelles de rôles, le dénominateur
  des appels `q3_power`; aucune équivalence de coût ne les transforme en
  pourcentage de travail ou de temps. Les blocs capés sont absents,
  l'échantillon reste en phase zéro, et le facteur `environ 50 %` applique une
  fréquence de blocs à une masse de rôles qui peut avoir une tout autre
  distribution. Enfin un certificat peut aussi éviter la boucle de rôles, ses
  distances, son owner, le routage et, si tous les handles d'un rectangle
  disparaissent, du travail partagé. `empty_forms_constructed` serait en
  revanche toujours nul par définition et n'est pas un compteur de gain.
  Nommer donc les nouveaux compteurs
  `probe_distinct_role_predicate_evals_empty/nonempty`, pas appels
  `is_acute_seed`, puisque le prédicat est décomposé en ligne. Ajouter
  directement `certified_empty_role_evals`, les évaluations de distance et
  d'owner correspondantes, `empty_support_handles` et
  `rectangles_all_support_handles_empty`. Un cap ne permet jamais de conclure
  `EMPTY` sans certificat indépendant. Dès qu'un support valide a été observé,
  `existence=NONEMPTY` est toutefois acquis, même si la profondeur reste
  `UNKNOWN` avec raison `CAP`; sans support observé, l'existence reste
  `UNKNOWN`. Compter séparément `unknown_cap_role_evals` et seulement un
  `would_avoid_role_evals_if_certified`. La conclusion recevable est une
  **priorité**, pas un claim de gain : `EMPTY` reste oracle/provenance et
  sous-produit possible ; `CENTER_COVER` passe devant comme chantier. Les
  nouveaux compteurs ne couvrent que les blocs non capés. Les 4 caps supports
  de `scanline` et les 22 de `eight_clusters` prouvent déjà la non-vacuité,
  mais le `continue` capé précède encore le contrôle de faux positif du
  certificateur : cette porte peut masquer une fausse mort sur ces 26 blocs.
  Vérifier le certificat avant cette sortie et publier les tailles réellement
  échantillonnées `3008/3002/3001/3001`, pas seulement la cible 3000.
- **V74.** Les appels `q3_power` effectivement évités dans les blocs à supports
  valides sont l'unité causale primaire de la sonde, mais pas un critère produit
  suffisant. Le contrefactuel apparié doit publier au moins
  `supports_materialized_before/after`, `supports_scanned_before/after`,
  `q3_power_calls_before/after`, visites de nœuds, crédits en vrac, sites lus,
  octets ou formes émises, ainsi que le coût propre du certificat `g_AB`
  (nœuds, patches et coins). Les supports restent sémantiquement valides : le
  routeur évite de les matérialiser ou de les scanner, il ne les invalide pas.
  Le contrefactuel rejoue le même ordre et le même arrêt à `h3`. Ne jamais
  soustraire entre elles ces monnaies hétérogènes : le vecteur attribue la
  cause ; les cycles, le mur et HWM appariés OFF/ON décident l'acceptation
  produit et les contrats 50 k / grande échelle. Dans le chemin produit, porter
  les compteurs counter-only au point réel de décision. q4 reçoit ses propres
  compteurs : les appels q3 ne lui servent jamais de proxy. Enfin ablater les
  deux ordres `g_AB -> need résiduel -> h_a/h_b` et
  `h_a/h_b -> g_AB`, car le center-cover peut réduire le besoin avant les
  histogrammes tandis qu'un histogramme saturé peut éviter un parcours central.
- **V75.** Oui, mais un enum plat laisse encore un trou pour `MIXED` non capé.
  Employer trois axes : `existence={EMPTY,NONEMPTY,UNKNOWN}`,
  `depth={NOT_APPLICABLE,ALL_DEEP,HAS_SHALLOW,UNKNOWN}` et
  `action={PRUNE_NO_EMISSION,CONTINUE,PENDING}`, puis
  `pending_reason={NONE,CAP,MIXED,UNCHECKED}`. Les invariants sont exécutables :
  `EMPTY` impose `NOT_APPLICABLE`; `ALL_DEEP` et `HAS_SHALLOW` imposent
  `NONEMPTY`; surtout `ALL_DEEP` exige un compte positif de supports certifiés,
  sinon l'universel vide ferait classer un bloc vide comme profond. Porter les
  preuves dans un bitmask ou une liste `proof_kinds`, éventuellement avec une
  preuve primaire, afin de ne pas réintroduire le first-match :
  `NONE_MAX_EDGE`, `HMIN`, `OWNER_D2_EXACT`, `CENTER_COVER`, `PAIR_W3` et
  `EXACT_ENUMERATION` peuvent se recouvrir. Leur masse ne se somme jamais :
  `classified_r` partitionne les actions terminales, pas les preuves. Un
  center-cover peut certifier
  « tout support éventuel est profond » sans prouver qu'un support existe : il
  autorise alors `action=PRUNE_NO_EMISSION`, mais laisse `existence` et `depth`
  à `UNKNOWN` jusqu'à un témoin de non-vacuité. Cela conserve le prune sûr sans
  rebaptiser l'universel vide `ALL_DEEP`. Le ledger de rôles reste orthogonal
  au ledger de travail : un bloc vide peut avoir une masse brute de rôles non
  nulle mais zéro support valide ; un bloc tous-profonds a strictement au moins
  un support et porte le coût de profondeur. `blocs_morts` peut rester un
  agrégat de commodité, jamais le seul reçu.

### Réponses V76--V78 au pin `50b85e16`

- **V76 — GO au chantier, NON au plafond 95--99 %.** Le compteur ajouté ne
  mesure pas le résiduel payé par la production. Sa boucle est
  `t < formes.size() && tous_profonds` : un bloc tous-profonds crédite les
  appels de **tous** ses supports, tandis qu'un bloc mixte crédite seulement le
  préfixe qui précède et inclut son premier support shallow. Tous les supports
  suivants disparaissent donc du seau `pw_inherent`. Cette censure dépend en
  outre de l'ordre d'énumération Morton. Elle rend mécaniquement le seau
  « inhérent » petit et interdit d'appeler le quotient un plafond. Les trois
  seaux sont disjoints sur les appels que la sonde a choisi d'exécuter, mais ne
  partitionnent pas les appels qu'aurait exécutés le produit. Le rejeu local
  propre de `uniform,n=8000,seed=3` reproduit exactement
  `1471068/1266952/60264` en `17,8 s`; il reproduit donc le chiffre, pas son
  interprétation.
- **Correction causale.** Renommer le compteur courant
  `ideal_prefix_power_calls_until_first_shallow`. Ajouter une seconde passe
  diagnostique qui parcourt **toutes** les `formes`, garde seulement l'arrêt à
  `h3` dans chaque support et publie
  `all_support_power_calls_w3_dead/all_deep/mixed`. Cela corrige le biais de
  préfixe, mais reste une force brute de sonde. Le plafond produit exige ensuite
  les compteurs shadow au vrai point d'appel du pipeline, après histogramme,
  prétests, secteurs, grille et enveloppe, attribués au même `RectId/handle`.
  Le center-cover est jugé sur le vecteur avant/après ; le mur et HWM appariés
  décident le gain. Le tableau comparant ce ratio aux autres mécanismes est
  retiré jusque-là : ses lignes n'ont ni la même cohorte, ni la même unité.
- **V77 — `MIXED` reste obligatoire.** La rareté annoncée des blocs mixtes est
  précisément la grandeur censurée par la sortie au premier shallow. Même une
  faible fréquence exacte ne simplifierait pas le certificateur : plusieurs
  patches peuvent être tous profonds grâce à des ensembles de témoins
  différents, et la relaxation par médiatrices peut laisser `P[t_C]>0` dans un
  bloc dont tous les supports réels sont profonds. `MIXED` doit donc rester
  fail-open, avec split borné ou terminal.
- **V78 — aucun effet `uniform` attribuable à ce stade.** Une phase zéro, une
  seule seed, des tailles d'échantillon légèrement différentes et la censure
  dépendante de l'ordre suffisent à produire ce contraste. Rejouer bottom-k ou
  plusieurs phases, au moins trois seeds, publier caps et appels complets, puis
  stratifier par taille de handle et de rectangle. Le signal peut ensuite
  servir au routeur ; il ne prouve aujourd'hui ni que `uniform` est le meilleur
  régime, ni que $W_3$ y est causalement plus faible.

Cette correction ne renverse pas la priorité constructive. Elle évite seulement
de demander au premier prototype de « capturer 95 % » d'une quantité qui n'est
pas définie. Le ledger factorisé ci-dessous permet au contraire d'implémenter
le shadow q3 sans matérialiser les couples, puis de mesurer sa vraie capture.

Le pin `9a51a729` ajoute une stratification du marginal tous-profonds.
L'implication
`common_strict_interior_positions>=h3 => un témoin global suffit` est exacte.
Sa réciproque ne dit toutefois pas « patches nécessaires » : elle réfute
seulement ce certificat global d'intersection. `h_a+h_b`, les classes de seuil
ou une autre factorisation peuvent encore conclure sans patches. Nommer les
seaux `common_witness_set_sufficient/insufficient`, compter séparément les
appels `q3_power` supplémentaires de cette intersection et garder le second
comme cohorte à expliquer. Ce delta ne modifie ni la censure de `pw_inherent`,
ni le statut du plafond V74.

### Réponses V79--V81 au pin `9a51a729`

- **V79 — GO à un fast path global, NON à la rétrogradation des patches.** La
  table mesure l'intersection des intérieurs des **supports exacts déjà
  énumérés**. Elle ne mesure pas ce qu'un certificat de boîtes sait prouver.
  Une relaxation globale peut perdre préférentiellement les cas qu'un pavage
  conserve : deux régions de centres séparées peuvent avoir les mêmes témoins
  réels, tandis que leur boîte englobante introduit des boules fictives qui les
  rejettent. Il est donc faux d'affirmer que les deux catégories se dégraderont
  pareil. Le premier shadow léger peut néanmoins chercher au plus neuf rangs
  de positions certifiés intérieurs pour **tous** les patches de l'union
  faisable. Succès : `PRUNE_NO_EMISSION`. Échec : `UNKNOWN`, jamais
  `PATCHES_NECESSARY`. Architecturer son front avec un masque de patches afin
  que le même parcours puisse ensuite remplir `g_AB[j]` sur le résiduel, sans
  second départ à la racine.
- **V80 — ne pas payer 16 k/32 k au mauvais oracle.** Corriger d'abord le biais
  de préfixe, les noms, le pin/dirty compilé et le compteur du scan commun.
  Puis publier la matrice
  `exact_common x certified_global x certified_patch`, trois seeds et un
  bottom-k ou plusieurs phases à la première taille soutenable. Les tailles
  `4k/8k/16k` servent ensuite à mesurer les pentes du **certificateur
  implémentable** ; `32k` ne devient utile que si caps et budget laissent la
  cohorte jugée non vacante. La stabilité actuelle à une seed et une phase ne
  fige rien.
- **V81 — oui, conserver fates et ledger.** Ils portent exact-once,
  `pending`, les caps, la distinction rôle/support et la fermeture globale ;
  leur valeur est correctness/provenance, pas un gain à défendre. Le ledger q3
  pondéré est désormais factorisé en temps linéaire en handles, donc il n'y a
  aucune raison de sacrifier cette preuve pour alléger le certificateur.

Les pourcentages `96,3--99,6 %` restent un diagnostic exact-common conditionnel
aux blocs marginaux non capés de cette sonde. Ils ne démontrent ni « global
92--98 % du travail produit », car ce dénominateur réemploie V74 censuré, ni
« patches 0,4--3,7 % », car l'absence de témoins communs n'est pas leur
nécessité et leur présence n'est pas leur certifiabilité. La note porte encore
le pin historique `1ff39ab9`; un rebuild local au vrai pin `9a51a729` reproduit
la ligne `uniform` (`867/31`, `1219444/47508`) en `18,8 s`, mais aucune commande
ni sortie brute n'est publiée comme reçu.

Le split de `C` n'est pas « identique » dans tous ses usages. Employé seulement
pour mieux reconnaître `EMPTY`, il répète effectivement le mécanisme peu
prometteur du raffinement post-séparation. Employé sous budget pour résoudre le
résiduel avec `existence=NONEMPTY` et `depth=ALL_DEEP`, il peut supprimer des
rescans et doit être évalué avec le contrefactuel V74. Le prochain geste de la fibre reste
le schéma d'état et le center-cover counter-only sur **tous** les blocs, sans
masquage : l'oracle exhaustif stratifie ensuite les cohortes non capées. En
parallèle, la relève exacte des histogrammes saturés reste le premier candidat
à une activation produit, car elle attaque la boucle quadratique connue. Le
routeur final entre `g_AB` et les histogrammes attend l'ablation des deux
ordres. Si la lane `EMPTY` est retouchée, requalifier `OwnerD2Exact` comme
baseline de lentille avant tout split ; celui-ci ne vient que si l'incertitude
résiduelle rembourse ses nœuds.

À `n<=14`, l'oracle attendu vérifie le prédicat idéal, l'implication $W_3$, le
ledger point par point et le compte exact des appels avec arrêt anticipé. Le
ledger global inclut aussi les rectangles morts par cœur et ferme
$3\binom{n_u}{3}$ ; la seule cohorte `AliveRect` ne suffit pas.

## Provenance exacte de la fibre

La WSPD partitionne les arêtes non ordonnées. Pour un rectangle `r=(A,B)`, les
handles de `rect_cover_handles` forment une antichaîne disjointe dans la
fenêtre proposée. Ils partitionnent des **rôles** `(arête, tiers)`, pas encore
les triangles acceptés. Identités distinctes, acuité puis vrai `EdgeKey`
restent des filtres obligatoires.

Comme $A\cap B=\varnothing$, la masse d'un bloc vaut :

$$m(A,B,C)=\lvert A\rvert\lvert B\rvert\lvert C\rvert-\lvert A\cap C\rvert\lvert B\rvert-\lvert B\cap C\rvert\lvert A\rvert.$$

La somme globale ferme $3\binom{n_u}{3}$ rôles, jamais
$6\binom{n_u}{3}$. Longueur maximale puis `EdgeKey` conservent exactement un
rôle par triangle. Le complément des handles reçoit explicitement le fate
`DEAD_OUTSIDE_WINDOW`. Sa sûreté q3 vient de l'identité suivante pour tout
tiers dont `ab` est l'arête maximale :

$$\lVert 2c-a-b\rVert^{2}=2\lVert c-a\rVert^{2}+2\lVert c-b\rVert^{2}-\lVert b-a\rVert^{2}\leq3\lVert b-a\rVert^{2}.$$

Le ledger d'un rectangle est donc `sum(handle_mass) + outside_mass =
|A||B|(n_u-2)`. Une capacité atteinte conserve le rôle en `pending` ; elle ne
le perd pas et ne développe pas silencieusement le produit.

## Réfutation permanente des $8^{3}$ coins

Prendre les deux extrémités, le segment de tiers et le témoin suivants :

```text
a  = (10, 0, 0)       b  = (50, 0, 0)
x- = (20,24, 0)       x0 = (30,24, 0)       x+ = (40,24, 0)
z  = (30,25, 0)
```

Les trois triangles sont strictement aigus et `ab` est leur arête maximale
stricte. Pourtant :

```text
q3_power(a,b,x-;z) = -57 600 000
q3_power(a,b,x0;z) = +38 400 000
q3_power(a,b,x+;z) = -57 600 000
```

Les deux coins distincts de la boîte plate `C` disent « intérieur strict » et
son point entier intérieur dit « extérieur strict ». `q3_power` n'est pas
séparément convexe dans le carrier. La fixture est préparée dans
`mhgp5_q3_skinny_center` et passe localement ; elle doit être épinglée avec le
prochain delta fonctionnel.

## Différentiel v4 : ne pas rebaptiser P1

Les 64 patches et le center-cover ne sont pas nouveaux. La v4 documente déjà
`P1`, avec ce pavage, et rapporte pour `P1a center-cover` des pentes rouges
`2,104/1,896`. Elle conclut que le certificat partait encore de la paire et
travaillait contre un univers beaucoup plus gros que la sortie. Reprendre les
patches seuls rouvrirait donc une piste réfutée sans répondre à son motif
d'abandon.

Le delta à falsifier est plus précis : une WSPD partitionne les arêtes en
rectangles ; `g_AB` ne parcourt les témoins qu'une fois par rectangle ; les
handles `C` ne font ensuite que masquer ces patches et produire `t_C`; les
bitsets des facteurs donnent `P[t_C]` sans matérialiser `A x B x C`. En q4,
les seuils mono-handle `s_H` groupent le résiduel avant le terminal axial ; le
stream exact `t_CD` reste un oracle sous cap. Cela constitue une architecture
différente à requalifier, pas une preuve de meilleur exposant. Le pire cas
demeure ouvert dans les visites `MIXED`, les couples q3 survivants, les faces
q4 et leur census.

Le reçu différentiel publie donc, sur les mêmes familles et tailles que P1,
`wspd_rectangles`, `g_ab_witness_node_pops`, `handle_masks`, `sum_P_t_c`,
`q3_weighted_roles_proposed`, classes `s_H`, faces q4, groupes axiaux et les
propositions réellement transmises au terminal. L'oracle publie séparément
`q4_handle_pairs_streamed`. Une baisse de constante ne rouvre pas la route : la pente de chacun
de ces générateurs, puis mur/HWM à 50 k, doit réfuter le motif v4. Aucun code ni
reçu v4 n'est importé ; seules ses contre-fixtures et ses mesures sont épinglées
comme différentiel.

## Certificat sûr : center-cover conditionné par $C$

Un fallback simple évalue la forme polynomiale exacte par intervalles entiers
dirigés sur `A,B,C,W`. `power_upper < 0` crédite un nœud témoin,
`power_lower >= 0` le rejette, et `MIXED` subdivise ou rend `pending`. Cette
route est sûre mais risque d'être lâche à cause des dépendances d'intervalles.

La forme à encadrer est exactement celle de `q3.hpp`. Avec `d=b-a`, `u=c-a`,
`y=z-a`, `D=d.d`, `E=u.u`, `F=d.u`, `G=D*E-F*F` et
`W=E*(D-F)*d+D*(E-F)*u`, poser :

$$\Pi(a,b,c;z)=G(y\mathbin{\cdot}y)-y\mathbin{\cdot}W.$$

Construire les intervalles par `add/sub/mul/square`, le carré prenant zéro
comme minimum s'il traverse zéro. L'identité de Gram autorise à intersecter la
borne de $G$ avec `[0,+inf)` sans perdre de valeur réelle. `Pi_upper < 0`
signifie `ALL_STRICT_INTERIOR`, `Pi_lower >= 0` signifie seulement que ce nœud
ne fournit aucun témoin, et tout autre résultat reste `MIXED`. Cette voie sert
aussi d'oracle indépendant du raccord par patches.

La route prioritaire réemploie les patches entiers déjà spécifiés, avec un seul
parcours témoin par rectangle :

1. construire une fois les 64 patches q3 `Q_j` du rectangle `(A,B)` et leur
   masque de médiatrice `AB` ;
2. pour chaque handle `C`, conserver le bit `j` seulement si les trois
   intervalles de médiatrice `AB`, `AC`, `BC` contiennent zéro ; un intervalle
   est impossible exactement si `L32 > 0` ou `U32 < 0`, tandis que toute
   égalité reste faisable ;
3. former l'union des masques non vides, puis parcourir les témoins une seule
   fois pour construire, hors `A union B`, les seuls crédits `g_AB[j]` utiles,
   tels que `max(L32(Q_j,A,W), L32(Q_j,B,W)) > 0` ;
4. si le masque d'un handle est vide, aucun support réel n'existe dans son
   bloc ; sinon condenser ses bits dans `t_C` et interroger les bitsets de
   facteurs ;
5. ne lancer un parcours dépendant de `C`, par exemple avec
   `L32(Q_j,C,W)>0`, que comme renforcement mesuré sur le résiduel.

Ces trois tests médiateurs séparés ne prouvent pas qu'un même triplet réalise
simultanément les égalités. Ils conservent donc un sur-ensemble, ce qui est le
bon sens fail-open. Pour q3 ils ignorent aussi la coplanarité du centre
distingué. Cette perte peut diminuer le prune, jamais créer une fausse mort.

L'implémentation de `L32` peut rester courte. Par axe, la fonction
`dist(t,P)^2 - max((t-x0)^2,(t-x1)^2)` est concave ; son minimum sur
l'intervalle du patch est donc atteint à l'une de ses deux extrémités. Les
bornes de `P` ajoutées dans la note antérieure sont inutiles mais inoffensives.
Le produit de boîtes est connexe et la différence de puissances continue : son
image est exactement l'intervalle `[L32,U32]`. Zéro dans cet intervalle prouve
seulement une égalité relaxée pour cette médiatrice, jamais les trois à la fois.

La réutilisation de `g_AB[j]` est sûre : tout vrai centre de `(a,b,c)` reste
dans au moins un bit du masque de son handle et, pour ce centre, le test témoin
positif relativement à `A` ou `B` prouve une puissance strictement intérieure,
indépendamment de `C`. Retirer d'autres patches ne change ni `Q_j`, ni son
antichaîne. `witness_node_pops` n'est inscrit qu'une fois par rectangle et ne
doit jamais croître par rescan multiplicatif en `C`; sa valeur peut néanmoins
dépendre de l'union des patches demandés par les handles. La réutilisation
cesse si `(A,B)`, la grille, la lane ou le pavage changent.

Le cache porte aussi `computed_patch_mask`. Un `g_AB[j]==0` avec ce bit absent
signifie « non calculé », jamais « calculé et nul ». La valeur zéro reste
fail-open pour la décision, mais confondre ces états rendrait faux les reçus,
les réemplois et tout calcul de coût incrémental.

Les crédits de patches différents ne sont ni sommés ni unis. Ils peuvent en
revanche utiliser des témoins différents, ce qui est précisément le gain que
le compte commun du probe ne mesure pas.

## Contrat de $h_0,h_a,h_b,h_c$

Soit `F` l'ensemble non vide des triplets distinct-ID, aigus et possédés du
bloc, et `I_t` l'ensemble des sites de puissance strictement négative pour
`t`. Comme `C` peut recouvrir les extrémités, les domaines physiques disjoints
sont :

$$D_A=A,\qquad D_B=B,\qquad D_C=C\setminus(A\cup B),\qquad D_0=P\setminus(A\cup B\cup C).$$

Les crédits complets s'écrivent :

$$H_0=D_0\cap\bigcap_{t\in F}I_t,\qquad H_A(a)=D_A\cap\bigcap_{t\in F:\,t_A=a}I_t,\qquad H_B(b)=D_B\cap\bigcap_{t\in F:\,t_B=b}I_t,\qquad H_C(c)=D_C\cap\bigcap_{t\in F:\,t_C=c}I_t.$$

L'univers est celui des rangs de positions uniques, avec `A` et `B` disjoints.
Ces ensembles définissent les **fibres sémantiques exactes** ; noter leurs
cardinalités $h_0^F,h_a^F(a),h_b^F(b),h_c^F(c)$ évite de les confondre avec
les tableaux historiques du code.

Une fibre ou une tranche fixant `a`, `b` ou `c` sans complétion valide reçoit
zéro, jamais une cardinalité vacante. Pour tout
`t=(a,b,c)` de `F`, les quatre ensembles sont disjoints et inclus dans `I_t` :

$$\mathrm{depth}(t)\geq h_0^F+h_a^F(a)+h_b^F(b)+h_c^F(c).$$

`corner_histograms` ne calcule pas ces intersections exactes dépendantes de
`F`. Il calcule des ensembles facteurs certifiés par $W_q$, une fois pour
`(A,B)`. Leurs cardinalités, notées conceptuellement
`h_a_factor(a),h_b_factor(b)`, sont des **minorants** de $h_a^F,h_b^F$ pour
toute fibre compatible. Par convention historique, le reste de cette note et
le code les appellent encore `h_a,h_b`; « exact » signifie seulement « compte
exact du sous-ensemble certifié par le facteur », jamais « cardinalité exacte
de la fibre `F` ». Après un split de `C`, les ensembles exacts de la fibre
enfant peuvent grandir, tandis que les facteurs cachés restent inchangés et
sûrs : ils s'héritent comme minorants, pas comme comptes exacts de l'enfant.

Le premier incrément doit néanmoins omettre $h_c$. Il réutilise les tableaux
`h_a(a),h_b(b)` de $W_3$, déjà calculés une fois par rectangle, et le crédit
`g_AB[j]` extérieur à `A union B`. Ce dernier n'est **pas** le vrai $h_0$ à
quatre strates : il peut contenir d'autres positions de `C`. Il reste
additionnable à $h_a,h_b$ tant que $h_c$ est absent. Le carrier effectivement
choisi ne peut pas être crédité dans le patch de son vrai centre, car sa
puissance y vaut zéro. Ces crédits restent sûrs même si `C` recouvre `A` ou
`B` : un tiers aigu vérifie `H<0`, tandis qu'un témoin $W_3$ exige `H>0`.

Une fixture interdit de réduire ces tableaux à deux scalaires. Avec
`a0=(4,2,0)`, `a1=(3,2,0)`, `b=(0,0,0)` et `c=(0,3,0)`, les deux triangles
sont aigus et `(ai,b)` est strictement maximal. `a1` est intérieur à la boule
de `a0,b,c`, alors que `a0` est extérieur à celle de `a1,b,c` :
`h_a(a0)=1` et `h_a(a1)=0`.

Le `tb` actuel n'est pas ce central additionnable : il exclut seulement les
sites apparaissant dans un triplet valide, pas toutes les plages `A` et `B`.
Un point inactif de `A` peut donc aussi vivre dans `h_a`. Le prochain probe
publie `central_outside_AB` ou conserve les rangs de positions.

`AliveRect::core` et `g_AB[j]` peuvent reconnaître le même site. Sans
identités, leur seule composition sûre est `max(core_AB,g_AB[j])`. Avec au
plus huit crédits dans un rectangle q3 vivant,
`collect_universal_ids`, malgré son nom, retourne actuellement des rangs de
positions `i32`; il permet de former explicitement l'union puis de chercher
seulement de nouvelles positions. Aucun crédit n'est hérité après un split
dont les patches changent.

Une condition simple de mort du patch `j` est :

$$\max(h_{\mathrm{core}},g_{AB,j})+\min_{a\in A}h_a(a)+\min_{b\in B}h_b(b)\geq h_3.$$

Tous les patches faisables doivent la satisfaire pour tuer le bloc entier.
Sur le domaine complet, le critère exact reste le minimum couplé de
`h0+ha+hb+hc` sur `F`. Une convolution des histogrammes est exacte seulement
sur un produit cartésien ; acuité et owner couplent généralement les rôles.
Elle donne sinon un surcompte de travail, pas une partition des survivants.

Un futur `h_c(c)` prend ses témoins dans `C` privé de `A union B`, et le vrai
central devient alors extérieur à `A union B union C`. `g_AB[j]` et $h_c(c)$
peuvent partager un autre site de `C` : avant de les composer, conserver les
rangs de positions et prendre leur union, ou reconstruire `h0_j(C)` hors `C`.
La fixture future minimale prend `a=(0,0,0)`, `b=(4,0,0)`, `c=(2,3,0)` et
`z=(2,1,0)`,
avec `c,z` dans le même `C` : `z` appartient à $W_3(a,b)$ et est strictement
intérieur à la circumboule de `(a,b,c)`, donc peut vivre à la fois dans
`g_AB[j]` et $h_c(c)$. L'auto-jointure de $h_c$ est capée par les 32 positions
d'un handle mais peut encore payer 1024 couples par bloc. Elle vient seulement
après les prunes `EMPTY/NONE_OWNER`, médiatrices et central, sur le résiduel
mesuré.

La version autoritaire transporte pour chaque source un
`CappedWitnessPosSet<h3>` trié de rangs `i32` de positions uniques, l'unité
conceptuelle `GeometryIndex`. Deux méthodes
appliquées au même domaine se composent par union ; sans positions, seulement
par `max`. Le profil courant refuse les positions dupliquées, donc position et
`PointId` sont en bijection, mais le tie-break d'owner ne change pas l'unité du
ledger témoin. Une future levée de ce refus exige un mapping explicite. Après
un split qui change les patches, un enfant ne transporte que les positions
explicitement revalidées sous son propre certificat ; sa recherche les ignore
ensuite. L'addition `parent_count + fresh_count` est interdite.

Si l'on veut récupérer dans $h_c$ les positions de $C$ qui recouvrent $A$ ou
$B$, il faut d'abord normaliser l'auto-jointure ordonnée. Pour un nœud
`N=(L,R)` :

$$\mathrm{Ord2}(N)=\mathrm{Ord2}(L)\mathbin{\dot\cup}(L\times R)\mathbin{\dot\cup}(R\times L)\mathbin{\dot\cup}\mathrm{Ord2}(R).$$

Les tiers extérieurs et les deux auto-jointures internes ferment alors la
masse exacte $\lvert A\rvert\lvert B\rvert(n-2)$ sans diagonale. Une somme de
cardinalités sur `C=root` sans cette normalisation est fausse.

Après saturation à `need=h3-h0`, un domaine restant réellement cartésien et
disjoint autorise les histogrammes `N_A[i],N_B[j],N_C[k]` et :

$$M_{surv}=\sum_{i+j+k<need}N_A[i]N_B[j]N_C[k].$$

La convolution coûte `O(|A|+|B|+|C|+need^2)` avec `need<=9`. Elle rend donc la
**combinaison** des crédits constante, pas leur calcul. Le verrou courant reste
`corner_histograms`, en `O(|A|^2+|B|^2)`. Sa relève parcourt les témoins par
nœuds, crédite un sous-arbre certifié, scinde `MIXED` et s'arrête après neuf
positions. Son coût n'est quasi linéaire que si le nombre de nœuds `MIXED` le
reste ; c'est une porte de mesure, pas une borne reçue.

## Relève directement intégrable de `corner_histograms`

Le prochain incrément utile ne demande ni nouvelle WSPD, ni nouveau carrier.
Il remplace d'abord chaque ligne quadratique de l'histogramme par une requête
saturée sur l'arbre spatial déjà construit :

```cpp
struct FactorQueryStats {
  u64 endpoint_queries = 0;
  u64 node_visits = 0;
  u64 none_prunes = 0;
  u64 bulk_nodes = 0;
  u64 bulk_positions = 0;
  u64 leaf_tests = 0;
  u64 diagonal_splits = 0;
  u64 saturated_endpoints = 0;
};

struct FactorQueryScratch {
  std::vector<NodeRef> stack;
};

u8 factor_witness_count_sat(const CloudIndex& ix, Lane lane, i32 support,
                            NodeRef partner, NodeRef witness_factor, u8 cap,
                            FactorQueryScratch* scratch,
                            FactorQueryStats* stats);
```

Pour `h_a(a)`, `support=a`, `partner=B` et `witness_factor=A`; pour
`h_b(b)`, échanger les rôles. Construire une fois par endpoint la boîte
ponctuelle `S={support}`, `Box(partner)` et, pour q3/q4, leur `core_ball`.
Préconditions : `support` est un rang valide de position unique dans
`witness_factor`, les plages de `witness_factor` et `partner` sont disjointes,
les deux `NodeRef` sont valides et non vides, `lane` est q2/q3/q4, `ix` est
valide sans positions dupliquées, `scratch` et `stats` sont non nuls, et le
scratch est vidé à l'entrée. Pour un `AliveRect`, tester d'abord
`h_core>=h_q`; ce cas tue le rectangle sans soustraction. Sinon calculer
`need=h_q-h_core` en `u64`, vérifier qu'il tient dans `u8`, et retourner
immédiatement si `cap==0`. La descente suit exactement ces règles :

1. si le nœud témoin contient `support`, le scinder avant tout crédit ; à la
   feuille diagonale, ne rien compter ;
2. si `hmax4_boxes(S,Box(partner),Box(Z)) <= 0`, rejeter `Z` pour toutes les
   lanes ;
3. en q2, `hmin_boxes(S,Box(partner),Box(Z)) > 0` crédite tout `Z`; en q3/q4,
   `box_vs_ball(Box(Z),core_ball) > 0` fait de même ;
4. tout autre nœud interne est `MIXED` et se scinde ; en particulier
   `box_vs_ball < 0` ne prouve pas que `Z` est hors du fuseau complet ;
5. une feuille non diagonale garde l'autorité actuelle
   `universal_over_corners(lane,S,Box(partner),z)` ;
6. chaque crédit est borné par `cap-count` et la requête s'arrête à `cap`.

Sous le profil sans positions dupliquées, un crédit de nœud ajoute sa
cardinalité de positions, jamais un poids de multiplicité. Les nœuds crédités
forment une antichaîne et aucun descendant n'est visité après leur crédit. Le
résultat autoritaire est `min(cap,compte_actuel)` ; `count==cap` signifie
seulement « au moins `cap` », pas que le compte complet est connu. Employer
`cap=need` suffit à toutes les décisions actuelles. L'addition
`h_core+h_a+h_b` reste sûre parce que le contrat courant place `h_core` hors
`A union B`, tandis que `A` et `B` sont disjoints ; changer un de ces domaines
exigerait des rangs de positions et une union explicite.

Après la porte de parité, les deux facteurs ne doivent pas être interrogés
aveuglément au même plafond. Poser `N` au plus grand seuil résiduel encore
utile et commencer par le facteur au plus faible devis, en première
approximation celui qui a le moins d'endpoints. Interroger ce facteur avec
`cap=N`, puis prendre le minimum saturé `m`. Si `m==N`, tous les couples sont
déjà tués et l'autre facteur n'est jamais interrogé. Sinon, interroger le
second facteur avec `cap=N-m`. Les valeurs sous leur cap restent exactes et les
valeurs saturées suffisent à décider tous les seuils `t<=N`. Le reçu ajoute
`second_factor_queries_avoided` et confronte l'ordre `A puis B` à `B puis A` ;
le choix de coût ne change ni les comptes ni l'ordre canonique des survivants.

« Exact » signifie ici égal au `corner_histograms` actuel sur la boîte continue
du partenaire. Cet histogramme est lui-même un minorant suffisant pour les
partenaires finis ; la requête ne prétend pas compter leur intersection
ponctuelle exacte. Distinguer `bulk_positions_certified` de
`bulk_positions_consumed=min(size(Z),cap-count)`. Les cumuls de visites et de
positions peuvent être cubiques sur la campagne : les receipt-er en `u128`, ou
les saturer avec un bit d'overflow.

La complexité d'un rectangle devient `O(|A|+|B|+V_A+V_B)`, où `V_A,V_B`
comptent **toutes** les visites de nœuds des requêtes d'endpoints. Elle peut
encore être quadratique si presque tout reste `MIXED`; ce compteur est donc la
porte de réfutation de l'idée. Dans les régimes où les boules-cœurs créditent
des sous-arbres ou où neuf témoins sont trouvés tôt, elle évite réellement les
auto-produits complets.

Le second étage doit retirer aussi le produit `A x B` déjà mort. Comme les
comptes sont saturés à `need`, construire les bitsets cumulatifs
`B_lt[t]={b : h_b(b)<t}` pour `1<=t<=need`, ainsi que la liste triée
`nonzero_words[t]` des indices de mots non nuls. Pour chaque `a`, les seuls
partenaires à émettre sont les bits de `B_lt[need-h_a(a)]`; un seuil nul émet
rien. Parcourir seulement `nonzero_words[t]`, puis les bits de poids faible à
fort, conserve l'ordre canonique `ua`, puis `ub`. Balayer tous les mots pour
chaque `a` laisserait un coût caché `O(|A|*ceil(|B|/64))` et est donc interdit.
Les compteurs des ancres mortes sont mis à jour en masse, sans construire ces
ancres. Pour conserver la sémantique actuelle des statistiques, appliquer
`anchors += total_pairs` et `anchors_killed_hist += killed_pairs` avant de
parcourir les seuls survivants, puis supprimer le `++anchors` historique de
cette boucle. L'invariant local est `delta(anchors)==total_pairs`.

Le ledger exécutable est :

```text
total_pairs    = |A| |B|
killed_pairs   = #{(a,b) : h_a(a)+h_b(b) >= need}
survivor_pairs = #{(a,b) : h_a(a)+h_b(b) <  need}
total_pairs    = killed_pairs + survivor_pairs
```

Le conditionnement par `C` ne demande pas un nouveau produit. Pour chaque
patch faisable `j`, poser `credit_j=max(h_core,g_AB[j])` et
`t_j=max(0,h_q-credit_j)`. Pour le masque non vide `M_C` d'un handle, le seuil
unique est :

$$t_C=\max_{j\in M_C}t_j=\max\left(0,h_q-\min_{j\in M_C}\max\left(h_{\mathrm{core}},g_{AB,j}\right)\right).$$

Un couple d'ancre `(a,b)` n'est pas tué par ce certificateur pour le handle
`C` exactement lorsque `h_a(a)+h_b(b)<t_C`. Il suffit donc de construire le
petit histogramme cumulatif
`P[t]=#{(a,b):h_a(a)+h_b(b)<t}` pour `0<=t<=h_q`, ou de réutiliser directement
les bitsets `B_lt[t]`. `P[t_C]==0` ferme le travail de ce handle sans émettre
`A x B`; `sum_C P[t_C]` donne un compteur structurel de couples proposés, pas
une masse de supports valides, car identités, acuité et owner restent dans leur
ledger séparé. Cette réduction est exacte pour le **certificateur** et ne
matérialise jamais `A x B x C`.

### Fermeture exacte du ledger pondéré q3

La masse brute de rôles ne se déduit pas du seul `P[t]` lorsque `C` recouvre
`A` ou `B`, mais elle se **factorise** sans énumérer les couples survivants.
S'il n'existe aucun handle actif, poser `N=0` et une masse proposée nulle ;
sinon poser `N=max_C(t_C)<=h3`, saturer `h_a,h_b` dans le bin `N`, puis définir
`A_i=#{a:h_a(a)=i}`, `B_j=#{b:h_b(b)=j}` et les préfixes `A_<r,B_<r`.
Pour tout `t<=N`, la saturation conserve exactement le prédicat `<t` et :

$$P[t]=\sum_i A_i B_{<t-i}.$$

Pour un handle `C`, poser `A_i^C=#{a in A intersect C:h_a(a)=i}` et
`B_j^C` symétriquement. La masse exacte de **rôles bruts distinct-ID** laissée
par le certificateur est :

$$M_C(t)=\lvert C\rvert P[t]-\sum_i A_i^C B_{<t-i}-\sum_j B_j^C A_{<t-j}.$$

La preuve est une simple double incidence. Le premier terme attribue
`|C|` tiers à chaque couple survivant ; les deux sommes retirent respectivement
les seules diagonales `c=a` et `c=b`. Elles sont disjointes parce que
`A intersect B` est vide, donc aucun terme de réaddition n'existe.

Il n'est même pas nécessaire de conserver un histogramme par handle. Pendant
le stream, accumuler par seuil `t_C` les trois petits tableaux
`W_t=sum|C|`, `X^A_ti=sum A_i^C` et `X^B_tj=sum B_j^C`, puis calculer :

$$M_{\mathrm{proposed}}=\sum_t W_tP[t]-\sum_{t,i}X^A_{t,i}B_{<t-i}-\sum_{t,j}X^B_{t,j}A_{<t-j}.$$

Les handles sont une antichaîne : leurs intersections avec les plages de `A`
et `B` sont disjointes. Former tous les bins coûte donc
`O(|A|+|B|+k+N^2)` par rectangle, avec `N<=9`, et une mémoire `O(N^2)` hors
tableaux d'endpoints. Aucun `A x B` ni `A x B x C` n'est parcouru. Un test
brut local sur `218700` configurations exhaustives petites, puis `100000`
configurations aléatoires, n'a trouvé aucune divergence avec la somme explicite
des poids. Ce rejeu d'audit ne remplace pas la future fixture CTest.

Ce nombre inclut encore les triplets qu'acuité ou owner rejetteront : ce n'est
ni une masse de supports valides, ni un nombre de candidats. Le ledger local
peut maintenant fermer exactement
`full=outside+empty+pending+depth_killed+proposed`, puis la tape complète ferme
`3*choose3(n_unique)` en ajoutant les rectangles morts avant `AliveRect`.
Les cinq termes sont des **actions primaires disjointes** sur la même masse
brute ; les `proof_kinds` peuvent se recouvrir mais ne créent pas une seconde
inscription. Toutes les multiplications des totaux positifs sont promues en
`u128` avant le produit. Les formules avec corrections sont accumulées en
`i128`, vérifiées non négatives, puis converties en `u128`; une soustraction
non vérifiée en arithmétique non signée est interdite. Un parent et ses enfants ne figurent jamais
simultanément dans ce ledger ; un split remplace le parent atomiquement. Si le
seuil varie à l'intérieur d'un handle, il faut le scinder, le stratifier ou le
laisser `PENDING`.

Cette fermeture règle la comptabilité pondérée q3, pas nécessairement son
terminal : l'émission effective peut encore être proportionnelle à
`M_proposed`. Elle permet toutefois de mesurer et de tuer en vrac sans cacher
un produit derrière le mot « ledger ».

ABI minimale proposée, avec tableaux de taille dix fixés au profil courant :

```cpp
struct Q3FactorBins {
  u8 cap;
  std::array<u64, 10> a, b, a_lt, b_lt;
  std::array<u128, 10> pair_lt;
};

struct Q3RoleLedger {
  u128 full, outside, empty, pending, depth_killed, proposed;
  bool closed;
};
```

Le scratch ajoute `weight_by_t[10]`, `a_intersection_by_t[10][10]` et
`b_intersection_by_t[10][10]`. `close_q3_roles` vérifie `RectId`, lane, cap,
antichaîne, epoch de grille et plages avant le premier cumul. Les préfixes
valent zéro pour un indice inférieur ou égal à zéro et leur total pour un
indice supérieur au cap ; cette convention retire toute branche fragile sur
`t-i`.

Elle rend aussi les deux ordres de coût explicites. Dans l'ordre center-first,
former les handles et leurs masques, calculer `g_AB` seulement sur leur union,
puis poser `N=max_C t_C` avant les requêtes de facteurs. Dans l'ordre
hist-first, calculer d'abord les facteurs au seuil de cœur, supprimer le
rectangle si leur minimum suffit, puis ne scanner `g_AB` que sur l'union des
patches des handles encore pertinents. Les runs appariés choisissent le routeur
sur les unités V74 ; aucun des deux ordres n'est un contrat sémantique.

Hors requêtes d'arbre, ce filtre coûte
`O(|B|+need*ceil(|B|/64)+|A|+survivor_pairs)` avec l'index de mots non nuls,
`need<=9` en q3 et `need<=8` en q4 au profil courant. Sans cet index, la borne
honnête est `O(|A|*ceil(|B|/64)+survivor_pairs)`. Le pire cas reste
proportionnel au nombre de survivants, ce qui est nécessaire puisque le
terminal les consomme ; le cas « toutes les paires tuées » ne parcourt plus
`A x B`.

« Streamé sans catalogue » ne suffisait pas : l'ancien stream `CD` bornait la
mémoire mais gardait `Theta(k^2)` décisions et jusqu'à
`Theta(k^2*|A|*|B|)` continuations. Il devient un oracle/ablation borné. Le
ledger q3 pondéré est maintenant `O(k)` après les facteurs, mais son terminal
peut encore transmettre `Theta(sum_C P[t_C])`. Le préfiltre q4 par classes de
`s_H` retire le carré des handles ; son terminal axial paie encore les faces
ternaires résiduelles, les groupes de racines, le scan de témoins et le census.
Le reçu publie séparément ces générateurs et leurs pentes. Les masses
conditionnées restent en `u128` : à dix millions,
`4 M*(3 M)^2=3,6e19` dépasse déjà `u64`.

À chaque réemploi du scratch, remettre tous les mots à zéro, reconstruire les
listes `nonzero_words` et masquer les bits hors plage du dernier mot. Un bit
périmé ne doit pouvoir ni émettre un faux partenaire, ni fausser le ledger.

Le contrat de prédicats et de compteurs doit être partagé par le CPU, les lanes
batched et la préparation device, afin de ne pas créer trois sémantiques
divergentes. Le corps à `std::vector` reste hôte ; une future primitive CUDA
emploiera son propre stockage sous les mêmes portes. Le premier raccord reste
CPU-reference et counter-only : comparer
chaque nouvelle valeur à `min(need,ancienne_valeur)` et l'ordre des survivants
à l'ancienne double boucle sur les trois lanes, puis activer la nouvelle
énumération derrière une option explicite. Compteurs minimaux :
`hist_endpoint_queries`, `hist_node_visits`,
`hist_leaf_tests`, `hist_bulk_positions`, `hist_saturated_endpoints`,
`hist_total_pairs` et `hist_survivor_pairs_iterated`.

Les fixtures permanentes couvrent un bulk non vide, tout `MIXED`, la diagonale,
un prune `hmax4<=0`, un fallback où seules les feuilles concluent, la coquille
stricte, les saturations à 1 et au seuil maximal, puis comparent masse et ordre
exacts des survivants. Une fixture doit avoir
`total_pairs>0`, `survivor_pairs=0` et aucune itération d'ancre. Les mutants
retirent respectivement le split diagonal, descendent après un bulk, utilisent
`cap-1` et ferment la coquille. C'est seulement après ces portes que la même
primitive alimente `g_AB[j]`; la convolution avec `C` reste interdite tant que
le sous-domaine n'est pas prouvé cartésien après acuité, owner et diagonales.

## Ordre d'implémentation transmis à Claude

```text
RectId(A,B), patches et positions du core
  -> h_a/h_b saturés + parité avec corner_histograms + bitsets B_lt[t]
  -> raw_cover/witness/seed handles C + masse de rôles + fates + masques
     AB/AC/BC
  -> union des patches actifs + global_common (succès=prune, échec=UNKNOWN)
  -> reprendre le même front pour remplir g_AB, sans rescan de racine
  -> seuil t_C + ledger q3 pondéré agrégé par seuil, sans A x B
  -> q3 : émission sparse seulement sur le résiduel
  -> q4 : classes s_H, carriers ternaires résiduels, puis terminal axial Top-r4
```

Le probe reste counter-only. Il se streame par rectangle ; il ne matérialise
pas une liste globale de millions de blocs et ne relance pas un census complet
pour chaque `C` au premier essai. En q3, un handle mort comme **seed** reste
disponible comme témoin ; seule sa capacité d'émission est filtrée. Quatre
contrats doivent être nommés séparément :

1. `carrier_partition` en q3 ou `completion_partition` en q4 est l'antichaîne
   disjointe et complète des sommets de support possibles ; le cover fermé de
   coefficient 3 est la fenêtre naturelle à requalifier ;
2. `seed_capability` est un état attaché à cette partition, pas une seconde
   partition à joindre ;
3. `certificate_source` est une source de positions sans doublon dont chaque
   crédit est prouvé sonore. Elle peut être **incomplète** : oublier un témoin
   ne fait que diminuer un minorant `g/h` ;
4. `exact_census_source` est complète pour l'absence, le rang axial ou le
   census autoritaire. En q4, le cover 3 ne suffit pas aux intérieurs : employer
   une descente de l'arbre entier, ou une fenêtre coefficient 4 dont la
   complétude a sa propre porte.

`raw_cover_handles` ne reçoit donc aucun de ces statuts par simple renommage.
Un même stockage peut implémenter plusieurs vues seulement si chacune cite son
théorème et sa porte. Sous le contrat différentiel actuel, élargir en q4 la
source de prune de 3 à 4 peut conserver la forêt tout en changeant
`digest_balls`; `mhgp5_q4_cover_fixture` l'établit déjà. Ce changement exige
une requalification explicite, pas une aliasation silencieuse. Le pseudo-flux
fixe l'ordre de développement après la parité des facteurs. Le hot path
conserve l'ablation center-first décrite plus haut : il ne doit pas payer
simultanément les deux ordres.

Un split de `C` raffine seulement ses vues de supports ; son parent reste dans
la source de certificats ou de census tant que cette vue n'est pas remplacée
explicitement. Pour un enfant, `Q/core/g_AB` et les **minorants facteurs**
`h_a/h_b` restent valides, `M_child` est inclus dans `M_parent` et
`t_child<=t_parent`; les facteurs capés au seuil parent restent donc
réutilisables. Ils ne deviennent pas les cardinalités exactes de la fibre
enfant. En revanche, un futur `h_c` scalaire ne s'hérite pas : son domaine
change avec le sibling ; filtrer les positions ou le recalculer. Un
split de `A` ou `B` change `RectId`, les patches et les domaines de
`h_a/h_b/g_AB` : tous ces crédits sont invalidés, sauf sets de positions typés
et explicitement revalidés sous le nouveau rectangle. Aucune cardinalité
scalaire du parent ne se transmet par héritage.

La porte exhaustive à `n<=14` vérifie chaque bloc pruné, le ledger des rôles et
les diagonales. Elle conserve les tangences `L32==0` et `U32==0`, vérifie que
le patch de tout circumcentre rationnel survit, tue les mutants qui unissent
des patches ou somment `core+g_AB`, et rend visible tout rescan témoin par `C`.
Le reçu publie patches visités/faisables, tests de médiatrices,
`witness_node_pops`, blocs entièrement morts, masse de rôles morte, blocs
capés, seeds et rescans réellement évités, coût ajouté, mur et HWM. Commande,
`HEAD`, worktree et sorties brutes sont obligatoires avant tout nouvel
exposant.

## Extension q4 : même tape, une strate de plus

Pour q4, `A x B x C` n'est pas encore le support complet : il reste un
quatrième sommet. Les deux sommets opposés à `AB` forment toutefois un rôle
**non ordonné** `{c,d}`. Les nommer `C`, puis `D`, est un ordre de parcours,
pas une seconde provenance sémantique. Le ledger reste
`Omega4={(e,{c,d})}`, de masse $6\binom{n_u}{4}$, et non douze occurrences par
support.

Le premier étage reste identique et sans rescan : la grille q4 et ses crédits
`g4_AB[j]` sont calculés une fois par `(A,B)`. Un premier handle applique
`AB/AC/BC`, puis un second ajoute `AD/BD/CD`. Employer les six tests séparément
resserre le sur-ensemble ; cela ne prouve ni leur réalisation simultanée, ni
la non-coplanarité, ni le bien-centrage, qui restent fail-open jusqu'au terminal
exact. Surtout, ne pas exiger que le handle visité en premier soit déjà la face
aiguë canonique : le terminal doit choisir le plus petit `PointId` parmi `c`
et `d` dont la face avec `AB` est aiguë, conformément à la règle exact-once
actuelle, ou employer un prédicat symétrique prouvé équivalent.

Une vue unique `support_handles` est donc incorrecte en q4. Le cover fermé de
coefficient 3 fournit, après sa porte, la `completion_partition` complète et
disjointe des deux sommets opposés possibles. `seed_capability` est attaché à
chacun de ses handles et peut valoir `YES/NO/UNKNOWN`; il ne crée pas une
seconde partition. Une `certificate_source` sonore, même incomplète, suffit
aux minorants `g4/h`, tandis que le ranking axial et le census exact lisent une
`exact_census_source` complète. `NONE_ACUTE` change seulement
`seed_capability`; il ne retire jamais un handle de la partition de complétion,
de la source de certificats ni du census. Une paire de handles passe le gate
seed si et seulement si
`seed_possible(C)||seed_possible(D)`, les inconnus restant possibles ; ce gate
n'est pas une preuve d'existence. Ne jamais joindre un stream orienté
`seed x completion` à `i<=j`, qui perdrait le cas où la complétion précède le
seed. La préparation `CellGrid` sépare de la même façon `witness_sites` et
`seed_sites`; réutiliser un seul vecteur rendrait un fate de seed destructif
pour le census exact.

La fixture q4 minimale suivante rend cette distinction exécutable :

```text
a=(0,0,0)  b=(6,0,0)  c=(1,-3,-1)  d=(1,1,-2)
```

`AB2=36` est strictement plus grand que les cinq autres carrés de longueur.
La face `ABc` est aiguë (`36<11+35`), tandis que `ABd` est droite
(`36=6+30`), et `q3_power(a,b,c;d)=180>0`. Le circumcentre a les coordonnées
barycentriques strictement positives `(12/49,22/49,25/98,5/98)` : le
tétraèdre est bien centré et `d` est une complétion q4 valide, bien qu'il ne
soit pas un seed valide. La porte permute l'ordre des deux handles et tue un
stream `i<j` qui suppose que le premier handle est le seed.

Deux fixtures ferment les futurs $h_c/h_d$. D'abord
`a=(0,0,0), b=(6,0,0), c=(2,3,0), d=(4,3,0)` possède `AB` strictement
maximale et deux faces `ABc/ABd` aiguës, mais `det=0` : la fibre q4 est vide,
donc toute intersection conditionnelle reçoit zéro et jamais la cardinalité
de son univers vacuant. Ensuite, pour le tétraèdre régulier
`a=(0,0,0), b=(4,4,0), c=(4,0,4), d=(0,4,4)` et le site
`z=(2,2,2)`, la profondeur vaut un. Si `c,d,z` vivent dans le même handle
diagonal `H`, le mutant qui donne deux fois le domaine `H` privé de `A union B`
produit `h_c+h_d=2`. La décomposition sûre pose le second domaine vide, ou
raffine `choose2(H)` en `choose2(L)`, `L x R`, `choose2(R)` afin de retrouver
deux domaines réellement disjoints sur le seul bloc croisé.

Le seuil q4 est `h4=smax-3`, soit huit pour `smax=11`, et ses patches sont ceux
de q4, jamais ceux de q3. Le crédit `g4_AB[j]` est sûr pour toute sphère du
patch parce qu'il se compare déjà au rayon porté par `a` ou `b`; il ne dépend
ni de `C`, ni de `D`. Les tableaux `h_a,h_b` employés ici sont ceux de $W_4$,
pas les crédits q3. Une mort q3 ne tue toujours pas une complétion q4.
`g4_AB[j]` porte explicitement des positions hors `A union B` avant toute
addition à `h_a+h_b`. Un même couple `{c,d}` peut survivre plusieurs patches, mais sa
masse n'est inscrite qu'une fois dans le ledger : les patches certifient une
décision, ils ne créent aucune provenance supplémentaire.

La réduction de seuil q3 se transporte sans catalogue de produits. Pour une
paire de handles visitée paresseusement, former son masque `M_CD` avec les six
médiatrices. Un masque vide ferme le bloc sans autre calcul ; sinon poser :

$$t_{CD}=\max_{j\in M_{CD}}\max\left(0,h_4-\max\left(h_{\mathrm{core}},g4_{AB,j}\right)\right).$$

Le même `P[t]` compte alors les couples d'ancres que ce certificateur laisse à
ce bloc de handles. Ce compteur ne vaut ni nombre de quadruplets ni nombre de
complétions. Les paires de handles restent streamées en `i<=j`; aucun tableau
`C x D` n'est conservé. Comme la médiatrice `CD` conditionne le masque final,
deux routes seulement sont honnêtes : scanner `g4_AB` une fois sur l'union
grossière des masques mono-handle, ou faire un premier stream des paires pour
former l'union exacte, scanner `g4_AB`, puis refaire un second stream pour les
continuations. Ce double passage doit apparaître dans les compteurs ; il ne
peut pas être caché sous un prétendu scan unique.

### Retirer le carré q4 avant la médiatrice `CD`

Le ledger pondéré possède lui aussi une forme exacte factorisée. Reprendre la
même construction de bins avec les `h_a,h_b` **q4** et le cap `h4`, puis poser :

$$L_H(t)=\sum_i A_i^H B_{<t-i}+\sum_j B_j^H A_{<t-j},\qquad X_{HK}(t)=\sum_{i+j<t}A_i^H B_j^K.$$

Pour deux handles distincts et disjoints `H,K`, puis pour la diagonale, les
masses laissées sont exactement :

$$M_{HK}(t)=\lvert H\rvert\lvert K\rvert P[t]-\lvert K\rvert L_H(t)-\lvert H\rvert L_K(t)+X_{HK}(t)+X_{KH}(t).$$

$$M_{HH}(t)=\binom{\lvert H\rvert}{2}P[t]-(\lvert H\rvert-1)L_H(t)+X_{HH}(t).$$

Elles ont été confrontées dans le même rejeu exhaustif et aléatoire sans
divergence. La borne de seuil ci-dessous a en plus passé `1048576` combinaisons
exhaustives de quatre patches. Ces replays ferment le calcul papier, pas une
porte produit. Évaluer les formules pour chaque `H,K` conserverait le carré que
l'on cherche à retirer.

La réduction sûre est plus grossière et beaucoup plus utile. Pour le masque
mono-handle de **complétion** `mu_H`, avant la médiatrice `CD`, poser `s_H=0`
si le masque est vide, sinon `s_H=max_{j in mu_H}(t_j)`. Ce masque conserve les
équations possibles du sommet opposé ; ce n'est jamais le masque d'acuité du
seed. Sinon la fixture `ABc` aiguë / `ABd` droite perdrait la complétion `d`.
Le masque exact d'une paire est inclus dans `mu_C intersect mu_D`; par
conséquent :

$$t_{CD}\leq\min(s_C,s_D).$$

Pour une ancre de score `u=h_a(a)+h_b(b)`, toute paire incidente à un handle
tel que `s_H<=u` est donc tuée par le certificateur, sans calculer `CD`. Les
seuls handles de complétion encore actifs satisfont `s_H>u`. Comme `h4=8`, il
n'existe que neuf classes de seuil ; les séparer aussi par le bit
`seed_possible` donne au plus dix-huit classes. Le stream des handles remplit
leurs unions et tailles en `O(k)`. Les bins d'intersection avec `A/B` coûtent
en plus `O(|A|+|B|)` avec une provenance `position -> handle/classe`, ou
réemploient des résumés par handle déjà construits ; ils ne sont pas gratuits.
Les formules traitent ensuite un nombre constant de paires de classes, avec un
facteur borné par `h4`. C'est un préfiltre q4 linéaire en handles **après ces
facteurs**, exact pour sa relaxation et fail-open pour la médiatrice omise.

L'implémentation n'a besoin que d'un tableau
`Q4HandleClass classes[9][2]`. Chaque case agrège `handle_count`, masse de
positions, union logique des handles, bins `A intersection U` et
`B intersection U`, plus le bit de non-vacuité du groupe. Ces unions ne
deviennent jamais des boîtes géométriques autoritaires : elles portent
seulement le ledger et la liste de carriers. Les décisions géométriques restent
celles des handles ou du terminal exact.

Avant le gate seed et les fates distinct-ID/non-vacuité, le nombre exact de
**slots de blocs grossiers** de cette relaxation est
`sum_s P[s]*(q_s*(q_s+1)/2+q_s*Q_greater_s)`. Ce n'est pas une masse de rôles
q4 ni le nombre de continuations non vides ; les formules pondérées sur les
unions de classes donnent la masse distinct-ID, puis les fates donnent les
continuations réelles.
Une paire de classes dont les deux capacités valent `seed_possible=false` est
écartée par le gate ; toute autre classe non certifiée reste `PENDING`, jamais
« existante » par construction.

Il ne faut pas raffiner ensuite jusqu'aux feuilles pour récupérer `CD`. La v4
a déjà mesuré cette auto-jointure `Sym2/CellPair` : `459477476` nœuds à
`n=4000`, contre `141468` couples plats. À l'inverse, son terminal axial a
remplacé `48791131` paires par `830044` groupes/racines sur sa cohorte, à sortie
q4 inchangée ; ces nombres sont un différentiel à requalifier, pas un reçu v5.
Le chemin constructif est donc : seuils mono-handle, carriers ternaires
résiduels issus de `seed_handles` avec `s_H>u`, puis `Q4SeedAxisTopR4` pour
chaque face fixe. Le replay exact v5 conserve seulement une complétion `y` dont
le handle vérifie aussi `s_H(y)>u`, puis réapplique distinct-ID, owner6,
positivité et seed canonique ; il borne les groupes de racines par
`2*(h4-p)<=16` sous ses préconditions requalifiées.

Tous les handles, y compris ceux dont le rôle d'apex a été tué par `s_H`,
restent disponibles à la `certificate_source`; aucune capacité de seed ne
filtre l'`exact_census_source`. Un site interdit comme apex peut encore être
intérieur et changer le top-r4. En q4, parcourir seulement les points du cover
3 pour ce ranking serait incomplet : la fixture `q4_source_fixture` place un
intérieur dans la fenêtre 4 mais hors de la fenêtre 3. Le terminal axial lit
donc l'arbre entier ou une source complète séparément prouvée ; seul le droit
d'être la complétion émise est filtré par `s_H(y)>u`. Une auto-jointure locale
`C x D` peut survivre en ablation counter-only sous budget, jamais comme
fallback non borné. Le coût encore ouvert devient le nombre de faces ternaires
résiduelles et le scan/top-k par face, plus le census, et non plus `k^2`.

Une éventuelle décomposition **par paire de handles**, désormais limitée à
l'oracle, ajoute nécessairement $h_d(d)$ :

$$D_A=A,\qquad D_B=B,\qquad D_C=C\setminus(A\cup B),\qquad D_D=D\setminus(A\cup B\cup C),\qquad D_0=P\setminus(A\cup B\cup C\cup D).$$

Les crédits $h_0,h_a,h_b,h_c,h_d$ sont définis par intersections universelles
sur leurs fibres non vides, exactement comme en q3. Le premier incrément q4
doit s'arrêter à `g4_AB + h_a + h_b`. Ajouter $h_c$, puis $h_d$, exige des
positions ou une repartition explicite à chaque nouveau handle. Si l'oracle de
paires est exécuté, il imbrique `C`, puis `D` seulement sur les masques
survivants et reste sous cap. Le chemin produit recommandé passe au contraire
du carrier ternaire fixe au terminal axial et ne construit pas ce produit.

Pour un bloc croisé `C!=D`, cette orientation fournit bien deux domaines
disjoints pour $h_c$ et $h_d$. Pour le bloc diagonal `C=D=H`, elle donne au
contraire `D_D=emptyset` : la paire de points est orientée, par exemple par
`PointId(c)<PointId(d)`, uniquement pour nommer les fibres, et le second crédit
scalaire vaut zéro. Poser à nouveau `D_D=H\(A union B)` puis sommer
`h_c+h_d` compterait deux fois le même domaine. Récupérer davantage exige des
sets de positions et leur union, jamais deux cardinalités nues ; l'orientation ne
préjuge toujours pas lequel de `c,d` devient le seed canonique.

### Ledger local des paires de handles — oracle exact

Les handles `H_i` d'un rectangle forment une antichaîne, donc leurs plages de
positions sont disjointes. Parcourir seulement `i<j` pour les blocs croisés et
`choose2(H_i)` pour les blocs diagonaux partitionne les paires non ordonnées.
Préconditions explicites : `A` et `B` sont disjoints ; les handles sont non
vides, sans doublon et rangés dans un ordre déterministe ; deux handles
distincts ont des plages disjointes ; chaque bloc diagonal est émis exactement
une fois. Toutes les tailles et intersections comptent des positions uniques, pas
`node_weight` ; l'identité position--ID ne vaut ici que parce que le profil
refuse les positions dupliquées.
Poser $n_X=\lvert X\rvert$, $\alpha_X=\lvert A\cap X\rvert$ et
$\beta_X=\lvert B\cap X\rvert$. Pour deux handles distincts `C,D`, la masse de
quadruplets à IDs distincts est :

$$m_4(A,B;C,D)=n_A n_B n_C n_D-n_D(n_B\alpha_C+n_A\beta_C)-n_C(n_B\alpha_D+n_A\beta_D)+\alpha_C\beta_D+\beta_C\alpha_D.$$

Pour le bloc diagonal d'un handle `H`, elle vaut :

$$m_4(A,B;H,H)=n_A n_B\binom{n_H}{2}-(n_H-1)(n_B\alpha_H+n_A\beta_H)+\alpha_H\beta_H.$$

Ces formules retirent les extrémités `a,b` sans ordonner `c,d`. Convertir
**chaque** cardinalité en `i128` avant la première multiplication C++, puis
exiger un résultat non négatif avant conversion en `u128`; affecter seulement
le produit déjà calculé à un `i128` serait trop tard. La borne `NodeRange` en
`i32` suffit ensuite à garder ces produits dans `i128`.

Pour chaque rectangle, calculer aussi en entier large
`full_r=|A||B|*choose2(n_u-2)`. `covered_r` est la masse sous handles avant
tout fate ou cap : les rôles `pending` en font partie. Exiger
`0<=covered_r<=full_r` avant `outside_r=full_r-covered_r`; si `pending` est
compté séparément, il faut au contraire l'ajouter explicitement à l'équation.
La masse hors fenêtre reçoit ensuite son fate. Sous la partition exacte des
paires par la WSPD et le profil sans doublons, la fermeture attendue est :

$$\sum_r(\text{covered mass}_r+\text{outside mass}_r)=\sum_r\lvert A_r\rvert\lvert B_r\rvert\binom{n_u-2}{2}=6\binom{n_u}{4}.$$

Ici `r` parcourt la **tape canonique complète des arêtes**, pas seulement le
vecteur `AliveRect`. Les rectangles tués par le cœur avant séparation, par la
retouche de coins ou pendant `postsep` reçoivent eux aussi un fate q4 et la
masse `|A||B|*choose2(n_u-2)` de la sous-tape qu'ils ferment. L'invariant
global prend donc la forme
`early_core_dead + postsep_dead + alive_covered + alive_outside =
6*choose4(n_u)` ; un ledger construit sur les seuls survivants ne peut pas
prouver la provenance globale.

Le calcul de `covered_r` ne doit pas recréer un coût quadratique en handles.
Si `U` est leur union disjointe, agréger
`n_U=sum(n_i)`, `alpha_U=sum(alpha_i)` et `beta_U=sum(beta_i)`, puis appliquer
directement la formule diagonale à `U`. L'identité
`m4(A,B;U,U)=sum_i m4(A,B;H_i,H_i)+sum_{i<j}m4(A,B;H_i,H_j)` donne le ledger
en `O(number_of_handles)`. Les couples géométriques restent streamés et
`pending=covered_r-classified_r`; cette identité de comptabilité n'autorise
aucun prune sur une boîte agrégée.

Le fate `DEAD_OUTSIDE_WINDOW` exige encore la preuve géométrique pour **les
deux** porteurs. Si `AB` est l'arête maximale, chacun de `c,d` vérifie
$\lVert 2x-a-b\rVert^{2}=2\lVert x-a\rVert^{2}+2\lVert x-b\rVert^{2}-\lVert b-a\rVert^{2}\leq3\lVert b-a\rVert^{2}$. La porte doit vérifier que
`rect_cover_handles` au coefficient 3 est un sur-cover de cette fenêtre pour
tout `(a,b)` de `A x B`; le seul ledger de masse n'autorise pas à tuer son
complément.

La fixture frontière q4 prend le tétraèdre régulier entier
`a=(0,0,0)`, `b=(1,1,0)`, `c=(1,0,1)`, `d=(0,1,1)`, avec IDs croissants.
Toutes ses arêtes ont carré `2`, `AB` gagne par `EdgeKey`, et chacun des deux
porteurs satisfait exactement `|2x-a-b|^2=6=3*D2`. Elle tue un cover ouvert,
un rejet sur égalité ou un coefficient inférieur à trois.

Une petite porte énumère directement les quadruplets pour les cas `C!=D` et
`C==D`, y compris chaque recouvrement possible avec `A` ou `B`, puis mute le
`choose2` en produit ordonné. Ce ledger est une preuve de provenance, pas une
autorisation de construire tous les couples de handles dans le hot path.

## Ablation structurelle différée

L'autre audit propose une partition `Lca3Forest` : pour chaque nœud interne
`u`, prendre ses enfants `L(u),R(u)` et, pour chaque ancêtre strict `v`, le fils
de `v` opposé au chemin vers `u`. Les blocs `L(u) x R(u) x C(u,v)` partitionnent
exactement les triplets en facteurs disjoints et leur nombre est au plus
`48(n-1)` sous les clés Morton48 distinctes.

Cette observation est mathématiquement utile comme comparateur de ledger, mais
elle ne remplace pas le premier incrément : la paire LCA n'est généralement
pas l'arête maximale et n'est pas WSPD-séparée. Ni le spindle ni le cover owner
actuels ne s'y appliquent. La tester comme oracle structurel est légitime ; la
présenter comme nouvelle route produit avant le probe fibré ne l'est pas.
