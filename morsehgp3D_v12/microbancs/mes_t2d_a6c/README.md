# Microbanc T2-d-A6c : la chaîne de l'ordre K seulement là où le noyau K de la base finit après G

8 octobre 2026. A6b a été **rejeté** sur G4 (session `a6b2`, reçu `receipts/g4_a6b_20261008`) : grandes trames 0,849,
ng02 0,956, mais ng00 1,009 et ng01 1,013 au-dessus de la garde de 1,01. Les journaux des deux mesures (`a6b`, `a6b2`)
disent pourquoi : sur ng00 et ng01, G finit environ 1 ms plus tard (le travail de la forêt pendant G augmente de
+47 et +34 ms-fils) parce que N ouvre tôt le noyau de l'ordre 5, dont la contraction, l'historique et le registre
tombent alors pendant G, en priorité sur ses tranches ; or sur ces trames le noyau 5 de la base finissait déjà **avant**
la fin de G (de 7,0 et 5,9 ms) : l'avancer ne rapporte rien. Sur les 37 trames `v12set` (ng00-02 comprises), le gain
d'A6b suit le **retard du noyau de l'ordre K de la base sur la fin de G** : retard négatif, 1,007 à 1,018 ; retard de
0 à 1,4 ms, 0,985 à 0,997 ; retard de 3,6 à 14 ms, 0,89 à 0,955 ; grandes trames (14 à 44 ms), 0,82 à 0,90.

**A6c** garde A6b tel quel (N, I, H, pont de publication CST-0242, aides après G) mais ne l'engage que si ce retard est
prédit positif, par une décision déterministe prise à l'admission sur le seul nombre de sites, connu avant la région
(`chain_engaged`, seuil `kChainSites` = 43 900, `src/tower/pipeline.hpp`) ; en dessous, chaque ordre suit le chemin
exact de la base (numérotation `number_order` et ouverture du noyau dans une étape, `build_history` d'un seul tenant,
aucune tâche d'indices). Sorties identiques dans les deux chemins (porte `mhgp12_tower_levers_bascule`).

```text
phase=exploration_v12_hors_registre
backend=cuda_g4 (catalogue) ; cpu_reference (G, T, M, V, R)
objet=full_pi0
quantification=quantized_u21_input_only
public_status=not_claimed
```

## Le critère, établi avant toute mesure d'A6c

[`critere_a6c.json`](critere_a6c.json) : pour chaque trame, le retard réel du noyau 5 de la base sur la fin de G (bras
avant des sessions `a6b` et `a6b2`, médianes), le retard prédit en validation croisée laisser-un-dehors, la décision et
le rapport A6b / base mesuré. Modèle : retard = −32,53 + 0,8093 × sites / 1000 ms (erreur quadratique 2,84 ms, 3,02 ms
en validation croisée ; un modèle à deux comptes, représentants de l'ordre 5 et cellules de tous les ordres, fait 2,35 ms
mais avec un coefficient négatif sur les représentants, sans lecture physique : non retenu). Seuil : retard prédit de
3 ms, soit 43 900 sites. Sur les 37 trames, aucune trame à retard réel négatif n'est engagée (la plus proche :
kitti_ng_08_000246, 41 691 sites, retard réel −4,6 ms, prédit 1,6), aucune trame à retard réel de plus de 3 ms ne l'est
pas (la plus proche : ng02, 45 845 sites, retard 3,6 ms, engagée). Les deux trames à petit retard réel positif
(37 543 et 38 341 sites, 0,5 et 1,4 ms) restent au chemin de la base : A6b n'y gagnait que 0,3 et 1,5 %. Le seuil est
calibré pour G4 à 48 fils ; la décision ne change que la performance.

## Pilote et règle

[`pilote_t2d_a6c.py`](pilote_t2d_a6c.py) est le pilote fermé d'A6b (cohorte fermée contre la commande et le manifeste
avant toute statistique) renommé : **avant** depuis l'archive épinglée de `8a0716e74` (A6b retiré, `src/` identique à
`47feedc96`), **après** depuis le paquet, **avant_bis** = le binaire d'avant joué comme un bras distinct (A/A) ; voie
par défaut de la sonde (Session recouverte, cache de blocs de 8 Gio dans les deux bras), voie appareil, 48 fils ;
identité FUL1 (ng00-02 K5 et K10, ng00 à un fil, uniformes, 37 trames `v12set`), grandes trames (21, 6 tours),
ng00-02 (5 tours × 10 passes). Il publie en plus, sans jugement, la décision de la chaîne par trame.

**Règle écrite d'avance (`REGLE_T2D_A6C`, 8 octobre 2026, 18:18 UTC)**, seuils de `REGLE_T2D_A6B` ni relâchés ni
resserrés : **adopté** si toutes les empreintes FUL1 de l'identité sont identiques, si la borne haute de l'IC 95 %
(bootstrap sur les tours, 10 000 tirages, graine 20261008) de la moyenne géométrique des rapports après / avant sur
les grandes trames est sous **0,95** et si celle de chacune de ng00, ng01, ng02 est sous **1,01** ; **rejeté** sinon ;
**refusé** si une prise manque, si un binaire change, si un journal manque ou a changé, si le GPU n'est pas vide, si
l'auto-test du juge échoue, si la cohorte jugée n'est pas celle de la commande, ou si la moyenne géométrique
avant_bis / avant sort de 1 ± 1,5 % sur les grandes trames ou sur l'une de ng00-02 (veto A/A).

Attendus, écrits avant la mesure : grandes trames ≈ 0,85 (les 21 sont engagées et suivent le chemin d'A6b, 0,849 dans
`a6b2`) ; ng00 et ng01 ≈ 1,00 (39 885 et 35 551 sites : chemin de la base) ; ng02 ≈ 0,956 (45 845 sites : engagée).

Mode `--essai` (local, voie CPU, sans CUDA, moins de prises) : verdict forcé « essai », jamais une mesure.
