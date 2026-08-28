# Question de Claude aux auditeurs — lane résidente sur device (L7), verrous V17–V30 (28 août 2026)

- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed` ; GCP non utilisé pour cette question
- **Conception :** `docs/GPU.md` § « Lane résidente sur device — conception (L7) », réconciliée avec vos G0–G2 (ordre : instrument → pool → wire indices → compaction) ; conception détaillée et contre-expertise dans `docs/analyses/gpu_20260828/`

Je retiens votre ordre (G0, G1, G2) comme premières livraisons ; la conception L7a–L7c (cover paresseux sur device, K0/K1 par rectangle, candidats compacts) vient après réception CUDA de G1. Les verrous ci-dessous sont ceux qui conditionnent L7a–L7c ; les trois premiers commits n'en dépendent pas.

- **V17 — ordre du cover.** Un counting sort stable par rounds (chunks de 8, curseurs de classe) sur device donne-t-il *le même* ordre que `anchor_cover_from_handles` (handles en ordre de pile, `u` croissant, stable par classe radiale), jugé par une porte de compteurs de sortie anticipée et par `raw_order_gate` à un fil ?
- **V18 — `di_to_double_d`.** Preuve `proof_internal` (décalage à droite avec bit collant, valeur ≤ 64 bits, un seul arrondi, `ldexp` exact) à inscrire dans `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` avant la porte de grille device ; la fixture $10^7$ tirages + milieux est complémentaire, non substitutive.
- **V19 — bornes séparables sans division (census device, L7c).** Minimiseur entier de $a t^2 + b t$ sur $[0, 65535]$ estimé en binaire64 puis évalué en DI128 sur $\lbrace \lfloor \hat t \rfloor, \lceil \hat t \rceil \rbrace$ : accord sur l'argument de convexité et $\left\vert \hat t - t^{\ast} \right\vert < 1$, avec fixture des cas où $\lfloor \hat t \rfloor \ne t^{\ast}$.
- **V20 — régime L.** Plancher de recevabilité d'un banc GPU : fraction routée hôte ≤ 1 % des sites-cover **et** ≤ 1 % du temps de lane hôte ; au-delà, banc déclaré non comparatif. Seuil d'arène (350 k sites par bloc) à confirmer à 10 M.
- **V21 — pile K0.** Profondeur de l'arbre radix sur clés Morton 48 bits distinctes ≤ 48 donc pile ≤ 49 : le chemin de débordement de K0 est retiré ou gardé comme garde non exercée.
- **V22 — ordre brut.** `raw_order_gate` à un fil avec rangs (`seed_rank`, `lens_rank`) remplace toute égalité d'ordre brut à plusieurs fils ; multiensemble + digest sont la seule égalité exigée en production.
- **V23 — `pid[u]` plat et V1.** `scanline_overlap_multiecho` passe-t-elle V1 après bucketisation ou est-elle refusée avant la lane (code 2 prouvé par une porte) ?
- **V24 — profondeur à 10 M.** Le filtre de profondeur à la génération n'est qu'un minorant ; accord pour le restreindre aux classes radiales ≤ classe du centre + rayon, avec borne écrite avant tout chiffrage L7b.
- **V25 — contrat K1.** $n_a^2 + n_b^2 \le 2^{17}$ (et non $n_a n_b$) comme contrat structurel des histogrammes de coins, routage hôte au-delà, mutant `corner-contract-na-nb`.
- **V26 — `isqrt128_floor_d`.** Boucle de correction bornée avec preuve (ou pas de Newton entier) à la place d'un « ±1 » sur `sqrt(double)` ; la corde n'est certifiée sur device qu'avec cette preuve.
- **V27 — mutants device.** `MHGP5_MUTANT_D` résolu en index constant dans `kMutants` (règle grep conservée) et masque `__constant__` par lot ; consignation du trou actuel sous `__CUDA_ARCH__`.
- **V28 — tuile et RLE par seau.** Le seau Morton du milieu d'ancre est une localité correcte ($\left\Vert c - m \right\Vert \le D/(2\sqrt{2})$ en q4) mais le RLE par seau du centre reçoit des émissions de plusieurs tuiles : votre solution 4 (runs de candidats + fusion/RLE externe) est-elle la seule à retenir pour L4 ?
- **V29 — requalification 200 k.** Le reçu 11 (`scanline` 200 k : lane q4 215 s sur 268 s) contredit le modèle par seed (~10 µs par seed) : quel chiffre fait foi et quelle sonde tranche avant qu'un budget 200 k entre dans un reçu ?
- **V30 — occupation.** L'occupation K2 (blocs par SM, spills, `%smid`) est un compteur de classe *mesure* imprimé dans chaque reçu, avec le mutant `single-block` ; aucun débit n'est cité sans lui.

## Réponse des auditeurs, requalifiée jusqu'au pin `700a38c7`

### Verdict de séquencement

**La direction est reçue comme conception bornée : conserver exactement
instrument → G0 → G1 → G2. Aucun des verrous V17–V30 ne justifie de détourner
ces trois premiers commits.** Ils conditionnent L7a–L7c et leurs chiffres, pas
le pool, le wire par indices ni la compaction stable déjà ordonnés.

Quatre corrections doivent cependant entrer dans la conception avant qu'elles
ne deviennent du code : la notation et la dérivation de V19 sont incomplètes,
V20 confond une
mesure hybride valide avec un banc device-dominant, V27 ne peut pas modifier
une constante device par lot concurrent, et la contradiction alléguée en V29
n'existe pas. Les réponses exactes suivent.

### V17 — même ordre du cover : accord conditionnel, porte directe requise

Le counting sort par rounds reproduit le scalaire si, pour chaque classe et
chaque round, il calcule un **préfixe exclusif des huit chunks dans l'ordre du
flux aplati** `(rang du handle, u croissant)`, puis avance une seule fois le
curseur de classe du total du round. Des `atomicAdd` concurrents par warp, même
séparés par une barrière entre rounds, ne fixent pas cet ordre. Il faut aussi
conserver le parcours LIFO réel : push gauche puis droite implique une visite
de la droite avant la gauche.

`counters_gate=strict` et les candidats bruts ne suffisent pas : une permutation
de deux sites du même bin peut ne changer ni verdict, ni compteur, ni candidat.
Ajouter avant eux une porte vectorielle directe
`CoverSiteD(u,dist2q) == anchor_cover_from_handles`, avec bins 0/31, égalités de
seuil, tailles de handles variables et sites d'une même classe traversant deux
handles et deux rounds. Le mutant doit inverser un préfixe inter-chunks, pas
seulement produire une permutation qui serait invisible en aval.

### V18 — `di_to_double_d` : accord après preuve exacte à 55 bits

La preuve `proof_internal` est obligatoire et la campagne aléatoire reste un
complément. Pour la magnitude unsigned `m` : si sa longueur est au plus 64,
utiliser la conversion u64 en RN ; sinon conserver exactement 55 bits
(53 significatifs + garde + round/collant), injecter dans le bit bas le OR de
tous les bits rejetés, appeler une conversion RN explicite, puis multiplier
par la puissance de deux avec `ldexp`, exacte car le résultat reste normal et
très loin du débordement binary64. Appliquer le signe après la conversion et
traiter `-2^127` sans négation signée.

Le collant sans les deux bits supplémentaires ne suffit pas :
`M = 2^125 + 2^72 + 1` fournit une contre-fixture de milieu. Graver zéro,
`-2^127`, `2^127-1` (arrondi vers `2^127`), parités paire/impaire du
significand et signes, puis tuer au minimum
`di-to-double-drop-sticky`. L'oracle borné doit arrondir l'entier exact par une
autre construction, pas simplement demander au même cast hôte de confirmer le
cast device.

### V19 — minimiseur séparable : refus de la preuve proposée, correctif court

Deux conversions entier→binary64 et une division donnent une borne de type
$\gamma_3 \simeq 3u$, pas `2^-52`. Il faut surtout lever l'ambiguïté de
$t^{\ast}$ : si ce symbole désigne déjà un minimiseur **entier**, la borne
stricte $\left\lvert \hat t-t^{\ast} \right\rvert<1$ suffit bien, car cet
entier appartient alors à `floor/ceil`. Ce qui manque est la dérivation de
cette borne depuis l'erreur sur le centre continu ; la seule borne
$\left\lvert \hat t-\tau \right\rvert<1$ ne suffirait pas.

Le contrat recevable est : conserver `a > 0` comme invariant de `BallKey` et
faire de `a <= 0` un refus/repli d'invariant ; traiter le clamp aux deux bornes
par des comparaisons DI128 exactes ; noter $\tau=-b/(2a)$ le centre
continu et prouver $\left\lvert \hat\tau-\tau \right\rvert < 4u\left\lvert \tau \right\rvert < 2^{-35} < 1/2$ sur `[0,65535]` ; évaluer ensuite exactement en DI128 `floor(hat)` et `ceil(hat)`, avec un tie-break déclaré. Ne pas appeler $\tau$ le minimiseur entier.

`synthese.txt` conserve actuellement l'autre contrat
`{floor(hat)-1,floor(hat),floor(hat)+1}` sous une borne plus faible `<1`.
Les deux stratégies peuvent être correctes, mais le code et les deux documents
doivent en choisir une seule. Aucun tie-break n'est nécessaire si l'API ne
consomme que la valeur minimale exacte ; il le devient si elle publie aussi un
représentant.

Fixture productible déjà trouvée : `a=(12478,60203,7775)`,
`b=(53169,17694,44276)`, `x=(31846,5381,8493)` donne
`tau=32167.689591063398`, le binary64 `32167.68959106339` et un minimiseur
entier 32168 ; elle tue le choix `floor` seul. L7c reste fermé jusqu'à cette
preuve, mais cela ne bloque aucun étage antérieur.

### V20 — régime L : séparer correction et éligibilité performance

Un repli hôte ne rend pas un reçu fonctionnel invalide : s'il rejoue le même
corps, conserve statut, objet et compteurs, le résultat est celui du pipeline
hybride mesuré. Le seuil 1 % peut en revanche être préenregistré comme critère
d'un claim **device-dominant** ; s'il est dépassé, garder le reçu et le nommer
hybride/non comparable au microbanc device, sans le jeter après coup.

Un ratio unique en sites-cover masque K1 et les complétions q4. Imprimer par
motif de repli au minimum : ancres, visites de handles, sites-cover, travail
`na²+nb²`, paires de lentille et tests de profondeur, avec numérateur et
dénominateur. Le temps doit être un `fallback_cpu_thread_ns_sum` comparé au
témoin CPU apparié ; une part du mur mixte n'est pas additive lorsque CPU et
GPU se chevauchent. `350 k` reste une politique d'arène à choisir par capacité
et distribution p50/p95/p99/p99,9/max à la taille cible, pas une propriété
déduite de la moyenne 35 k.

### V21 — pile K0 : accord, garde défensive conservée

Sous V1, les positions distinctes donnent des clés Morton 48 bits distinctes.
Le préfixe commun croît strictement le long de chaque arête interne ; une DFS
LIFO de l'arbre complet garde au plus un frère par niveau, donc 49 entrées
suffisent et 64 est sûr. K0 est encore plus borné :
`rect_cover_handles` cesse de développer un nœud dès que sa plage contient au
plus 32 positions. Un préfixe utile de 43 bits ne peut déjà contenir que 32
clés ; les nœuds développés ont donc au plus 42 bits utiles communs, soit une
pile K0 au plus 44.

La famille `M=2^48-1`, `M xor 2^k` pour `k=0..47` exerce la profondeur 48 de
l'arbre **complet**, mais ne doit pas être annoncée comme overflow de K0 : la
coupe à 32 l'arrête avant. Garder le test de capacité comme défense contre un
index corrompu ou futur, sans prétendre atteindre naturellement son overflow
sous le profil. `overflow-drop` doit être exercé sur l'arène/K1 ; une porte à
cap artificiellement réduit doit être nommée comme telle.

### V22 — ordre brut : accord borné, contrat de production plus large

L'ordre brut global à plusieurs fils reste non spécifié. À un fil, le tuple de
reconstruction doit être complet : `(rang original du rectangle vivant, ua,
ub, seed_rank, lens_rank)` ; le rang du rectangle après LPT n'est pas
canonique. Employer des rangs u32 : u16 déborde avant la limite M de 350 k.
Ajouter une couture mêlant routes hôte/device et frontière de lot.

En petite porte, comparer le multiensemble compact **expansé**, puis le vecteur
post-RLE élément par élément et les compteurs exacts. À l'échelle, le digest est
le reçu compact de ce contrat, pas son unique juge. Statuts, refus et compteurs
stricts restent donc contractuels même si l'ordre brut T > 1 ne l'est pas.

### V23 — `scanline_overlap_multiecho` passe V1

La famille n'est pas refusée : son générateur déduplique les XYZ dans un
`std::set` avant de créer les `InputPoint`; les multi-échos partagent parfois
`x,y`, mais pas `z`. Ses portes 8 k/32 k produisent la cardinalité demandée et
la conformité pipeline passe. Ce n'est pas la bucketisation qui autorise les
doublons : une vraie entrée API à positions répétées est bucketisée pour être
diagnostiquée, puis `run_pipeline` rend `unsupported_degeneracy`, code 2, avant
toute lane.

Ajouter une porte G1 explicite : sur cette famille,
`input_count == unique_count`, chaque bucket a longueur 1 et le `pid[u]` plat
égale l'identité représentante ; dans le bras adverse à doublon réel, un
compteur de lane override reste nul et le code vaut 2. Le `pid[u]` plat est
alors licite sur tout le domaine accepté.

### V24 — profondeur radiale : principe sûr, formule proposée insuffisante

Additionner une « classe du centre » et une « classe du rayon » ne majore pas
le terme croisé et n'est pas une preuve. Pour `m=(a+b)/2`, tout site strictement
intérieur à une boule `(c,R)` vérifie sur une seule ligne physique :
$\left\lVert 2z-a-b \right\rVert^2 = 4\left\lVert z-m \right\rVert^2 < 4(\left\lVert c-m \right\rVert+R)^2 \le 8(\left\lVert c-m \right\rVert^2+R^2)$.
Cette dernière borne se compare rationnellement aux seuils des bins sans
racine, mais elle doit être intersectée avec le cover de production courant
`dist2q <= 3*D2`. À la borne de Jung q4, elle monte à `4*D2` : elle ne retire
alors aucun site du cover courant. C'est donc une optimisation candidate, pas
la résolution générale du poste de profondeur.

La formule directement codable évite toute conversion : pour la forme q4
`det=d>0`, `np=N` et `V=d*(a-b)+2*N`, poser
$U=2\left\lVert V \right\rVert^2+8\left\lVert N \right\rVert^2$ en U192 ; tout
intérieur vérifie `dist2q*d*d < U`. Pour le bin `j`, son plus petit entier est
`L_j=ceil(j*(bound+1)/32)` ; dès que `L_j*d*d >= U`, les bins suivants sont
exclus. La stricte inégalité et l'intersection avec `bound=3*D2` sont
contractuelles.

Une solution potentiellement plus forte, à mesurer avant de figer le grand
kernel, est une descente radix **sur l'intersection exacte** cover coefficient
3 / boule candidate : élaguer si la boîte est hors du cover ou si le minimum
séparable de la puissance est non négatif ; ajouter toute la plage si la boîte
est entièrement dans le cover et si son maximum de puissance est strictement
négatif ; sinon descendre, avec arrêt à `h4`. Cela réutilise V19, conserve
exactement `depth_killed[2]` et `digest_balls`, et évite le produit
`candidats × cover`. Une descente sur la boule dans l'index **entier** ne serait
pas équivalente ici : elle tuerait des candidats supplémentaires hors du cover
coefficient 3 et changerait le digest différentiel, même si le census aval
préservait la forêt.

Un sous-ensemble arbitraire resterait fail-open pour la forêt, mais casserait
l'égalité des candidats, compteurs et digests avec le chemin CPU courant.
Avant un budget L7b, comparer au full cover `depth_killed`, candidats et digest,
en strict et dans le mutant non strict, avec frontières de bin. Le nombre de
tests économisés est une mesure, pas un compteur exact partagé avec le CPU.

### V25 — contrat K1 : accord conditionnel

`na²+nb² <= 2^17` borne le travail des deux histogrammes, implique
`na*nb <= 2^16` et borne `na+nb`; il est donc un budget suffisant plus
pertinent que le seul produit. Ce n'est toutefois pas un contrat mathématique
intrinsèque. Le travail exact vaut `na*(na-1)+nb*(nb-1)` et l'émission d'ancres
vaut `na*nb`. Le contrat le moins conservateur porte séparément ces deux caps ;
le proxy par somme de carrés reste acceptable comme politique de capacité
épinglée, mais augmente le routage hôte près de la frontière. Par exemple,
`(257,255)` satisfait les deux caps exacts mais échoue au proxy de deux unités.

Calculer tous les produits en u64 avant toute écriture et router le rectangle
entier vers le même corps hôte au-delà. Un bloc CUDA ne peut pas avoir `2^16`
threads : « un thread par paire » doit devenir une boucle stridée/chunkée dans
un bloc avec préfixes déterministes, ou plusieurs blocs suivis d'un scan global.
Graver `(256,256)`, `(257,255)` et un cas déséquilibré tel que `(512,1)` ; ce
dernier tue le mutant fondé sur le seul produit. La porte juge la décision de
routage et sa non-vacuité, pas seulement le digest, puisque le repli exact peut
masquer le mutant dans l'objet final.

### V26 — racine entière : choisir et documenter le domaine

Pour l'utilitaire générique actuel `v < 2^120`, le « ±1 » est faux : avec
`q=2^60-33, v=q²`, l'estimation observée vaut `q-95`, et un voisin donne une
surestimation de 32. Sous conversion et `sqrt` correctement arrondis,
$\left\lvert r_0-\sqrt{v} \right\rvert < 2u\,2^{60} < 256$ ; une boucle capée
à 1024 est donc sûre. Vérifier exactement `r² <= v < (r+1)²` et router hôte si
le cap est malgré tout atteint.

Pour la seule corde u16, une preuve plus forte est possible : `D²,lax,lbx <
2^34`, `G < 2^68`, donc `J/2 < 2^103` et sa racine est sous `2^51,5` ; la même
borne d'arrondi donne une erreur absolue inférieure à 1. Une seule
correction exacte suffit dans ce helper **resserré**, toujours suivie du
postcontrat. Choisir l'un des deux contrats et réconcilier `design.txt` avec
`synthese.txt`; ne mélanger ni leur borne ni leur fixture. Le seed
`a=(62929,52878,40824)`, `b=(33363,6378,5973)`,
`x=(12897,18977,21365)` force déjà une correction et tue le chemin sans
correction.

### V27 — mutants device : index oui, constante « par lot » non

Le trou actuel sous `__CUDA_ARCH__` est réel et doit rester consigné. En
revanche, une mémoire `__constant__` est globale au contexte : la réécrire pour
le lot k pendant qu'un autre stream exécute le lot k−1 rend le verdict
concurrent non défini. Deux choix sûrs : masque copié **une fois** avant tout
launch de la cible de test puis immuable jusqu'au drainage, ou masque passé par
valeur/descripteur à chaque kernel.

Le registre compte 62 noms à `d090f2cb` et exactement 64 à `fb7e9d40` ; un u32
est donc insuffisant, mais aucun index `>63` n'existe encore. Utiliser un tableau
de mots dimensionné depuis `kMutants`, avec index constexpr global et
`static_assert`. Les cibles produit gardent la constante false et refusent
l'injection. Tuer maintenant un mutant d'index `>=32`; exercer le troisième mot
avec un mutant test-only ou lors du 65e nom. Interdire une réécriture seulement
tant que des kernels peuvent lire le masque ; après drainage complet, une
nouvelle campagne peut légitimement charger un autre masque. Rejouer deux
découpages de lots avec le même code 4.

La porte d'exhaustivité du registre doit reconnaître `MHGP5_MUTANT_D` en plus
de `MHGP5_MUTANT`.

### V28 — RLE externe : solution 4 retenue d'abord, pas unique

La nécessité n'est pas « solution 4 » mais la réconciliation globale de toutes
les occurrences d'une `BallKey` avant publication, avec représentant minimal
par arité puis représentation de niveau, et ordre global de clé pour le digest.
Le seau Morton du midpoint n'apporte qu'une localité : la borne q4
$\left\lVert c-m \right\rVert^2 \le D^2/8$ ne donne ni ownership, ni taille de
seau, ni RLE indépendant.

Les runs triés par vague suivis d'une fusion/RLE globale sont le premier jalon
le plus simple et déjà retenu dans `ECHELLE.md` § 4.3. Restent aussi corrects :
shuffle append-only vers un owner calculé depuis la clé exacte, partition radix
par plages de `BallKey`, ou arbre LSM, à condition de comparer les clés
complètes et de restituer leur ordre global. Un RLE local avant fusion peut
garder le minimum local : le minimum des minima reste le représentant global.
Le tableau L4 d'`ECHELLE.md`, encore formulé « seaux Morton du centre », doit
être aligné avec le § 4.3.

Fixture : runs de tailles 1/2/3/7, même clé émise depuis deux tuiles midpoint
avec arités/niveaux différents, clés encadrantes, permutation des tuiles, spill
et replay. `rle-per-midpoint-tile` doit laisser à tort deux exemplaires.

### V29 — le reçu 11 et le modèle ne se contredisent pas

Le reçu `82f613d3` reste l'autorité pour son pin : q4 `t_rects_ms = 214544,4`
ms à 200 k. Ses branches disjointes comptent au moins 277 911 630 seeds tués
par cellules, 491 912 062 par le cœur et 171 517 481 par la corde, soit
941 341 173 seeds. À 10 µs de temps-fil par seed et 48 fils, ce **minorant**
donne un contrôle de cohérence inférieur de 196,1 s, proche des 214,5 s
mesurées. Le reçu réfute donc le modèle contenant l'hypothèse `seeds ×4`; il ne
prouve pas encore un modèle prédictif exact à 10 µs par seed.

`c95cfa95` imprime désormais `GenerateStats::seeds` par lane et les
complétions q4 : ce premier raccord est reçu. Avant toute projection, ajouter
les tests de sites cœur/corde, paires de complétion, candidats de profondeur et
évaluations de puissance, avec mur et temps-fil. Rejouer 50/100/200 k appariés ;
puis seulement normaliser la sonde K2 device, chauffée et répétée, par ces
compteurs. Il n'est pas utile d'ouvrir une session G4 uniquement pour
« trancher » une contradiction déjà réfutée arithmétiquement.

### V30 — occupation : trois objets, pas un compteur `%smid`

Séparer dans le reçu : ressources de build (`regs/thread`, smem, local/stack,
spills et hash du cubin), plafond théorique (`max_active_blocks_per_sm` avec
block/smem réels), et mesure dynamique (occupation atteinte/warps actifs sur un
sidecar profiler, plus `active_sm_count` et distribution travail/cycles par
`%smid`). `%smid` décrit le placement ; il ne mesure pas le nombre simultané
de blocs sur un SM. Les timings opposables restent ceux du run non profilé,
apparié et à digests/compteurs égaux.

`single-block` est une ablation de scheduling et une porte de non-vacuité, pas
un mutant fonctionnel censé changer le digest. Ce paquet est obligatoire avant
un débit revendiqué pour K2/L7a/L7b, pas pour les verdicts G0, les octets G1 ou
les intermédiaires stables G2.

### Coutures transverses à corriger avant L7a

1. `CoverSiteD` doit porter sans ambiguïté les formes déjà consommées par les
   kernels : `u0,u1,u2 = 2*z-a-b`, `geom_u` global et `dist2q`; alors
   `q=dist2q-D2`. Les indices `x_site`, `lens_sites` et `skip_a/skip_b`
   restent des offsets **locaux** au lot. Les deux espaces d'indices ne se
   remplacent jamais l'un l'autre.
2. La sentinelle `0xffffffff` de `CandD` n'est licite qu'après une garde de
   bibliothèque `unique_count <= INT32_MAX`, posée avant les casts actuels de
   `CloudIndex`. Un `PointId` externe reste un u32 arbitraire et ne sert pas
   de sentinelle géométrique.
3. `GpuBackendContext` possède `GpuGeometry` immuable puis le pool, dans cet
   ordre de déclaration afin que la destruction inverse draine le pool avant
   la géométrie. Chaque slot asynchrone possède ses buffers pinned, événements,
   séquence et statut jusqu'à `collect`; aucun span ne survit à
   `AnchorScratch`. Le masque de mutants est figé avant les jobs ou passé en
   argument, jamais réécrit entre deux lots concurrents.
4. L'arène est dimensionnée par les blocs **lancés**, pas par les seuls blocs
   simultanément résidents. Avec 376 `blockIdx` et 8 Mio par bloc, elle coûte
   environ 3,0 Gio, pas 1,5 Gio. Soit `gridDim` possède autant de slabs, soit
   un lease de slabs sans collision est prouvé. Deux compute streams exigent
   aussi deux arènes ou une dépendance d'événement qui interdit leur usage
   simultané. Le préflight imprime la formule et la capacité réelle en sites.

### Passage de relais minimal

1. Finir les raccords CPU et le schéma d'instrument déjà ouverts, sans GCP.
2. Livrer G0, G1 puis G2 avec leurs égalités locales actuelles.
3. Avant L7a, fermer les verrous de correction V17, V18, V21, V23, V25 et V27 ; poser les hooks V20/V30, puis mesurer seulement quand les kernels existent.
4. Avant L7b, fermer les verrous de correction V24 et V26 ; V29 conditionne ses projections et reçus 200 k/10 M, pas l'écriture du kernel.
5. Garder V19 pour L7c ; il ne doit pas retarder les étapes précédentes.

GCP non utilisé pour cette réponse.
