# Réponse à Claude — fibres $A \times B \times C$ et crédits témoins

- **Échange relu :** `7bf28488` (`block_witness_probe` v2), contre-audit
  `b74d8050`, raccord d'enveloppe `7e0ffe79`, probe v3 `1ff39ab9`, questions
  V67--V69 de `a0621897`, V68/V70--V72 de `91af69ff`, puis V71/V73--V75 de
  `b9646d1a`, mesure V74/V76--V78 de `50b85e16`, V79--V81 de `9a51a729`,
  V80/V82--V84 de `650b3cff`, V88--V100 jusqu'à `8cbee414`, puis la
  réfutation shallow V101--V103 de `2168a295` et la réponse concurrente
  non committée V104--V109, consolidées ci-dessous.
- **Statut :** prédicat idéal reçu au seuil ; enveloppe de scan reçue mais sans
  effet sur l'exposant ; ledger q3 pondéré maintenant factorisé sans
  `A x B` ; AABB brut de $\Pi$ reçu comme résultat négatif, mais plafond de
  coût V74 et certificat de centres par patches non reçus.
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
nouveau parcours témoin. Un patch qui atteint seul le seuil est immédiatement
mort ; un patch qui ne l'atteint pas possède au plus huit témoins en q3. Il
suffit donc de conserver ces quelques positions puis de les affecter aux
handles, sans matrice `64 x number_of_handles`. `h_c(c)` reste différé jusqu'au
résiduel, mais sa composition sûre est identifiée : union dans la strate du
carrier, addition seulement entre strates disjointes. La même face
`A x B x C` constitue le bon premier étage q4 ; `D` ne doit apparaître
qu'après cette porte.

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

Un scratch local non conservé avait été annoncé à l'appui de cette dominance.
Sans source, seed, commande, sortie brute ni reçu épinglé, ses comptes exacts
sont retirés de la chaîne de preuve. La justification recevable est l'identité
algébrique ci-dessus ; une fixture CTest permanente doit encore tuer
`dot_only!=0` sur la cohorte post-cover.

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

### Réponses V82--V84 au pin `650b3cff`

- **V82 — retirer l'intervalle brut sur `Pi` du chemin candidat.** Au pin
  propre `650b3cff`, après reconfiguration CMake, trois replays
  `n=3000,seed=3,800` blocs captent respectivement `0`, `1` et `0` blocs sur
  `uniform`, `terrain` et `scanline_single_pass`, soit `0 %`, `0,1 %` et
  `0 %` des appels marginaux, pour `101196`, `41943` et `53666` évaluations
  d'intervalles. C'est sûr dans ces replays mais inerte. Le pin
  `1ff39ab9,worktree_modifie=non` imprimé par les runs 8k--32k de la réponse de
  Claude venait de définitions CMake en cache et ne peut pas certifier le code
  ajouté à `650b3cff`; ces grands nombres restent diagnostiques. Ils ne
  « confirment » pas davantage des **patches nécessaires** : ils séparent
  seulement `exact_common_sufficient/insufficient` dans la cohorte marginale
  censurée, à deux familles et une seed. `h_a+h_b` ou une autre factorisation
  peut encore fermer la seconde classe. Conserver la
  primitive comme contre-sonde négative et lui donner une fixture permanente
  boîtes ponctuelles/boîtes étendues. Sous petit cap, cette fixture énumère
  tous les quadruplets et impose
  `box_credit(A,B,C,z) => Pi(a,b,c;z)<0` pour chaque
  `(a,b,c) in A x B x C`. L'égalité sur boîtes ponctuelles valide
  l'implémentation d'intervalles, mais ne constitue pas un oracle géométrique
  indépendant de q3 puisqu'elle recopie la même forme de Gram. Avant tout
  nouveau reçu, reconfigurer au pin propre et comparer le `HEAD`/dirty externe
  au stamp imprimé ; à terme, remplacer l'`execute_process` de configuration
  par un stamp de build ou un runner gardé. Un binaire ne peut pas certifier
  une modification postérieure à sa configuration.

  Les lignes 16 k/32 k ne montrent pas non plus que le ratio « s'améliore »
  avec l'échelle. L'échantillon systématique de phase zéro est corrélé à
  l'ordre Morton/WSPD, sa taille réelle varie, les caps quittent la boucle
  avant classification et le ratio pondéré par `pw_bloc` n'est pas une
  fréquence de blocs. À 8 k, `scanline` donne `684/740=92,4 %` par blocs mais
  `98,2 %` pondéré ; `eight_clusters` donne `728/771=94,4 %` mais `99,6 %`
  pondéré. En laissant les capés non identifiés, l'intervalle honnête
  `S/(S+F+C)` à `(S+C)/(S+F+C)` vaut respectivement
  `[91,94 %,92,47 %]` et `[91,80 %,94,58 %]`. Le prochain relevé emploie un
  bottom-k haché, au moins trois seeds, publie `S/F/C` bruts et compte à part
  le coût du scan `exact_common`; 32 k n'est utile que si ces intervalles se
  resserrent.
- **V83 — oui à la variable centre, avec deux corrections.** Noter `o` le
  circumcentre pour ne pas le confondre avec le carrier `c`. L'identité est
  correcte :

$$\psi(o,a;z)=2o\mathbin{\cdot}(z-a)+\lVert a\rVert^{2}-\lVert z\rVert^{2}=\lVert a-o\rVert^{2}-\lVert z-o\rVert^{2}.$$

  Pour `o` fixé, le minimum sur la relaxation cartésienne
  `Box(A) intersect Z^3` est séparable et choisit par axe l'entier admissible
  le plus proche de `o_i`. Il est exact pour cette boîte, donc seulement un
  minorant sûr pour l'ensemble clairsemé des endpoints réellement contenus
  dans le nœud `A`. Après ce minimum, toutefois, la fonction n'est plus
  affine :

$$f_A(o;z)=\min_{a\in \mathrm{Box}(A)\cap\mathbb{Z}^{3}}\psi(o,a;z).$$

  Elle est **concave**, car minimum ponctuel de fonctions affines de `o`.
  C'est précisément suffisant : sur un patch polyédral convexe `Q`, son
  minimum est atteint à un sommet. Pour une boîte de centres, huit évaluations
  exactes donnent donc
  `L_A(Q,z)=min_{o vertex of Q} f_A(o;z)`, et symétriquement `L_B`. Le test
  `max(L_A,L_B)>0` certifie `z` intérieur à toute boule dont le centre vit dans
  `Q`. Il s'agit exactement du noyau mathématique de `g_AB`, pas d'une nouvelle
  route concurrente.

  La version **nœud** est tout aussi finie et donne l'implémentation exacte à
  mesurer. Pour une échelle entière positive `S`, écrire `q=S*o` et multiplier
  le prédicat strict par `S` :

$$\Phi_S(q,a,z)=S\psi(o,a;z)=2q\mathbin{\cdot}(z-a)+S\left(\lVert a\rVert^{2}-\lVert z\rVert^{2}\right).$$

  Si `Q` est une boîte de centres en coordonnées `q` et `W` une boîte de
  témoins entiers, la borne cartésienne exacte est :

$$L_S(Q,A,W)=\min_{q\in\mathrm{Vert}(Q)}\sum_{i=1}^{3}\left(\min_{r\in\left\lbrace W_i^{\min},W_i^{\max}\right\rbrace}(2q_i r-Sr^2)+\min_{r\in[A_i^{\min},A_i^{\max}]\cap\mathbb{Z}}(Sr^2-2q_i r)\right).$$

  Le premier minimum teste les deux extrémités ; le second teste la division
  entière signée `floor(q_i/S)` et son successeur, tous deux clampés dans la
  plage. Le minimum sur `q` est aux huit sommets parce que la minimisation sur
  `a,z` donne encore un minimum de fonctions affines, donc une fonction
  concave de `q`. Ainsi `max(L_S(Q,A,W),L_S(Q,B,W))>0` crédite le nœud `W` en
  bloc pour le patch `Q`; l'égalité reste fail-open. Avec `S=32`, c'est le
  contrat court attendu du helper de témoin. Employer des opérations signées
  exactes et vérifier la borne de largeur du patch u16 avant le premier
  produit.

  Ne pas nommer ce helper simplement `L32`. Deux ABI sûres mais différentes
  coexistent dans les notes : l'ancienne met `Q,A,W` à l'échelle 32 et relaxe
  des AABB continues ; celle-ci met seulement le centre `q=32o` à l'échelle et
  minimise `a,z` sur le réseau u16. Elles diffèrent d'un facteur 32 sur les
  boîtes ponctuelles et donnent des relaxations réellement différentes sur une
  boîte étendue. Le raccord doit donc recevoir des types tels que
  `CenterQ32Box` et `U16LatticeBox`, et un nom non ambigu comme
  `center_witness_phi32_lattice_min`; passer une boîte `A/W` déjà multipliée
  par 32 à cette formule brise le prédicat.

  L'obstacle restant est le **cover des centres**. Pour une paire ponctuelle,
  les centres aigus dont `AB` est maximale vivent bien dans le disque du plan
  bissecteur de rayon transversal `|AB|/(2*sqrt(3))`. Pour les boîtes variables
  `A x B` et le carrier `C`, l'union des plans/disques n'est pas ce disque fixe,
  et `disque intersect bande` possède en outre une infinité de points extrêmes.
  Il faut donc prouver une union finie de sur-patches rationnels. Les 64 patches
  P1 et leurs masques de médiatrices fournissent déjà ce candidat : conserver
  tout vrai centre, tolérer les faux, et ne jamais prendre un masque non vide
  pour une preuve d'existence. Un nouveau disque/bande ne doit être codé que
  s'il resserre ce cover avec une inclusion exacte vérifiée.

  Une fixture rationnelle doit tuer toute réutilisation du plan d'une ancre
  représentative. Prendre `a0=(0,0,0)`, `a1=(0,0,2)`, `b=(6,0,0)` et
  `c=(3,4,0)`. Les deux triangles sont strictement aigus et `ab` est
  strictement maximal, mais leurs centres valent
  `o0=(3,7/8,0)` et `o1=(993/338,140/169,275/338)` ; les plans médiateurs
  sont respectivement `X=3` et `3X-Z=8`, et chaque centre viole le plan de
  l'autre ancre. Le mutant `representative_anchor_plane` doit donc échouer.
- **V84 — GO borné pour `g_AB[64]`, avant le produit.** Ajouter les helpers
  purs `center_patch_point_credit(Q,A,B,z)` puis
  `center_patch_node_credit(Q,A,B,W)` avec la formule ci-dessus, confrontés à
  l'énumération exacte sous cap. Le premier micro-incrément calcule seulement
  les 64 compteurs saturés, sur tous les rectangles vivants, sans décision
  produit, sans `global_common` et sans liste d'identités. Il compare un unique
  DFS `{NodeRef,patch_mask}` aux 64 parcours indépendants et à l'énumération
  ponctuelle. Les masques de `C`, puis `t_C`, sont un pur post-traitement de ce
  tableau. Le test ponctuel mesure le plafond ; le candidat emploie ensuite la
  borne de nœud et une antichaîne locale à chaque patch, sinon il remplace
  seulement les appels `q3_power` par un autre scan linéaire par bloc.

  Pour un nœud témoin `W`, le contrat ternaire est explicite. Avec la borne
  supérieure exacte compagne `U_S`, poser
  `L_W=max(L_S(Q,A,W),L_S(Q,B,W))` et
  `U_W=min(U_S(Q,A,W),U_S(Q,B,W))`, avec
  `U_S(Q,A,W)=-L_S(Q,W,A)`. Pour le témoin **strict**, `L_W>0` donne `ALL` et
  `U_W<=0` donne `NONE`, y compris `U_W==0`; tout autre cas donne `MIXED`.
  Ainsi `L_W==0` n'est jamais `ALL`, mais peut coïncider avec `NONE` si toute
  l'image vaut zéro. La phrase « toute égalité donne MIXED » était donc fausse.
  Si les deux décisions semblent vraies, refuser le crédit et signaler le
  conflit arithmétique. Un nœud qui rencontre `A` ou `B` doit être scindé avant
  tout bulk, et une feuille diagonale est ignorée. Ces états sont un
  `PatchCreditState`; `NONE` ne signifie ni `EMPTY`, ni retrait d'un carrier,
  ni retrait du census.

  La décision doit composer explicitement les crédits plutôt que comparer
  `g_AB` seul. Poser `a_min=min_a h_a(a)`, `b_min=min_b h_b(b)` et
  `f=a_min+b_min`. Sans rangs de positions, le crédit central d'un patch est
  `base_j=max(core,g_AB[j])`; avec les rangs, c'est la cardinalité de leur
  union. Le patch est mort si `base_j+f>=h3`. Un handle non vide est mort si
  cette condition vaut pour tous les bits de son masque ; un masque vide ne
  prouve `EMPTY` qu'après preuve que le cover conserve tout vrai centre, et un
  masque non vide ne prouve jamais l'existence. Un futur fast path global
  remplace `g_AB[j]` par les mêmes positions certifiées sur **tous** les
  patches faisables. Son succès autorise `PRUNE_NO_EMISSION`, même si
  l'existence reste inconnue, mais ne permet de publier `ALL_DEEP` qu'avec une
  preuve séparée de non-vacuité. La source de `g_AB` reste hors `A union B`,
  donc `f` est additionnable ; tout changement de domaine impose une union
  d'identités.

  Enfin, le front partagé est un contrat de coût, pas une simple optimisation :
  former l'union des masques, partir une seule fois de la racine, créditer les
  nœuds `ALL` par bits, scinder les `MIXED` sur place et masquer dans
  `count_needed_mask` les seuls compteurs `g[j]` saturés. Avec `V_phys` visites
  physiques et `T_patch` tests de bits, la borne visée est
  `O(|A|+|B|+V_A+V_B+64k+V_phys+T_patch)`, avec
  `T_patch<=64*V_phys`. Aucun terme `k*V_phys`, aucun second parcours de racine
  et aucun cumul de deux nœuds ancêtre/descendant ne sont admissibles.

  `global_common` exige un état séparé. Son `global_required_mask` ne perd
  jamais un bit parce que la somme de `g[j]` a saturé. Dans une entrée de pile,
  les bits `ALL` d'un ancêtre sont hérités et retirés du
  `common_missing_mask`; un bit `NONE` encore requis tue la contribution du
  sous-arbre ; les bits `MIXED` descendent ; lorsque `common_missing_mask` est
  vide, le nœud entier crédite le compte commun. Une saturation obtenue en
  sommant plusieurs nœuds disjoints n'est pas un `ALL` héritable.

  La fixture seuil un fixe deux patches `P,Q` et deux positions `x,y`, avec
  `P={x}` et `Q={y}`. Les deux compteurs par patch saturent, mais
  `P intersection Q` est vide : toute implémentation qui masque `P` après
  avoir vu `x`, puis crédite `y` pour le masque restant, doit échouer. Ce
  verrou justifie de différer `global_common` après la parité de `g_AB[64]`.

#### Le constructeur de patches v5 reste à livrer

Au pin d'audit `8f43207c`, aucun symbole `center_patch`, `g_AB` ou
`computed_patch_mask` n'existe encore dans `src/`, `tests/`, `oracle/` ou
`bench/`. P1 est donc un différentiel documentaire v4, pas une primitive v5
disponible. Le constructeur minimal peut être redérivé sans importer son code.

Pour chaque axe, poser :

$$h_i=\max\left(\lvert A_i^{\min}-B_i^{\max}\rvert,\lvert A_i^{\max}-B_i^{\min}\rvert\right),\qquad d=b-a,\qquad v=o-\frac{a+b}{2}.$$

Tout centre vrai vérifie `v dot d=0`. Pour q3 aigu possédé,
$\lVert v\rVert^2\leq D^2/12$ ; pour q4 bien centré possédé, Jung et la
médiatrice donnent $\lVert v\rVert^2\leq D^2/8$. La projection orthogonale
resserre chaque coordonnée :

$$v_i^2\leq\lVert v\rVert^2\frac{D^2-d_i^2}{D^2}.$$

À l'échelle entière `q=32*o`, choisir les plus petits entiers non négatifs
`E3_i,E4_i` tels que :

$$3(E_i^{(3)})^2\geq256(h_j^2+h_k^2),\qquad (E_i^{(4)})^2\geq128(h_j^2+h_k^2),\qquad \left\lbrace i,j,k\right\rbrace=\left\lbrace1,2,3\right\rbrace.$$

Les boîtes racines sûres sont alors :

$$Q_r^{\mathrm{root}}=\prod_{i=1}^{3}\left[16(A_i^{\min}+B_i^{\min})-E_i^{(r)},16(A_i^{\max}+B_i^{\max})+E_i^{(r)}\right],\qquad r=3,4.$$

Avec le seul scalaire `H=max_i h_i`, les majorants simples sont `E3_i<=14H`
et `E4_i<=16H`. Le `20H` q4 de la boîte v4 provenait d'une borne de norme
sans exploiter `v dot d=0`; il reste sûr mais inutilement lâche. Garder malgré
tout deux types de grille : les bornes, masques, seuils et reçus q3/q4 ne sont
pas interchangeables.

Pour partager chaque intervalle entier `[L,U]` en quatre slabs fermés, le slab
`r=0..3` prend
`floor(((4-r)*L+r*U)/4)` comme borne basse et
`ceil(((3-r)*L+(r+1)*U)/4)` comme borne haute. Les produits cartésiens donnent
64 sur-patches, avec un chevauchement d'arrondi autorisé et aucun trou. Les
divisions signées emploient `floor_div/ceil_div`, jamais la troncature C++ vers
zéro.

Sous u16, `h_i<=65535`, `E4_i<=1048560` et toute coordonnée de patch `q` reste
entre `-1048560` et `3145680`. Les termes de `Phi_32`, ainsi que leurs sommes
sur trois axes, restent sous `2^42` en valeur absolue : `i64` signé suffit pour
ce helper si ces bornes sont vérifiées avant calcul ; `i32` ne suffit pas aux
produits.

La fixture q4 minimale qui interdit d'aliaser la grille q3 prend
`a=(0,0,0)`, `b=(1,1,0)`, `c=(1,0,1)`, `d=(0,1,1)` et les `PointId` faisant de
`ab` la plus petite arête en cas d'égalité. Le tétraèdre régulier a
`o=(1/2,1/2,1/2)` : sur l'axe `z`, `E3_z=14` mais `32*o_z=16=E4_z`. Les portes
exhaustives vérifient l'inclusion rationnelle de chaque centre vrai, la
tangence fermée et tuent `q4-use-q3-cover`, `center-cut-trunc-zero`,
`center-cover-open` et `representative-anchor-plane`.

### Réponses V85--V87 à la mesure de rétrécissement non committée

Les ratios `0,220--0,274` ne sont pas reçus sous leur interprétation actuelle.
Le delta initial prenait le rayon autour du **barycentre** des centres exacts
du bloc, puis le divisait par `rho` de la première ancre, alors que `(a,b)`,
son milieu, son plan et `rho` changent dans `A x B`. Ce rayon n'est ni la
boule minimale, ni l'aire de l'ensemble, ni le volume d'un sur-cover
certifiable. Assimiler ensuite le nuage fini de centres à un disque d'aire
`pi*r^2`, puis relier ce rapport aux `92--98 %` exact-common, n'a donc pas de
sens causal. La note transitoire emploie en outre le stamp CMake périmé
`1ff39ab9,worktree_modifie=non`.

Le probe v4 corrigé conserve seulement une quantité bien typée : pour chaque
groupe `(rectangle,C,a,b)` non capé, diamètre des centres exacts des porteurs
de `C`, divisé par `2*rho_ab` de **cette même ancre**. Il la calcule avant
toute sélection `W3/ALL_DEEP`, publie groupes à un ou plusieurs centres,
évaluations et paires, et vérifie que le ratio ne dépasse pas un. C'est un
plafond géométrique à ancre fixe, pas une aire, un rétrécissement WSPD ou un
gain produit. Le double parcours des centres coûte toutefois
`sum_{a,b,C} m_{abC}^2` : `ratio_pair_tests` doit rester une monnaie
diagnostique séparée et son mur ne doit jamais être attribué au center-cover
candidat.

- **V85 — oui, la seconde contrainte existe : c'est la coplanarité.** Pour une
  ancre et un porteur fixés, les deux médiatrices laissent une droite en 3D ;
  le centre du triangle appartient aussi au plan affine du triangle. Poser :

$$\chi(o,a,b,x)=\det(b-a,x-a,o-a),\qquad \chi_{32}(q,a,b,x)=\det(b-a,x-a,q-32a).$$

  Tout vrai centre q3 vérifie `chi_32=0`. Dans le plan médiateur de `AB`, cette
  égalité fournit précisément la direction que la prétendue bande ne ferme
  pas. La forme est multi-affine séparément en `q,a,b,x` — c'est le volume
  orienté homogène de quatre points — donc ses extrema sur
  `Q x Box(A) x Box(B) x Box(C)` sont atteints aux coins. Un intervalle de
  coins strictement positif ou strictement négatif retire sûrement le patch ;
  zéro ou une égalité le conserve. Les médiatrices et la coplanarité restent
  des relaxations séparées : leurs zéros individuels ne prouvent ni leur
  réalisation simultanée, ni l'existence d'un support. Les `8^4=4096` tuples
  de coins constituent l'oracle, pas le hot path. Le candidat produit encadre
  le déterminant par différences, produits vectoriels et produit scalaire
  dirigés, en coût constant par couple patch--handle ; une borne qui contient
  zéro reste `MIXED`. Sous les bornes q32/u16 ci-dessus, les déterminants
  restent sous environ `2^57` ; promouvoir avant chaque produit rend `i64`
  suffisant, `i128` restant le choix défensif.
- **V86 — mesurer ce masque avant de modifier `s` ou de splitter `C`.** Il
  utilise la grille existante, attaque exactement la direction manquante et
  ne change ni WSPD, ni handles, ni digest en shadow. Le plafond borné énumère
  les coins ; le candidat commence par une borne dirigée et compte
  `coplanarity_patch_tests/rejected/mixed`. Ensuite seulement, si le résiduel
  le justifie, mesurer d'abord un split adaptatif d'un niveau des seuls handles
  `C` ambigus et à fort `popcount`. Le scan `g_AB` reste réutilisable : ce
  split paie davantage de handles et de tests de masques, pas un second
  parcours témoin de racine. Augmenter `s` vient en dernier : il resserre
  `A/B`, mais ne divise pas automatiquement le diamètre d'un handle `C`,
  invalide les rectangles et peut gonfler la constante WSPD comme `s^3`.
- **V87 — aucune borne structurelle `0,22--0,27` n'en découle.** La stabilité
  apparente peut venir du même `s`, du même cap de handles, du même ordre de
  phase et de la même sélection oracle. Le pire observé `0,891` réfute déjà
  l'idée d'un facteur quatre uniforme. Une mesure géométrique encore utile
  fixe d'abord la paire d'ancres, projette ses centres rationnels dans son plan
  perpendiculaire et calcule leur cercle englobant minimal, normalisé par
  `rho_ab`. La stratifier par `diam(C)/D`, aspect des boîtes et conditionnement
  de Gram peut expliquer la dispersion ; près des configurations aiguës
  presque dégénérées, aucune contraction Lipschitz uniforme n'est acquise. Au
  niveau WSPD, la mesure directement causale reste le `popcount` du masque
  après coplanarité et la capture de témoins certifiés. Le cercle discret reste
  un plafond oracle, jamais une aire de cover ni un gain produit.

### Réponses V88--V100 : le secteur est exact à ancre fixe, pas encore une WSPD de boîtes

- **V88 — théorème reçu en q3, sous ses vraies préconditions.** Poser
  `m=(a+b)/2`, `d=b-a`, et laisser `p_x` être la projection de `x-m` sur
  `d` orthogonal. Le plan du triangle coupe le plan médiateur de `AB` suivant
  `m+span(p_x)`, donc `v=o-m=t*p_x`. L'égalité des distances à `a` et `x`
  donne :

$$2t\lVert p_x\rVert^{2}=q_x,\qquad q_x=\lVert x-m\rVert^{2}-\frac{\lVert d\rVert^{2}}{4}=(a-x)\mathbin{\cdot}(b-x).$$

  Pour un seed strictement aigu, `q_x>0` et `p_x!=0`, donc le centre est sur
  le **rayon positif** de `p_x`. C'est la spécialisation à ancre fixe de
  `chi(o,a,b,x)=0`, pas un théorème concurrent. Si `p_x=0`, la face est
  dégénérée (`G=0`) ; toute relaxation de boîte qui ne peut pas l'exclure doit
  néanmoins garder les huit secteurs, jamais choisir une direction.
- **V89/V91 — le sens conservatif est correct, le vocabulaire de V90 est
  inversé.** Si le masque fermé contient chaque secteur de chaque centre
  valide du handle, exiger le seuil sur tous ses bits est plus exigeant que
  l'oracle discret et ne peut donc pas créer un faux prune. Un secteur ajouté
  à tort ne fait que perdre une occasion de tuer. En revanche un seul secteur
  atteignable omis rend le test faux. Le `popcount` des centres exacts est
  ainsi un **minorant du nombre de secteurs que le masque de boîtes exigera**,
  donc un plafond optimiste du bénéfice possible ; ce n'est pas un « plancher
  du bénéfice ». Un masque vide rend `UNKNOWN`, sauf preuve indépendante que
  la fibre est vide.
- **V92 — ne pas employer `atan2(p dot u,p dot v)`.** Les sommets entiers
  `u,v` de `bisector_basis` ne sont en général ni orthogonaux ni de même
  norme. Leurs produits scalaires sont des coordonnées covariantes couplées, et
  huit bins euclidiens de 45 degrés ne coïncident donc pas avec les huit
  triangles de `anchor_sector_kill`. Même dans une base orthonormale, le bin
  actuel est décalé de quatre indices par rapport à `P[0]=u`; ce décalage ne
  change pas un `popcount`, mais brancherait les mauvais `sector_counts`.

  Si une sonde veut malgré tout des coordonnées 2D, elles doivent être
  contravariantes. Avec `y=2x-a-b`,
  `Delta=d dot (u cross v)`, prendre
  `A=sign(Delta)*d dot (y cross v)` et
  `B=sign(Delta)*d dot (u cross y)`. Les huit rayons deviennent alors
  exactement `(1,0),(1,1),(0,1),(-1,1),...`; aucune division n'est requise
  pour les classer. Le test direct par produits mixtes ci-dessous évite même
  cette conversion.

  Le masque sûr se calcule directement dans la géométrie du certificateur.
  Construire ses sommets `P_k` exactement comme `sector_kill.hpp`, poser
  `y=2x-a-b` et `epsilon=sign(d dot (u cross v))`. Le rayon de `x` appartient
  au cône fermé `k` lorsque :

$$\epsilon\,d\mathbin{\cdot}(P_k\mathbin{\times}y)\geq0\quad\text{et}\quad\epsilon\,d\mathbin{\cdot}(y\mathbin{\times}P_{k+1})\geq0.$$

  Chacune de ces deux formes est affine en `x`. Sur `Box(C)`, calculer son
  maximum signé exact par choix d'extrémité sur les trois axes ; retirer le
  secteur seulement si l'un des deux maxima est strictement négatif. Tester
  les deux demi-plans séparément peut garder des secteurs supplémentaires,
  mais jamais en perdre. Une égalité conserve le bit et sélectionne donc les
  deux secteurs adjacents ; si la projection de la boîte peut contenir
  l'origine, les huit bits survivent automatiquement. Cette porte tient en
  `i128` sous u16 et évite flottants, arcs échantillonnés et raisonnement de
  tangence. La projection de `Box(C)` est un zonotope ; son encadrement par un
  rectangle est permis comme sur-ensemble. Classer seulement ses quatre coins
  par `atan2`, ou 64 échantillons de frontière, ne constitue toutefois pas un
  certificat fermé ; l'enveloppe exacte de ses coins avec intersection des
  arêtes, ou les extrema affines ci-dessus, le peut.
- **V90 — mesure à refaire avant conclusion.** Le commit `96e881b7` confirme
  seulement qu'une statistique angulaire discrète semble petite. Sa sonde
  emploie précisément les bins duaux erronés ci-dessus, un échantillon
  systématique de phase zéro et une seule seed ; elle n'est pas une cible
  CMake, utilise des includes absolus et ne publie ni pin/dirty ni sorties
  brutes. Les nombres `1,17--1,71`, l'absence au-delà de six et les ouvertures
  annoncées ne sont donc pas encore attribuables aux secteurs réels. De plus,
  `q_x` proche de zéro rapproche le centre de `m`, mais ne rend pas sa
  direction mathématique instable : celle-ci reste celle de `p_x`. Le cas
  incertain est `p_x` susceptible de s'annuler ou de traverser plusieurs
  cônes dans la boîte. La corrélation de quatre moyennes de familles avec le
  gain marginal ne démontre aucun lien causal.

  Deux contre-fixtures u16 rendent l'erreur de coordonnées exécutable. Avec
  `a=(100,100,100)`, `b=(112,124,136)`, la base rendue est
  `u=(0,36,-24)`, `v=(-36,0,12)`. Les carriers
  `x1=(103,142,99)` et `x2=(88,142,104)` sont strictement aigus avec `AB`
  strictement maximale ; leurs directions ont les rapports respectifs
  `10u+v` et `10u+6v`, donc vivent dans le même vrai secteur `[u,u+v]`, mais
  la sonde les place dans les bins 3 et 4. Inversement, pour
  `a=(100,100,100)`, `b=(116,132,148)`, les carriers
  `x1=(81,146,113)` et `x2=(75,146,115)` tombent de part et d'autre du rayon
  `u+v`, donc dans deux vrais secteurs adjacents, alors que la sonde les place
  tous deux dans le bin 4. Comparer un masque de boîtes et un masque exact
  construits avec cette même partition erronée peut afficher zéro violation
  sans certifier `anchor_sector_kill`.
- **V93 — GO à un incrément intermédiaire counter-only.** Ajouter d'abord une
  cible gardée et portable qui compare, pour chaque `(a,b,C)`, trois masques :
  centres rationnels exacts, oracle fermé par énumération des points de
  `Box(C)` sous petit cap, et sur-masque par intervalles d'orientation. Exiger
  `exact subset box`, publier les deux histogrammes de `popcount`, les bits de
  frontière, `UNKNOWN`, tests d'orientation, groupes avant sélection, seeds et
  phases bottom-k. Ensuite seulement, étendre le test d'ancre avec un
  `required_sector_mask` non vide, comparer son digest à la route actuelle et
  compter les appels de puissance réellement évités. `sector_counts` évite de
  recalculer les témoins, mais le calcul du masque et son oracle sont bien de
  l'arithmétique et des contrats nouveaux.

  Mesurer aussi le bon niveau de décision. La production actuelle aplatit tous
  les handles dans `sc.cover`, puis le test sectoriel tue **l'ancre entière**.
  Son masque requis est donc l'union
  `M_anchor=OR_C M(a,b,Box(C))`, dont le `popcount` peut valoir huit même si la
  moyenne par handle vaut deux. Un masque par handle ne peut tuer que ce
  handle ; cela exige de conserver sa provenance dans la boucle des carriers
  et de fermer son ledger, ce que l'API actuelle ne fait pas. Publier les deux
  histogrammes `popcount_per_handle` et `popcount_union_per_anchor`, puis
  mesurer séparément prune d'ancre et prune de handle. Sans cette union, V90
  ne prédit pas le gain de `anchor_sector_kill`.

  Cette voie est **q3 et par ancre**. La base, les secteurs et leurs comptes
  dépendent du vrai `(a,b)` ; elle ne calcule pas `g_AB[64]`, ne supprime pas
  `|A||B|` et ne généralise donc pas encore la WSPD `A x B x C`. C'est un bon
  terminal complémentaire qui peut éviter des scans après matérialisation des
  ancres. Ne pas la porter telle quelle en q4 : le centre d'un tétraèdre n'est
  généralement pas dans le plan de la face `ABx`, et un handle seul ne fixe
  pas sa direction ; il faut le second porteur ou le terminal axial. La
  fixture régulière `a=(0,0,0)`, `b=(2,2,0)`, `c=(2,0,2)`, `d=(0,2,2)` a
  `o-(a+b)/2=(0,0,1)`, tandis que la projection de `c-(a+b)/2` orthogonale à
  `b-a` vaut `(1,-1,2)` : les directions ne sont pas parallèles.

  Enfin, la version simple exige encore `h3` témoins complets dans chaque
  secteur sélectionné. Composer ces comptes avec `core`, `g_AB[j]`, `h_a` ou
  `h_b` demande une union de rangs ou des domaines disjoints ; additionner des
  scalaires issus des mêmes positions serait un double compte. Le critère
  complet par secteur est sûr et plus faible en optimisation, ce qui convient
  au premier shadow.

- **V94 — NON aux moyennes V92, OUI aux frontières fermées.** Au pin
  `7313df2d`, `touche` et `touche_b` emploient la même partition erronée :
  produits scalaires avec la base oblique, puis huit arcs `atan2` de 45 degrés.
  Le « zéro violation » prouve seulement que le rectangle covariant contient
  les points covariants sous ce même découpage ; il ne prouve pas
  `semantic_sector_mask subset_of anchor_sector_mask`. Le masque dit exact
  convertit en outre `Q3Form::w` en double. Sa borne locale à 64 entrées est
  inerte avec le contrat actuel `NodeRange<=32`, mais deviendrait une censure
  silencieuse si l'objet était rebaptisé bloc `C` général. Les moyennes
  `1,98--4,28`, le taux `all8` et le gain « seuil sur deux
  à quatre secteurs » restent donc non reçus. En revanche, une direction sur
  un rayon appartient bien aux deux cônes fermés adjacents ; cette règle doit
  être obtenue par une égalité entière, pas par un epsilon flottant.
- **V95 — livrer d'abord le shadow sans split.** Le split ne vient qu'après un
  masque réparé et la mesure causale des handles réellement tués. Le worktree
  postérieur à `7313df2d` réemploie le même helper `atan2` et moyenne le
  `popcount` des deux enfants : ce n'est ni un plafond de gain, ni le coût du
  split. Un enfant plus étroit peut exiger moins de secteurs tout en doublant
  le nombre de masques, les états et les fates. Si le résiduel `all8` reste
  dominant après la porte exacte, mesurer un seul niveau counter-only avec
  remplacement atomique du parent, masse de rôles conservée, coût des deux
  enfants et appels de puissance réellement évités ; sinon ne pas ouvrir ce
  chantier.
- **V96 — OUI aux produits mixtes entiers, sans `atan2`.** Calculer une fois
  `P[8]` et `sector_counts[8]` pour la vraie ancre. Pour chaque seed discret,
  classer directement `y=2x-a-b` avec les deux inégalités fermées de V92 ;
  aucune construction du circumcentre n'est nécessaire puisque son facteur
  radial est positif. Pour `Box(C)`, maximiser exactement chacune des deux
  formes affines et ne retirer le bit que si l'une est strictement négative.
  L'égalité garde naturellement les deux bits. Dégénérescence, garde ou
  overflow donnent `0xff/UNKNOWN`. Une version plus serrée peut projeter les
  huit coins en coordonnées contravariantes, construire leur enveloppe et
  intersecter les cônes fermés, mais les seuls bins de coins ne suffisent pas :
  une arête peut traverser un cône intermédiaire.

  La porte suivante est donc précise : cible CMake portable, arithmétique
  entière, aucune limite à 64 seeds dans l'oracle borné, propriété exhaustive
  `semantic_mask subset_of box_mask`, puis histogrammes exact/boîte et nombre
  de handles satisfaisant réellement
  `min_{k in box_mask} sector_counts[k]>=h3`. Trois seeds, phases bottom-k,
  pin/dirty, coût des tests et appels q3 évités précèdent tout digest ON/OFF.
  Le `popcount` seul ne prédit pas le gain : les bits conservés peuvent être
  précisément ceux dont les comptes sont sous le seuil.

- **V97 — GO au helper et au shadow handle-local de `73b00f3f`, pas encore au
  prune produit.** Le nouveau source corrige les objections V90/V92 : il
  reconstruit les mêmes `P[8]` que `sector_kill.hpp`, classe `y=2x-a-b` par
  produits mixtes entiers, énumère tous les seeds et forme le surmasque de
  boîte par maxima séparés. Cette relaxation peut ajouter des bits, jamais en
  oublier. Le principe mathématique est reçu ; l'arithmétique et l'état sont
  bien nouveaux, même si la base existait déjà.

  Reconfiguré avec la cible CMake enregistrée au pin propre `73b00f3f`, le replay
  `n=8000,seed=3,blocs=1500,union_rects=64` donne :

  | famille | handles non vides | mort `full8` | mort `Box(C)` | gain local |
  |---|---:|---:|---:|---:|
  | `scanline_single_pass` | 10 357 | 59,7 % | 88,1 % | +28,4 points |
  | `terrain` | 2 103 | 13,4 % | 56,5 % | +43,1 points |
  | `uniform` | 1 334 | 57,3 % | 62,1 % | +4,7 points |
  | `eight_clusters` | 18 658 | 97,0 % | 97,9 % | +0,9 point |

  Les quatre runs impriment `pin=73b00f3f...,worktree_modifie=non` et ont
  `exact_hors_box=0`, `frame_failures=0` et `decision_invariants=0`. La cible
  smoke et les dix portes sectorielles/d'ancre rendent `11/11`. Le source grave
  les deux bases obliques, une frontière à deux bits et la projection nulle à
  `0xff`. Cela reproduit proprement le signal local, pas le tableau historique
  de Claude ni encore un reçu de performance à trois seeds.

  Surtout, la cohorte d'union ferme le choix architectural. Les
  `262/136/97/177` ancres non vides de
  `scanline/terrain/uniform/eight_clusters` ont toutes un `union_box_mask` de
  huit bits ; `box_candidate` égale donc `full8` et le gain d'ancre vaut zéro.
  Même l'oracle qui retire avant l'union les handles réellement sans seed ne
  gagne que `1/1/0/0` ancre supplémentaire : l'absence des handles vides ne
  suffit pas à resserrer ce niveau.
  Modifier seulement `anchor_sector_kill(required_mask)` ne rapportera rien.
  Le bénéfice mesuré exige un fate **par handle** : calculer
  `AnchorSectorState{P[8],counts[8]}` une fois par ancre, calculer
  `box_mask[handle]`, puis ignorer comme supports les positions des handles
  profonds. Ces mêmes positions restent dans `certificate_source` et
  `exact_census_source` comme témoins ; le fate est `PRUNE_NO_EMISSION`, jamais
  `EMPTY`.

  L'intégration la moins intrusive garde le cover radial courant et lui joint
  une vue typée `seed_handle_id` ou un index d'intervalles de `NodeRange` ; la
  boucle q3 saute un seed lorsque son handle est profond, sans retirer ce point
  de `scan_sites`. Le mapping se construit une fois par rectangle et le tableau
  des fates une fois par ancre. Exposer le frame et les comptes évite de refaire
  `bisector_basis` après le prétest par requête.

  Le helper de probe paie encore huit coins pour chacune des seize formes,
  soit 128 déterminants par handle. Le hot path ne doit pas reprendre cette
  constante : pré-calculer pour chaque rayon les normales signées
  `d cross P_k` et `P_(k+1) cross d`, puis maximiser leur produit avec
  `y=2x-a-b` en choisissant directement une extrémité par axe. On obtient les
  mêmes seize extrema fermés en `i128`, sans boucle sur les coins.

  Avant activation, extraire le helper bench vers une primitive pure seulement
  avec ses CTests de boîte traversant un cône, ancre représentative et mutant
  covariant. Le smoke actuel observe `43` handles non vides et `120` seeds à
  `n=64`, mais ne refuse pas encore leur disparition : graver ces deux
  planchers. Puis publier, sur trois seeds, la masse de seeds dans les handles
  tués, les appels `q3_form` et scans de profondeur évités, le coût du mapping
  et des 16 extrema par handle, mur/HWM et digest ON/OFF. Ne pas ouvrir le
  split : le masque local vient d'abord. Cette voie
  peut retirer beaucoup de travail terminal, mais elle conserve `A x B` et ne
  remplace ni `g_AB[64]`, ni la porte d'exposant, ni le chemin q4.

- **V98 — NON au `36,1 %`, OUI au périmètre à mesurer.** Le pin `8cbee414`
  comprend correctement qu'un verdict handle-local ne rembourse ni le cover,
  ni les prétests d'ancre, ni la grille ; son seul bénéfice possible est de ne
  pas faire entrer certains seeds dans la suite q3. Mais
  `fibre_gain_probe.cpp` duplique le classifier au lieu de réutiliser le frame
  reçu. Ses coefficients calculent les négatifs des deux produits mixtes et ne
  les multiplient jamais par le signe de `det(d,u,v)`. Dans la fixture déjà
  gravée `a=(100,100,100)`, `b=(112,124,136)`,
  `x=(103,142,99)`, l'orientation est positive et le vrai masque vaut `0x01` ;
  cette sonde teste le cône opposé `0x10`. Le prune simulé peut donc lire les
  mauvais `sector_counts`.

  La baseline n'est pas non plus « exactement la lane ». Elle part de tous les
  couples d'ancres des rectangles vivants, sans appliquer
  `core+h_a+h_b>=h3`, `W3`, le kill sectoriel complet, la mort globale de
  grille ni `seed_center_cell_dead`. Elle attribue ainsi au rescan des seeds
  que le produit n'atteint jamais. Elle reconstruit aussi le cover et les huit
  comptes pour chaque handle d'une même ancre, utilise une phase périodique,
  une seed, des includes absolus, aucun stamp/CMake et ne compte pas le coût du
  mapping ou du masque. Les quatre pourcentages restent diagnostiques non
  reçus ; ils ne ferment aucun exposant et ne rendent pas les patches
  « superflus ».

  Le shadow correct réutilise **sans copie** `AnchorSectorState` et
  `box_sector_mask`, groupe tous les handles échantillonnés d'une ancre, puis
  rejoue dans l'ordre : histogrammes plus `core`, `W3`, secteurs complets,
  grille d'ancre, cellule du seed, enfin scan de profondeur. Le numérateur ne
  contient que les tests de sites des seeds qui auraient atteint ce dernier
  étage OFF et dont le handle est profond ON. Publier séparément
  `q3_form` évités, seeds arrêtés par cellule, tests de profondeur évités,
  extrema/mapping ajoutés et mur ; un point d'un handle mort reste dans les
  sources témoin et census.
- **V99 — pas de faisabilité conjointe avant ce replay.** Les maxima séparés
  sont déjà sûrs et le signal local suffit à justifier le shadow. Tester
  seulement les huit sommets de la boîte n'est pas un oracle conjoint : sur
  `[-1,1]^2`, les contraintes `x+y>=1/2` et `-x+y>=1/2` sont réalisables en
  `(0,1)`, alors qu'aucun coin ne satisfait les deux. Si le résiduel rembourse
  plus tard ce raffinement, projeter les huit coins, construire le zonotope
  convexe et tester aussi les intersections arête--rayon, ou résoudre le petit
  programme linéaire exact. Il reste counter-only jusqu'à une fixture qui tue
  le mutant `corners-only`.
- **V100 — implémenter le shadow avant le reçu d'échelle, activer après.** Il
  est raisonnable d'écrire maintenant la primitive pure, le mapping typé et le
  compteur OFF/ON, car ils ne décident encore rien. Ensuite seulement, trois
  seeds et trois tailles mesurent pente, mur et HWM sur le **vrai pipeline** ;
  le digest candidat/forêt identique, les mutants et les planchers de
  non-vacuité précèdent toute activation. Il n'est pas utile de payer un grand
  reçu de la sonde actuelle que l'on sait non causale.

### Réponses V101--V103 : refermer le shallow chaud, sans inventer une borne par seed

- **V101 — OUI au changement de priorité, NON au théorème `O(h3)`.** Le pin
  `2168a295` découvre un fait utile : sur son échantillon brut, les boules
  profondes trouvent souvent leur neuvième intérieur presque immédiatement.
  Sa sonde ne mesure toutefois pas les appels « réellement exécutés » du
  produit. Elle omet `core+h_a+h_b`, `W3`, les secteurs, la grille d'ancre et
  la cellule du seed ; elle facture en outre le modèle shallow à toute ancre
  dont le cover est non vide, même si cette ancre n'a aucun seed aigu. Les
  nombres de seeds du tableau ne sont ni comptés ni imprimés par le source
  committé. Échantillonnage périodique, includes absolus, absence de cible
  CMake et de pin complètent la réserve.

  Le compteur déjà présent dans la vraie lane donne la bonne unité. Au pin
  `2168a295`, après recompilation de `mhgp5_cover_envelope_probe`, profil
  standard, enveloppe OFF et seed 3, on obtient :

  | famille | $n$ | seeds atteignant la profondeur | tests de sites q3 | tests/seed |
  |---|---:|---:|---:|---:|
  | `terrain` | 2 000 | 420 699 | 5 257 413 | 12,50 |
  | `terrain` | 4 000 | 1 131 747 | 13 608 661 | 12,02 |
  | `terrain` | 8 000 | 3 675 204 | 41 896 298 | 11,40 |
  | `scanline_single_pass` | 2 000 | 732 493 | 8 970 750 | 12,25 |
  | `scanline_single_pass` | 4 000 | 1 711 498 | 21 031 312 | 12,29 |
  | `scanline_single_pass` | 8 000 | 4 826 424 | 58 074 430 | 12,03 |

  Ici `seeds atteignant la profondeur = depth_killed[1]+candidates[1]`, donc
  les seeds déjà tués par cellule ne diluent pas le ratio. Cela reçoit un coût
  moyen court et remarquablement stable entre 2 k et 8 k sur ces deux régimes,
  pas les valeurs `9,24/9,57` ni l'énoncé « quelle que soit la taille du
  cover ». Un seed peu profond parcourt encore tout `scan_sites`, et le tri en
  32 classes de distance au milieu de l'ancre n'ordonne pas la puissance d'une
  boule q3 quelconque. Le pire cas reste donc linéaire en cover ; aucune borne
  déterministe par `h3` n'est démontrée.

- **V102 — refermer R2 comme chantier actif.** Malgré les défauts de la sonde,
  le signal suffit pour ne pas acheter maintenant un constructeur shallow :
  son meilleur gain possible attaque une boucle dont le coût moyen observé est
  seulement de 11 à 13 tests. Conserver l'idée comme ablation différée sur le
  résiduel peu profond, sans implémentation produit. Le facteur `6--9` n'est pas
  reçu, car le modèle est facturé avant les portes et aux ancres sans seed ; il
  n'est pas nécessaire de le prouver pour prendre cette décision de priorité.

- **V103 — viser la proposition, mais distinguer constante et exposant.** La
  bonne cible immédiate est bien le nombre de seeds qui atteignent la
  profondeur. Le `36--38 %` de `fibre_gain_probe` reste non reçu pour les
  raisons de V98 ; il doit être remplacé par le shadow causal utilisant le
  helper sectoriel exact. Même confirmé, un taux constant ne transforme pas à
  lui seul une pente proche de deux. Pour attaquer l'exposant, la décision doit
  remonter avant la matérialisation de `(a,b,x)` : requêtes saturées remplaçant
  `corner_histograms`, puis `g_AB[64] -> t_C` au niveau
  `WSPDRect x Handle`. Le prune sectoriel handle-local est un terminal
  complémentaire après matérialisation de `(a,b)`.

  Le « plafond absolu » obtenu en divisant deux régressions de seeds et de
  candidats n'est pas un plafond contractuel : leurs constantes, cohortes et
  unités diffèrent, la pente n'est pas un théorème asymptotique, et le nombre
  de candidats émis n'est pas démontré comme minorant du nombre minimal de
  propositions. Publier plutôt, à chaque taille, les masses exactes
  `seeds_before_gate`, `seeds_after_gate`, `depth_killed` et `candidates`, puis
  la pente de chacune avec intervalle entre trois seeds.

  Le raccord de provenance peut être fait sans table dense et sans octet de
  cover supplémentaire. `CoverPoint` occupe déjà 16 octets (`i32` suivi du
  padding d'alignement puis `i64`) : placer un `u32 handle_id` dans ce padding,
  garder `dist2q` en dernier et graver `static_assert(sizeof(CoverPoint)==16)`.
  `cover_query` et `rect_diametral_candidates` écrivent un sentinel ;
  `anchor_cover_from_handles` écrit l'indice du handle. Le counting-sort stable
  copie alors la provenance avec le point, comme le compactage d'enveloppe.
  Cela évite une table `position -> handle` de taille `n` par worker, rédhibitoire
  à plusieurs dizaines de millions de points, et évite une recherche binaire
  par seed. Le handle ne sert qu'au droit d'émission ; `scan_sites`, grille,
  témoins et census continuent à lire tous les points.

  Extraire parallèlement un `AnchorSectorState` typé depuis
  `anchor_sector_kill` : frame, comptes saturés au seuil et seize normales
  signées des demi-plans. La route par candidats diamétraux doit rendre ce même
  état au survivant au lieu de le recalculer sur le cover. Calculer le fate
  paresseusement au premier seed aigu de chaque handle et le mémoïser pour
  l'ancre ; les seize extrema utilisent directement un choix d'extrémité par
  axe, jamais huit coins. En shadow, conserver le scan OFF pour compter les
  tests effectivement évitables après cellule. À l'activation, tester le fate
  avant `q3_form` économise aussi la forme et le localisateur. Le même skip doit
  exister dans `scan_anchor_q3` et dans la boucle de construction de
  `build_q3_batch`; sinon les routes hôte et batched divergent.

  Deux fixtures ferment cette couture : handles disjoints dont les points
  s'entrelacent après le tri radial, un seul étant profond ; et parité de
  `AnchorSectorState` entre la route candidats diamétraux et le cover exact.
  La première exige un digest candidat identique tout en vérifiant que les
  points du handle tué restent dans `scan_sites`; un mutant qui les retire de
  la source témoin doit mourir. Publier enfin le nombre de handles évalués,
  seeds par handle tué, seize extrema payés, formes q3 et tests de sites évités,
  puis le mur. Avec seulement 11--13 tests par seed, cette amortisation par
  handle est désormais la vraie porte de rentabilité.

### Réponses V104--V109 : fermer la piste sectorielle chaude, garder son marginal après `tau`

- **V104 — correction reçue, chiffre retiré.** Le contrôle d'orientation
  confirme que le masque de `fibre_gain_probe` était faux sur la moitié des
  frames et n'était même pas un surmasque. Le `36,1 %` est définitivement
  rétracté. La prochaine mesure doit appeler un helper partagé extrait de
  `anchor_sector_kill`; aucune quatrième transcription du prédicat n'est
  recevable.
- **V105/V107 — la fonction objectif est marginale, mais le tableau ne la
  mesure pas encore.** Le bon numérateur est le travail effectivement évité
  sur les seeds qui auraient atteint la profondeur dans la lane OFF, après
  histogrammes, `W3`, secteur d'ancre, grille et cellule du seed. Le probe
  courant part d'`AliveRect`, appelle directement `anchor_sector_kill` au lieu
  de `anchor_kill_cumulated`, puis crédite les douze tests à **tous** les seeds
  aigus du handle. Il ne rejoue donc ni `corner_histograms`, ni `W3`, ni la
  grille, et son gain reste un plafond généreux, pas une économie causale.
  Le coût doit compter les opérations réellement ajoutées et le mur, pas
  postuler qu'un extremum vaut un test de puissance.

  Le ratio `0,04` sur `eight_clusters`, déjà obtenu avec ce numérateur
  favorable, disqualifie bien une activation sectorielle inconditionnelle ;
  aucune nouvelle présentation comptable ne la rend rentable. Les ratios
  supérieurs à un sur `terrain/scanline` restent seulement une invitation à
  mesurer le marginal réel. Employer quatre bras appariés
  `baseline/tau/sector/tau+sector` : le seul gain attribuable au secteur est
  `tau+sector - tau`, avec les mêmes seeds et le même ordre. Une règle pilotée
  par le nom de famille est interdite ; seule une propriété géométrique locale,
  calculée moins cher que ce qu'elle évite, pourrait router le résiduel.
- **V108 — le test de rayon proposé ne certifie pas un secteur.** L'inégalité
  de `narrow_cone_pregate` borne le diamètre angulaire autour de la direction
  centrale, mais ignore l'alignement avec les huit frontières. Une boîte
  ponctuelle dont la direction centrale est exactement sur une frontière
  vérifie la condition avec `G=0` et atteint pourtant les deux secteurs
  adjacents. Une boîte mince autour de cette frontière fournit la version non
  dégénérée. Le cas `t=G=0` passe aussi l'inégalité alors que la direction est
  indéfinie. Le commentaire « contenu dans un secteur » est donc faux ; le
  helper peut rester un classificateur diagnostique, jamais une porte de
  décision sous cette forme.

  Une pré-porte sonore et plus directe existe néanmoins. Déterminer le secteur
  de la projection du centre de boîte, puis évaluer seulement ses deux
  demi-plans frontières. Pour chaque forme linéaire de coefficients `c_i`, la
  variation exacte sur la boîte en coordonnées `w` vaut
  `sum_i abs(c_i)*(hi_i-lo_i)`. Si les deux valeurs au centre sont strictement
  supérieures à leur variation, toute la boîte vit dans ce secteur ; égalité,
  centre sur frontière ou projection nulle restent fail-open. Ce fast path
  demande deux formes et deux rayons de support, pas les seize extrema. Il doit
  être confronté à `box_sector_mask`, puis chronométré en instructions ou mur :
  `O(1)` ne signifie pas coût nul.
- **V109 — priorité à `g_AB/tau`, composition seulement sur le résiduel.** Les
  deux preuves sont sémantiquement compatibles, mais les payer toutes deux
  partout serait une erreur de routeur. `g_AB[64]`, puis `tau(c)`, vient en
  premier parce qu'il décide au niveau `WSPDRect x Handle`, se transpose à la
  face q4 et attaque le nombre de propositions. Le secteur reste une ablation
  q3-only après `tau`, activable seulement si son marginal causal est positif.
  Les fates sont primaires et disjoints : une face tuée par `tau` n'est jamais
  recréditée au secteur. Si le quatrième bras ne gagne rien au-delà de `tau`,
  supprimer la piste sectorielle au lieu de conserver deux terminaux.

La campagne de masses V106 peut continuer comme diagnostic de la vraie lane,
avec trois graines, commandes et sorties brutes. Elle ne remplace ni le shadow
`tau`, ni son oracle de prune, et un quotient de pentes reste interdit comme
plafond théorique.

Pré-lecture du `q3_patch_block_probe.cpp` encore concurrent : l'invariant
`mask_empty -> true_acute_seeds==0` est la bonne première porte et l'omission
de coplanarité reste fail-open. Le probe ne mesure toutefois que l'absence de
centre, ni `g_AB`, ni `h_c`, ni `tau`; ne pas le décrire comme le seul
mécanisme qui attaque la proposition. Sa grille de centres à l'échelle 4 est
une relaxation distincte du type `CenterQ32Box` spécifié pour `Phi32` et doit
porter un type et un oracle séparés. Enfin, `break` après les premiers
`blocs_cible` réintroduit l'échantillon de phase Morton déjà rejeté : choisir
les blocs par bottom-k haché avant toute fréquence ou comparaison entre
familles. Attendre cible CMake, pin propre, plancher de seeds non nul et sorties
brutes avant de recevoir ses taux.

Fixtures permanentes minimales : direction exactement sur une frontière
(deux bits), boîte dont une arête projetée traverse un cône sans coin intérieur,
boîte projetée contenant l'origine (huit bits), `p_x=0`, secteur inaccessible
peu profond contre secteur atteignable profond, mutant d'ancre représentative,
et rejet explicite de l'emploi du masque q3 en q4.

La porte V85 minimale compare l'intervalle dirigé de `chi_32` aux 4096 tuples
de coins, vérifie que le patch de chaque centre rationnel vrai survit, garde
`chi_32==0` et tue les mutants `drop-coplanarity`, `coplanarity-open` et
`coplanarity-representative-anchor`. Le reçu pertinent reste la matrice
`exact_common x certified_global x certified_patch` et le coût du front
partagé, pas le rayon empirique des centres déjà connus par l'oracle.

Le split de `C` n'est pas « identique » dans tous ses usages. Employé seulement
pour mieux reconnaître `EMPTY`, il répète effectivement le mécanisme peu
prometteur du raffinement post-séparation. Employé sous budget pour résoudre le
résiduel avec `existence=NONEMPTY` et `depth=ALL_DEEP`, il peut supprimer des
rescans et doit être évalué avec le contrefactuel V74. Le prochain geste de la
fibre reste le schéma d'état et le center-cover counter-only sur **tous** les
blocs, avec un front masqué mais sans `global_common` ni décision produit :
l'oracle exhaustif stratifie ensuite les cohortes non capées. En
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

## Différentiel v3/v4 : ne pas rebaptiser P1

Les 64 patches et le DFS masqué ne sont pas nouveaux. Le commit v3
`b312638c` contient déjà, dans
`prototype/center_cover_mass_probe.cpp`, une pile
`(nœud,masque de patches)`, un crop et une antichaîne par patch ; cette route
`P1a center-cover` porte les pentes rouges `2,104/1,896`. Le landing CUDA
enregistré `95dd8036` utilisait au contraire 64 parcours logiques indépendants
et n'avait pas de run natif à sa réception. Enfin, le commit v4 `40b309c3`
mutualisait déjà une traversée haute du cover par rectangle WSPD, puis
filtrait localement le résultat pour chaque ancre, avec un gain cumulé annoncé
de 37 fois. Ni `L32`, ni le pavage, ni le front masqué pris isolément ne
constituent donc le delta v5.

Le delta à falsifier est exactement :

`WSPD rectangle -> g_AB[64] une fois -> masques C -> t_C -> ledger pondéré`.

Il doit supprimer les filtres/scans locaux par ancre de la v4. Pour `R`
rectangles, `k_r` handles, `V_r` nœuds physiques et `T_r` tests
patch--nœud, sa facture est
`O(64R+64*sum_r k_r+sum_r(V_r+T_r)+facteurs A/B)`, avec
`T_r<=64V_r`. Cette écriture retire `k_r*V_r`, mais ne prouve aucun pire cas
sous-quadratique : `sum_r k_r` peut être quadratique, `sum_r V_r` peut valoir
`Theta(Rn)`, les facteurs courants gardent `|A|^2+|B|^2` et l'émission
résiduelle garde `sum_C P[t_C]`.

Le reçu différentiel publie donc, sur les mêmes familles et tailles que P1a,
`wspd_rectangles`, `sum_handle_masks`, `g_ab_witness_node_pops`,
`patch_node_tests`, `ALL/NONE/MIXED`, feuilles, high-water, `sum_P_t_c`,
`q3_weighted_roles_proposed`, classes `s_H`, faces q4, groupes axiaux et les
propositions réellement transmises au terminal. Le ratio
`patch_node_tests/witness_node_pops` ne suffit pas : la pente de chacun des
deux termes doit rester visible. L'oracle publie séparément
`q4_handle_pairs_streamed`. Une baisse de constante ne rouvre pas la route :
la pente de chaque générateur, puis mur/HWM à 50 k, doit réfuter le motif
historique. Aucun code ni reçu v3/v4 n'est importé ; seules leurs
contre-fixtures et mesures sont épinglées comme différentiel.

## Certificat sûr : center-cover conditionné par $C$

Le repli brut par intervalles sur `A,B,C,W` n'est plus une route candidate au
pin `650b3cff` : il est sûr lorsque sa borne supérieure est strictement
négative, mais les dépendances l'ont rendu pratiquement inerte. Le conserver
seulement comme contre-sonde bornée `pointwise_raw_aabb_pi_certificate`, avec
fixtures ponctuelles et petites boîtes étendues ; il n'est pas un oracle
géométrique indépendant puisqu'il réévalue la même forme de Gram que q3.

La forme à encadrer est exactement celle de `q3.hpp`. Avec `d=b-a`, `u=c-a`,
`y=z-a`, `D=d.d`, `E=u.u`, `F=d.u`, `G=D*E-F*F` et
`W=E*(D-F)*d+D*(E-F)*u`, poser :

$$\Pi(a,b,c;z)=G(y\mathbin{\cdot}y)-y\mathbin{\cdot}W.$$

Construire les intervalles par `add/sub/mul/square`, le carré prenant zéro
comme minimum s'il traverse zéro. L'identité de Gram autorise à intersecter la
borne de $G$ avec `[0,+inf)` sans perdre de valeur réelle. `Pi_upper < 0`
signifie `ALL_STRICT_INTERIOR`, `Pi_lower >= 0` signifie seulement que ce nœud
ne fournit aucun témoin, et tout autre résultat reste `MIXED`. Cette voie sert
uniquement à falsifier le raccord par patches sur un domaine borné ; elle ne
justifie ni un rescan par handle, ni un fallback produit.

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

Un seul `computed_patch_mask` ne suffit pas dès qu'un budget peut interrompre
le front. Chaque patch porte `UNSEEN`, `PARTIAL`, `EXHAUSTED` ou `SATURATED`,
ainsi que le seuil de saturation utilisé. `PARTIAL` fournit un minorant
utilisable mais reste `UNKNOWN` s'il n'atteint pas le seuil ; `EXHAUSTED`
signifie que la source a été entièrement visitée ; `SATURATED` certifie
seulement `g[j]>=cap` et ne devient pas un compte exact réutilisable avec un
cap supérieur. Des masques `requested/exhausted/saturated` peuvent encoder ce
contrat. Un zéro partiel ou non vu n'est jamais un zéro calculé.

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
`(A,B)`. Pour tout triplet valide `t`, chaque témoin facteur appartient à
`I_t` et minore donc sûrement `depth(t)`. Si la fibre fixée est **non vide**,
le sous-ensemble facteur est inclus dans son intersection exacte et sa
cardinalité, notée conceptuellement `h_a_factor(a)` ou `h_b_factor(b)`, minore
bien $h_a^F(a)$ ou $h_b^F(b)$. Si la fibre est vide, la convention impose au
contraire $h_a^F=0$ ou $h_b^F=0$ : aucune comparaison numérique avec le compte
facteur, éventuellement positif, n'est revendiquée ; la sûreté est vacante
faute de support. Par convention historique, le reste de cette note et le
code appellent encore ces comptes `h_a,h_b`; « exact » signifie seulement
« compte exact du sous-ensemble certifié par le facteur », jamais
« cardinalité exacte de la fibre `F` ». Après un split de `C`, ils restent des
crédits sûrs pour chaque support enfant valide, mais ne deviennent ni une
preuve de non-vacuité, ni les comptes exacts des fibres enfants.

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

Un futur `h_c(c)` prend ses témoins dans `C` privé de `A union B`. `g_AB[j]`
et $h_c(c)$ peuvent partager un autre site de `C` : les additionner nus reste
donc faux. La fixture minimale prend `a=(0,0,0)`, `b=(4,0,0)`, `c=(2,3,0)`
et `z=(2,1,0)`, avec `c,z` dans le même `C` : `z` appartient à $W_3(a,b)$ et
est strictement intérieur à la circumboule de `(a,b,c)`, donc peut vivre à la
fois dans `g_AB[j]` et $h_c(c)`. La sous-section suivante donne toutefois une
repartition scalaire par handles qui évite de conserver tous les rangs.

### Algèbre stratifiée des témoins : `+` entre domaines, `max` dans un domaine, pire patch entre alternatives

Les handles `H_i` sont des nœuds d'une antichaîne et leurs plages de positions
sont disjointes. Définir les **strates témoins**, qui ne changent pas la
partition des rôles carriers :

$$S_i=\mathrm{range}(H_i)\setminus(A\cup B),\qquad S_\bot=P\setminus\left(A\cup B\cup\bigcup_i S_i\right).$$

Un carrier `c` peut encore appartenir physiquement à `A` ou `B` lorsqu'il est
distinct de l'endpoint effectivement choisi ; seule sa source de témoins
locale retranche toutes ces positions. Cette différence de rôles se calcule
sur les plages de positions, jamais par soustraction d'AABB.

Pour un patch `j`, soit `G_j` un sous-ensemble certifié par `g_AB[j]`, hors
`A union B`, et poser `g_{r,j}=|G_j intersect S_r|`, avec `r=bot` pour la strate
extérieure. Pour le handle carrier `H_i`, poser
`g_{not i,j}=sum_{r!=i} g_{r,j}`. Si `C_{i,j}(c) subseteq S_i` est un facteur
local de cardinalité `h_{c,j}(c)`, alors les strates donnent immédiatement :

$$\left\lvert G_j\cup C_{i,j}(c)\right\rvert\geq g_{\neg i,j}+\max\left(g_{i,j},h_{c,j}(c)\right).$$

Avec le `core` historique seulement scalaire, le crédit patch-spécifique sûr
est donc :

$$b_{i,j}(c)=\max\left(h_{\mathrm{core}},g_{\neg i,j}+\max\left(g_{i,j},h_{c,j}(c)\right)\right).$$

Si le prochain DFS ventile aussi un sous-ensemble certifié du cœur en comptes
`k_r`, on récupère davantage sans identités individuelles :

$$b_{i,j}(c)=\sum_{r\neq i}\max\left(k_r,g_{r,j}\right)+\max\left(k_i,g_{i,j},h_{c,j}(c)\right).$$

Ces formules résument l'algèbre correcte : addition seulement entre strates
physiquement disjointes ; `max` entre certificateurs d'une même strate, car
leurs ensembles peuvent être emboîtés ; minimum numérique du crédit, ou
maximum du besoin résiduel, entre patches alternatifs. Le minimum sur les
patches n'est jamais la cardinalité de leur intersection : deux patches
peuvent chacun fournir un témoin différent.

La ventilation ne doit toutefois pas prendre la forme d'une matrice
`64 x number_of_handles`. Poser `H=h_q`. Dès que le DFS a certifié
`|G_j|>=H`, ce patch est `SATURATED_GLOBAL` : `G_j` seul tue tout support dont
le centre appartient au patch, indépendamment du handle carrier. Le patch
reçoit un fate de profondeur, pas un fate géométrique `EMPTY`, et sa
provenance devient volontairement opaque. Aucun accès à une cellule locale ni
aucune soustraction depuis ce total capé n'est ensuite autorisé.

Si la source est épuisée avant saturation, alors `|G_j|<=H-1` : huit positions
au plus en q3 et sept en q4 au profil courant. Il est donc moins coûteux et
plus exact de conserver directement ces `GeometryIndex` triés, puis de les
affecter après coup à un unique `S_i` ou à `S_bot`. Un arrêt sur budget donne
de même `PARTIAL_UNDER_CAP` avec un sous-ensemble utilisable comme minorant ;
seul `EXHAUSTED_UNDER_CAP` affirme que le certificateur a épuisé sa source.
La machine d'états par patch est ainsi
`UNSEEN/PARTIAL_UNDER_CAP/EXHAUSTED_UNDER_CAP/SATURATED_GLOBAL`, avec le cap
dans la clé.

Le DFS masqué reste unique. Pour chaque bit d'un nœud, `NONE` retire le bit,
`MIXED` descend et `ALL` crédite la plage hors `A union B`. Si le nouveau total
atteint `H`, le bit devient immédiatement `SATURATED_GLOBAL`. Sinon, cette
plage contient nécessairement au plus `H-1-count` positions utiles : les
matérialiser ne peut donc jamais dépasser le petit stockage promis, et le bit
ne descend pas sous le nœud `ALL`. Sous le profil courant sans positions
dupliquées, le compte est la taille de plage et non un poids de multiplicité ;
le helper doit refuser explicitement une autre unité.

Le post-mapping emploie les plages de handles triées et disjointes. Il ne
filtre jamais un témoin selon `seed_capability` ou selon le masque du handle
qui le contient : un site peut témoigner pour un patch auquel il ne fournit
aucun carrier. Raffiner artificiellement les handles sans changer leur union
doit laisser identiques visites, tests de patches, fates et positions
certifiées ; seuls leurs labels de strate changent. La borne candidate devient
`O(V_phys+T_patch+64*H*log(k))`, ou sans logarithme avec un curseur, et non
`O(64*k+k*V_phys)` **pour le DFS et sa provenance**. La construction séparée
des surmasques géométriques des `k` handles peut encore coûter `O(64*k)` ; ce
terme n'est ni supprimé ni caché par le stockage sparse. Publier
`patch_node_tests`, `witness_node_pops`,
`bulk_positions_certified`, `sparse_ids_materialized`, `saturated_global`,
`exhausted_under_cap`, `partial_under_cap`, `postmap_handle_hits` et
`postmap_outside_hits`.

Le mutant historique `min(H,total)-min(H,local)` reste faux, mais sa fixture
doit maintenant vérifier plus fortement qu'un patch saturé refuse tout accès
local. Pour un rectangle vivant, `core<H` fournit la même économie : tenter
`collect_universal_ids(...,cap=core)`, revalider ces positions sous le
`RectId` courant, puis former les `k_r` seulement si au moins `core` positions
sont réellement récupérées. Le `max(parent_core,fresh_core)` de `postsep` ne
transporte aucune provenance ; si la récupération échoue, conserver la formule
scalaire avec le `max` extérieur et ne jamais inventer des `k_r`.

Le facteur local utilise directement la variable centre. Pour `q=32o`, poser :

$$\Phi_{32}(q,c,z)=2q\mathbin{\cdot}(z-c)+32\left(\left\lVert c\right\rVert^2-\left\lVert z\right\rVert^2\right).$$

`Phi32>0` équivaut à `z` strictement intérieur à la sphère de centre `o`
passant par `c`. Ainsi
`center_witness_phi32_lattice_min(Q_j,{c},W)>0` crédite tout nœud témoin
`W subseteq S_i` pour le patch `j`. La requête auto-jointe scinde tout nœud
contenant `c` et ignore la feuille diagonale ; tester seulement
`Box(C) x Box(C)` resterait bloqué par `Phi32(q,c,c)=0`. Le compte naturel est
patch-spécifique `h_{c,j}(c)` et s'arrête au besoin résiduel. Les 32 positions
par handle bornent l'oracle plat, pas le coût produit à accepter sans mesure.

Pour un support ponctuel `c`, le minimum cartésien exact teste les huit coins
de `Q_j` et, sur chaque axe de `W`, ses deux extrémités : la quadratique en
`z` est concave. Pour traiter un nœud entier de supports `C`, ses huit coins ne
suffisent **pas**. À chaque coin `q` du patch et sur chaque axe, le terme en
`c` est convexe ; son minimum lattice teste `floor(q_i/32)` et son successeur,
tous deux clampés dans l'intervalle de `C`. La contre-fixture unidimensionnelle
`q=96`, `C=[0,6]`, `z=2` donne `Phi32=256` aux deux coins supports, mais
`Phi32=-32` au support intérieur `c=3`. Elle doit tuer
`box-support-corners-only`.

Cette formule autorise une auto-jointure dirigée de l'arbre déjà construit,
sans imposer la double boucle plate du handle. Un état
`(support_node,witness_node,patch_mask)` emploie
`L(Q,C,W)>0 -> ALL`, `-L(Q,W,C)<=0 -> NONE` et descend sinon ; l'identité
`Phi32(q,c,z)=-Phi32(q,z,c)` justifie la borne supérieure. Une diagonale se
scinde avant tout crédit. Un `ALL` crédite le nœud témoin à tous les supports
du nœud gauche, avec saturation individuelle au besoin ; une implémentation
peut différer cette mise à jour ou profiter du cap `|H_i|<=32`. Cette route
reste susceptible de visiter un produit quadratique si les boîtes se
recouvrent : `support_witness_node_pairs`, `ALL/NONE/MIXED` et les mises à jour
effectives constituent donc sa porte de coût, pas une promesse d'exposant.

Pour un surmasque conservatif non vide `M_i(c)` de patches possibles, condenser
les crédits en un seul seuil de carrier :

$$\tau_i(c)=\max_{j\in M_i(c)}\max\left(0,h_3-b_{i,j}(c)\right).$$

Une ancre `(a,b)` tue alors ce carrier dès que
`h_a(a)+h_b(b)>=tau_i(c)`. Un masque vide reçoit un fate d'absence séparé ; il
ne passe jamais par le maximum vide avec une valeur neutre inventée. Au profil
q3, `tau` possède dix valeurs `0..9` : `tau=0` signifie que tous les patches
encore géométriquement possibles sont déjà morts par profondeur, et non que le
masque est vide. Cette condensation n'énumère pas `A x B x C`. Avec
`P[t]=#{(a,b):h_a(a)+h_b(b)<t}`, agréger les carriers par leur seuil. Si
`C_t=#{c:tau(c)=t}` et si `X^A_{t,r}` compte les carriers `c in A` de seuil
`t` et de score `h_a(c)=r` (`X^B` symétriquement), la masse brute distinct-ID
laissée par ce certificateur vaut :

$$M_3=\sum_t C_tP[t]-\sum_{t,r}X^A_{t,r}B_{<t-r}-\sum_{t,r}X^B_{t,r}A_{<t-r}.$$

Les deux corrections sont disjointes puisque `A` et `B` le sont. La
combinaison coûte `O(H+h3^2)` après le calcul des scores, avec `H` la masse des
handles, et généralise la formule à seuil unique déjà vérifiée. Le terminal
n'énumère que le résiduel ; acuité, owner et existence gardent leurs fates
séparés.

Des tirages locaux non conservés avaient été annoncés sans divergence. Sans
source, seed, commande, sortie brute ni reçu épinglé, ils ne reçoivent pas les
formules papier. La dérivation reste une proposition algébrique à graver dans
des fixtures CTest déterministes de recouvrement, saturation et diagonale ;
elles doivent couvrir les dix seuils q3 `tau=0..9`, en particulier les deux
bords.

La fixture géométrique stratifiée minimale fixe
`a=(0,0,0), b=(12,0,0), c=(6,9,0)`, `H0={c,(6,3,0)}` et
`H1={(3,3,0)}` sur le patch ponctuel `q=32o=(192,80,0)`. Les deux autres positions
sont intérieures, celle de `H0` appartient simultanément à `g_0` et à `h_c`,
et la profondeur vaut deux : `g_rest+max(g_0,h_c)=2`, tandis que
`g_total+h_c=3` est le mutant à tuer. La fixture structurelle q4 prend un
handle de cinq feuilles : les quatre produits de frères au LCA doivent avoir
une masse totale `choose2(5)=10`, chaque paire exactement une fois.

La version autoritaire transporte un `PatchSparseCredit` : fate, cap, nombre
et tableau de `h_q-1` `GeometryIndex` au plus. Le tableau n'est lisible que
pour `PARTIAL_UNDER_CAP` ou `EXHAUSTED_UNDER_CAP` ;
`SATURATED_GLOBAL` est opaque. Deux méthodes
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
brut local non conservé avait été annoncé sans divergence avec la somme
explicite des poids. Faute de source, commande et reçu épinglé, ce compte n'est
pas reproductible et ne qualifie pas la formule ; la future fixture CTest doit
comparer les poids à l'énumération explicite.

Ce nombre inclut encore les triplets qu'acuité ou owner rejetteront : ce n'est
ni une masse de supports valides, ni un nombre de candidats. Le ledger local
peut maintenant fermer exactement
`full=outside+empty+pending+depth_killed+proposed`, puis la tape complète ferme
`3*choose3(n_unique)` en ajoutant les rectangles morts avant `AliveRect`.
Les cinq termes sont des **actions primaires disjointes** sur la même masse
brute ; une table de précédence ou une affectation terminale exacte-once est
obligatoire, car `proof_kinds` peut se recouvrir et `M_proposed` contient
encore des rôles que l'acuité ou l'owner rejetteront. Ces preuves concurrentes
ne créent jamais une seconde inscription. Toutes les multiplications des
totaux positifs sont promues en `u128` avant le produit. Les formules avec
corrections sont accumulées en `i128`, vérifiées non négatives, puis converties
en `u128`; une soustraction non vérifiée en arithmétique non signée est
interdite. La borne `NodeRange` en `i32` garde ici les produits q3 sous
`i128`, mais l'ABI vérifie cette borne avant toute conversion `u128 -> i128`.
Un parent et ses enfants ne figurent jamais simultanément dans ce ledger ; un
split remplace le parent atomiquement. Si le seuil varie à l'intérieur d'un
handle, il faut le scinder, le stratifier ou le laisser `PENDING`.

Cette fermeture règle la comptabilité pondérée q3, pas nécessairement son
terminal : l'émission effective peut encore être proportionnelle à
`M_proposed`. Elle permet toutefois de mesurer et de tuer en vrac sans cacher
un produit derrière le mot « ledger ».

ABI minimale proposée, avec tableaux de taille dix fixés au profil courant :

```cpp
struct Q3FactorBins {
  RectId rect;
  Lane lane;
  u64 grid_epoch;
  NodeRef a_node, b_node;
  u8 decision_cap;
  // Un bin par endpoint est requis pour former A_i^C et B_i^C.
  std::vector<u8> ha_bin, hb_bin;
  std::array<u64, 10> a, b, a_lt, b_lt;
  std::array<u128, 10> pair_lt;
};

struct Q3CarrierHandle {
  NodeRef carrier;
  u8 threshold;
  PrimaryFate primary_fate;
};

struct Q3RoleLedger {
  u128 full, outside, empty, pending, depth_killed, proposed;
  bool closed;
};
```

Le scratch ajoute `weight_by_t[10]`, `a_intersection_by_t[10][10]` et
`b_intersection_by_t[10][10]`. `close_q3_roles` reçoit en plus une frontier
opaque de `Q3CarrierHandle`, complète, disjointe et triée ; il vérifie
`RectId`, lane, cap, antichaîne, epoch de grille et plages avant le premier
cumul. Les `proof_kinds` ne choisissent jamais `primary_fate`. Les préfixes
valent zéro pour un indice inférieur ou égal à zéro et ne sont définis que
pour un indice au plus égal au cap ; un indice supérieur est un rejet de
contrat, car un bin saturé `cap` signifie « au moins cap », pas « exactement
cap ». Ici `t-i<=cap` par construction. Si le second facteur est interrogé au
petit cap `N-m`, son sentinel doit être canonisé au bin global `N` avec la
preuve que tout endpoint du premier facteur vaut au moins `m`, ou bien l'ABI
porte deux caps et restreint les préfixes à leur domaine. Mélanger le petit cap
de requête avec `Q3FactorBins{decision_cap=N}` est interdit.

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
  -> carrier_partition C complète/disjointe + masse de rôles + fates +
     masques AB/AC/BC ; seed_capability attachée séparément
  -> certificate_source et exact_census_source typées, sans autorité sur la
     taille des carriers ni sur leurs intersections avec A/B
  -> g_AB[64] counter-only par un unique DFS masqué : patch saturé opaque,
     sinon au plus h_q-1 GeometryIndex puis post-mapping vers les handles
  -> q3 : h_c par Phi32 sur le résiduel, tau(c) + ledger pondéré, sans A x B
  -> seulement après parité : global_common avec état commun séparé
  -> q3 : émission sparse seulement sur le résiduel
  -> q4 : tau4(c) tue les faces A x B x C avant D, puis classes s_H sur les
     faces résiduelles et terminal axial Top-r4 ; h_d reste dans l'oracle CD
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

Un split de `C` raffine seulement ses vues de supports. La partition de rôles
remplace atomiquement le parent par les enfants ; une source de témoins ou de
census peut garder le parent **ou** ses enfants, jamais les deux sans une
déduplication explicite des positions et une nouvelle antichaîne. Pour un
enfant, `Q/core/g_AB` et les comptes facteurs `h_a/h_b` restent des crédits
sûrs pour chacun de ses supports valides, `M_child` est inclus dans `M_parent`
et `t_child<=t_parent`; les facteurs capés au seuil parent restent donc
réutilisables. Ils ne deviennent ni les cardinalités exactes de la fibre
enfant, ni une preuve que celle-ci est non vide. `existence=NONEMPTY` du parent
ne s'hérite qu'à l'enfant contenant un support témoin identifié ; les autres
enfants restent `UNKNOWN`. De même, un prune universel peut porter
`action=PRUNE_NO_EMISSION`, mais `depth=ALL_DEEP` exige la non-vacuité propre
de l'enfant. En revanche, un futur `h_c` scalaire ne s'hérite pas : son domaine
change avec le sibling ; filtrer les positions ou le recalculer. Un
split de `A` ou `B` change `RectId`, les patches et les domaines de
`h_a/h_b/g_AB` : tous ces crédits sont invalidés, sauf sets de positions typés
et explicitement revalidés sous le nouveau rectangle. Aucune cardinalité
scalaire du parent ne se transmet par héritage.

La porte exhaustive à `n<=14` vérifie chaque bloc pruné, le ledger des rôles et
les diagonales. Les intervalles de **médiatrice** conservent zéro comme patch
faisable ; le crédit de témoin strict interdit `L_W==0 -> ALL` et impose
`U_W==0 -> NONE`. Elle vérifie que le patch de tout circumcentre rationnel
survit, tue les mutants qui unissent des patches ou somment `core+g_AB`, et
rend visible tout rescan témoin par `C`. La fixture `P={x},Q={y}` tue aussi
tout partage du masque de saturation avec `global_common`.
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

### La vraie généralisation commune : tuer la face `A x B x C` avant `D`

La lane actuelle confirme le bon découpage algorithmique :
`process_anchor_q4` choisit d'abord une face aiguë `(a,b,c)`, calcule son cœur
et sa corde, puis seulement parcourt les complétions `d`. Le premier objet q4
à factoriser n'est donc pas `A x B x C x D`, mais la même tape de faces
`A x B x C` qu'en q3, avec des constantes et des patches q4 distincts.

Employer `h4=smax-3`, les grilles q4 et les tableaux q4 `h_a^4,h_b^4`. Pour un
carrier `c` du handle `H_i`, construire un surmasque non vide `M_i^4(c)` avec
les médiatrices `AB/AC/BC`. En q3, la coplanarité du circumcentre ferme en plus
la direction normale ; en q4 elle est **interdite**, car les centres des
sphères passant par la face vivent sur la droite normale au plan `abc`. Un
masque non vide ne prouve toujours aucune complétion, et son vide n'est un fate
d'absence que si le cover des centres a sa propre preuve d'inclusion.

Le même `Phi32` calcule alors `h_{c,j}^4(c)` : des positions strictement
intérieures à toutes les sphères du patch passant par `c`. Une sous-source
sonore, même le cover coefficient 3, donne encore un minorant sûr mais
volontairement incomplet. Pour capter tous les intérieurs potentiels, employer
l'arbre entier ou un cover coefficient 4 prouvé complet ; le cover coefficient
3 reste la partition des complétions et ne reçoit jamais une autorité de
complétude témoin. Toute position certifiée hors de ce cover 3 appartient au
bucket extérieur `S_bot`, jamais à un handle inventé.

Pour un patch q4 sous le cap, poser :

$$b_{i,j}^{(4)}(c)=\max\left(h_{\mathrm{core}}^{(4)},g_{\neg i,j}^{(4)}+\max\left(g_{i,j}^{(4)},h_{c,j}^{(4)}(c)\right)\right).$$

Puis condenser les alternatives :

$$\tau_i^{(4)}(c)=\max_{j\in M_i^{(4)}(c)}\max\left(0,h_4-b_{i,j}^{(4)}(c)\right).$$

Dès que `h_a^4(a)+h_b^4(b)>=tau_i^4(c)`, toute complétion q4 de la face
`(a,b,c)` est profonde : la lane peut supprimer cette seed **avant** son scan
de cœur, sa corde et la boucle sur `D`. Aucun `h_d` n'est requis à ce premier
étage. Une éventuelle complétion `d` ne peut pas avoir été faussement comptée
comme témoin universel de sa propre sphère : au centre réel,
`Phi32(q,c,d)=0`, tandis que le crédit exige une stricte positivité pour tous
les centres du patch.

Cette mort ne retire `c` ni des témoins, ni de la partition des complétions, ni
du census. Elle masque seulement son droit d'émission comme seed. Elle est
compatible avec l'exact-once : si un tétraèdre peu profond existe, son centre
appartient à un bit de `M_i^4(c)` et sa face canonique ne peut satisfaire la
preuve de mort. Le choix du plus petit `PointId` entre les faces aiguës reste
au terminal ; ni l'ordre Morton ni l'ordre des handles ne choisissent le seed.

Les mêmes histogrammes que q3 ferment la masse brute des **slots de faces**
distinct-ID par classes `tau^4`, sans construire `A x B x C`. Ce nombre n'est
pas une masse de supports q4 : acuité, owner, existence de `D` et choix du seed
canonique dépendent encore du quatrième point. Le ledger autoritaire reste
`6*choose4(n_unique)`. Publier séparément
`q4_seed_faces_raw`, `q4_seed_faces_depth_dead_before_d`,
`q4_seed_faces_residual`, `q4_completion_pairs_avoided` et
`q4_hc_node_product_visits`.

Le premier raccord est counter-only. Sur `n<=14`, chaque face annoncée morte
doit vérifier par énumération que toute complétion valide possède au moins
`h4` intérieurs stricts ; ON/OFF conserve `digest_balls` et la forêt. Les
mutants prioritaires sont `q4-use-q3-patches`, `q4-require-coplanar-center`,
`q4-cover3-claimed-complete`, `q4-add-g-plus-hc`, `q4-tau-empty-mask-zero` et
`q4-drop-dead-seed-from-census`. Mesurer ensuite le coût du calcul de `h_c`
contre les scans de cœur, morceaux de corde et paires `(seed,d)` réellement
évités ; une masse de faces tuée n'est pas encore un gain produit.

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
paire de handles visitée paresseusement, former son masque **raffiné mais
conservatif** `M_CD` avec les six médiatrices. Un masque vide ferme le bloc
sans autre calcul ; sinon poser :

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

Elles avaient été confrontées dans le même scratch non conservé. Les comptes
annoncés ne sont pas reproductibles et ne ferment donc pas le calcul papier.
Une fixture permanente doit comparer les formules et la borne de seuil à
l'énumération exhaustive de petits handles et de quatre patches. Évaluer les
formules pour chaque `H,K` conserverait le carré que l'on cherche à retirer.

La réduction sûre est plus grossière et beaucoup plus utile. Pour le masque
mono-handle de **complétion** `mu_H`, avant la médiatrice `CD`, poser `s_H=0`
si le masque est vide, sinon `s_H=max_{j in mu_H}(t_j)`. Tout bit faisable dont
`g4_AB[j]` n'a pas été calculé contribue avec `g=0`, donc avec le seuil
fail-open `h4-max(core,0)` ; l'omettre sous-estimerait `s_H` et rendrait faux
le prune `s_H<=u`. Ce masque conserve les
équations possibles du sommet opposé ; ce n'est jamais le masque d'acuité du
seed. Sinon la fixture `ABc` aiguë / `ABd` droite perdrait la complétion `d`.
Le masque raffiné d'une paire, qui reste un sur-ensemble et dont la non-vacuité
ne prouve jamais l'existence d'un tétraèdre, est inclus dans
`mu_C intersect mu_D`; par conséquent :

$$t_{CD}\leq\min(s_C,s_D).$$

Pour une ancre de score `u=h_a(a)+h_b(b)`, toute paire incidente à un handle
tel que `s_H<=u` est donc tuée par le certificateur, sans calculer `CD`. Les
seuls handles de complétion encore actifs satisfont `s_H>u`. Comme `h4=8`, il
n'existe que neuf classes de seuil ; les séparer aussi par le bit
`seed_possible` donne au plus dix-huit classes. Le stream des handles remplit
leurs unions et tailles en `O(k+H)`, où `H=sum_H |H|` compte les positions
parcourues. Sous le cover actuel seulement, `|H|<=32` donne `H=O(k)` ; cette
constante est une précondition, pas une identité générale. Les bins
d'intersection avec `A/B` coûtent en plus `O(|A|+|B|)` avec une provenance
`position -> handle/classe`, ou réemploient des résumés par handle déjà
construits ; ils ne sont pas gratuits. Ne jamais allouer ou remettre à zéro un
tableau de taille `n` par rectangle : utiliser les seules plages du cover ou
des stamps.
Les formules traitent ensuite un nombre constant de paires de classes, avec un
facteur borné par `h4`. C'est un préfiltre q4 linéaire en handles **après ces
facteurs**, exact pour sa relaxation et fail-open pour la médiatrice omise.

L'implémentation n'a besoin que d'un tableau
`Q4HandleClass classes[9][2]`. Le second axe est
`certified_no_seed/seed_possible`, où `seed_possible` regroupe `YES` et
`UNKNOWN`; `NO` signifie que tout le handle est certifié non aigu, jamais
« aucun seed observé ». Une propriété oracle doit vérifier le lemme utilisé
par le gate exact-once : tout support q4 admissible possède au moins une des
deux faces incidentes à l'owner qui soit aiguë. Chaque case agrège
`handle_count`, masse de positions, union logique des handles, bins `A intersection U` et
`B intersection U`, plus le bit de non-vacuité du groupe. Ces unions ne
deviennent jamais des boîtes géométriques autoritaires : elles portent
seulement le ledger et la liste de carriers. Les décisions géométriques restent
celles des handles ou du terminal exact.

Avant le gate seed et les fates distinct-ID/non-vacuité, le nombre exact de
**slots de blocs grossiers** de cette relaxation est
`sum_s P[s]*(q_s*(q_s+1)/2+q_s*Q_greater_s)`. Ce n'est pas une masse de rôles
q4 ni le nombre de continuations non vides : ici
`q_s=#{H:s_H=s}` et `Q_greater_s=sum_{r>s}q_r`. Les formules pondérées sur les
unions de classes donnent la masse distinct-ID, puis les fates donnent les
continuations réelles. Toute paire retirée avant `CD` reçoit seulement
`PRUNE_NO_EMISSION` ; cette relaxation ne distingue pas `EMPTY` de la
profondeur universelle.
Une paire de classes dont les deux capacités valent `certified_no_seed` est
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
positivité et seed canonique ; il borne le **nombre de groupes de racines** par
`2*(h4-p)<=16` sous ses préconditions requalifiées. Cette borne ne limite ni
les IDs d'un groupe, ni une coquille d'égalité : les ties restent complets,
avec compteurs `root_group_sites/max_tie/shell` et fate explicite en cas de
cap. Les comparaisons de racines et d'extrémités restent exactes dans la
largeur requalifiée ; ni `double`, ni `i128` non prouvé ne décide.

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
sur leurs fibres non vides, exactement comme en q3. La porte de **face** peut
désormais ajouter $h_c$ à `g4_AB+h_a+h_b`, grâce aux positions sparse sous le
cap, sans connaître `D`. C'est seulement l'ajout de $h_d$ et la composition
simultanée des deux carriers qui exigent une répartition par paire de handles.
La ventilation scalaire ci-dessous ferme cet oracle, mais ne doit pas
précéder la porte ternaire. Si l'oracle de paires est exécuté, il imbrique `C`,
puis `D` seulement sur les masques survivants et reste sous cap. Le chemin
produit recommandé passe au contraire de la face ternaire résiduelle au
terminal axial et ne construit pas ce produit.

Plus précisément, si `K,G_j,A_a,B_b,C_c,D_d` désignent les **ensembles de
positions** certifiés par `core`, `g4_AB[j]` et les quatre fibres, le vrai
minorant composable est :

$$\mathrm{depth}(a,b,c,d)\geq\lvert A_a\rvert+\lvert B_b\rvert+\lvert K\cup G_j\cup C_c\cup D_d\rvert.$$

`A_a` et `B_b` sont disjoints, et `C_c,D_d` le sont dans un bloc croisé sous
les domaines ci-dessus ; en revanche `K` et `G_j` peuvent recouvrir chacun de
ces deux derniers ensembles. Sans rangs de positions, le meilleur ajout
scalaire général est donc
`h_a+h_b+max(core,g4_AB[j],h_c+h_d)`, jamais
`max(core,g4_AB[j])+h_c+h_d`. Sur la diagonale, `h_d=0`. Avec des rangs,
prendre l'union capée ; retirer d'abord `C union D` de la source centrale est
aussi sûr, mais rend le calcul dépendant de la paire de handles et le réserve
à l'oracle. Les différences `C\(A union B)` et `D\(A union B union C)`
définissent seulement les domaines **témoins** : elles ne retirent aucun site
de la partition des carriers, où distinct-ID reste un fate séparé.

La ventilation q3 se transporte proprement à un bloc croisé de handles
disjoints `H_i,H_k`. Avec `g_rest,j=sum_{r not in {i,k}}g_{r,j}` et des
facteurs patch-spécifiques calculés par la même primitive `Phi32`, on obtient :

$$b_{i,k,j}(c,d)=\max\left(h_{\mathrm{core}},g_{\mathrm{rest},j}+\max\left(g_{i,j},h_{c,j}(c)\right)+\max\left(g_{k,j},h_{d,j}(d)\right)\right).$$

La somme des deux maxima est légitime parce que les deux strates sont
disjointes ; le `max` extérieur reste obligatoire tant que le cœur historique
n'est pas ventilé. Avec des comptes `k_r` du cœur par strate, remplacer ce
`max` extérieur par la somme des `max(k_r,g_{r,j})`, exactement comme en q3.
Le bucket extérieur est indispensable en q4 : les complétions vivent dans le
cover 3, mais un témoin intérieur peut être dans la fenêtre 4 ou dans la source
arbre entière. La stratification se fait donc par intersection physique avec
les handles de complétion, jamais en renommant le cover 3 comme source témoin.

Pour un bloc croisé `C!=D`, cette orientation fournit bien deux domaines
disjoints pour $h_c$ et $h_d$. Pour le bloc diagonal `C=D=H`, elle donne au
contraire `D_D=emptyset` : la paire de points est orientée, par exemple par
`PointId(c)<PointId(d)`, uniquement pour nommer les fibres, et le second crédit
scalaire vaut zéro. Poser à nouveau `D_D=H\(A union B)` puis sommer
`h_c+h_d` compterait deux fois le même domaine. Récupérer davantage exige des
sets de positions et leur union, jamais deux cardinalités nues ; l'orientation ne
préjuge toujours pas lequel de `c,d` devient le seed canonique.

Une amélioration scalaire existe toutefois sans IDs : remplacer chaque
diagonale `choose2(H)` par sa partition canonique suivant le radix tree :

$$\mathrm{Sym2}(H)=\biguplus_{N\in\mathrm{Int}(H)}\left(\mathrm{range}(L(N))\times\mathrm{range}(R(N))\right).$$

Chaque paire non ordonnée de positions distinctes apparaît exactement au nœud
LCA de ses deux feuilles, les deux facteurs sont disjoints, et un handle de
`m<=32` positions produit exactement `m-1` blocs croisés. On peut alors
additionner $h_c$ et $h_d$ avec la formule stratifiée précédente. Si `g4` reste
ventilé seulement au niveau du handle parent, le repli sûr est
`g_outside_H+max(g_H,h_c+h_d)`. Pour un nœud interne strict `N`, le reste fin
inclut aussi `H minus range(N)`, et pas seulement les autres handles. Ventiler
`L(N)`, `R(N)` et le complément de leur union récupère les deux maxima séparés
sans soustraire un total capé. Cette décomposition est une partition combinatoire, pas
une WSPD locale ni une preuve de séparation géométrique ; elle ferme seulement
le double compte diagonal et ne résout pas le carré des handles distincts. Le
hot path seuil--axial reste donc prioritaire, tandis que cette forme devient
l'oracle propre de $h_c/h_d$.

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
