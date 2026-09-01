# Analyse — matrice locale fils × inflight × join (diagnostic NON décisionnel)

Reçu scellé au commit `1069bc20` (binaires `mhgp6` référence et
`mhgp6_profile` attribution, 8 vCPU locaux). STATUT : **directionnel
seulement** — la charge concurrente de la session (builds, portes CTest de
l'auditeur et les miennes) a co-tourné avec plusieurs cellules ; l'auditeur a
explicitement requalifié `ref_uniform16000_t8_i2_j0` comme contaminée et
relevé que les cellules t1 i1/i2 font varier des étages antérieurs au fold de
33-37 % (bruit machine ≈ 2× vs la campagne G4 propre du matin : 16000 t8 ici
71 s contre ~35 s sur VM chargée normalement). **Cette matrice ne décide pas
le design A** ; la matrice décisionnelle est celle de la session G4 (48 vCPU
au repos). Les lectures ci-dessous n'engagent que les CONTRASTES intra-reçu.

## Lectures directionnelles (arbre de décision pré-enregistré § 5.10)

1. **join=1 DÉGRADE le mur** partout où il est mesuré : 16000 t8 80,5 s vs
   71,0 s (j0) ; 50000 t8 281,7 s vs 258,6 s ; fold-mur 25,9 vs 17,0 s et
   91,3 vs 62,5 s. Le recouvrement A/réduction est NET-POSITIF — la branche
   « borner la concurrence A/B » de l'arbre n'est PAS indiquée.
2. **Attribution du B isolé (prof, 50k t8 j1, cumuls sur K ; fenêtres
   internes de reduce_fold, jamais un mur)** :
   `materialisation_tri_copie = 14,9 s` (37 % de 39,7 s de fenêtres) >
   `pre = 7,0` > `post_remplissage = 6,8` > `touch = 5,1` >
   `partition = 3,4` > **`unite = 1,7 s (4 %)`**. Le cœur DSU (unions) est
   minuscule ; la masse est dans le remplissage/tri/copie des deltas et les
   passes de rôles — exactement ce que le palier `CompactDelta` puis l'amont
   précalculé du design A visent. **Branche indiquée : CompactDelta.**
3. **Digest forêt : 16,2 s cumulées à 50k** (hors fenêtres, dans le cycle de
   vie du worker B) — un consommateur discret du même ordre que la
   matérialisation ; à mesurer séparément dans la matrice G4 (runs avec et
   sans `--digest`).
4. inflight 2 vs 1 (t8 j0) : 71,0 vs 73,3 s (16000) — effet faible dans le
   bruit ; t4 : 134,8 vs 144,3 s. Rien n'indique un gain à inflight > 2 sur
   8 vCPU.

## Ce que la session G4 devra trancher (matrice propre)

T ∈ {16, 24, 32, 48} × inflight {1, 2, 4} × join {0, 1}, cœurs physiques
épinglés vs SMT, runs avec/sans digest, binaire de référence NON instrumenté
pour tout mur, attribution par `mhgp6_profile` sur les points clefs —
protocole § 5.10, arbre de décision pré-enregistré inchangé.
