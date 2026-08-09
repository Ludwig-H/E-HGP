# Audit courant de MorseHGP3D v3

Date du snapshot : 9 août 2026 UTC.

Cadre annoncé : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_oracle_and_gpu_candidate_under_audit`,
`profile=quantized_u16_input_only`,
`mode=math_locks_plus_gpu_differential`,
`public_status=not_claimed`.

Cet audit porte uniquement sur `morsehgp3D_v3`. Il ne modifie aucun prototype,
n'ouvre aucune phase et ne promeut aucun résultat public. Le snapshot courant
`f851374` conserve les sources wavefront de `04555bd`, ajoute les résultats G4
documentaires de `78583f1`, leur interprétation à 100 ms dans `444b851`, puis un
profileur CPU à densité fixe. Aucun artefact brut de la session G4 n'est
versionné avec ces commits.

| objet | empreinte SHA-256 |
| --- | --- |
| `HEAD` | `f851374cb628f88eafd9a2efaf7e293eb62e1d62` |
| `CMakeLists.txt` | `7c770bcc16ed57410b7b6cda32854e8029f7e4ee06b6722cfa0a256bb67817ef` |
| `prototype/scale_profile.cpp` | `e6c31f544d8275b3f89affde11b52e11972dd7e76cf9b556112c96a43d96aacb` |
| `prototype/order_k_flats.hpp` | `02ad6f58632de60d47e0b2bbcdf6205d8a3b9d1cab1474dd9d8b566593e9e81a` |
| `prototype/order_k_device_core.hpp` | `79382cf2857fb8da4efcecda8b9a164643fb4013c9a56cd6152f102daa155a3d` |
| `prototype/flats_differential.cpp` | `14c690031debf7214ae0fcd40ced0fd1a4169a06b34b0f035ca7103692384fa3` |
| `prototype/device_wavefront_job.hpp` | `cffe45646eb46ec44f4818ce8c8f0a3e7251084d8fb05c0cb79fbfae243fa31f` |
| `prototype/device_wavefront_kernel.cu` | `bebc6684ccacd763d28d2f336b9cfd17b356914addf37786afbe0c7440901ccc` |
| `prototype/device_wavefront_qualification.cpp` | `3ae284cd1e431ec22ccfe30efa4c3afef8cc91c5b87c92d696f84c2b088cbf89` |
| `audits/check_gate_d_fold_f0.py` | `34149092cd1b06762085800ac9d575c0cb8022e3a1c273c7d1955d2f4e768294` |

## Verdict

**NO-GO de justesse owner, NO-GO F0, et NO-GO de qualification GPU/replay.**

Le premier `.cu` est un progrès réel : il définit un lancement CUDA optionnel
et un même corps source pour CPU/device. Les quatre portes hôte, dont une
campagne forçant 27 refus, sont vertes. Cette avancée ne ferme toutefois pas le
contrat annoncé : les refus sont précisément exclus de l'oracle et ne sont
jamais rejoués. Un mutant qui refuse tous les sommets reste vert.

Le kernel ne constitue pas encore une wavefront. `navigate_shallow` construit
et mémorise d'abord tous les sommets sur CPU; le kernel calcule ensuite seulement
un masque d'admissibilité des couples. Il ne produit ni voisin, ni parent, ni
enfant, ni tâche, ni run.

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

Les commentaires du nouveau CMake et des unités wavefront invoquent encore une
ancienne implémentation comme discipline. La consultation était autorisée pour
conseiller Claude, mais le contrat v3 doit être écrit intrinsèquement : aucun
ancien kernel, statut ou résultat ne constitue une preuve du live.

Un probe Clang 18 device-only antérieur produisait 144 octets de local par
thread et une forte pression de registres virtuels. La session G4 rapportée ne
conserve aucun diagnostic `ptxas`, spill, stack, registre ou occupation; elle ne
permet donc toujours pas de relier le débit observé aux ressources du cubin.

## Audit du diagnostic G4 et de son interprétation à 100 ms

Le README rapporte quatre mesures kernel-only : 128 955 sommets en 0,224 ms,
71 084 en 0,170 ms, 19 019 en 0,323 ms et 2 542 en 2,020 ms, toutes avec zéro
écart CPU/device sur le `VertexVerdict` borné. En l'absence des sorties brutes,
elles sont conservées comme **diagnostics déclarés**, pas comme reçus
reproductibles.

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

Il ne peut cependant être « le seul chiffre qui décide » les 100 ms :

- la densité `1e-3` est codée en dur et le profil LiDAR est une nappe uniforme
  synthétique, sans famille sanctionnée, digest d'entrée ni quantile;
- les nuages de statut non `kOk` sont retirés de la moyenne; `decided>0` permet
  donc une moyenne partielle sans ledger des refus;
- la déduplication du générateur emploie `std::find` dans un vecteur et coûte
  $O(n^2)$ hors chrono;
- le temps navigation exclut `CertifiedIndex::build`, le temps catalogue
  reconstruit son propre parcours, et la sortie « sans accélérateur » est
  ambiguë puisque l'index est actif;
- ni source directe, ni fold, ni forêts, ni couverture, ni verticales, ni
  octets, ni pipeline GPU ne sont mesurés.
- la cible n'a ni CTest permanent, ni plancher de nuages décidés, ni reçu
  canonique des paramètres et compteurs.

La porte propre publie toutes les graines et tous les statuts, sépare taille du
terrain, taille de sortie et travail par étage, puis mesure des quantiles sur les
familles enregistrées. Un ratio observé reste un diagnostic; il ne devient une
borne à 50 k qu'après un théorème ou une exécution effectivement à 50 k.

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

### 3. Tester la source critique directe comme hypothèse, pas comme acquis

La nouvelle piste « sphères critiques directement » exige une source terminale
de supports `U` d'arité au plus quatre et de rang fermé au plus `s_max`, sans
propriétaire obtenu en parcourant d'abord le terrain. Une surgénération est
acceptable si chaque branche élaguée fournit un certificat entier
`rank > s_max` et si chaque émission porte miniboule, census global complet
`(I,S)`, support canonique, propriétaire et clef de déduplication exacts.

La note de source directe ferme actuellement `sphère certifiée -> cofaces`; elle
ne construit pas encore le stream de sphères certifiées sans partir d'un
propriétaire visité. C'est ce verrou de complétude et de coût qu'il faut fermer
avant de remplacer la reverse-search. Sous la propre hypothèse du README d'un
pipeline coûtant dix à trente fois le microkernel, un filtre par 6,5 laisserait
encore 1,54 à 4,62 budgets; il ne peut donc pas être « exactement » le facteur
manquant.

Le compteur profilé porte en outre sur `flat_catalogue(...,s_max)`, donc sur le
rang **fermé** au plus `s_max`. La source Gabriel **ouverte** doit encore traiter
les supports à peu d'intérieurs stricts mais grand extra-shell. Un élagage de la
source directe doit compter des témoins distincts strictement intérieurs ou
produire la coquille complète; le ratio du profileur porte sur un autre univers
scientifique et ne mesure pas la masse de cette source.

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
