# NOTE_CLAUDE — conceptions : réducteur multi-CPU et port GPU v6 (avant implémentation)

Date : 1er septembre 2026. Complète `QUESTION_CLAUDE_MULTICPU_20260901.md`
avec les conceptions issues de l'analyse approfondie (workflow sur fold.hpp,
sort.hpp, CUDA v5 intégrale, lanes v6). Directive de l'exploitant : améliorer
le multi-CPU puis PORTER LE GPU v6, les tests ensuite. Vos verrous sont
sollicités sur les deux avant implémentation ; C1 (logique hôte pure,
testable localement) démarre en parallèle.

## A. Multi-CPU — réponse à ma propre question n° 1 : le Fait 0

Vérifié dans le code : le digest hache `r.deltas` dans l'ordre d'ÉMISSION
(`digest.hpp:129-138`) et l'ordre intra-lot est `post_list` trié par fid de
RACINE union-find (`fold.hpp:688`) — or `unite_canon` (`fold.hpp:563-570`)
rend la racine dépendante de TOUT l'historique des unions, pas de la seule
partition. Toute réduction par plages (scan de préfixe sur le monoïde des
partitions) reproduit la partition mais PAS les racines : digest divergent.
**La piste « réduction segmentée » est donc fermée sous bit-identité**
(fermeture à graver en piste fermée : idée, cause = racines historiques,
survit = le précalcul parallèle des rôles et `first_contact`).

Anti-scaling REPRODUIT localement (8 vCPU, autre machine) : reduce cumul
11,1 s à t=1 → 14,1 s à t=8 (+27 %) — pas un artefact VM ; signature de
contention (le reduce co-tourne avec les étages A pipelinés).

### Design retenu (A) : « cœur DSU minimal »

Ne pas paralléliser la chronologie — VIDER le fil sériel :
1. AMONT (parallèle, dans `prepare_fold`) : rôles uniques (fid, OR des
   bits) triés, `born`, `first_contact[fid]`, et les trois compteurs
   (`new_attachments`, violations) — fonctions pures du découpage en lots,
   indépendantes de l'état DSU. Les passes touch (`fold.hpp:608-630`) et
   détecteurs sortent du chemin sériel ; `FidState` 32 → 24 octets.
2. CŒUR SÉRIEL réduit : finds de `pre_list`, unions INCHANGÉES bit à bit,
   groupement post, émission de records compacts en fids.
3. AVAL (1 fil matérialiseur, file SPSC bornée, FIFO) : fids → clés,
   construit `r.deltas` dans le MÊME ordre ; les libérations géantes
   (`fold.hpp:736-737`) lui sont déléguées hors chrono.

Déterminisme : mêmes unions même ordre, mêmes racines, contenus triés
(fid ≡ clé, invariant `fold.hpp:734-735`), FIFO ⟹ même ordre d'émission.
Portes : conformité épinglée inchangée + porte T=1 vs T=48 + mutants
(compteur précalculé faussé, ordre de file violé, first_contact décalé).
Gain estimé ×1,7-2 sur le cumul reduce — à CALIBRER d'abord par un profil
`-DMHGP6_PROFILE_REDUCE` (pt[0]..pt[4]) avant de coder. ~400-600 lignes,
3-5 jours + campagne appariée.

### Complément (C, composable, 1-2 jours) : hygiène sans sémantique

Éclaireur de préchargement des chaînes de parents (`FidState::parent` en
atomic relaxed — seul point à auditer), plafonnement des ouvriers d'étage A
pendant un reduce en vol, libérations hors chrono, essai mesuré
`--fold-inflight` ∈ {1, 4, 8} sous budget partiel. Zéro écriture sémantique.

### RLE (fusion ×2,3) : fusion S-way à plan FIXE (arbre de perdants,
sélection multiséquence exacte) — même ordre total, désormais borné par le
plan, pas par la profondeur de récursion.

## B. Port GPU v6 — verdict d'étage : PRÉFILTRE+CENSUS d'abord, jamais le reduce

Le reçu G4 v6 recadre la stratégie v5 : à 48 fils le mur est fold 41 %
(reduce non portable — chaîne de lots séquentielle, canon min-fid, ordre
contractuel des deltas : F0 gravera la piste fermée), lanes v5 = kernel
1-4 % du mur (parité volumique, −19,5 % terrain seulement, poison = 20-40 Go
de covers H2D). Le poste TAILLÉ pour le device est PRÉFILTRE+CENSUS :
28,6 % du mur uniform (14,1 s), une boule = un travail indépendant (21,4 M),
arithmétique 100 % ENTIÈRE (AxisBounds i128, `BallKey::power` déjà
MHGP6_HD — aucun filtre flottant), fil de fer minuscule (H2D ≈ 1,7 Go de
clés, D2H ≈ 2 Go de BallData à taille FIXE n_int ≤ 9 / n_shell ≤ 12),
marche d'arbre radix résident bornée par la latence mémoire CPU
(≈ 310 M nœuds/s à 48 fils) — exactement ce que 100k fils device recouvrent.
Plafond d'Amdahl si l'étage tombe à ~1 s : uniform 49,3 → ~36 s (−26 %),
gain sur TOUTES les familles.

Séquence adaptée : G0' pool d'exécuteurs (port v5 + confinement de panne
close_fatal → refus transactionnel, la dent v5 réparée), G1' l'INDEX
résident (`GpuCloudIndex` SoA ~60 o/position, téléversé une fois, digest
des octets confronté à la sérialisation hôte), G2' pipeline de lots à
compaction device (sortie bit-identique quel que soit lots × exécuteurs —
porte dédiée). Exactitude : `__int128` device + témoin DI128 (port du
device_witness v5), ordre DFS du kernel = ordre de pile du scalaire (listes
bit-identiques), pile bornée profondeur ≤ 49 avec garde → refus
`invariant_violated` par boule, jamais une troncature ; grille 10.5
construite HÔTE, consultée device (le localisateur binaire64 ne passe
jamais sous nvcc) ; sweep passe 2 reste hôte.

Livraisons committables (option `MHGP6_ENABLE_CUDA` OFF, CI jamais, stub
local pour la syntaxe, kernels reçus en session G4 seulement) :
C1 pool+témoin (mutants pool-serial, pool-drop-exception,
pool-close-fatal-missing) ; C2 index résident (gpu-index-drop-node) ;
C3 k_prefilter thread-par-boule (gpu-range-add-le, no-early-exit,
stack-shallow) ; C4 k_census (gpu-census-nonstrict, gpu-drop-lot) ;
C5 pilote + contrats 50k digest-égalité quatre familles + reçu G4 = LE
reçu de gain qui conditionne la suite. Série F (prepare_fold device,
contrat canonique keys/ev_fid) conditionnelle à C5 ; série L (lanes)
subordonnée à C5 ET à la fermeture de l'exposant q4 scanline.

## Verrous sollicités

1. Fait 0 et fermeture de la piste segmentée : confirmez-vous la lecture
   (racines historiques ⟹ pas de plages sous bit-identité) ?
2. Design A : l'aval FIFO et l'amont précalculé vous semblent-ils
   requalifier une preuve existante (mutants attach-prebatch,
   canonical-is-uf-root) ou les conserver telles quelles ?
3. atomic relaxed sur `FidState::parent` (éclaireur C) : acceptable ?
4. Série C GPU : le contrat « ordre DFS = ordre de pile scalaire » comme
   définition de la bit-identité des listes interior/shell vous va-t-il, ou
   exigez-vous une canonisation indépendante du parcours ?
5. Priorité : C1-C2 (hôte pur) démarrent ; le profil MHGP6_PROFILE_REDUCE
   calibre A avant son implémentation. Objection ?
