# Addendum — primitive de sweep extraite, kernel sans allocation, compteurs à unités séparées

Date : 17 août 2026. Base de mesure : `ec0c8d9` (cœur de seed livré) ;
code livré dans le commit portant ce reçu. Audit exécuté :
`audits/AUDIT_CIBLE_48E446_SWEEP_RECU_ET_KERNEL_SANS_ALLOC_20260817.md`
(§ 1, § 2, § 3 — l'ordre utile complet).

## § 3 — la primitive extraite et la causalité des mutants

`axial_two_sided_sweep(sites, n, p, h, flags, gid)` vit désormais seule
dans `ball_stream.hpp` : aucune géométrie, aucune allocation — seuils
bornés des deux côtés, classification en fenêtre $[L,U]$ (ties inclus),
$\leq 16$ groupes en tableaux fixes triés par $\mu$, préfixes/suffixes,
$d_j$ exact et verdict par groupe, `gid` par site (0xff hors fenêtre).
La borne $2k \leq 16$ est prouvée en tête de primitive (au plus $k$
valeurs distinctes par côté en fenêtre) ; un débordement n'est possible
que sous mutant de classification et il est rendu bruyant sur le canal
de réception.

La porte synthétique `--axial-sweep-gate` grave les multisets de
l'audit :

- $\lbrace \mu_+ = 0 ;\ \mu_- = 1, 2, 3 ;\ p = 0, h = 3 \rbrace$ :
  normalement la positive meurt par le côté OPPOSÉ (sous $L=1$), le
  groupe $\mu=1$ meurt en fenêtre ($d_j = 3$), $\mu = 2, 3$ vivent.
  Un vrai `ignore-opposite` — désormais CAUSAL : chaque racine ne lit
  plus le seuil du côté opposé, et un groupe d'un seul signe perd le
  terme opposé de $d_j$ — CONSERVE la positive ; `reverse-negative`
  INVERSE les verdicts de $\mu=1$ et $\mu=3$. Exactement le contrat
  demandé.
- un multiset MIXTE (même $\mu$ exact porté par les deux signes : un
  seul groupe, npos=1/nneg=1) et un multiset à TIES au seuil ($U$ porté
  deux fois, $p$ compté dans $d_j$).

Cinq mutants meurent causalement sur la primitive seule (short-group,
drop-ties, ignore-opposite, reverse-negative, depth-nonstrict) ; la
porte appariée reste la porte d'intégration (7 mutants, fixtures
géométriques, `kAxialVerify`). La causalité trop forte de mon reçu
précédent est corrigée par post-scriptum : la fixture 1513/49 tue à la
classification, elle n'isolait pas `reverse-negative` — maintenant la
primitive le fait, et le commentaire de la fixture dit la vérité.

## § 3 (fin) — réception étendue

Sous `kAxialVerify`, $d_j$ est recoupé par le scan `q4_power < 0` pour
TOUS les groupes en fenêtre, groupes MORTS compris : un membre
quelconque a $B \neq 0$, donc le tétraèdre est non coplanaire
(`det != 0`, orientation canonisée par `q4_form`) même s'il échoue
ensuite à l'owner ou au centre. Les membres d'un même groupe partagent
le même $\mu$ exact, donc la même sphère : le premier membre suffit.

## § 1 — kernel sans allocation par seed

`std::vector<MuGroup>` (+ `members` par groupe), `pos_before`,
`neg_after` dynamiques : SUPPRIMÉS. Tableaux fixes de 16, `gid` dans un
tampon réutilisé entre les seeds, seconde passe sur les sites qui
n'appelle `valid_completion` que pour les groupes vivants (meilleur
représentant canonique maintenu en place — mutant first-rep préservé,
premier valide). C'est la forme kernel demandée, directement portable
GPU (l'utilisateur a demandé ce jour la parallélisation puis l'écriture
GPU : ce kernel est l'unité de travail réglière candidate).

## § 2 — compteurs à unités séparées

`axial_groups_killed_two_sided` (racines ET groupes additionnés) est
remplacé par quatre compteurs propres, publiés par le probe :
`racines_croisees` (positives sous $L$ / négatives sur $U$),
`groupes_fenetre`, `groupes_tues_dj` (le VRAI nombre de formations q4
évitées après groupement), `appels_completion`.

## Mesures (audit § 1, sorties identiques au compte près)

eight_clusters n=1000, s=8, smax=11, axial : `t_gen = 35,2 s`
(inchangé vs 34,9 s), 219 653 événements ; décomposition :
`t_core = 27,0 s`, `t_AB = 0,70 s` (contre 1,43 s : −51 %),
`t_reduce = 2,17 s` (absorbe désormais les seuils, autrefois dans
t_AB), `t_emit = 0,21 s`. Compteurs : 6 891 478 racines croisées,
2 474 709 groupes en fenêtre (~5,9 par seed survivant),
**1 302 966 groupes tués par $d_j$**, 1 172 222 appels
`valid_completion` pour 87 043 émissions. uniform n=1600 axial :
26,8 s (parité avec la baseline maintenue), 532 181 événements.

Verdict honnête sur l'hypothèse de l'allocateur : en AVAL du cœur de
seed (qui tue 90 % des seeds avant le sweep), les allocations ne
pesaient plus — `t_AB + t_reduce` est stable (2,76 → 2,87 s). La
divinité discrète avait déjà été dépossédée par le cœur, qui a
supprimé 90 % de ses fidèles ; l'audit ayant été écrit au pin
`48e4467` (avant le cœur), son hypothèse était raisonnable sur l'état
qu'il voyait. Le gain de forme (kernel fixe, GPU-prêt, compteurs
propres) reste acquis.

## Post-scriptum — deux micro-optimisations du cœur essayées et reverties

Mesurées sur eight_clusters n=1000 axial, décision par le chiffre :

1. **Budget d'atteignabilité** (borne supérieure exacte
   `count + poids de pile`, abandon dès `budget < h`) : NEUTRE —
   562,27 → 561,95 M nœuds (−0,06 %). La borne ne mord qu'en toute fin
   de descente : un survivant a déjà payé presque tout quand elle
   tombe sous $h = 8$.
2. **Ordre de visite par $P$ au milieu de boîte** (enfant le plus
   intérieur d'abord) : NÉGATIF — −3 % de nœuds mais `t_core`
   26,5 → 32,3 s (+22 %) : deux évaluations `q3_power` par nœud interne
   coûtent plus que les visites économisées ; les témoins denses se
   trouvent vite dans n'importe quel ordre.

Les deux sont reverties (le commentaire de `seed_core_kills` en garde
la trace). Leçon : la constante restante n'est pas dans la descente
individuelle mais dans l'INDÉPENDANCE des descentes par seed — les
leviers réels sont le traitement groupé par ancre (b8c4a4d § 3) et la
parallélisation (directive utilisateur).

## Conséquence pour la suite (§ « ordre utile », point 5)

`t_core = 27 s` sur `t_gen = 35 s` : le poste dominant est la descente
du cœur (562 M de nœuds, ~127 par seed). Le choix du point 5 de
l'audit tombe donc sur le **top-k sur l'arbre + traitement groupé par
ancre** (b8c4a4d § 2), pas sur le dispatch adaptatif (l'axial est déjà
à parité partout) ni sur le plan des centres (`AB_pairs` ne domine
plus : 0,7 s).
