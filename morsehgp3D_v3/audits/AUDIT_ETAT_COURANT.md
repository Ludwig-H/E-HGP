# Audit courant de MorseHGP3D v3

Date du snapshot : 9 août 2026 UTC.

Cadre annoncé : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_oracle_and_gpu_candidate_under_audit`,
`profile=quantized_u16_input_only`,
`mode=math_locks_plus_gpu_differential`,
`public_status=not_claimed`.

Cet audit porte uniquement sur `morsehgp3D_v3`. Il ne modifie aucun prototype,
n'ouvre aucune phase et ne promeut aucun résultat public. Les sources sont celles
du commit `04555bd`, sauf le filtrage des options C++ dans `CMakeLists.txt`,
présent comme delta live de Claude au moment du scellement.

| objet | empreinte SHA-256 |
| --- | --- |
| `HEAD` | `04555bdd6ff67810bd8db35c4baf18b9eae0063b` |
| `CMakeLists.txt` live | `6cffa15d014e2f817aa5723565a02bbeff1ea523f92fcae2a2b732400ad2ce64` |
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
enfant, ni tâche, ni run. Aucun `nvcc`, `ptxas` ou GPU G4 n'a encore qualifié le
code.

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

Le delta CMake live filtre maintenant `-Wall -Wextra -Werror` sur le seul C++ :
c'est une correction utile. Restent à fermer avant G4 : compilateur NVIDIA et
toolkit corrigé pour `__int128`, architecture exactement `120-real` avant
`enable_language`, contrôle runtime du device, tous les retours CUDA, et reçu
`nvcc/ptxas/PTX/cubin`. Le temps publié est kernel-only; il exclut allocations,
copies et surtout la construction CPU de tout le lot.

Les commentaires du nouveau CMake et des unités wavefront invoquent encore une
ancienne implémentation comme discipline. La consultation était autorisée pour
conseiller Claude, mais le contrat v3 doit être écrit intrinsèquement : aucun
ancien kernel, statut ou résultat ne constitue une preuve du live.

Un probe Clang 18 device-only produit du PTX structurel, mais pas pour `sm_120`;
il déclare 144 octets de local par thread et une forte pression de registres
virtuels. Ce résultat prouve seulement une forme compilable par Clang. Seul
`ptxas` puis l'exécution sur G4 donneront les ressources et le débit réels.

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
- Le lancement `.cu` est séparé et l'option CUDA échoue fermée en l'absence de
  compilateur.
- Le masque 64 bits ne décale jamais de 64 : les deux slots du flat 31 occupent
  les bits 62 et 63.
- `git diff --check` est vert sur le snapshot documenté.

Ces crédits ne prouvent ni les refus, ni le parent, ni le voisin, ni une
exécution device.

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

### 3. Fermer les tâches avant le débit

Une tâche porte snapshot/digest, racine structurelle, sommet, curseur exact et
segment de sortie. Ses slots d'adjacence sont tous classés; donation du
sous-arbre, convention d'émission de sa racine et retrait du domaine du donneur
sont un seul point de linéarisation. Le segment ne devient public qu'au commit;
sinon un replay duplique son préfixe.

Ensuite seulement viennent owner par supports, census exact cappé, runs à clef
de niveau 384 bits, fold de lots complets et reçus 50 k/G4. La construction
détaillée est dans
[`NOTE_VERROUS_MATHEMATIQUES_GPU.md`](NOTE_VERROUS_MATHEMATIQUES_GPU.md).

## Porte exigée avant une session G4 qualifiante

La présence du `.cu` rend une future session utile, mais la porte actuelle est
encore censurée. Avant de facturer G4 :

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

GCP non utilisé pour cet audit.
