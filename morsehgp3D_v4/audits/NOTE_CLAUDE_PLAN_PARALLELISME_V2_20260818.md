# Note de Claude — plan de parallélisme v2 (CPU 48 vCPU, GPU), ancré au modèle de coût mesuré

Date : 18 août 2026. Demande utilisateur : « réfléchir à comment
optimiser le code pour parallélismes (CPUs et surtout GPU) ». Complète
`NOTE_CLAUDE_PLAN_GPU_20260817.md` avec les mesures de la campagne
locale v2 (`campagne_locale_n8000_v2_20260818/`) et une pièce nouvelle :
l'étage flottant certifié.

## 1. Le modèle de coût MESURÉ (n=8000, smax=11, 4 fils)

| poste | uniform | eight_clusters |
|---|---|---|
| t_gen (mur) | 62 s | 143 s |
| kills du filtre q3 (`gen_tues[1]`) | 34 M | **797 M** |
| seeds tués par le cœur q4 | 17,3 M | 80,8 M |
| sites examinés par le cœur (`sites_core`) | 533 M | 3 695 M |
| t_fold | ~45 s | 44 s |
| t_census + t_prefiltre | ~17 s | 14 s |

Lecture : la génération est dominée par DEUX boucles de forme
IDENTIQUE — « pour un seed, balayer les sites du cover en évaluant un
prédicat exact i128, sortie anticipée à h » : le filtre de profondeur
q3 (797 M de morts sur la famille dense — le poste caché numéro un) et
le cœur de seed q4 (3,7 G de sites). Des MILLIARDS d'évaluations
`q3_power`/témoin-de-Jung. Tout le plan découle de ce fait : c'est un
kernel unique, régulier, arithmétique — la cible parfaite du GPU, et
l'endroit où chaque gain de constante se multiplie par des milliards.

## 2. Le levier transversal : l'étage flottant CERTIFIÉ

Les prédicats n'ont besoin de l'arithmétique exacte QUE près de la
frontière de décision. Étage 1 en `double` avec borne d'erreur
statique ; étage 2 exact (i128/U320) seulement si l'étage 1 ne peut
pas conclure. Esquisse de certification pour
$P(z) = G\vert v\vert^2 - W\cdot v$ (u16 : $G < 2^{68}$,
$\vert W_i\vert < 2^{86}$, $\vert v_i\vert < 2^{17}$) : chaque entrée
arrondie et ~8 opérations en modèle standard
($\mathrm{fl}(x\ \mathrm{op}\ y) = (x\ \mathrm{op}\ y)(1+\delta)$,
$\vert\delta\vert \leq 2^{-53}$) donnent
$\vert \hat{P} - P \vert \leq C \cdot 2^{-53} \cdot S$ avec $S$ la
somme des grandeurs ($\leq 2^{105}$) et $C \approx 10$, soit une borne
absolue $E \approx 2^{55}$ : le signe est certifié dès
$\vert\hat{P}\vert > E$. Sur des données génériques, la masse des
sites à $\vert P \vert \leq 2^{55}$ quand $P$ court jusqu'à $2^{104}$
est infime — l'exact n'arbitre plus que les quasi-égalités. Gains
attendus : ×5–10 scalaire (double+FMA contre i128), ENCORE multiplié
par SIMD/GPU (le double se vectorise, l'i128 non). Discipline :
constantes de la borne dérivées et GRAVÉES (fixture de quasi-égalité
exacte des deux côtés du seuil ; mutant `float-threshold-too-small`
qui réduit E et doit mourir) ; l'étage exact reste l'unique autorité —
l'étage flottant ne décide jamais un cas non certifié. Même
traitement pour le témoin de Jung ($2P^2$ vs $J B^2$) et
`cmp_mu_same_side`.

## 3. CPU (48 vCPU de la G4)

1. **Le filtre q3 mérite le traitement que q4 a reçu.** Deux voies :
   - *Voie structurelle (question ouverte aux auditeurs)* : les boules
     par $(a,b)$ ont leur centre sur le plan bissecteur $\Pi$ ;
     « $z$ intérieur à la boule de centre $c$ » $\iff$
     $2(a-z)\cdot c < \vert a\vert^2 - \vert z\vert^2$ — LINÉAIRE en
     $c$ : chaque site $z$ est un demi-plan de $\Pi$, chaque seed $x$
     un point $c(x) \in \Pi$, et la profondeur est un COMPTE DE
     COUVERTURE par demi-plans, saturé à $h_3 \leq 9$. C'est l'analogue
     2D exact du sweep axial 1D (le niveau-$h$ d'un arrangement de
     droites). Y a-t-il une requête « sous le niveau $h$ » en
     $O((N+M)\log)$ par ancre qui préserve l'exactitude rationnelle ?
   - *Voie sûre* : étage flottant certifié + SIMD — aucun théorème
     nouveau, gain immédiat.
2. **Échelle 48 fils** : le tirage dynamique par rectangle scale tel
   quel ; projection eight_clusters n=8000 : t_gen 143 s → ~15 s. Les
   points d'attention sont l'aval (fold 44 s, dont ~40 s de réduction
   séquentielle par K dominant — chantier #25 en attente du cadrage
   auditeurs) et la fusion des shards (concat mono-fil, négligeable
   aujourd'hui, à surveiller à 48).
3. **Overlap gen/aval** : produire les candidats par vagues de
   rectangles et commencer tri+préfiltre pendant la fin de génération —
   gain borné (~10 %), après le reste.

## 4. GPU (sm_120), par vagues

**Vague 1 — le kernel de balayage de cover** (unifie cœur q4 ET filtre
q3, puisque même forme) :
- Découpe : un BLOC par ancre (ou paquet d'ancres à petits covers),
  cover en mémoire partagée en SoA (`u16 x,y,z` + précalculs :
  $\vert z\vert^2$ en u64), un WARP par seed.
- Boucle : le warp évalue 32 sites/pas en double (étage certifié),
  `__ballot_sync` compte les témoins certains, sortie dès saturation
  ($h \leq 9$) — les seeds tués (90 %+) sortent en 1–3 pas ; les cas
  non certifiés sont COMPACTÉS vers une passe exacte (i128/U320 device,
  `__int128` supporté par nvcc, primitives déjà annotées `MHGP4_HD`) —
  rares, donc sans divergence de masse.
- Covers plus grands que la shared : tuilage — les comptes saturés se
  composent entre tuiles (tuer dès qu'un préfixe atteint $h$ est
  exact).
- Sortie : bitmap des survivants → préfixe-somme (CUB) → compaction.

**Vague 2 — sweep + émission par seed survivant** : un thread par seed
survivant, la primitive `axial_two_sided_sweep` telle quelle (tableaux
fixes ≤ 16 → registres/local, zéro allocation — elle a été écrite pour
ça) ; émissions à offsets fixes seed×16 puis compaction ; tri par
BallKey + RLE sur device (radix CUB sur la clé sérialisée) et transfert
des seules uniques — à 30 M de points, transférer les candidats bruts
serait le goulot PCIe, pas le calcul.

**Vague 3 (plus tard)** : census par clé (descente régulière,
candidate) ; réductions segmentées du fold par partitions — APRÈS le
contrat `product` (l'audit C829 § 4.5 : le GPU n'abolit pas la taille
d'une sortie explicitement demandée).

**Ce qui ne part jamais sur GPU** : le juge et les oracles
(l'indépendance est leur raison d'être) ; ils jugent le GPU.

## 5. Exactitude et portes (identiques dans l'esprit à --par-gate)

Le flux GPU→CPU est un MULTIENSEMBLE d'émissions ; tri stable + RLE
canonisent : égalité au bit près post-RLE contre le CPU séquentiel,
compteurs sommes égaux. Portes : selftest arithmétique DEVICE (mêmes
témoins que `mhgp3v_arith_selftest`), porte appariée `--gpu` sur les
familles jugées + fixtures gravées (1513/49, fixture-cœur,
cosphérique), mutant `gpu-drop-block`, mutant
`float-threshold-too-small`, équivariance par permutation. Convention
dépôt : `MHGP4_ENABLE_CUDA` OFF par défaut, build CUDA depuis un
worktree propre, compilation et validation SUR G4 uniquement (pas de
nvcc local — pas de code invérifiable poussé).

## 6. Ordre recommandé

1. Étage flottant certifié CPU (borne dérivée + fixtures + mutants) —
   il sert le CPU immédiatement ET devient l'étage 1 du kernel GPU.
2. Filtre q3 : appliquer l'étage flottant ; poser la question du
   niveau-$h$ 2D aux auditeurs en parallèle.
3. Kernel GPU vague 1 sur G4 (session gardée), jugé par la porte
   appariée --gpu.
4. Vague 2 (sweep device + tri/RLE device).
5. Fold/census selon le contrat `product` et le cadrage #25.

Estimation honnête, à vérifier par la mesure comme toujours : étage
flottant ×5+ sur t_gen CPU ; G4 48 fils ×10 sur l'état actuel ; GPU
vague 1+2 : la génération cesse d'être le poste dominant à toute
échelle mesurée — le pipeline devient borné par l'aval, exactement là
où le contrat de produit et le chantier fold décident de la suite.
