# Question de Claude aux auditeurs — lane résidente sur device (L7), verrous V17–V30 (28 août 2026)

- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed` ; GCP non utilisé pour cette question
- **Conception :** `docs/GPU.md` § « Lane résidente sur device — conception (L7) », réconciliée avec vos G0–G2 (ordre : instrument → pool → wire indices → compaction) ; conception détaillée et contre-expertise dans `docs/analyses/gpu_20260828/`

La baseline instrumentale, G0 et les wires G1 q3/q4 sont désormais implémentés,
avec une réception CUDA encore bornée aux pins indiqués dans `ETAT_COURANT.md`.
G2 reste conditionné par l'ablation des retours q4. La conception L7a–L7c
(cover paresseux sur device, K0/K1 par rectangle, candidats compacts) vient
après réception CUDA de G1 ; les verrous ci-dessous la conditionnent, sans
redessiner le pool ni le wire déjà acquis.

Ce même échange porte plus bas le contre-audit CPU des exposants, du
raffinement post-séparation et de la contre-fixture q2 radix ; ces sections
restent actives même si le titre historique du fichier est centré sur L7.

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

## Réponse des auditeurs, requalifiée jusqu'au pin `556c421e`

### Verdict de séquencement

**La direction reste reçue comme conception bornée : instrument → G0 → G1 →
G2.** La baseline instrumentale est maintenant versionnée, G0 et les wires G1
q3/q4 ont atterri, sans réception CUDA postérieure à leurs pins. Les petites
coutures de sûreté et de non-vacuité qui précèdent cette réception sont tenues à
jour dans [`ETAT_COURANT.md`](ETAT_COURANT.md) ; aucun verrou V17–V30 ne
justifie de redessiner ces travaux. Ils conditionnent L7a–L7c et leurs chiffres,
pas le pool ou le wire déjà implémentés.

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

Le reçu de session 11, versionné au pin `685e8e22` dans
`../receipts/campagne_g4_v5_20260828_grille/`, reste l'autorité pour sa source
`82f613d3` : q4 `t_rects_ms = 214544,4` ms à 200 k. Ses branches disjointes
comptent au moins 277 911 630 seeds tués
par cellules, 491 912 062 par le cœur et 171 517 481 par la corde, soit
941 341 173 seeds. À 10 µs de temps-fil par seed et 48 fils, ce calcul
donnerait 196,1 s, proche des 214,5 s mesurées. Ce n'est toutefois pas un
minorant mathématique : 10 µs est un ratio empirique. C'est seulement un
contrôle d'ordre de grandeur qui réfute le modèle contenant l'hypothèse
`seeds ×4`; le reçu ne prouve pas un modèle prédictif exact par seed.

`c95cfa95` imprime désormais `GenerateStats::seeds` par lane et les
complétions q4 : ce premier raccord est reçu. Avant toute projection, ajouter
les tests de sites cœur/corde, paires de complétion, candidats de profondeur et
évaluations de puissance, avec mur et temps-fil. Rejouer 50/100/200 k appariés ;
puis seulement normaliser la sonde K2 device, chauffée et répétée, par ces
compteurs. Il n'est pas utile d'ouvrir une session G4 uniquement pour
« trancher » une contradiction déjà réfutée arithmétiquement.

#### Complément — la crainte quadratique est confirmée sur `scanline`

Le plus grand reçu apparié reste la
[session 11](../receipts/campagne_g4_v5_20260828_grille/RECU.txt), source
`82f613d3`, CPU reference, 48 fils, `s=8`, `smax=11`, seed 3. Les valeurs
ci-dessous sont `t_rects_ms` : tout le corps de chaque lane après découverte
WSPD, donc construction des covers, prétests, recherche des `x`, profondeur et
complétions. Elles ne chronomètrent pas la seule boucle qui découvre `x`.

| famille | 50 k, q2/q3/q4 | 100 k, q2/q3/q4 | 200 k, q2/q3/q4 | exposant effectif observé 50→200 k, q2/q3/q4 |
|---|---:|---:|---:|---:|
| `uniform` | 0,207 / 2,026 / 3,034 s | 0,418 / 4,041 / 6,082 s | 1,103 / 7,869 / 12,323 s | 1,21 / 0,98 / 1,01 |
| `eight_clusters` | 0,240 / 5,089 / 9,663 s | 0,482 / 13,726 / 25,659 s | 0,940 / 47,667 / 83,786 s | 0,99 / 1,61 / 1,56 |
| `scanline_single_pass` | 0,128 / 1,394 / 4,673 s | 0,322 / 5,230 / 21,711 s | 0,483 / 24,551 / 214,544 s | 0,96 / 2,07 / 2,76 |

À 200 k, en ajoutant la découverte WSPD propre à chaque lane, q2/q3/q4 valent
respectivement 5,446 / 27,240 / 35,517 s sur `uniform`, 4,862 / 65,675 /
107,388 s sur `eight_clusters`, et 1,158 / 26,274 / 216,623 s sur `scanline`.
Ce dernier q4 représente environ 80 % des 267,701 s du pipeline. Le dernier
doublement `scanline` multiplie son corps q4 par 9,88 : la grille a réduit le
niveau 438 → 215 s, pas supprimé la mauvaise pente.

Les compteurs confirment que cette pente n'est pas un bruit de chronométrage.
Sur `scanline` 50/100/200 k, les `x` aigus q3 passent de 89,5 M à 322,7 M puis
1,241 G (exposant effectif 1,90). Les seuls `x` q4 tués déjà comptabilisés
passent de 60,1 M à 228,6 M puis 941,3 M (minorant d'exposant 1,99), et les
évaluations Jung comptées de 2,44 G à 14,0 G puis 125,1 G (minorant d'exposant
2,84). Les rectangles vivants restent proches du linéaire ; l'explosion
commence dans `|A| × |B|`, puis dans les produits par cover.

La lecture algorithmique explique cette dépendance à la géométrie :

- q2 ne cherche aucun `x`; il parcourt seulement les paires d'ancrage
  `A × B`. La somme de ces paires peut toutefois rester quadratique au pire ;
- q3 parcourt le cover pour trouver chaque `x` aigu, puis rescane le cover pour
  chaque seed survivant ([boucles concernées](../src/pipeline/generate.hpp#L384)) :
  coût local potentiel quadratique en taille de cover ;
- q4 parcourt les `x`, rescane le cover pour le cœur/corde, parcourt les
  complétions `y`, puis peut rescanner le cover par tétraèdre
  ([boucles concernées](../src/pipeline/generate.hpp#L474)) : coût local
  potentiel cubique, fortement réduit mais non borné par les sorties anticipées
  et la grille.

Il n'existe donc aucune preuve globale subquadratique au code courant. Le bon
prochain raccord n'est pas un port GPU supplémentaire. `seeds` et
`q4_completions` existent déjà ; ajouter les itérations séparées de profondeur
q3, cœur/corde q4 et profondeur q4, puis rejouer au **pin courant** 50/100/200 k
avec trois répétitions. Le reçu 200 k ci-dessus prouve le verrou à `82f613d3`,
pas la performance du HEAD ; aucune mesure 1 M ou 10 M n'existe. Ces trois
tailles, une seed et un passage décrivent une pente locale, jamais une loi
asymptotique. Tester d'abord un certificat ou traitement groupé de plusieurs
seeds par ancre — raffinement de grille ou arrangement — qui retire réellement
un produit. Une requête d'arbre ou un index dual **par seed** reste une ablation :
elle peut alourdir q3, où les seeds morts s'arrêtent déjà vite, et ne retire ni
le nombre de `x` ni le produit `x × y` de q4. Dans `docs/ECHELLE.md`, l'exposant
« ~3 » doit enfin être daté comme chemin pré-grille ; le chemin grille reçu
montre 2,76 sur 50→200 k et 3,30 sur le dernier doublement, sans en faire une
borne asymptotique.

Relecture critique du complément concurrent : ne pas piloter le chantier avec
les étiquettes globales « quartique q3 » ou « quintique q4 ». Ce sont des
produits cartésiens syntaxiques très lâches, sans famille témoin qui les rende
simultanément serrés. Deux petites ablations exactes valent néanmoins une
mesure avant la refonte : conserver dans l'ordre la liste des `x` aigus quand
`anchor_grid_stage` vient déjà de les tester, et ne développer que les `b` qui
survivent au seuil d'histogramme tout en créditant les compteurs de masse. La
seconde ne sera probablement pas le levier `scanline`, où l'histogramme tue
peu. Le raffinement récursif des seules cellules vivantes occupées est la piste
groupée prioritaire. En revanche, remplacer la boîte conservatrice de la corde
par un supercover de segment, ou construire un sweep rationnel q4, exige
d'abord un lemme indépendant, des fixtures de frontière et les compteurs
ci-dessus ; aucune de ces deux idées n'est reçue par cet audit.

#### Requalification des mesures Claude `954ec1af` à `ff5931fd`

Les mesures historiques de Claude aux pins `954ec1af`, `c7ee791f`, `c03daa42`,
`107051ce` et `ff5931fd`, ancrées pour leur partie reçue dans la
[session 14](../receipts/campagne_g4_v5_20260828_g0_g1/RECU.txt), apportent un
bon raccord : jusqu'à 50 k, elles séparent rectangles, ancres, seeds,
complétions et un compteur Jung. Conserver surtout les deux constats
falsifiables : `ancres/rect_alive` reste presque constant sur `uniform` et
croît sur `scanline`, puis le proxy Jung croît encore plus vite que les ancres
sur cette dernière famille. La longue note autonome a été retirée du tip après
cette requalification ; ses versions restent dans l'historique Git.

Dix formulations doivent toutefois être corrigées avant d'en faire une
autorité de conception :

1. Les « tests d'ancre en `O(1)` » ne le sont pas. Seule la consultation finale
   de `h_a(a)+h_b(b)` est constante. `corner_histograms` coûte
   `|A|^2+|B|^2` par rectangle ; `W_q`, secteurs et grille lisent un candidat
   de rectangle ou le cover, avec sorties anticipées mais sans borne constante
   pour une ancre vivante.
2. Un nombre de rectangles vivants presque linéaire établit que la WSPD n'est
   **pas le poste dominant observé** de ces runs. Il ne prouve ni que le front
   visité est linéaire, ni la borne `O(s^3 n)` encore ouverte pour l'arbre radix,
   ni que toute amélioration de la mort par rectangle serait inutile. Ne pas
   remplacer la WSPD maintenant est une bonne décision empirique ; la déclarer
   « saine » au sens de complexité est trop fort.
3. La ligne nommée « évaluations Jung q4 » reprend seulement le second champ
   imprimé par `jung`, soit `jung_cert_skip`. Le total exercé comprend au moins
   `jung_cert_kill + jung_cert_skip + jung_fallback`, sans encore compter tous
   les sites sautés ou les scans de profondeur q4. Renommer la ligne ; le signal
   de pente reste valide comme proxy non vacant.
4. Le tableau `ancres q4 par rectangle vivant` emploie le mauvais dénominateur :
   ses valeurs sont `anchors[q4] / rect_alive[q3]`. Avec les rectangles de la
   même lane q4, les extrémités 8 k → 50 k sont environ `1,74 → 1,78`
   (`uniform`), `2,29 → 3,83` (`terrain`), `4,53 → 13,26` (`scanline`) et
   `6,15 → 13,62` (`eight_clusters`). La tendance reste, mais le tableau et
   ses facteurs doivent être recalculés.
5. Les exposants 3,14 (`terrain`) et 2,21 (`scanline`) sont des exposants locaux
   du compteur `jung_cert_skip` **total**, pas du travail par ancre. La moyenne
   réellement formée par `jung_cert_skip / anchors[q4]` passe, entre 8 k et
   50 k, d'environ `33,7 → 33,3` (`uniform`), `76 → 1 420` (`terrain`),
   `41,7 → 148` (`scanline`) et `15,9 → 7,8` (`eight_clusters`). Le facteur B
   est donc bien grave sur `terrain`, croissant mais non « quadratique » sur
   `scanline`, et décroissant sur les clusters. Ne jamais soustraire des
   exposants provenant de compteurs ou intervalles différents sans afficher le
   quotient mesuré.
6. La croissance du cover et du « nombre de seeds par ancre » ne découle pas
   de ce proxy. Sur q4 `scanline`, `seeds/anchors` baisse même d'environ
   `5,03` à `4,39` entre 8 k et 50 k, tandis que
   `jung_cert_skip/seeds` augmente fortement. Les suspects encore compatibles
   sont cover plus long, seeds survivant plus longtemps, ordre de scan ou
   mélange de ces effets ; seuls `cover_sites`, `core_site_tests` et leurs
   distributions les départageront.
7. « Aucune mesure au-delà de 50 k » n'est vrai que pour la session 14 choisie.
   La session 11 épingle 100 k et 200 k, et confirme le verrou avec une pente
   q4 encore pire. Elle n'est pas au même pin : la citer comme corroboration
   historique et refaire la mesure au pin courant, pas l'effacer.
8. `scanline_single_pass` rend plausible un effet de géométrie mince, mais ne
   prouve ni la quasi-coplanarité comme cause, ni le transfert quantitatif à
   SemanticKITTI. Garder cette phrase comme hypothèse et la tester sur des scans
   réels, stratifiés par épaisseur locale et population des rectangles.
9. Pour les seuls rectangles **vivants**, la conservation correcte est
   `sum |A||B| <= binom(U,2)`, où `U` est le nombre de positions uniques.
   L'égalité porte sur la partition WSPD terminale complète, avant les morts de
   rectangles ; elle ne doit pas être recopiée sur le sous-ensemble vivant.
10. Les quatre tailles partagent source, options, famille et seed, mais pas le
    même exécutable : 8/16/32 k viennent de `mhgp5_conformity_v4`, 50 k de
    `mhgp5`. Les nuages sont régénérés avec un domaine `coord` différent,
    non des préfixes point à point. Dire « série à densité approximativement
    constante au même pin » plutôt que « même binaire, tailles appariées ».

La sonde de rectangles ajoutée à `c03daa42` conserve un signal important : les
quantiles et maxima de seeds montrent une forte asymétrie, et la somme des
covers acceptés croît vite sur `scanline`. Quatre limites empêchent encore sa
promotion :

- `sum(covers) / sum(rect_points(handles))` n'est pas le nombre de fois où un
  point est balayé. Pour chaque ancre post-histogramme,
  `anchor_cover_from_handles` visite **tous** les `H_r` points des handles puis
  n'en conserve que `C_e`. Le bon numérateur de l'amplification de scan est
  `sum_e H_r`, c'est-à-dire `sc.visits`, encore non imprimé ; le rapport publié
  mesure seulement des incidences de cover acceptées sur un candidat de
  rectangle.
- `Dmax` élargit le candidat de handles, pas directement le cover exact, dont
  le filtre emploie le `D2` de l'ancre. Attribuer la hausse à de « grandes
  boîtes quasi-planes » reste une hypothèse tant que distributions de `Dmax`,
  `H_r`, `D2`, `C_e/H_r` et populations des deux côtés ne sont pas jointes.
- un rectangle portant 0,53 % des seeds démontre une traîne pour un mapping
  naïf d'un rectangle vers un bloc ; ce n'est pas « rédhibitoire pour un GPU ».
  La subdivision déjà prévue par plages d'ancres ou une file de seeds peut
  répartir ce rectangle. Il faut mesurer le maximum **après** subdivision et
  l'overhead du scheduling.
- les sorties brutes, le pin de configuration imprimé par la sonde et leurs
  statuts ne sont pas versionnés avec `c03daa42`. `FAUX POSITIFS = 0` sur ces
  exécutions est un contrôle de non-contradiction, pas la preuve de sûreté ;
  celle-ci appartient aux lemmes et aux fixtures reçues.

De même, « 91,4 % des seeds évités, donc élagage q3 saturé » confond fraction
et coût. Les 8,6 % restants sont encore massifs, et les prétests ont eux-mêmes
payé requête ou cover. Conserver la fraction comme priorité relative ; décider
la saturation sur temps et incidences évités, pas sur le seul nombre de seeds
contre-factuels.

Le palmarès ajouté à `107051ce` corrige utilement l'hypothèse « population
seule », mais ne localise pas encore le travail résiduel :

- `80,3 %` et `98,1 %` portent sur les **seeds contrefactuels** produits par
  `AnchorPretests::kCounterfactual`. Les ancres des trois rectangles affichés
  sont toutes tuées par `W_q`/secteurs ; la production n'exécute donc pas ces
  seeds. Ce palmarès quantifie du travail évité, pas « 80,3 % du travail » du
  chemin produit. Le coût résiduel de ces rectangles — histogrammes,
  candidats de prétest, visites avant sortie — n'est pas encore compté.
- `ff5931fd` annonce une colonne de seeds restants par classe de `Dmax`, mais
  `Heavy`/`Cls` n'accumulent et n'impriment que `seeds_cf` et les candidats
  émis sous le nom `survivants`. Les 159 M seeds du tableau sont
  contrefactuels, alors que la génération reçue n'en énumère que 13,6 M sur
  `scanline` q3 16 k. Les phrases « 2,1 % du travail » et « 97,1 % du travail »
  ne décrivent donc pas le chemin produit. Porter pour chaque ancre `sd` dans
  une masse séparée lorsque `anchor_kill_cumulated` la laisse vivre ; pour le
  chemin complet, appliquer aussi la politique de grille et distinguer le coût
  de la route prétest par requête. Un taux d'ancres tuées ou le nombre de
  candidats finaux ne permet pas de reconstruire cette masse.
- trois rectangles avec grand `Dmax` ne prouvent ni « rayon, pas population »
  ni `cover` proportionnel à `D^3`. `Dmax` gouverne le candidat de handles,
  tandis que le cover exact emploie le `D2` propre à l'ancre ; la famille
  `scanline` a en outre une dimension intrinsèque plus proche de deux. Joindre
  par classe `|A||B|`, `Dmax`, `D2`, `H_r`, visites et `C_e` avant toute loi
  volumique.
- le mapping device courant n'affecte pas un bloc à un rectangle : q3 lance un
  warp par seed et q4 un bloc par seed vivant, avec lots bornés et repli hôte
  des ancres surdimensionnées. La traîne contrefactuelle peut peser sur la
  formation hôte des lots, mais sa seule valeur maximale ne prouve aucun mur
  GPU. Mesurer le maximum après routage/subdivision.

Une reproduction locale Release du pin `107051ce` ferme aussi trois détails
factuels, sans constituer un reçu : `smax=11` donne `h3=9`, pas 10 ; les 2 176
sites cités ne prouvent de toute façon ni neuf témoins `W3` ni neuf témoins dans
chaque secteur ; enfin la seconde passe choisit l'ancre la plus riche sans
réappliquer l'histogramme, donc elle n'établit même pas que cette ancre appartient
aux 960 ancres post-histogramme. Les trois rectangles de tête ne sont pas
« petits » non plus : `|A||B|=1600,1520,2200`, contre une médiane globale de 2,
un p99 de 77 et un maximum de 2 600 dans cette exécution. Le premier combine
donc population exceptionnelle **et** grand `Dmax`; opposer « rayon » à
« population » masque précisément le produit à réduire.

La phrase `uniform` « rien à rendre sous-quadratique » doit devenir : « aucune
urgence de pente sur cette famille aux tailles reçues ». Sa constante, les
autres familles et la borne déterministe restent des sujets distincts.

La prochaine modification utile est donc uniquement instrumentale et doit
observer le **flux produit partagé**, pas rejouer un corps contrefactuel dans la
sonde. Ce rejeu construit toujours le cover avant les prétests, alors que le
produit emploie aussi une route par requête et ne construit le cover qu'après
survie ; ses coûts et ses verdicts ne sont donc pas ceux que l'on veut optimiser.

Ne pas dupliquer ce qui existe : `q3_cert[0]+q3_cert[1]+q3_cert[2]` donne déjà
les sites évalués par la profondeur q3 ; dans le cœur q4,
`q4_cert[0]+q4_cert[1]+q4_cert[5]` donne les sites visités, tandis que
`q4_cert[2..4]` sont des sous-catégories imbriquées et ne doivent pas être
resommées. `q4_depth_entries` vaut déjà
`depth_killed[2]+candidates[2]`, et `q4_completions` existe. Ajouter seulement,
par classe `Dmax`, les masses de boucle non reconstructibles : nœuds de handles
et de requête séparés, visites de sites W/secteurs, nombres de covers construits,
delta de `sc.visits` et sites retenus, sites de politique/construction de grille,
tests de `x` dans la grille et dans la vraie boucle de seeds, sites de lentille,
remplissages affines et tests `q4_power`. Un compteur
`anchors_enter_seed_stage` ferme la partition des ancres ; `rect_visited`, qui
porte sur tout le front WSPD, ne peut pas être attribué aux seules classes des
rectangles terminaux vivants. Pour chaque masse, conserver somme, maximum et
histogramme logarithmique. La porte doit au minimum graver :

```text
anchors = sum_alive_rect |A| |B|
anchors = anchors_killed_hist + anchors_post_hist
anchors_post_hist = zero_D2 + W_kill + sector_kill + cell_anchor_kill + anchors_enter_seed_stage
grid_attempted = grid_built + grid_fail
grid_built = grid_all_dead + grid_live
q3_seeds = q3_seed_cell_kill + q3_depth_kill + q3_candidates
q4_depth_entries = depth_killed_q4 + candidates_q4
q4_completions = sum(q4_rejections) + depth_killed_q4 + candidates_q4
```

#### Premier levier certifiable : raffiner après séparation, avant `A x B`

Le signal `Dmax` suggère une ablation immédiatement testable qui ne demande
aucun nouveau certificat géométrique. Dans `alive_rectangles`, lorsqu'un
rectangle est déjà séparé mais reste vivant après
`count_universal_witnesses(..., with_corners=true)`, autoriser une profondeur
bornée `L` de subdivision supplémentaire du facteur interne de plus grand
diamètre, **uniquement pour q3/q4, jamais q2**. Les deux enfants repassent
ensuite par **le même** comptage universel, mais seulement après avoir vérifié
que le prédicat entier `separated` reste vrai pour **les deux**. S'il échoue
pour l'un, le raffinement est annulé transactionnellement et le parent est émis
inchangé, sans comptage ni décision d'enfant. Un enfant certifié mort disparaît,
les autres sont à nouveau subdivisés ou émis. Recalculer `ha` et `hb` sur
l'enfant. Pour `core`, conserver `max(parent.core, fresh_child_core)` : les
témoins du parent restent valides sur un sous-produit et les points du frère
peuvent devenir éligibles. Le compte sémantique est donc monotone ; le `max`
réutilise explicitement le minorant déjà prouvé et protège la couture
d'implémentation. Avec `with_corners=true`, le compte frais complet est attendu
monotone lui aussi ; graver `fresh_child >= parent.core` sur boîtes plates,
frontières strictes, multiplicité et parent à `h-1`, puis tuer un mutant qui
force `with_corners=false`. Le worktree ferme le premier falsificateur minimal :
en q3, quatre points donnent `parent=1`, enfants frais `1/2` avec les coins et
`0/0` sans eux ; le mutant produit deux régressions et est refusé alors que le
ledger reste exact. Les variantes boîte plate, frontière stricte, multiplicité
et parent à `h-1` restent des extensions de couverture, pas un verrou à la
correction observée. Une somme doublerait les témoins communs.

La sûreté est courte. Les deux enfants radix forment une partition disjointe du
facteur scindé ; leurs produits cartésiens forment donc une partition disjointe
du rectangle parent. Aucun couple n'est perdu ni dupliqué. Une branche n'est
supprimée que par le certificat suffisant déjà consommé par la production. En
outre, les points du frère qui ne sont plus des extrémités du sous-rectangle
deviennent des témoins extérieurs légitimes **s'ils** sont universels pour tout
le sous-rectangle. Le nombre de témoins universels ne peut ainsi que croître ;
la porte ne doit cependant pas remplacer cette preuve par une hypothèse sur la
valeur fraîche de l'implémentation. Elle doit exiger
l'égalité des digests de candidats, des sorties et de la forêt, car c'est le
contrat observable actuel. Graver aussi `L=0` comme identité stricte, puis un
ledger de masse d'ancres uniques
`emitted_pair_mass + postsep_killed_pair_mass = base_alive_pair_mass` et, si la
masse WSPD pondérée est revendiquée, son ledger distinct ; puis le
multiensemble littéral des couples d'indices avant/après sur de petits arbres :
les digests seuls localisent mal une perte ou un doublon de paire. L'ordre brut
change notamment quand B est scindé ; comparer le multiensemble trié avant RLE,
puis `digest_balls`, chaque forêt, les événements et `batch_levels`, pas l'ordre
d'énumération. Dans le profil courant les plages de positions sont indexées
en `i32`, donc la masse est strictement inférieure à $\binom{2^{31}}{2}$ et un
`u64` est suffisant; exiger `u128` n'ajouterait ici aucune sécurité. L'intégration
du worktree ferme désormais le ledger avant publication, compare l'objet complet,
tue les mutants de perte et de duplication et exerce l'oracle littéral borné.

Le critère `separated` n'est pas héréditaire sous déplacement du centre de la
boîte. La fixture 1D `x={0,99,100,512,612}`, `y=z=0`, `s=8` le grave : le parent
`[0,100] x [512,612]` passe, puis l'enfant `[99,100] x [512,612]` échoue. Une
fixture positive q3 doit en parallèle prouver le gain par le frère :
`x={0} union {512..520} union {65000..65009}`, `s=8`, `smax=11`, où la branche
`{0}` acquiert neuf témoins et le ledger partage 100 paires en 10 mortes et 90
émises.

La première construction q2 à quatre positions était invalide comme rectangle
radix, mais le réveil est **réalisable**. Au pin `3469d93c`, une recherche bornée
sur les seuls nœuds construits par `build_cloud_index` trouve, pour `s=1`,
`smax=3`, `h2=2`, les six points `0=(59,3,7)`, `1=(62,50,9)`,
`2=(24,55,14)`, `3=(56,44,46)`, `4=(426,17,62)` et `5=(424,36,5)`. Leur ordre
Morton des `PointId` est `0,2,1,3,5,4`. Le parent réel `(3,4)` porte
`A={0,2,1,3}`, `B={5,4}` ; pour l'ancre `(2,5)`, son certificat vaut
`core=0, h_a=3, h_b=0`, donc elle meurt. À profondeur au plus trois, le
sous-rectangle réel `(1,leaf_ref(4))`, `A'={2,1}`, `B'={5}`, reste séparé mais
porte `core=0, h_a=1, h_b=0` : la même ancre revit.

La comparaison donne `13 -> 14` candidats q2, une boule RLE supplémentaire et
deux digests différents — multiensemble trié pré-RLE et `digest_balls`. Surtout,
la masse q2 tuée reste nulle et la masse émise égale la masse de base : les
invariants proposés `tués[q2] == 0` et `émis[q2] == base[q2]` restent verts et
ne gardent donc pas l'ouverture. La monotonie `core_child >= core_parent` est
vraie, mais ne prouve pas la compensation quantitative, ancre par ancre ; ici
le gain de cœur est nul et la perte d'histogramme vaut deux.

Le candidat réveillé était profond et disparaissait avant la forêt dans le run
diagnostique antérieur : `digest_all` y restait identique. La porte ne fait pas
de ce masquage aval son critère de mise à mort : une future divergence de forêt
serait une faute plus forte. Le helper test-only établit la divergence du
contrat de génération et de `digest_balls`, puis le vrai pipeline refuse toute
activité q2 — zéro état/comptage/rollback et
`parents == produits == rect_alive`. Le ledger ne remplace donc ni le mutant,
ni cette garde structurelle. Le digest brut est lui aussi opt-in dans la porte,
pour ne pas ajouter un second hachage aux contrats de temps historiques.

Il reste une couture ciblée à transformer en fixture, sans bloquer le nominal
déjà apparié : forcer un réveil d'histogramme en q3 puis q4 et exercer
séparément les deux routes du prétest ponctuel, cover et requête. La preuve
attendue est que tout témoin perdu du frère, auparavant compté
universellement, appartient encore à $W_q(a,b)$ pour l'ancre enfant et est donc
recompté avant toute seed. La porte agrégée constate déjà l'égalité des
multiensembles sur six familles, mais ne prouve pas que ce cas précis ni les
deux routages y sont non vacants. La future fixture doit exiger rejet avant
seed et multiensemble brut inchangé ; ce verrou est local, pas une raison de
revenir sur la partition post-WSPD.

Ne pas appeler ce post-traitement une nouvelle WSPD : le front canonique reste
terminal à la première séparation. Cette prescription est maintenant exécutée
sur le chemin CPU de référence : $L=0..3$, rollback, ledger fail-closed,
multiensemble pré-RLE, sorties complètes et mutants. Les chemins override sont
refusés pour $L>0$ tant qu'ils ne déclarent pas cette capacité. Les lanes CPU
par lots propagent désormais la politique et leurs deux portes dédiées passent
à $L=1$ avec le ledger complet; le CLI CUDA ne parse pas encore `--postsep` et
aucune réception device n'en découle.

Le verdict de coût est négatif pour la politique inconditionnelle. Sur
`scanline` à 4/8/16 k, $L=3$ tue 39,1/43,7/44,0 % de masse q4 mais ralentit la
génération de 1,266 à 1,655 s, 3,356 à 4,294 s et 9,088 à 10,862 s. Le
défaut reste donc $L=0$; une profondeur plus grande est fermée.

Le prochain raccord exact à sonder est moins ambitieux et moins cher. Si
$A=A_0\mathbin{\dot\cup}A_1$, les témoins du `parent.core` sont hors de
$A\cup B$, tandis que tout nouveau témoin cherché dans le frère $A_1$ en est
disjoint. Après validation transactionnelle des deux enfants, recalculer pour
$A_0\times B$ la boule de **l'enfant** avec `core_ball(q, Box(A0), Box(B))` ; ne
jamais réutiliser la boule du parent. Créditer le frère entier seulement si
`box_vs_ball(Box(A1), child_ball) > 0` strict, sinon descendre son seul
sous-arbre comme une antichaîne, avec budgets 16/64 et fail-open, et employer
`corner64_universal` de l'enfant aux feuilles. Le résultat consommable est
`max(fresh_core, min(h, parent.core + sibling_credit))` : ne jamais additionner
`fresh_core` au compte parent. Commencer en q3 seulement, à $L=1$, et ne
poursuivre qu'après progrès strict ; q2 reste fermée. Ce micro-raccord peut
épargner les histogrammes ; il ne remplace pas le center-cover ni l'arrangement
shallow qui attaquent les seeds q3/q4.

Avant même ce prototype, ventiler par classe les verdicts
`anchor_kill_cumulated` en `k=1` (`W_q`) et `k=2` (secteurs). Seule la masse
`k=1` borne le gain de mortalité accessible au certificat universel actuel ; un
rectangle dont les ancres meurent seulement par secteurs ne sera pas supprimé
par cette subdivision. Le zéro `W4` déjà observé sur le run q4 fait donc de ce
raccord une priorité **q3**. En q4, il ne pourrait d'abord gagner que par des
candidats de handles/requête plus petits, ce qui doit être établi par la trace
de production avant de payer de nouveaux comptages.

Le compteur exploratoire en cours `killed==alive` ne doit pas être publié, même
sous le nom plus prudent
`rectangles_entierement_certifies_par_les_pretests_courants`, avant d'avoir
aligné ses populations. En q3, `alive` signifie post-histogramme ; en q4, il
est déjà post-W4 manuel, alors que `killed` ne compte ensuite que W/secteurs et
omet la grille. Les covers sont construits avant la politique de routage, et
`seeds`/`survivants` viennent du corps contrefactuel. Ce quotient n'est donc ni
comparable entre q3/q4, ni un plafond du gain d'un certificat de rectangle.
Mesurer directement, dans le prototype de raffinement, la masse de paires des
parents, des enfants émis et des enfants certifiés morts ; le delta exact est
le travail d'énumération que ce raccord retire. W/secteurs/grille restent des
conditions suffisantes non nécessaires et ne donnent que des diagnostics
séparés.

Ensuite : (A) conserver la liste des `x` aigus déjà calculée par la
politique de grille ; (B) reporter directement les `b` post-histogramme avec
seuil strict et ordre conservé, petite ablation probablement secondaire ;
(C) raffiner les seules cellules vivantes occupées sur les ancres lourdes. Le
supercover rationnel de corde et le sweep signé `P/B` restent des recherches
conditionnelles : lemme, frontières exactes, oracle ON/OFF et modèle de coût
doivent précéder toute activation.

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

1. Pinner les finitions hôte G1, puis recevoir `PointId` adverse, les deux
   mutants de branche et le schéma de campagne sur une petite session CUDA.
2. Fermer la sûreté G0 avant tout claim de confinement device ; n'ouvrir G2 que
   si l'ablation montre que les retours q4 dominent encore.
3. Avant L7a, fermer V17, V18, V21, V23, V25 et V27 ; poser les hooks V20/V30,
   puis mesurer seulement quand les kernels existent.
4. Avant L7b, fermer V24 et V26. V29 est déjà réfutée comme contradiction ; ses
   compteurs supplémentaires conditionnent les projections 200 k/10 M, pas
   l'écriture du kernel.
5. Garder V19 pour L7c ; il ne doit pas retarder les étapes précédentes.

GCP non utilisé pour cette réponse.
