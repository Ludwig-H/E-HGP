# Microbanc T2-d-A6b : la chaîne de l'ordre K sans pénaliser G, avant / après sur G4

8 octobre 2026. A6 (numérotation par morceaux N, indices de racine I, historique par morceaux H) a été **rejeté** sur
G4 (session `t2da6b`, reçu `receipts/g4_t2da6_20261008`) : grandes trames 0,914 (IC 0,910–0,917) mais ng00 1,033 et ng01
1,043. Les journaux rapatriés montrent pourquoi : G fait le même travail (temps-fils de G inchangé) mais finit 1,5 à
5 ms plus tard sur ng00-02 et 13 ms plus tard sur les grandes trames, parce que les tâches d'indices (≈ +100 ms-fils
par passe sur ng00) passaient avant les étapes et les tranches de G. **A6b** garde N, I, H et le pont de publication
CST-0242, et change seulement l'ordre de réclamation des travaux : noyau, étapes, tranches de G, puis seulement tâches
d'indices. Une aide ne prend donc jamais un fil tant qu'une tranche de G reste à réclamer (porte
`mhgp12_tower_levers_priorite` : compteur `hint_jobs_during_g` nul ; mutant `aide_avant_g`, l'ordre d'A6, tué). Sorties
identiques quel que soit l'ordre (feuilles indicées : même preuve qu'A6). Deux portes de plus reprennent les résidus de
CST-0242 : `mhgp12_tower_levers_tranches`, la fixture de l'auditeur Codex (`a6_prefixe_tranches` : 513 cellules aux
tranches réelles du produit, historique du noyau contre un oracle par ensembles sous 83 dates légales d'indice ;
l'indice futur que le pont de publication exclut donne un historique faux, détecté, ou un refus) ;
`mhgp12_tower_region_fermeture`, l'entrelacement aide / clôture du noyau forcé dans le vrai corps (une tâche d'indices
en cours fait attendre la clôture ; une tâche jouée après la levée de `hint_closed` ne touche plus aux tampons ;
mutants `fermeture_sans_attente` et `aide_sans_garde`, manifeste de la tour à 66).

```text
phase=exploration_v12_hors_registre
backend=cuda_g4 (catalogue) ; cpu_reference (G, T, M, V, R)
objet=full_pi0
quantification=quantized_u21_input_only
public_status=not_claimed
```

[`pilote_t2d_a6b.py`](pilote_t2d_a6b.py) reprend le pilote d'A6 tel qu'intégré sur `main` (cohorte fermée contre la
commande et le manifeste avant toute statistique, fermeture de l'auditeur) : **avant** depuis l'archive épinglée de
`47feedc96`, **après** depuis le paquet, **avant_bis** = le binaire d'avant joué comme un bras distinct (A/A) ; voie par
défaut de la sonde (Session recouverte, cache de blocs de 8 Gio dans les deux bras), voie appareil, 48 fils ; identité
FUL1 (ng00-02 K5 et K10, ng00 à un fil, uniformes, 37 trames `v12set`), grandes trames (21, 6 tours), ng00-02 (5 tours ×
10 passes).

**Règle écrite d'avance (`REGLE_T2D_A6B`, 8 octobre 2026, 14:47 UTC)** : **adopté** si toutes les empreintes FUL1 de
l'identité sont identiques, si la borne haute de l'IC 95 % (bootstrap sur les tours, 10 000 tirages, graine 20261008) de
la moyenne géométrique des rapports après / avant sur les grandes trames est sous **0,95** (garder l'essentiel du gain
mesuré pour A6, 0,914) et si celle de chacune de ng00, ng01, ng02 est sous **1,01** (plus strict que les 1,02 d'A6 :
l'A/A de `t2da6b` vaut 0,998–0,999 avec des demi-largeurs d'au plus 0,004, une perte de 1 % est donc résoluble) ;
**rejeté** sinon ; **refusé** si une prise manque, si un binaire change, si un journal manque ou a changé, si le GPU
n'est pas vide, si l'auto-test du juge échoue, si la cohorte jugée n'est pas celle de la commande, ou si la moyenne
géométrique avant_bis / avant sort de 1 ± 1,5 % sur les grandes trames ou sur l'une de ng00-02 (veto A/A).

Mode `--essai` (local, voie CPU, sans CUDA, moins de prises) : verdict forcé « essai », jamais une mesure.
