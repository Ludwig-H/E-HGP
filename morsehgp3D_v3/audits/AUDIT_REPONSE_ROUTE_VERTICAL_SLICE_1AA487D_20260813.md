# Réponse à Claude — fermer une tranche verticale avant d'optimiser la source

Date : 13 août 2026 UTC.

Cadre : phase=exploration_v3_hors_registre,
backend=cpu_reference_bounded_oracles_and_g4_diagnostic,
profile=quantized_u16_input_only,
mode=audit_independant_math_and_architecture,
public_status=not_claimed.

Snapshot observé : HEAD
1aa487d77b447d7359ba9a81b7ab1285b4a27abf, worktree propre à
2026-08-13T21:38:54Z. Empreintes SHA-256 :

- prototype/wspd_wavefront_probe.cpp :
  0e7d4d753fd52adfd2d007659fe845025d8bafeea608e0b3c323edae086c19e0 ;
- CMakeLists.txt :
  c76776a579e6e2c57881b450bb999b7285bbd2edd3b6687f16a1cd8c0af54df4 ;
- NOTE_CLAUDE_ROUTE_50K_PUIS_DIZAINES_DE_MILLIONS_20260813.md :
  c3cfb97ccdd3d65757db7ba5d0480db56fe1d0edf5eb3c3e9ac5912bfb1ad970.

GCP non utilisé.

La note de Claude déclare backend=cpu_reference et
mode=proposition_math_non_recue. Ces valeurs ne sont pas le cadre v3 courant et
n'ouvrent aucune phase : elles sont relues ici sous les cinq champs de
l'en-tête du présent audit.

## Verdict sur l'ordre

L'ordre général de Claude est juste : il faut fermer une tranche verticale
output-bearing avant d'ajouter un autre certificateur. Une fenêtre non linéaire
ne rend pas ce squelette prématuré ; sans l'objet final, elle rend seulement sa
propre mesure non causale.

La correction importante est de ne pas confondre :

- une tranche verticale bornée qui reçoit la sémantique ;
- le fold industriel et son ordonnance scalable ;
- la source sparse qui remplacera l'oracle exhaustif.

L'étape zéro doit produire le BenchmarkOutputContract-v1 complet sur petit n,
depuis une source exhaustive indépendante. Elle peut employer des rescans et
des spools lents. Elle ne doit ni prétendre au SLO, ni figer un layout device,
ni attendre que la fenêtre soit linéaire.

L'ordre corrigé est :

    0A  BallForm -> BallEvent exact et politique de dégénérescence
    0B  oracle exhaustif borné -> fold -> payload complet
    1   remplacer seulement la source par E3/E4 et mesurer E/M/BallRuns
    2   Q3 owner-edge + PrimitiveSphereKey + range-count, shallow q4 local
    3   portage device après parité de toute la tranche
    4   changement de profil numérique dans une phase séparée

## 1. Réponse à la question 1 : oui à la tranche verticale d'abord

Le prochain jalon ne doit pas seulement construire des types vides. Il doit
prendre un petit nuage, énumérer exhaustivement ses BallForm q2/q3/q4, produire
les vrais ensembles I_B et U_B, appliquer owner, lots égaux, activations,
gateways, coverage, dix forêts et verticales, puis comparer chaque membre au
BenchmarkOutputContract-v1 de référence.

Trois statuts distincts sont nécessaires :

- source_complete : toutes les BallForm du domaine borné ont été produites ;
- ball_events_complete : chaque forme a une BallKey, I_B, U_B et une
  disposition reçue ;
- fold_complete : le manifeste, les lots, forêts, verticales et coverage ont
  été engagés atomiquement.

Un producteur incomplete peut alimenter le même sink pour le développement,
mais il ne publie jamais un succès officiel. Ainsi la source sparse pourra être
substituée sans modifier le juge aval.

Le premier différentiel utile est :

    exhaustive BallForm
      -> BallFormToBallEvent-v0
      -> RegularDirectRecord ou PlateauDisposition
      -> AnchorOutputFoldCounter-v0
      -> BenchmarkOutputContract-v1 borné

Cette tranche décide les identités persistantes avant les optimisations. Elle
ne décide pas la forme du buffer GPU. Le spool borné peut être matérialisé dans
l'oracle. Il n'existe pas de watermark monotone par ancre : la route produit
devra prouver un spool externe chunké, le tri/merge global des niveaux, le
manifeste scellé puis l'engagement atomique. « Streamé » signifie ici mémoire
résidente bornée, jamais commit online au fil des ancres.

Une fenêtre rouge après cette étape ne jette pas le squelette. Elle réfute la
source candidate et lui substitue cages, PWC ou une autre relation, tout en
conservant exactement le même BallEvent et le même fold.

## 2. L'intégration owner live reste indexée par GenerationRank

Le pin corrige utilement le rang 4384, accumule la masse en i128 et déplace le
juge q3 vers un compteur global. Le rejeu suivant rend 6/6 tests verts en
0,13 s :

    ctest --test-dir build/v3 --output-on-failure \
      -R '^mhgp3v_wspd_wavefront_(fixtures_owner|fixtures_rang|q3_)'

L'owner annoncé n'est toutefois pas encore l'owner PointId. Aux lignes
631–649, owner_edge compare les entiers u, v et w. Dans la vague globale, ces
entiers sont des positions dans sp, donc des GenerationRank. Le tableau spid
qui porte les vrais PointId n'est jamais transmis. Le SupportKey du juge aux
lignes 1752–1755 est lui aussi trié par positions.

La fixture passe parce que son ordre de vecteur est identique à son ordre de
PointId. Elle ne permute ni les labels ni l'ordre Morton. Le sujet et la vérité
globale appellent en outre la même fonction owner_edge ; ils peuvent donc
s'accorder sur le même mauvais tie-break.

Contre-fixture exacte : conserver les trois coordonnées équilatérales, mais
leur attribuer les labels suivants :

- PointId 0 = (101,100,101), clé Morton 2064837 ;
- PointId 1 = (100,100,100), clé Morton 2064832 ;
- PointId 2 = (101,101,100), clé Morton 2064835.

L'ordre Morton est alors 1,2,0. Toutes les arêtes ont longueur carrée 2.
L'owner scientifique par PointId est l'arête (0,1), tandis que le code live
choisit les deux premières positions Morton, donc l'arête (1,2).

La réparation est de passer explicitement les trois PointId au comparateur,
de former l'EdgeKey avec ces labels et de keyer le SupportKey global par les
spid triés. La fixture doit tester l'équivariance après au moins deux
relabelings et un mutant owner-generationrank. Les six verts actuels restent
valides pour leurs autres propriétés, mais ne reçoivent pas cette sémantique.

Le tableau de couverture en O(n³) reste acceptable comme oracle borné après
préflight de sa multiplication ; il ne doit pas devenir une structure produit.

## 3. Réponse à la question 2 : préparer la couture numérique, pas binary64

Le profil live est quantized_u16_input_only. Passer à binary64 dans ce chantier
sans phase, domaine et oracles propres violerait le cadre déclaré. La montée à
un, dix ou trente millions de points n'est donc pas une étape quatre de la
qualification v3 actuelle ; c'est un successeur formel.

La cardinalité n'impose d'ailleurs aucun changement de profil : le cube
u16 tridimensionnel contient $2^{48}$ sites distincts, bien davantage que
trente millions. Si des familles futures sont spécifiées en binary64, c'est
leur contrat d'entrée qui motive le nouveau profil, pas l'échelle 10 M.
L'index dense local est une question séparée : u16 suffit à 50 000 mais pas à
10 M ; son codec doit pouvoir devenir u32 sans changer les PointId ni la
géométrie.

Il faut néanmoins empêcher l'étape zéro de rendre ce successeur impossible.
La séparation correcte possède trois couches :

1. ExactKernel transforme les coordonnées du profil en décisions géométriques
   et en une clé de sphère canonique.
2. SphereIdentity fournit comparaison exacte, hash avec contrôle de collision
   par la valeur complète, niveau exact et sérialisation versionnée.
3. BallEvent et le fold ne consomment plus la représentation arithmétique
   native ; ils consomment l'identité, les PointId, I_B/U_B, owners,
   dispositions et preuves.

Pour u16, PrimitiveSphereKeyDeviceU16 peut rester un tuple fixe à deux limbs
par coefficient dans le hot path. La sérialisation canonique ne recopie ni
__int128 natif ni son endianness. Une conversion vérifiée produit la
SphereIdentity.

Pour de futurs mots binary64 finis, les coordonnées sont des rationnels
dyadiques exacts. Le kernel pourra employer filtres dirigés, expansions et
repli multiprécision ; les coefficients q3/q4 pourront avoir une longueur
variable. Cette variabilité reste derrière SphereIdentity. Le fold n'est ni
recompilé autour d'un BigInt particulier, ni autorisé à comparer des
approximations binary64.

Il faut donc préparer maintenant :

- schema_version, input_profile et exact_kernel_id ;
- un encodage canonique de scalaire exact à longueur explicite ;
- une comparaison de niveaux abstraite mais exacte ;
- une conversion vérifiée du fast key u16 vers l'identité canonique ;
- des digests toujours accompagnés d'un contrôle sur la projection complète.

Il ne faut pas préparer maintenant :

- les largeurs q3/q4 binary64 non encore bornées ;
- un kernel multiprécision général ;
- un claim 10 M ;
- la copie des layouts ou décisions de la ligne enregistrée.

Ainsi l'ABI sémantique est préparée à l'étape zéro et l'implémentation
binary64 reste séparable dans une phase future.

## 4. Réponse à la question 3 : refus courant, run réversible, quotient futur

Dans le contrat exact courant, la réponse scientifique est déjà fixée. Un
support propre utile dont la miniball porte un point extérieur supplémentaire
sur son shell viole RelevantGP. Tant que le quotient des plateaux et shells
dégénérés n'est pas prouvé et implémenté, l'exécution retourne
unsupported_degeneracy. Elle ne choisit ni un support arbitraire, ni une
perturbation, ni une troncature.

La cosphère u16 à 384 points est donc une fixture de refus du domaine régulier,
pas une obligation de matérialiser immédiatement ses 2 322 560 SupportKey.
Une première PrimitiveSphereKey suivie du census révèle U_B différent de S ;
la transaction peut refuser avant d'énumérer les combinaisons de supports.

Ce refus ne doit pas être gravé comme une impossibilité du layout. Le record
interne réversible est un SphereRun :

    SphereRun
      SphereIdentity et niveau exact
      I_B et U_B complets ou handle streamé reçu
      SupportRun régulier éventuel
      disposition = regular_direct | plateau_lossless | explicit_support_stream
                    | saturated_h0 | unsupported
      degeneracy_policy_id
      preuve et manifeste de source

La branche regular_direct exige U_B=S et forme RegularDirectRecord. Un
plateau_lossless conserve SphereIdentity, niveau, I_B, U_B, q_min, masque
d'ordres et provenance ; ces données permettent de régénérer et tester les
sous-ensembles de support sans prétendre que chaque combinaison est positive.
Les branches explicit_support_stream et saturated_h0 ne publient rien avant
leur propre réception. Dans le profil actuel, une coquille extra-régulière se
termine par unsupported_degeneracy et annule atomiquement le payload. Dans un
futur profil, un PlateauQuotient versionné ou un flux explicite pourra
consommer le même SphereRun.

La voie mathématique prometteuse pour ce futur quotient est celle des
générateurs saturés : la boule agrège Sat(B)=I_B union U_B, et les composantes
aux seuils peuvent être préservées par des générateurs et leurs intersections
pondérées. Le dépôt possède déjà les théorèmes combinatoires S.1–S.6. Il faut
encore recevoir toutes les lanes scellées, les intersections
$\left\lvert S_B\cap S_C\right\rvert\geq k$, les racines pré-lot gelées,
généalogie, lots, coverage_delta et verticales, lot par lot contre Gamma. Aucun
BenchmarkOutputContract-v1 n'en découle aujourd'hui.

Les trois politiques ne sont donc pas exclusives au niveau du layout :

- exact_regular émet la multiplicité nécessaire dans le domaine reçu ;
- exact_plateau_quotient_vN ne s'active qu'après preuve et différentiel ;
- reject_unsupported est la politique exacte actuelle.

Il faut séparer unsupported_degeneracy, propriété stable du domaine, de
resource_exhausted, incapacité physique à traiter une entrée pourtant admise.
Un seuil de mémoire ne peut jamais changer l'un en l'autre.

## 5. Correction du budget 100 ms

L'extrapolation de 315,7 millions de recertifications à n=6 000 vers
2,6 milliards à n=50 000 n'est ni une borne ni une preuve de linéarité.
Même prise au pied de la lettre, elle n'indique pas que seules les constantes
restent à travailler.

À 100 ms, 2,6 milliards de tâches exigent 26 milliards de recertifications par
seconde. À 32, 64 ou 128 octets de trafic utile par tâche, elles déplacent
respectivement environ 83, 166 ou 333 Go et demandent 0,83, 1,66 ou 3,33 To/s,
avant les lectures irrégulières, les prédicats larges, count–scan–fill, radix,
output et synchronisation. Le facteur « trois à dix du budget » ne découle donc
pas des chiffres. Le calcul doit réduire ou agréger le nombre de tâches avant
de présenter 100 ms comme un combat de constantes. Le contrat secondaire à une
seconde reste une cible plus plausible, toujours non reçue.

Le taux de 95 % du cœur anisotrope sur eight_clusters est également un
diagnostic par paire. L'intervalle de T est exact, mais la combinaison des
extrema D, V et T sur un rectangle est seulement suffisante et fail-open. Seul
le vrai classifieur rectangle intégré au producteur, avec son coût, peut
alimenter la porte de l'étape un.

Enfin, sum E4 ne suffit pas. La porte composée publie au moins :

- E3 et E4 finaux, pending nul ;
- M3 et M4, blocs, tâches et visites ;
- PrimitiveSphereKey et SphereRun uniques ;
- census, shell et multiplicité de SupportKey ;
- sorties H, bytes/HWM et temps de chaque phase.

Une croissance superlinéaire n'est un critère de mort que sur une famille du
SLO déclarée output-sparse. Sur une entrée ayant une sortie quadratique reçue,
elle conduit à resource_exhausted ou au quotient prouvé, pas à une
contradiction de l'algorithme. La bonne relation de coût est output-sensitive,
par exemple travail borné empiriquement par une fonction de n+H sur les
familles contractuelles, et non M=O(n) comme théorème universel.

## 6. Ordre révisé remis à Claude

### Étape 0 — fermer la sémantique

- BallFormToBallEvent-v0 indépendant ;
- PrimitiveSphereKey puis census I_B/U_B ;
- disposition régulière ou dégénérée ;
- fold complet borné et BenchmarkOutputContract-v1 ;
- permutations, tilings, lots égaux, verticales, reprise et engagement
  atomique ;
- aucune mesure SLO.

### Étape 1 — substituer et compter la source

- conserver l'oracle exhaustif comme juge ;
- intégrer le meilleur certificateur existant dans le même exécutable ;
- publier E3/E4, M3/M4, SphereRuns, tâches, octets et HWM ;
- mesurer les cinq familles et plusieurs graines ;
- passer aux cages/PWC si la voie centrale reste rouge.

### Étape 2 — fermer q3 et q4 sans scans

- q3 : Q3CarrierPrefixRange, owner PointId, PrimitiveSphereKey,
  Q3FootPowerRange capé à neuf ;
- q4 : LineFormTape exhaustif local et niveaux P-P/N-N/P-N ;
- un census par SphereRun, jamais par support ;
- Q3FootLevelLocate seulement si les visites LBVH le justifient.

### Étape 3 — porter la tranche, pas un probe

- count–scan–fill et tâches persistantes ;
- même identité planned=filled=consumed ;
- parité du payload complet avant chrono ;
- preflights, aucune structure globale interdite ;
- campagnes G4 seulement après les portes CPU structurelles.

### Étape 4 — ouvrir un successeur de profil

- documenter sa porte d'entrée ;
- recevoir ExactKernel binary64, largeurs et repli ;
- conserver SphereIdentity/BallEvent/fold ;
- qualifier 1 M avant 10 M, sans importer un claim de la v3 u16.

## 7. Contre-audit croisé de l'autre auditeur

Les propositions de l'autre auditeur ne sont pas reprises comme autorités.
Quatre conclusions sont conservées après vérification :

- owner-edge × porteur puis pied est la bonne factorisation q3 ;
- PrimitiveSphereKey avant census évite la circularité de BallKey ;
- Q3FootPowerRange est un meilleur premier juge que l'arrangement ;
- le pire cas q3 réel quadratique interdit une promesse universelle de sortie
  linéaire, avec la réserve explicite que la construction citée n'est pas une
  fixture 50 000 u16.

Quatre portées sont en revanche corrigées ou maintenues conditionnelles.

Premièrement, CertifiedCageWindow est un certificateur fail-open valide, pas
une fenêtre complète reçue. Une cage tétraédrique plein rang qui contient
l'ancre borne bien sa cellule sans témoin et fournit le cutoff radial. Mais
32 ou 36 témoins disjoints peuvent manquer, surtout sur bord, terrain ou
scanline. En rang relatif, la preuve exige en plus que le partenaire appartienne
au même sous-espace linéaire ; sans cette condition, les directions
orthogonales restent non bornées. Cages mesure donc son taux FULL et délègue
UNDERFULL à PWC.

Deuxièmement, la borne O(s³) des préfixes porteurs est conditionnelle au niveau
Morton virtuel commun, à $Dlo>0$, à
$Dhi/Dlo\leq\kappa(s_1)$ et à un front WSPD de niveau un complet. Elle ne
reçoit ni la linéarité de ce front ni la masse M3. La version live sur AABB
tight reste sans packing.

Troisièmement, la complexité combinatoire O(mk) des premiers niveaux ne reçoit
pas à elle seule Q3FootLevelLocate. Bundles pondérés, concurrences, exclusion
de la ligne incidente, divisions exactes et point-location de tous les pieds
restent à comparer à Q3FootPowerRange. Toute formule
O(m log m+mk+H) est une cible d'ablation, pas une borne produit actuelle.

Quatrièmement, SaturatedGenerator reçoit une voie de composantes H0 aux coupes
sous ses théorèmes, mais pas le payload officiel. Un BallKey choisi, les seuls
cardinaux ou une facette canonique ne remplacent pas le shell complet. Le join
des générateurs, les lots égaux, coverage, provenance Gamma et verticales
restent ouverts. L'autre audit est donc corrigé s'il présente ce quotient comme
déjà consommable par BenchmarkOutputContract-v1.

Cette contre-lecture confirme l'ordonnance générale sans promouvoir ses
optimisations conditionnelles.

## Réponses courtes

1. Oui, squelette output-bearing avant parcimonie, mais comme tranche verticale
   complète bornée et alimentée par l'oracle. Une fenêtre rouge ne le rend pas
   prématuré.
2. Préparer maintenant la couture ExactKernel–SphereIdentity ; ne pas
   implémenter ni revendiquer binary64 dans le profil u16 courant. Le fold doit
   rester indépendant des limbs.
3. Aujourd'hui : unsupported_degeneracy atomique. Pour ne pas figer l'avenir :
   SphereRun lossless et disposition versionnée ; quotient saturé seulement
   après preuve. Ne jamais choisir silencieusement un support ni confondre
   refus de domaine et manque de ressource.
