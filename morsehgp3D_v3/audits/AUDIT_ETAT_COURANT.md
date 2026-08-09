# Audit courant de MorseHGP3D v3

Date du snapshot : 9 août 2026 UTC.

Cadre annoncé : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_oracle_and_gpu_candidate_under_audit`,
`profile=quantized_u16_input_only`,
`mode=math_locks_plus_gpu_differential`,
`public_status=not_claimed`.

Cet audit porte uniquement sur `morsehgp3D_v3`. Il ne modifie aucun prototype,
n'ouvre aucune phase et ne promeut aucun résultat public. Le snapshot de code
audité est `40ad152` : il conserve les sources wavefront, ajoute le profileur à
densité fixe, puis committe le probe de paires et ses claims d'échelle. Le delta
worktree k-NN `130e316e...` est épinglé séparément. Aucun artefact brut de la
session G4 n'est versionné avec ces commits.

| objet | empreinte SHA-256 |
| --- | --- |
| snapshot de code et de claims audité | `40ad1522356e8ca0c5c144b441ad6dc0367810fe` |
| `CMakeLists.txt` | `f6650252fde309be1e2a81d15b1254383bdff7af0e8c805e6bc233c56b0d2db3` |
| `prototype/scale_profile.cpp` | `e6c31f544d8275b3f89affde11b52e11972dd7e76cf9b556112c96a43d96aacb` |
| `prototype/admissible_pair_probe.cpp` | `8c89ccb627d7d0d531897b95ec24f56a473578744f16299d052133dd0fba6cc8` |
| `prototype/admissible_pair_probe.cpp` worktree postérieur | `130e316ed956cc6a540642ded9fed21456f4c2c57b00ecb4e821f4c2cea86b8d` |
| `prototype/order_k_flats.hpp` | `02ad6f58632de60d47e0b2bbcdf6205d8a3b9d1cab1474dd9d8b566593e9e81a` |
| `prototype/order_k_device_core.hpp` | `79382cf2857fb8da4efcecda8b9a164643fb4013c9a56cd6152f102daa155a3d` |
| `prototype/flats_differential.cpp` | `14c690031debf7214ae0fcd40ced0fd1a4169a06b34b0f035ca7103692384fa3` |
| `prototype/device_wavefront_job.hpp` | `cffe45646eb46ec44f4818ce8c8f0a3e7251084d8fb05c0cb79fbfae243fa31f` |
| `prototype/device_wavefront_kernel.cu` | `bebc6684ccacd763d28d2f336b9cfd17b356914addf37786afbe0c7440901ccc` |
| `prototype/device_wavefront_qualification.cpp` | `3ae284cd1e431ec22ccfe30efa4c3afef8cc91c5b87c92d696f84c2b088cbf89` |
| `audits/check_gate_d_fold_f0.py` | `34149092cd1b06762085800ac9d575c0cb8022e3a1c273c7d1955d2f4e768294` |

## Verdict

**NO-GO de justesse owner, NO-GO du probe de paires, NO-GO F0, et NO-GO de
qualification GPU/replay.**

Le premier `.cu` est un progrès réel : il définit un lancement CUDA optionnel
et un même corps source pour CPU/device. Les quatre portes hôte, dont une
campagne forçant 27 refus, sont vertes. Cette avancée ne ferme toutefois pas le
contrat annoncé : les refus sont précisément exclus de l'oracle et ne sont
jamais rejoués. Un mutant qui refuse tous les sommets reste vert.

Le kernel ne constitue pas encore une wavefront. `navigate_shallow` construit
et mémorise d'abord tous les sommets sur CPU; le kernel calcule ensuite seulement
un masque d'admissibilité des couples. Il ne produit ni voisin, ni parent, ni
enfant, ni tâche, ni run.

Le commit `40ad152` revendique en plus un univers de paires admissibles
$O(n\log n)$ et projette 7,5 millions de candidates à 50 k. Son
`minimum_halfplane_count` est pourtant réfuté par une fixture entière : il rend
5 au lieu de 2 et supprime une paire critique. Les masses publiées sont des
sous-estimations du vrai filtre; quatre tailles finies ne prouvent de toute
façon aucun `Big-O`. Cette conclusion d'échelle est retirée de l'état courant.

Le commit rapporte une compilation `nvcc` et quatre exécutions sur G4 avec zéro
écart CPU/device. C'est un résultat positif ciblé pour le préfixe borné, mais pas
un reçu qualifiant : commandes, sorties brutes, version patch du toolkit, hash
du binaire, PTX/cubin, rapport `ptxas`, digest d'entrée et répétitions ne sont pas
conservés. Surtout, les refus restent exclus de l'oracle; le texte « rejoués par
l'hôte » contredit le code.

Deux défauts antérieurs restent bloquants : le chemin owner tronque un
déterminant `i128`, et les deux modèles F0 rejettent ensemble une naissance
autorisée par leur contrat écrit.

## P0 — troncature du signe owner toujours présente

`owner_rays_ok` passe encore la valeur brute de `orient3d_exact`, de type
`i128`, à `tangent_sign(int, ...)`. La conversion implicite perd les bits hauts;
à la valeur `INT_MIN`, la négation suivante est un comportement indéfini.

Fixtures reproduites :

| fixture | catalogue normal | catalogue owner | observation |
| --- | ---: | ---: | --- |
| tétraèdre axial, échelle 1290 | 7 | 7 | déterminant encore sous `INT_MAX` |
| tétraèdre axial, échelle 1291 | 7 | **4** | trois paires perdues |
| tétraèdre alterné, échelle 1024 | 10 | 10 en Release | UBSan signale `-INT_MIN` |
| tétraèdre alterné, échelle 1025 | 10 | **4** | six paires perdues |

La campagne u16 déterministe suivante rend encore un désaccord, owner 16 contre
19 enregistrements normaux :

```sh
mhgp3v_flats_differential --clouds 1 --points 8 --coord 65536 --smax 2 --seed 20260809
```

`-Wconversion` désigne exactement l'appel fautif. La correction testée hors
dépôt est de réduire par `sign_of` avant l'appel, puis de graver les frontières
1290/1291 et 1023/1024/1025, le mutant de troncature, UBSan et l'unicité du
propriétaire.

## P0 — le refus du microkernel n'est ni jugé ni rejoué

Le CTest `mhgp3v_device_wavefront_refusal` publie :

```text
sommets=2542 flats=15346 couples admissibles=7109 refuses=27
flats/sommet max=32 (capacite 32)
0 desaccords
OK
```

Ce `OK` ne porte que sur les sommets `kOk`. La boucle de référence exécute
`continue` pour chaque `kFlatOverflow`; `summarise` compte ensuite le refus mais
écarte ses flats et son masque. Le plancher `--min-refused 10` prouve donc que
la branche a été prise, pas que son résultat a été conservé.

La session G4 ne change pas ce fait. Elle compare le même `VertexVerdict` borné
entre hôte et device, y compris son statut de refus; elle n'exécute ensuite
aucune référence non bornée pour les refus. Le README et le message de commit
affirment pourtant que les 27 sommets sont « rejoués par l'hôte ». Aucun appel,
compteur ou résultat de replay n'existe dans le source.

Contre-exemple géométrique permanent : sept points entiers sur la sphère de
centre `(100,100,100)` et de rayon 25, sans quadruplet coplanaire :

```text
(75,100,100) (76,93,100) (76,100,93) (76,100,107)
(80,85,100) (80,88,91) (80,91,112)
```

Avec le centre comme point intérieur, le sommet est valide de niveau 1. Le CPU
non borné rend exactement $\binom{7}{3}=35$ flats. Le sujet s'arrête à 32,
rend `kFlatOverflow` et le masque partiel `0x940800000009`; le juge ne compare
jamais les trois flats restants. À coquille 32, le maximum générique vaut 4 960.

La vacuité a été confirmée par mutation hors dépôt : forcer
`kFlatOverflow` après chaque évaluation laisse vertes les trois campagnes, avec
respectivement `3318/3318`, `661/661` et `573/573` sommets refusés, zéro flat,
zéro couple et zéro désaccord. Il faut au minimum :

1. des planchers séparés de nuages traités, sommets acceptés, flats, couples,
   kernels lancés et décisions;
2. un oracle non borné exécuté pour chaque statut;
3. le ledger `refused = replayed + pending + fatal` par raison;
4. l'égalité exacte, avec multiplicité, entre le CPU complet et l'union des
   résultats committés et rejoués.

Les refus d'`admit`, notamment `shell>32`, sont encore plus silencieux : ils sont
omis du batch et de `total_refused`. `kClosureOverflow` est au contraire
impossible après admission puisque toute fermeture est incluse dans une
coquille de taille au plus 32; ce statut doit être une violation d'invariant,
pas un fallback normal.

Le reçu agrège en outre `vertices=count`, refus compris, sans publier
`accepted`; son champ `mismatches` n'est jamais alimenté par `summarise`.
`flat_high_water` exclut les refus et les statistiques d'admission
`samples/accepted/reasons` sont perdues. Enfin, un intérieur de 31 ou 32 points
est accepté par le format alors que la coupe produit prouve la borne 30 : ces
valeurs doivent être `invalid_contract`, pas des admissions ordinaires.

## P1 — ce kernel ne décide pas encore la reverse-search

`evaluate_vertex` énumère les flats et met deux bits d'admissibilité par flat.
Il n'appelle ni `neighbour_along`, ni `backward_pair_admissible`, ni
`decide_child`. Une admissibilité de retour positive ne suffit pas : un couple
antérieur peut être admissible et imposer `Reject`.

Un masque nul ne certifie même pas l'ordre. Les six premiers points de la
fixture précédente portent 20 flats et un masque nul; toute permutation des 20
flats conserve `(flat_count,mask)`. La porte actuelle ne voit donc pas une
régression de clef canonique sur ce cas.

Le batch n'est pas un découpage de tâches : il est la sortie matérialisée du
parcours CPU avec `seen`. Sur la fixture à sept points, le vrai arbre parent
possède 18 sommets et six sous-arbres racine disjoints de tailles
`1,1,2,6,1,6`; un descendant apparaît pourtant dans le batch avant son parent
canonique. L'indice du batch n'est ni un `task_id` structurel ni un ordre
topologique.

## P1 — contrat de job et enveloppe CUDA ouverts

`WavefrontJob` transporte des pointeurs et tailles bruts sans authentifier :

- le profil u16 et le digest du nuage;
- `root_size==4` et l'indépendance de la base;
- les bornes des identifiants;
- coquille/intérieur triés, uniques et disjoints;
- `level==interior_size` et les capacités;
- les multiplications de tailles avant allocation.

`point_count` n'est jamais lu par l'évaluateur. Un job
`root_size=0, root_base=nullptr` obtient encore `kOk`; une entrée malformée peut
donc devenir un accès hors limites device au lieu d'un `invalid_contract` avant
lancement. Les queues et le padding de `BoundedVertex`/`WavefrontJob` ne sont
pas initialisés avant leur copie, ce qui interdit aussi tout digest byte-stable.

Le CMake filtre maintenant `-Wall -Wextra -Werror` sur le seul C++ : c'est une
correction utile. L'enveloppe reste ouverte. `enable_language(CUDA)` précède le
fallback `CMAKE_CUDA_ARCHITECTURES=120-real`; CMake initialise normalement la
variable pendant cet appel, si bien que le fallback peut ne jamais agir. Une
surcharge arbitraire reste acceptée. Le build n'impose ni compilateur NVIDIA,
ni version patch du toolkit corrigée pour `__int128`, ni architecture exactement
120, ni politique d'avertissements CUDA. Le temps publié est kernel-only; il
exclut allocations, copies et surtout la construction CPU de tout le lot.

Le contrat v3 doit rester intrinsèque : seules les invariants, sources et reçus
du snapshot v3 peuvent qualifier ce kernel. Un commentaire d'intention ne
remplace ni la garde CMake ni le reçu device.

La compilation Clang 18 device-only du header v3 produisait 144 octets de local par
thread et une forte pression de registres virtuels. La session G4 rapportée ne
conserve aucun diagnostic `ptxas`, spill, stack, registre ou occupation; elle ne
permet donc toujours pas de relier le débit observé aux ressources du cubin.

## Audit du diagnostic G4 et de son interprétation à 100 ms

Le README rapporte quatre mesures kernel-only : 128 955 sommets en 0,224 ms,
71 084 en 0,170 ms, 19 019 en 0,323 ms et 2 542 en 2,020 ms, toutes avec zéro
écart CPU/device sur le `VertexVerdict` borné. En l'absence des sorties brutes,
elles sont conservées comme **diagnostics déclarés**, pas comme reçus
reproductibles.

Le journal local externe permet de retrouver les paramètres des deux grandes
campagnes : `clouds=3,points=120,coord=4000,smax=8,seed=5` et
`clouds=2,points=200,coord=8000,smax=6,seed=9`. Le chemin hôte reconstruit au
même snapshot reproduit exactement `128955/515820/340781` puis
`71084/284336/186885` pour sommets/flats/admissions, avec zéro écart sur `kOk`.
C'est un renforcement positif de la provenance des masses; le timing device et
le binaire restent non reçus tant que ce journal n'est pas scellé dans le dépôt.

Le transport hôte/device est un résultat positif ciblé. Son interprétation
quantitative doit toutefois distinguer les appels, les résultats admissibles et
le pipeline :

1. la première campagne a 515 820 flats et donc 1 031 640 appels directionnels;
   `0,224 ms` correspond à environ 4,61 milliards d'appels par seconde, tandis
   que les 340 781 résultats admissibles correspondent à 1,52 milliard par
   seconde. « Un milliard d'évaluations » mélange ces deux métriques;
2. le maximum 32 de la campagne de refus n'est pas une moyenne : les 15 346
   flats publiés plus les 27 préfixes de 32 donnent 16 210 flats évalués, soit
   6,38 par sommet. Le facteur 450 confond aussi de petits lancements sous-remplis,
   la charge par sommet et la divergence; aucune de ces causes n'est isolée;
3. multiplier un terrain hypothétique par 575 M sommets/s suppose que la
   distribution de coquilles/flats reste celle de la ligne la plus favorable,
   alors que la propre campagne dégénérée tombe à 1,3 M sommets/s.

Le chrono entoure seulement le kernel. Chaque `BoundedVertex` occupe 268 octets
et chaque verdict 16 octets. La ligne 128 955 transfère donc environ 34,6 Mo de
vertices et 2,1 Mo de verdicts hors de la fenêtre 0,224 ms. Un terrain
hypothétique de 50 à 150 millions demanderait environ 14,2 à 42,6 Go pour ces
deux flux, sans compter le nuage, les allocations ni la représentation CPU à
vecteurs. Le live matérialise en outre tout `seen_vertices` avant le lancement.

Le commit `444b851` corrige ensuite le budget primaire à 100 ms, mais sa nouvelle
conclusion ne découle toujours pas de la mesure. Les 1 096,8 sommets par point à
`n=300` ne sont pas une borne inférieure à `n=50 000`; ce profil avait en outre
une densité décroissante, faute que `f851374` reconnaît explicitement. Diviser
55 millions par 575 millions donne bien 95,7 ms **sous ces deux hypothèses**, pas
une mesure du terrain ni du pipeline. Aucun parcours GPU complet n'existe pour
recevoir le facteur « dix à trente » ou le verdict « quinze fois trop lent ».

Le ratio 6,5 ne ferme pas davantage une décision d'architecture. À `n=300`, il
compare les sommets visités aux sommets bien centrés; le rapport au catalogue
complet publié vaut environ 4,66 et ce catalogue est dominé par les arités deux
et trois. « Énumérer directement les sphères critiques » est une piste
constructive importante, mais aucune borne inférieure n'exclut encore
élagage, sauts, compression ou requêtes groupées du parcours. Elle devient une
obligation à prouver, pas une condition déjà démontrée.

Le chrono G4 entoure seulement un microkernel sur une entrée déjà produite et
copiée. `neighbour_along`, parent, source, census, owner, tri, fold, couverture,
verticales, copies, mémoire et sortie ne sont ni inclus ni bornés. Dire que ce
prédicat domine le pipeline avant d'avoir mesuré ces étages inverse la charge de
la preuve.

Le contrôle GCP de l'auditeur a été strictement en lecture seule. Les deux VM
labellisées `project=e-hgp`, dont `ehgp-blackwell-spot-ai1a` démarrée à l'heure
compatible avec la session, sont actuellement `TERMINATED`, de type
`g4-standard-48`, `SPOT`, action `STOP`. Cela crédite l'état final GCE observé;
le dépôt ne contient toutefois ni handoff de génération, ni log du double
coupe-circuit, ni reçu de révocation de la clé pour cette session.

## P1 — le profileur à densité fixe est utile, pas décisionnel seul

Le commit `f851374` corrige une faute de protocole réelle : le profil cube
antérieur faisait croître chaque côté comme `sqrt(n)` et diminuait donc la
densité. `scale_profile.cpp` propose maintenant un cube à volume proportionnel
à `n` et une nappe synthétique d'épaisseur bornée. En Release, la commande
`--points 100 --smax 11 --repeats 2 --seed 20260809` reproduit 805,5 sommets
par point, 159,28 sphères par point et les arités
`1,00/20,11/77,36/60,80`. C'est un nouveau diagnostic positif et reproductible
côté CPU.

Le commit `70ead99` publie ensuite quatre tailles `n=100/200/400/800`. Les
valeurs de sommets par point `805,5/1 011,5/1 171,9/1 271,9` et de catalogue par
point `159,3/219,8/266,3/299,9` sont utiles : les incréments observés diminuent
sur cette fenêtre. Elles ne prouvent toutefois aucune convergence. Les nuages
sont indépendants, une seule densité et trop peu de graines sont publiées, et
trois incréments n'identifient ni une asymptote géométrique ni même une fonction
bornée. Une loi logarithmique, une puissance lente ou une nouvelle transition
au-delà de 800 restent compatibles avec ces quatre points.

Le tableau ne suit même pas un protocole homogène. Au binaire Release identique
et à la graine `20260809`, la ligne cube `n=100` publiée correspond à
`repeats=2` (`805,5/159,28`), tandis que `n=200` correspond à `repeats=1`
(`1011,5/219,78`); avec `repeats=2`, cette dernière vaut
`1013,5/216,80`. Le premier incrément catalogue et les ratios qui en découlent
comparent donc des estimateurs différents. Commande, nombre de répétitions et
dispersion doivent apparaître par ligne avant toute régression.

Les lignes cube `n=400/800` ont été recertifiées avec une répétition et la même
graine; elles retrouvent exactement `1171,9/266,28` puis `1271,9/299,94`. La
nappe correspondante rend `1162,1/240,47` puis `1250,2/257,00`. Si les sommets
diffèrent de moins de 2 %, les sorties diffèrent de 9,7 % puis 14,3 % : une seule
densité ne permet pas d'attribuer causalement la masse au seul paramètre densité,
et les deux familles n'ont pas de limite commune prouvée.

Dans le modèle cube uniforme continu, une homothétie globale préserve d'ailleurs
la combinatoire Delaunay/order-k : la densité absolue n'agit sur ces ratios que
par quantification et effets de bord. La nappe à épaisseur `z=40` n'est pas une
homothétie lorsque `n` croît. Dire que « la densité décide » n'est donc ni une
conclusion causale de la table ni un invariant commun aux deux profils.

Les asymptotes `1 430` sommets/point et `390` sphères/point sont donc les sorties
d'un modèle choisi après observation, sans ajustement documenté, intervalle ni
validation hors échantillon. Les masses `7,1e7/1,9e7` à 50 k, les `0,124 s` et
le facteur `15--40` qui en découlent sont des scénarios conditionnels, pas un
écart « mesuré ». Le facteur `10--30` du pipeline reste lui-même sans pipeline
GPU complet ni reçu.

Il ne peut cependant être « le seul chiffre qui décide » les 100 ms :

- la densité `1e-3` est codée en dur et le profil LiDAR est une nappe uniforme
  synthétique, sans famille sanctionnée, digest d'entrée ni quantile;
- les nuages de statut non `kOk` sont retirés de la moyenne; `decided>0` permet
  donc une moyenne partielle sans ledger des refus;
- la déduplication du générateur emploie `std::find` dans un vecteur et coûte
  $O(n^2)$ hors chrono;
- le temps navigation exclut `CertifiedIndex::build`, le temps catalogue
  contient un second parcours via `flat_catalogue`, et la sortie « sans
  accélérateur » est ambiguë puisque l'index est actif;
- le catalogue emploie `use_index=true,use_owner=false`; il ne mesure donc ni le
  chemin owner actuellement faux sur u16, ni une source critique directe;
- `std::uniform_int_distribution` n'est pas une spécification de flux portable
  entre bibliothèques standard; une graine sans digest du nuage ne suffit pas à
  un reçu inter-machine;
- ni source directe, ni fold, ni forêts, ni couverture, ni verticales, ni
  octets, ni pipeline GPU ne sont mesurés.
- la cible n'a ni CTest permanent, ni plancher de nuages décidés, ni reçu
  canonique des paramètres et compteurs.

La porte propre publie toutes les graines et tous les statuts, sépare taille du
terrain, taille de sortie et travail par étage, puis mesure des quantiles sur les
familles enregistrées. Un ratio observé reste un diagnostic; il ne devient une
borne à 50 k qu'après un théorème ou une exécution effectivement à 50 k.
Sous l'hypothèse non validée de 19 millions de sorties, `100 ms / 19 M` donne
bien 5,3 ns par sortie; cette division ne transforme ni les 19 millions en borne
ni un compteur d'appels/admissions du microkernel en budget de prédicats aval.

## P0 `40ad152` — le minimum de demi-plan du probe de paires est faux

Le commit `40ad152` ajoute
`prototype/admissible_pair_probe.cpp` (`SHA-256 8c89ccb627d7d0d531897b95ec24f56a473578744f16299d052133dd0fba6cc8`)
et son branchement CMake (`SHA-256 f6650252fde309be1e2a81d15b1254383bdff7af0e8c805e6bc233c56b0d2db3`).
L'objectif est positif : mesurer la masse des paires laissées par un lemme
nécessaire exact. Son oracle angulaire et la conclusion quasi linéaire du commit
sont toutefois faux.

`minimum_halfplane_count` ne teste que les directions portées par les points,
puis compte la frontière avec `cross>=0`. Le minimum d'un demi-plan **fermé**
peut être atteint entre deux directions : en plaçant la frontière juste après un
groupe collinéaire, ce groupe appartient au demi-plan ouvert complémentaire,
alors que tous les tests live le remettent sur la frontière et le comptent.

Fixture entière, statut `kOk` et `RelevantGP` :

```text
centre=(100,100,100), rayon^2=194
support={(113,100,95),(113,100,105),(87,105,100),(87,95,100)}
extras={(114,100,100),(115,100,100),(116,100,100)}
paire testee={0,1}, s_max=4
```

Les trois extras sont dans la boule diamétrale de la paire, hors de la sphère
critique et sur le même rayon projeté. Le demi-espace `x<=113` ne compte que les
deux extrémités, donc le minimum exact vaut 2. Le live rend 5 et incrémente
`missing=1` pour une paire vraie. Le programme peut ainsi imprimer `ECHEC` contre
un lemme correct.

La fixture est auto-certifiante : les quatre vecteurs support relatifs au centre
sont `(13,0,-5),(13,0,5),(-13,5,0),(-13,-5,0)`, tous de norme carrée 194,
affinement indépendants et de moyenne nulle. Le centre est donc strictement
intérieur à leur convexe et la sphère est bien la miniboule de support quatre.
Le probe complet temporaire qui produit `status=ok`, `truth01=1`,
`live_least=5` et `missing=1` a le SHA-256 source
`d084861fe9ec13ed26674d374df630a992345b5151e026b20a0a3b1a5bd9246d`
et le binaire `9517fb9c...`.

Les campagnes aléatoires restent un diagnostic utile : à `n=20/50/100/200`,
elles publient respectivement `190/1176/3885/10706` paires admises contre
`180/812/2113/5171` paires dites vraies, avec `missing=0`; le filtre seul prend
0,91 s à `n=200`. Mais l'erreur live **surestime** le minimum et rejette trop de
paires. Le sweep corrigé ne peut qu'augmenter `ADMIS`; ces masses sont donc des
sous-estimations de l'univers conforme, jamais un crédit de parcimonie. Quatre
tailles et une graine ne distinguent pas davantage une croissance linéaire d'une
croissance quadratique à 50 k.

Les quatre lignes publiées ne partagent pas non plus le même estimateur :
`n=100` correspond à deux répétitions, les tailles suivantes à une seule.
Le « zéro sur 424 250 paires » additionne une fois chaque $\binom{n}{2}$ alors
que l'exécution a effectivement testé deux nuages à 100 points, soit 429 200
couples nuage--paire. Même après correction du sweep, ces quatre observations
ne peuvent prouver un `Big-O` ni une masse à 50 k.

La réparation mathématique est un sweep exact par groupes de rayons primitifs :
calculer le maximum de points dans un demi-plan **ouvert** par fenêtre circulaire
et deux pointeurs, puis utiliser
`minimum_closed=always_inside+m-maximum_open`. Antipodes, rayons confondus et
points à l'origine ont des fixtures distinctes; le contre-exemple ci-dessus est
permanent. Une porte de mutation remplace le sweep par les seules directions
live et doit rougir.

Résultat positif distinct : pour la source Gabriel ouverte, employer la boule
diamétrale **ouverte** $D_{pu}^{\circ}$ et
$A(p,u)=2+\min_H\lvert(X\setminus\{p,u\})\cap D_{pu}^{\circ}\cap H\rvert$.
Si `p,u` sont sur une sphère de support `q`, le demi-espace dirigé vers son
centre ne contient, dans $D_{pu}^{\circ}$, que des points strictement intérieurs.
Ainsi $A(p,u)\leq2+\lvert I\rvert\leq q+\lvert I\rvert\leq s_{\max}$, sans
hypothèse sur l'extra-shell. Une vérification indépendante confirme la preuve,
y compris le centre diamétral, les projections nulles et les points de
frontière. La micro-fixture
`p=(99,100,100),u=(101,100,100),w=(100,100,100)` avec deux extra-shells
orthogonaux rend `open_A=3`, `closed_exact_A=4` et `closed_live_A=5`
(source temporaire SHA-256
`9311a2a163e4f77b2ed15f5d6c706ff34cf96da463bbe21923f1c8cfca4014c3`).

Même corrigé, ce probe ne décide pas seul la source industrielle. Sa « vérité »
vient de `flat_catalogue(...,s_max,...,verify_census=false,use_index=true)` :
elle partage les prédicats du sujet, porte sur le catalogue de rang fermé et
n'est pas un oracle indépendant de la source Gabriel ouverte à extra-shell.
Un mutant catalogue vide donne `truth=0,missing=0,OK`, car aucun plancher
`min_true` n'existe. Les statuts non `kOk` sont ignorés, `decided>0` suffit,
et ni seed, répétitions demandées, densité, digest ni CTest ne scellent le reçu.

Le calcul exhaustif balaye toutes les paires et tous les points, puis jusqu'au
carré des projections; son pire cas est quartique et sa cible `n<=5000` n'est
pas une enveloppe de performance. À 50 k, le seul census point--boule ferait
`n*binom(n,2)=62 498 750 000 000` tests avant le sweep. `ball_points` ne
publie pas ce nombre de tests, seulement les points retenus dans les boules.
Ce programme reste un diagnostic CPU borné; il ne génère pas la source.

Le delta worktree `130e316e...` ajoute des rangs k-NN sans corriger le sweep.
Ses maxima et histogrammes sont donc calculés sur un ensemble `ADMIS` déjà
tronqué par le P0. La construction `rank_of` matérialise $n^2$ entiers et trie
$n$ listes **avant** `t0` : son coût $O(n^2\log n)$ et sa mémoire $O(n^2)$
sont exclus du temps affiché. `min(rank_a(b),rank_b(a))` qualifie seulement
l'union symétrisée des k-NN avec tie-break PointId; les ex æquo géométriques ne
sont pas groupés. Les buckets sont enfin décalés : la classe imprimée `2`
contient le rang 1, et `rank_max_true` ignore toute paire vraie que le filtre a
déjà supprimée. Ce diagnostic ne devient interprétable qu'après réparation du
filtre ouvert, chrono séparé de la construction, sémantique des ex æquo et
calcul de la vérité indépendamment d'`ADMIS`.

## NO-GO F0 inchangé

Le script imprime encore `PASS` en exécution normale et sous `python3 -O`, mais
Warshall et DSU partagent la même garde fautive. Une composante tout $N_a$ avec
une `DirectHyperedge` doit créer une naissance sous le contrat général; les deux
chemins rendent `error`. Inversement, un record direct tout $N_a$ peut être
masqué par un second record portant un `L` dans la même composante.

La solution mathématique est déjà fournie dans
[`NOTE_VERROUS_MATHEMATIQUES_PRIORITAIRES.md`](NOTE_VERROUS_MATHEMATIQUES_PRIORITAIRES.md) :
garder le fold source-agnostique, valider la régularité par record brut avant
projection, reconstruire une vérité indépendante depuis `RawBatch`, et
remplacer les 27 obligations basées sur `assert` par des échecs explicites.

## Résultats positifs conservés

- Les cinq portes flats Release passent : 4 990 cas, 328 560 sommets admis,
  2 703 016 couples concordants et zéro désaccord sur leur petit domaine.
- Les quatre portes `device_wavefront` hôte passent en Release.
- Les mêmes quatre portes passent sous ASan/UBSan; aucune alerte n'est observée
  sur les chemins CPU exécutés.
- Le commit rapporte quatre lancements G4 `sm_120` et zéro écart bit à bit entre
  le `VertexVerdict` hôte et device; l'option CUDA échoue fermée localement en
  l'absence de compilateur.
- Le masque 64 bits ne décale jamais de 64 : les deux slots du flat 31 occupent
  les bits 62 et 63.
- Le lemme de paire diamétrale ouverte a été vérifié indépendamment, puis sur
  59 154 incidences support--paire sans écart; la micro-fixture
  `open_A/closed_exact_A/closed_live_A=3/4/5` sépare les trois contrats.
- Le théorème `center-cover + degree` a été contrôlé sur 4 105 supports propres
  aléatoires et dix oracles `RelevantGP` bornés sans contre-exemple. Il reste
  conditionnel à sa capability et non implémenté.
- L'inventaire GCE en lecture seule confirme les cibles labellisées arrêtées.
- `git diff --check` est vert sur le snapshot documenté.

Ces crédits prouvent une exécution device déclarée et une égalité du payload
borné; ils ne prouvent ni les refus, ni le parent, ni le voisin, ni le pipeline.

## Aide mathématique et ordre d'implémentation transmis à Claude

### 1. Réduire le microkernel exact

Sous u16, chaque `orient3d` est strictement inférieur à $2^{51}$ et la somme des
quatre orientations racine à $2^{53}$. Le microkernel entier tient donc en
`int64_t`; `i128` reste nécessaire à `next`, pas à ce hot path.

Les deux directions se calculent en un seul scan. Si
`o_z=orient(base,z)`, la direction moins exige tous les `o_z>=0`, puis
`o_h>0` au niveau positif ou la somme racine négative au niveau zéro; la
direction plus emploie les inégalités opposées. Un probe CPU indépendant a
reproduit les deux appels live sans désaccord sur les campagnes permanentes.

Sur une coquille sphérique authentifiée de points distincts, trois points ne
sont jamais collinéaires. La base canonique d'un flat est donc simplement ses
trois plus petits identifiants. L'ordre des flats est l'ordre de ces triples.
Pour décider le parent, chaque page réduit sa plus petite clef admissible; une
réduction lexicographique entre pages remplace le masque fixe et reste exacte
au-delà de 32 flats.

### 2. Construire le premier vrai `next` GPU exact

Baseline sans mosaïque : un bloc par `(v,closure,direction)`, premier scan de
tout le nuage pour réduire le paramètre extrême exact en `i128`, puis second
scan pour compacter **tous** les ex æquo du minimum en ordre d'identifiants.
Cette baseline est une vérité de qualification, pas encore l'architecture 50 k :
en position générale, quatre flats et deux directions donnent environ `16*n*V`
visites de points pour deux passes. Sous la seule hypothèse diagnostique
`V=50` à `150` millions à `n=50 000`, cela ferait $4\cdot10^{13}$ à
$1,2\cdot10^{14}$ visites. Il faut donc certifier un index terminal
output-sensitive, ou une fusion prouvée des requêtes, avant toute extrapolation.

Fixture permanente :

```text
0=(0,0,0) 1=(4,0,0) 2=(0,4,0) 3=(0,2,2)
4=(0,0,4) 5=(0,0,2) 6=(4,4,2)
v: shell={0,1,2,3}, flat={0,1,2}, direction=+1
```

Le point 4, rencontré d'abord, donne l'événement plus lointain `t=2`; 5 et 6
donnent ensemble le minimum `t=1`. Le voisin attendu est
`shell={0,1,2,5,6}`, `interior={3}`, `level=1`. Cette fixture tue
`first-valid-wins`, la perte d'un ex æquo, le mauvais sens et l'oubli du
transfert intérieur. Elle contient aussi une arête parent positive et une autre
arête à retour admissible mais rejetée par un parent antérieur.

### 3. Source directe certifiée : verrou mathématique désormais formulé

Une voie exacte sans propriétaire shallow est maintenant démontrée sous une
capability séparée `center-cover + degree`. Pour chaque arité
$q\in\{2,3,4\}$, poser $t_q=s_{\max}-q+1$. Une partition canonique de la boîte
du nuage authentifie, dans chaque feuille fermée, $t_q$ PointId distincts dont
la distance carrée maximale à tout centre de la feuille est strictement
inférieure à un entier $Q_q$.

Si une miniboule propre $B_U$ vérifie $q+\lvert I(B_U)\rvert\leq s_{\max}$,
son rayon carré est strictement inférieur à $Q_q$; sinon les $t_q$ témoins de
la feuille de son centre seraient tous intérieurs. Dès lors, pour une ancre
$p\in U$, tout point de la boule fermée appartient au voisinage exact
$N_q(p)=\{x\neq p:\lVert x-p\rVert^2<4Q_q\}$. Énumérer une fois chaque support
par $p=\min U$, tester le bien-centrage, puis effectuer le census dans ce
voisinage est complet sans sommet d'arrangement ni mosaïque.

L'ordre non circulaire est essentiel : localiser le centre rationnel, tester
d'abord la banque de témoins; tous intérieurs donnent
`AboveInteriorWindow`. Sinon un témoin non intérieur prouve
$\mathrm{beta}<Q_q$ **avant** le census local. Le fallback racine
$Q_q=\sum_i\mathrm{span}_i^2+1$ rend la méthode totale, mais peut donner
$N_q(p)=X$ et ne prouve aucun SLO.

La vérification indépendante n'a trouvé aucun écart sur 4 105 supports propres
aléatoires, dont 4 085 dans la fenêtre, ni sur dix comparaisons exhaustives du
critère `RelevantGP`. Ce crédit porte sur les lemmes, pas sur une implémentation.
La porte de coût doit recevoir le cover et sa construction, le degré complet,
les CSR, les masses combinadiques, les pas du locator, le tri/groupement et les
replays. Avec $d_q(p)=\lvert N_q(p)\rvert$ et $d_q^+(p)$ le degré vers les
identifiants supérieurs, elle publie au moins
$C_q=\sum_p\binom{d_q^+(p)}{q-1}$,
$T_q=\sum_p d_q(p)\binom{d_q^+(p)}{q-1}$ et
$H_q\leq T_q+t_qC_q$.

Cette voie sépare deux univers : le catalogue fermé exige
$\lvert I\rvert+\lvert S\rvert\leq s_{\max}$; la source Gabriel ouverte exige
seulement $q+\lvert I\rvert\leq s_{\max}$ et doit grouper tous les supports
par `SphereKey` avant de développer l'extra-shell. Le profileur fermé et son
ratio de 6,5 ne dimensionnent donc pas cette source.

Il reste une incompatibilité contractuelle explicite à résoudre avant
implémentation : la norme courante exige encore un shell complet pour tout
support rencontré avec $\lvert I\rvert\leq s_{\max}-2$. Le terminal
`AboveInteriorWindow` par arité est mathématiquement suffisant pour rendre
l'antécédent utile impossible, mais il doit être versionné dans le contrat; il
ne peut pas être déclaré conforme par simple optimisation.

### 4. Fermer les tâches avant le débit

Une tâche porte snapshot/digest, racine structurelle, sommet, curseur exact et
segment de sortie. Ses slots d'adjacence sont tous classés; donation du
sous-arbre, convention d'émission de sa racine et retrait du domaine du donneur
sont un seul point de linéarisation. Le segment ne devient public qu'au commit;
sinon un replay duplique son préfixe.

Ensuite seulement viennent owner par supports, census exact cappé, runs à clef
de niveau 384 bits, fold de lots complets et reçus 50 k/G4. La construction
détaillée est dans
[`NOTE_VERROUS_MATHEMATIQUES_GPU.md`](NOTE_VERROUS_MATHEMATIQUES_GPU.md).

## Porte exigée avant la prochaine session G4 qualifiante

La session déclarée confirme que le microkernel se lance; sa porte reste
censurée. Avant une nouvelle session prétendant qualifier davantage :

1. fermer le replay des 35 flats et le mutant all-refused;
2. authentifier le job et l'enveloppe CUDA;
3. imposer des planchers acceptés/refusés/rejoués et kernels lancés;
4. comparer les signatures complètes, pas seulement compte et masque;
5. sceller commit, diff, toolkit, driver, architecture, binaire, PTX/cubin,
   ressources `ptxas`, digest d'entrée et répétitions.

Le reçu 50 k final doit en plus publier temps bout-en-bout par étage, octets et
high-waters par conteneur, ledger des tâches, drains CPU, concordance exacte des
runs et arrêt ciblé GCP certifié. Le débit kernel-only du microkernel ne peut pas
valider le contrat industriel.

GCP utilisé uniquement en lecture seule pour vérifier l'état final des cibles;
aucune VM créée, démarrée, arrêtée ou modifiée par l'auditeur.
