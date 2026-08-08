# Feuille de route — déployer la version exacte et industrielle

> **Statut : plan.** Aucun claim, aucune porte ouverte ou fermée, aucun `public_status` modifié.
> Rédigée le 8 août 2026, dépôt à `9b2071a` (arbre propre, synchronisé avec `origin/main`,
> quatre VM G4 relues `TERMINATED`). Elle succède à
> [`PLAN_DE_ROUTE_CONTRATS_50K.md`](PLAN_DE_ROUTE_CONTRATS_50K.md), qui traite du seul contrat
> de performance ; **elle en élargit le périmètre au déploiement**, ce que ce document ne couvre pas.

---

## 0. État du dépôt après l'interruption de session

**Rien n'a été perdu et rien ne coûte.** `git status` propre, aucun stash, `main` aligné sur
`origin/main`, et les quatre VM du projet relues `TERMINATED` — la session interrompue n'en avait
laissé aucune allumée.

Ce que l'interruption a laissé, en revanche, et qui compte pour la suite :

- **Un `ctest` interrompu à 223/234** — `build/morsehgp3d-cpu-release/Testing/Temporary/LastTest.log.tmpe15cb`,
  démarré le 7/8 à 23:12, arrêté à 23:41 ; `CTestCheckpoint.txt` conserve 24 indices de reprise.
  Le dernier échec enregistré est `morsehgp3d.phase3_build_configuration` (20:15), corrigé depuis par
  `96c99db`. **Aucun résultat de suite n'est donc disponible pour l'état actuel du code.**
- **Le build local est en retard sur les sources.** Dernier binaire à 23:09, alors que les quatre
  correctifs de narrowing (`f8ed6ab`…`b3389c0`) touchent des fichiers modifiés entre 23:23 et 00:17 —
  dont `direct_saddle_arm_seed_journal.hpp`, qui est un en-tête. **Un `ctest` immédiat testerait des
  binaires antérieurs aux correctifs** ; reconfigurer et rebâtir avant toute mesure locale.
- 245 tests déclarés localement (preset `cpu-release`, seul configuré ici) ; le chiffre de 254 est
  celui du conteneur, qui construit aussi les cibles CUDA.
- **Le codespace est contraint** : 2 vCPU, 7 Go de RAM, `/workspaces` à 89 % (3,5 Go libres).
  `/tmp` a 38 Go. Cohérent avec la directive : les builds et suites complets vont sur G4.

---

## 1. « Déployer la version industrielle » recouvre trois obligations indépendantes

Le dépôt les distingue déjà, mais nulle part au même endroit. Aucune ne se déduit des deux autres.

| | obligation | état | dépend d'un verrou de performance ? |
|---|---|---|---|
| **O1** | le binaire coordinateur v4 et sa porte d'entrée | **rien de commencé** | **non** |
| **O2** | les quatre portes de campagne P0 → P3 | aucune franchie, P0 non exécutable | oui |
| **O3** | la dette de qualification ouverte | six éléments nommés | non |

**C'est la première correction que la lecture du dépôt impose : deux obligations sur trois ne sont
bloquées par aucun mur mesuré.** Elles sont bloquées par du travail non fait. Le plan de route existant
ne parle que de O2, et le mot « déployer » n'y figure pas.

### O1 — le coordinateur v4 (`morsehgp3d/tests/profiling/phase15_true_hgp_scale_campaign_v4.json`)

Le plan de campagne déclare `entry_gate_satisfied = false` et
`execution_status = blocked_until_true_hgp_v4_product_coordinator_binary_and_entry_gate`.
Quatre manques, vérifiés dans les sources :

1. **Le protocole binaire n'existe pas.** `--phase15-true-hgp-v4-capabilities-json` et
   `--phase15-true-hgp-v4-session-jsonl` n'apparaissent que dans le harnais Python ; **aucune source
   C++ ne les mentionne**. Or `requires_complete_binary_capability_handshake = true`. Le binaire ne
   peut donc pas répondre à l'échange de capacités : la porte est fermée structurellement, pas
   par insuffisance.
2. **Le runner n'a que des modes de diagnostic.** `complete_resident_diagnostic`,
   `right_censorable_full_pipeline_diagnostic`, `fail_fast_capacity_diagnostic`,
   `durable_archive_recertification`. Le plan v4 exige un mode produit dont la frontière de
   chronométrage est `warm_e2e` avec **onze étapes nommées** (`cloud_generation`, `canonicalization`,
   `source_construction`, `source_recertification`, `k1_closed_cut`, `horizontal_reduction`,
   `vertical_maps`, `hartigan_manifest`, `condensation`, `output_seal`, `warm_e2e`). Le runner en
   publie vingt-huit, sous d'autres noms et d'autres regroupements. **C'est un travail de regroupement
   et de scellement, pas un nouveau pipeline** — la tour complète existe. C'est ce qui le rend
   faisable dès aujourd'hui, à toute petite taille.
3. **Le pic VRAM n'est pas instrumenté.** La porte exige un pic $< 80\,\%$ de la VRAM (76,8 Go sur 96).
   Le protocole `--warm-e2e-repetitions` publie le résident hôte (`VmHWM`) et déclare honnêtement
   `device_peak_instrumented = false`, `gate_vram_ceiling_evaluable = false`.
   **Tant que ce champ n'existe pas, P0 est inévaluable même si le p95 passait.**
4. **Le spool transactionnel du protocole v4** — identité `(git_sha, binary_sha256, plan_sha256,
   capabilities_sha256)`, chaîne de reçus `sha256_v1`, verrou POSIX mono-écrivain, reprise re-hachant
   binaire et plan puis recertifiant et rejouant. La discipline existe (archive durable 15L) ; elle
   n'est pas branchée sur ce protocole.

### O2 — les portes de campagne

| porte | charge | familles et répétitions | plafond mural | condition |
|---|---:|---|---:|---|
| P0 | 50 000 | 3 familles, 2 échauffements + 10 mesures chacune (30 mesures) | 30 s | tous certificats valides et **p95 agrégé `warm_e2e` $<$ 1 s** |
| P1 | 1 000 000 | `affine_uniform_binary64`, graine 5101 | 600 s | P0 réussie ; checkpoint forcé, processus neuf, digests identiques |
| P2 | 10 000 001 | idem | 3 600 s | P1 réussie |
| P3 | 30 000 000 | idem | 7 200 s | P2 réussie |

Objectif produit **primaire** conservé par le plan de test : p95 $<$ **100 ms**. Le seuil de 1 s est une
porte de progression **secondaire**. Le binaire mesuré est celui **sans budget** (directive du 7 août).

Les trois familles de P0 sont `affine_uniform_binary64`, `jittered_dyadic_grid3d` et
**`balanced_multiscale_clusters`** — c'est-à-dire précisément la famille où toutes les restrictions
certifiées mesurées jusqu'ici s'effondrent (§3.3). **Le contrat 50 k n'est pas indépendant de la
famille, et la porte l'impose.**

### O3 — la dette ouverte

1. **La suite complète n'a jamais tourné sur G4 depuis les quatre correctifs de narrowing** (`fad3bb9`) :
   obligation explicitement laissée ouverte faute de temps de session. Record actuel : 252/254.
2. **`check_scope.py` échoue à HEAD** — `docs/math/AUDIT_PROBE_LOCAL_MORTON_SUPPORTS_3_4.md:28-30`,
   scope périmé `Perg-HGP`. Dette préexistante, vérifiée aujourd'hui.
3. **L'arité 4 n'a jamais été exercée en parité produit.** `uniform_latin` — famille par défaut du
   runner et famille de la parité device–hôte certifiée du 7/8 — ne contient **aucun quadruple minimal
   bien centré** à $n=32/64/128$. Les 171 événements de cette parité sont exclusivement des triples.
   À rejouer sur `eight_clusters` (735 quadruples à $n=32/K=10$).
4. **`every_prune_fully_recertified = false`** sur l'étage paire : 12 620 régions intégralement
   recertifiées + 4 096 échantillonnées sur 8 102 972.
5. **Sept portes `*_next_gate_satisfied` à `false`** en phase 15, et `exit_gate_satisfied = false`.
6. **Deux obligations de preuve** : **M.1** (jalon `v1_interactive_scalable`) et la fidélité du porteur
   paresseux (phase 10 fermée *administrativement*).

---

## 2. Le contrat 50 k étage par étage, avec la mesure qui l'établit

Budget de référence, contrat A (1 s de p95 sur 48 cœurs $=$ 48 cœur-secondes) :
**21 680 ns par record à $K{=}5$** ($N_{\text{out}} = 2{,}21\cdot10^{6}$),
**2 694 ns à $K{=}10$** ($N_{\text{out}} = 1{,}78\cdot10^{7}$).

| étage | mesure | source | écart au contrat A |
|---|---|---|---:|
| amont (canonicalisation 3,1 ms + LBVH 15,3 ms) | **18,4 ms** | `q1prime_k2_50k.json` | **tient** |
| étage paire — chemin **hôte**, celui du runner aujourd'hui | 12,76 % de la partition en 899,9 s, couverture $\sim t^{0{,}786}$ ⇒ **3 h 26** extrapolées, **indépendant de $K$** | `q1prime_k2_50k.json` | $1{,}2\cdot10^{4}\times$ |
| étage paire — lanceur **natif device**, rang fermé 11 | **2,434 s**, partition exacte $7\,962\,604 + 1\,242\,012\,396 = 1\,249\,975\,000$, `unresolved_pair_mass=0` | `q3_rank11_50k.json` | 2,4 $\times$ |
| recertification paire hôte | 1,072–1,272 µs/record, **strictement linéaire et mono-thread**, 54 % du coût au rang 11 : 8,628 s ⇒ **0,180 s sur 48 cœurs** | idem | tient une fois parallélisée |
| étage higher — **volume** | $\text{visites} \sim n^{4{,}007}$ (1,95 à 2,91 $\times\binom n4$) ; univers $2{,}604\cdot10^{17}$ ; **ne termine à aucune taille $\ge 400$** | `sw_*.json`, `q2_devtiled_n400/n1000.json` | $\infty$ |
| étage higher — **coût unitaire** | **192,6 à 207,6 µs par visite de produit, CONSTANT sur $n=12\ldots64$** | `sw_*.json` | $\sim 10\times$ à $K{=}5$, $\sim 75\times$ à $K{=}10$ |
| aval (fermeture de descente de facette) | $\text{ms/nœud} = 0{,}0313\,n^{1{,}060}$ (11 tailles, une par cœur, G4) ⇒ **2 995 ms/nœud à 50 000** ; nœuds/événement **constant** 7,24–10,62 | `fad3bb9` | **$1{,}1\cdot10^{6}\times$** |

Deux lectures que ces chiffres imposent et qu'il faut garder en tête partout :

- **L'hôte est lent à chercher, pas à certifier.** 23,2 µs par visite de nœud en recherche
  (407 ns/prédicat $\times$ 57 prédicats) contre **74 ns sur device** — facteur 5 078. Mais le même hôte
  **recertifie** la sortie device à 1,08 µs/record. La séparation chercher/certifier est le seul
  résultat d'ingénierie qui ait déjà fait tomber un ordre de grandeur.
- **Le coût du lanceur n'est pas déterminé par son travail.** À compteurs bit-à-bit identiques
  (mêmes 32 875 936 visites, même `output_digest_fnv1a`), le temps du lanceur passe de 2,434 s à
  7,480 s selon le contexte — **facteur 3,073, cause non attribuée**. Tant qu'elle ne l'est pas,
  aucune promesse sur l'étage paire ne tient.

---

## 3. Les verrous, dans l'ordre que la mesure impose

### 3.1 V-A — la substitution du lanceur natif n'est pas câblée *(ingénierie, débloque tout le reste)*

Q3 a qualifié le lanceur natif à 50 000 points **au rang du contrat**. Le runner produit porte encore
la session paire hôte. Conséquence directe et mesurée : **Q1 est restée sans réponse pour cette seule
raison** — à 50 000 points l'étage paire consomme les 900 s de délai, `higher_support = 0`,
`batch_plan = reducer_setup = reducer_stream = 0,0 ms`. Aucune question sur l'aval n'est atteignable
tant que ce câblage n'est pas fait.

### 3.2 V-B — l'aval : la **pente**, pas la constante *(ingénierie, mais l'attribution manque)*

`ms/nœud` $= 0{,}0313\,n^{1{,}060}$. A1 (`b825dd9`) a gagné **une constante** (1,65 $\times$ à 2,42 $\times$)
et **n'a pas touché l'exposant** — la G4 l'a établi sur 11 tailles là où 5 points sur une plage de
2,3 $\times$ avaient produit un faux $n^{0{,}559}$ et un « facteur 110 » inexistant.

Résidu nommé, profil callgrind à $n=24$ **après** A1, coûts exclusifs :
**comparaison de grands entiers 33,25 % + `subtract_unsigned` 29,12 % = 62 % dans deux fonctions.**
Cause désignée : `ExactRational::operator<=>`, qui construit la différence entière complète
$n_1d_2 - n_2d_1$ — **vérifié dans `include/morsehgp3d/exact/rational.hpp:107` et
`level.hpp:106`** : deux multiplications, une soustraction, au moins deux allocations, puis deux
comparaisons à zéro, par comparaison.

**Le plan de route existant désigne comme suite immédiate la cure R1-d sur ces comparaisons. C'est le
levier C, plafonné par Amdahl à $\le 12{,}8\times$, et il ne touche pas l'exposant par construction —
alors que le critère écrit dans le même document est « la pente doit baisser ».** Le faire avant
d'attribuer le terme $O(n)$, ce serait répéter exactement l'erreur d'A1 : optimiser la fonction que le
profil met en tête sans avoir compté ses appels.

### 3.3 V-C — l'étage higher : deux facteurs indépendants, dont un seul est de la recherche

**Facteur 1, le volume — c'est le verrou de recherche, et il est entier.**
L'énoncé exact, tel que le rapport RNG-HGP le rend :
*trouver les centres $c$ dont la boule des $K+1$ plus proches voisins porte au moins trois points sur sa
frontière, sans énumérer $\binom n3+\binom n4$.*

Ce qui est acquis : le **lemme d'énumération par supports** (§8.1 de la note) valide l'architecture
existante — énumérer des supports de taille 2, 3, 4 seulement, indépendamment de $K$ — et **lève** la
conclusion « la seule route exacte est la construction d'ordre $k$ », laquelle contredisait l'invariant
d'architecture d'`AGENTS.md`. Voir [`../math/AUDIT_RNG_HGP_SIMPLEXES_GPU.md`](../math/AUDIT_RNG_HGP_SIMPLEXES_GPU.md) §7.

Ce qui est fermé, et qu'il ne faut pas ré-ouvrir : **toute la famille des préfiltres par voisinage**, et
on sait maintenant *pourquoi*. Le critère $\bigcap B(x,2r)$ est de niveau **Rips**, le problème de niveau
**Čech** ; le facteur de Jung $\alpha_p > 1$ les sépare. Cela explique d'un coup les deux réfutations
mesurées : la restriction certifiée $D \le 2R(p)$ à 26 directions retire 16 % des paires sur
`uniform_latin` et **exactement 0 %** sur `eight_clusters` ; et le préfiltre par rang d'arête est
**prouvablement incomplet à tout seuil** (queue de rang à 16 et 38, et la boule diamétrale d'un côté
d'un triangle acutangle atteint $1{,}366\,R$).

**Facteur 2, le coût unitaire — c'est de l'ingénierie, et il est chiffré.**
200 µs par visite de produit, **constant en $n$**, donc encore 200 µs à 50 000 points. Contre un budget
de 21,7 µs par record à $K{=}5$ : il manque **$\sim 10\times$** même avec un générateur parfait, et
$\sim 75\times$ à $K{=}10$. Leviers nommés, aucun mesuré :
`terminal_classification_native_cuda` encore **faux** (centre, rayon, boule fermée, rang, événement
sont sur CPU) ; **le comptage borné à $K+2$**, qui n'existe pas comme primitive ;
le nombre de threads explorateurs **jamais varié** (`kThreadsPerBlock=128`, slots plafonnés à 1024,
soit $\le 8$ blocs sur 188 SM).

### 3.4 V-D — le coordinateur v4 *(pur travail, aucun verrou)* — §1, O1

### 3.5 V-E — la dette *(pur travail, aucun verrou)* — §1, O3

---

## 4. La feuille de route

Trois fils avancent **en parallèle**, parce que deux d'entre eux ne dépendent d'aucun résultat de
recherche. Les faire attendre le troisième, c'est garantir qu'il n'y aura rien à déployer même si le
troisième aboutit.

### Fil α — local, sans GPU, dans l'ordre

**α1 — Attribuer le terme $O(n)$ de l'aval. *Avant* toute cure.**
Compter, par callgrind, le **nombre d'appels** aux deux fonctions résiduelles **par nœud de fermeture**,
à $n = 12, 16, 24, 32, 48$ — exactement l'instrument qui a corrigé la prémisse d'A1 (823 recalculs de
digest trouvés par comptage, là où le grep en annonçait huit). Mesurer aussi la **largeur en bits** des
opérandes en fonction de $n$. Trois issues, trois correctifs **différents** :

- **H1 — le nombre de comparaisons par nœud croît en $n$** : il existe un balayage linéaire dans la
  fermeture. Correctif **structurel** (index, tri unique, recherche dichotomique). **Fait tomber la pente.**
- **H2 — le nombre est constant, la largeur des opérandes croît en $n$** : une somme de rationnels non
  dyadiques accumule des dénominateurs (candidat : les masses $S_\tau = \sum \psi(\rho(\sigma))$ du
  chapitre 9 ; noter que `ExactRational::from_binary64` produit des dénominateurs **dyadiques**, donc
  les niveaux seuls ne peuvent pas croître). Correctif **représentationnel** (dénominateur commun,
  largeur fixe). **Fait tomber la pente.**
- **H3 — ni l'un ni l'autre** : les 62 % sont une constante, la cure R1-d est plafonnée à
  $\le 12{,}8\times$ et **ne fera pas tomber la pente** ; le terme $O(n)$ est ailleurs, et **A2**
  (mémoïsation du manifeste de source de forêt, qui hache le nuage) reste le candidat nommé et
  jamais exécuté.

*Critère de sortie : une **attribution** — un terme $O(n)$ nommé, avec son site et son compte — et non
un pourcentage.* Coût : quelques heures, aucun GPU.

> **Ne pas transporter la réfutation du filtre fp64 ici.** Elle porte (`1ed4ebf`) sur un repli en
> **intervalle entier de largeur fixe à 4 membres** — le moteur higher, où le filtre était déjà
> superflu. Le repli de l'aval est du **rationnel non borné**, exactement le cas où le dépôt a déjà
> mesuré 97,8 % de filtrage sur l'étage paire. Le verdict d'un étage ne vaut pas pour l'autre.

**α2 — Le correctif désigné par α1**, avec son critère écrit : sortie scientifique identique **et la
pente `ms/nœud/point` doit baisser**. Remesurer sur les mêmes tailles, en parallèle sur la G4 (une
taille par cœur) — c'est ce protocole qui a démasqué l'artefact de plage.

**α3 — Câbler le lanceur natif dans le runner (V-A).**
Sortie falsifiable : le runner rend à 50 000 points la partition de la qualification autonome —
7 962 604 candidats, 1 242 012 396 élagages, somme $= 1\,249\,975\,000$, `output_digest_fnv1a` identique.

**α4 — Paralléliser la recertification paire.** 8,628 s → 0,180 s, chiffré. Boucle strictement linéaire,
indépendante par record.
*Réserve à traiter une fois pour toutes : c'est le **premier multithread du chemin exact*** — trois
fichiers seulement en portent aujourd'hui, tous hors de ce chemin. Poser la discipline maintenant :
partition déterministe, réduction en ordre canonique, **digest indépendant du nombre de threads**, et
un test qui l'épingle. Ensuite fermer `every_prune_fully_recertified`.

**α5 — Le comptage borné à $K+2$** (idée retenue de l'audit RNG-HGP, §3.1).
Primitive exacte : rendre $\min(|X\cap B|,\,K+2)$ et s'arrêter. Aucune décision perdue. Elle vise le
facteur $\sim 10$ du coût unitaire de l'étage higher, et **se mesure sans GPU** sur le recensement
exhaustif existant, à classification identique au record près. Critère : le coût **par candidat rejeté**
doit baisser d'un ordre, sinon on l'écrit et on abandonne.

**α6 — Le rapport $|\operatorname{RNG}^{\mathrm{HGP}}_K| / |\operatorname{Gab}_K|$**, sur le même
recensement, famille `eight_clusters` **obligatoire**. Quelques heures. C'est la seule grandeur qui
déciderait si le critère RNG-HGP vaut quelque chose pour l'aval ; personne ne l'a mesurée.

**α7 — Construire le harnais d'entrée synthétique de la fermeture de descente de facette.**
Il existe pour le réducteur de hiérarchie de points (2 792 ms sur tour synthétique 50 k) et **pas** pour
cette fermeture. Sans lui, la constante de l'aval **à la taille du contrat** restera inconnue — or
toutes les constantes actuelles viennent de $n \le 64$, et le dépôt a déjà appris deux fois que les
constantes des tout petits nuages ne valent pas à l'échelle.

### Fil β — le coordinateur v4 (O1), en parallèle, dès maintenant

Rien dans β ne dépend de α ni de γ. β est le **seul** fil dont le coût est prévisible.

**β1** — le mode produit et le regroupement des onze étapes de `warm_e2e` ; à valider à $n$ minuscule.
**β2** — l'échange de capacités et le protocole de session v4 côté C++.
**β3** — l'instrumentation du **pic VRAM**, sans laquelle P0 est inévaluable. Commencer par l'audit
d'empreinte par record contre le minimum théorique (audit RNG-HGP §3.2) : c'est plus rapide, et cela
dit d'avance si la porte est atteignable à $1{,}8\cdot10^{7}$ records.
**β4** — le spool transactionnel v4 sur la discipline 15L existante.
**β5** — fermer O3 : `check_scope.py`, la parité arité 4 sur `eight_clusters`, les sept `next_gate`.

*Sans β, il n'y a rien à déployer, quelle que soit la vitesse atteinte.*

### Fil γ — le verrou de recherche (V-C, facteur 1)

C'est le seul travail dont l'issue n'est pas prévisible, et il ne faut pas le confondre avec les autres.

L'écart est désormais énonçable en une phrase : **l'étage paire élague par un certificat de distance
ancré, et il y arrive parce que le rayon d'une paire *est* la moitié de sa distance** — d'où la
partition exacte de $1{,}25\cdot10^{9}$ paires en 2,434 s, à 5,04 visites par record. **L'étage higher
explore le produit** — mesuré $\sim n^{4{,}007}$ — parce que pour un support de taille 3 ou 4 le centre
n'est plus déterminé par une distance mais **libre dans un compact** (cascade de Jung).

Le schéma que la note externe nomme (§10.3) est *« agrandir adaptativement la région explorée jusqu'à ce
que les bornes de distance aux cellules non visitées excluent tout support ou témoin manquant »*, et le
dépôt possède déjà la primitive : `box_minimum_squared_distance_exceeds_level`, livrée par R1-c. Ce qui
manque est le certificat qui borne cette expansion pour un support de taille 3 ou 4 — c'est-à-dire
exactement ce que la restriction $D \le 2R(p)$ tentait et que la mesure a réfuté sur nuage aggloméré.

**γ ne doit pas bloquer α ni β, et α ne doit pas attendre γ** : à $K{=}5$, tous les étages sauf l'aval
tiennent à un facteur 3 près une fois α fait, et l'aval se travaille sans le générateur (fil α, harnais
synthétique α7).

### Puis, et seulement alors

**P0**, sur les trois familles, sur le binaire sans budget, avec le pic VRAM instrumenté. Puis P1, P2, P3
selon la matrice, chaque porte conditionnant la suivante.

---

## 5. Ce qui ne doit pas être ré-exploré

Chacun est un résultat **mesuré**, pas une opinion.

| piste | pourquoi elle est close | trace |
|---|---|---|
| réglages de tuile de l'étage higher (T1, T2) | le coût est proportionnel au travail **exploré**, invariant sous partitionnement ; $\sim0{,}02\,\%$ de $C(512,3)+C(512,4)$ par 150 s **quels que soient les boutons** | `phase15_unbounded_frontier_t2_conclusion` |
| baisser $K$ pour débloquer l'étage paire hôte | le mur hôte est **indépendant de l'ordre** (5,38 % interpolé à $K{=}2$ contre 5,78 % scellés au rang 6) | `q1prime_k2_50k.json` |
| filtre fp64 devant les portes du moteur higher | A/B mesuré $\sim 2\,\%$, du bruit ; le repli y est déjà un intervalle entier de largeur fixe | `1ed4ebf` |
| préfiltres par voisinage, à tout seuil | critère de niveau Rips sur un problème de niveau Čech (facteur de Jung) ; 0 % sur `eight_clusters`, queue de rang à 38, incomplétude **prouvée** | `APPORTS_RAPPORT_RNG_HGP.md` §2 |
| localité dyadique seule | le propriétaire dyadique est une **fonction** de $U$ donc une **partition** : énumérer par propriétaire refait l'univers terme pour terme (gain 240 $\times$) | `GERMINATION_LOCALE_SUPPORTS_3_4.md` |
| la porte de bon centrage comme source de sensibilité à la sortie | fraction bien centrée **constante en $n$** (28,0–28,9 % des triples, 9,7–11,3 % des quadruples) ⇒ $5{,}8\cdot10^{12}$ triples et $2{,}5\cdot10^{16}$ quadruples bien centrés à 50 k | recensement $n=32/64/128$ |
| mosaïque de Delaunay d'ordre supérieur comme produit | invariant d'architecture d'`AGENTS.md` **et** piste archivée ; et le lemme d'énumération par supports montre qu'elle est **inutile** | `AUDIT_RNG_HGP_SIMPLEXES_GPU.md` §7 |
| tailles de nuage intermédiaires | discipline scellée : tout petits nuages → **directement 50 000** → dizaines de millions, **rien entre** | directive du 6/8 |

---

## 6. Ce que cette feuille de route ne promet pas

- **Elle ne promet pas P0.** Le contrat A exige que le verrou γ soit fermé, et γ est une question
  ouverte dont l'issue n'est pas prévisible. Aucune date n'est proposée.
- **Elle ne promet pas le contrat B (100 ms).** À $K{=}10$ il faudrait 267 ns par record tous étages
  réunis quand la seule classification en coûte aujourd'hui 65 700. Le contrat B n'est pas une version
  plus rapide du contrat A.
- **Elle ne promet pas que les fils α et β suffisent.** Ils ferment l'étage paire, ils rendent l'aval
  mesurable à l'échelle, ils rendent le produit **déployable** — ils ne ferment pas l'étage higher.
- **Rien de ce qui suit ne peut promouvoir un `public_status`.** Seuls M.1 et la migration
  contractuelle `versioned_direct_morse_exact_proof_basis_activation` le peuvent (`AGENTS.md`).

Ce que la feuille de route **affirme**, en revanche : deux tiers du déploiement (O1 et O3) ne sont
bloqués par aucun mur, et l'étage paire est **déjà** à un facteur 2,4 du contrat au rang du contrat.
Ce n'était vrai d'aucune des trois versions précédentes de ce plan.

---

## 7. Divergences de documentation à corriger

Relevées lors de la rédaction, non corrigées ici :

1. **`ROADMAP_IMPLEMENTATION_MORSEHGP3D.md` a $\sim 6$ h 30 de retard** sur le journal Phase 15 et sur
   `implementation_status.toml`. Elle porte encore le mur paire hôte et le rang fermé 6 ; ni les 2,434 s
   au rang 11, ni l'arbitrage $K$ (facteur 2,03), ni le théorème de germination (**0 occurrence**), ni le
   défaut de quantum du coupe-circuit n'y figurent.
2. **Ses lignes 2685 et 2691 donnent chacune « les portes suivantes, dans l'ordre »**, la première privée
   du verrou ① que la seconde rouvre. Une lecture littérale de 2685 omet le verrou dominant. À réécrire
   en un seul endroit.
3. **La porte préalable « recensement de la sortie utile » y est déclarée ouverte alors qu'elle a été
   franchie dans le même commit** ; le chiffre $N_{\text{out}} \approx 1{,}8\cdot10^{7}$ n'apparaît nulle part
   dans la roadmap.
4. **`CONTRAT_50K_BILAN.md` est périmé** : il présente la germination locale certifiée comme le levier
   acquis ($4{,}4\cdot10^{8}$ candidats), prémisse que `PLAN_DE_ROUTE_CONTRATS_50K.md` §I2 réfute ensuite.
   Les deux documents sont en contradiction ouverte.
5. **`PLAN_DE_ROUTE_CONTRATS_50K.md` §1, §3 (V2) et §4 reposent sur le chiffre $4{,}4\cdot10^{8}$** que son
   propre §I2 invalide.
