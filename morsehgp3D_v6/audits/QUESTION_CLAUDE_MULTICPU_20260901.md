# QUESTION_CLAUDE — le multi-CPU est-il améliorable ? (saturation mesurée du fold `reduce`)

Date : 1er septembre 2026. Demande de l'exploitant : investiguer
l'amélioration du parallélisme CPU avant d'engager le port GPU v6. Données :
reçu `session_g4_20260901_d98f47296d67_1788245493` (48 vCPU = 24 cœurs × 2
SMT, un seul nœud NUMA).

## Le diagnostic chiffré (uniform, cumuls par étage, médianes ABBA)

n=16000, t=1 → t=48 (mur 184,1 s → 14,16 s, ×13,0) :

| étage | t=1 | t=48 | speedup | part du mur à 48 |
|---|---|---|---|---|
| gen (wspd+rects) | 92,0 s | 3,82 s | **×24,1** | 27 % |
| rle (tri+dédup) | 1,34 s | 0,59 s | ×2,3 | 4 % |
| préfiltre | 25,1 s | 2,01 s | ×12,5 | 14 % |
| census | 24,5 s | 2,02 s | ×12,2 | 14 % |
| expansion | 1,98 s | 0,55 s | ×3,6 | 4 % |
| fold (mur, inflight=2) | 40,9 s | **5,63 s** | ×7,3 | **40 %** |

Dedans, le cumul du fold : tri ×21,9, intern ×11,7, fusion ×10,9 — mais
**`reduce` : 6,58 s à t=1 → 7,62 s à t=48 (×0,86, ANTI-SCALE)**.

n=50000, t=8 → t=48 (mur 133,9 → 49,0 s) : `reduce` **24,7 → 27,9 s**
(pendant que gen passe de 45,7 à 12,8 s) ; le fold-mur fait **20,4 s = 42 %
du mur** à 48 fils. Le pic en vol mesuré reste 2 (= inflight).

## Lecture

La fraction série d'Amdahl globale (4,4-5,7 %) est presque entièrement la
réduction de l'étage B : elle est aujourd'hui le plafond du multi-CPU. Un
`reduce` parallélisé à ×10 pousserait le mur 50k de 49 s vers ~30 s et le
speedup global de ×13 vers ~×20-24 ; le gain vaut aussi pour toute campagne
d'échelle (et le régime K=5, où le fold pèse relativement plus).

## Questions aux auditeurs (verrous mathématiques et d'implémentation)

1. **Que contient exactement `reduce` (étage B)** au sens du § 9.1 — la
   réduction union-find/arbre de fusion par K est-elle réductible en
   SEGMENTS à ordre de fusion FIXE (déterminisme bit-identique exigé par la
   doctrine) ? Une réduction par blocs avec table de raccord (les unions
   inter-blocs rejouées dans l'ordre canonique) préserve-t-elle le théorème
   et le digest ?
2. **Pourquoi `reduce` CROÎT-il avec T** alors que son travail est constant
   — contention (atomiques, faux partage, allocateur) ou attente mesurée
   dans le cumul ? Un profil ciblé (perf) est-il recevable comme mesure
   locale non-reçu ?
3. **`fold_inflight` > 2** : pipeliner 4-8 ordres K recouvrirait les
   réductions sérielles par les étages A parallèles — au coût mémoire
   (inflight+2) × événements, DÉSORMAIS gardé par le budget partiel. Y
   voyez-vous un obstacle de preuve (l'ordre des callbacks K reste
   strictement croissant) ?
4. **rle ×2,3** : la fusion du tri parallèle est-elle multiway-parallélisable
   sous bit-identité (fusion à plan de fusion fixe) ?
5. Priorité relative : l'exploitant enchaîne sur le PORT GPU v6 — le
   `reduce` restera CPU dans G0-G2. Faut-il fermer le multi-CPU d'abord (le
   GPU des lanes ne touche pas ce mur), ou les deux chantiers sont-ils
   indépendants à vos yeux ?

Mes propositions par défaut si vous n'y opposez pas de verrou : (a) profil
perf local du reduce pour trancher contention vs sérialité ; (b) prototype
de réduction segmentée à ordre canonique derrière une porte d'équivalence
digest-à-digest + mutant d'ordre ; (c) essai `--fold-inflight=8` mesuré en
local (jamais un claim), budget partiel actif.
