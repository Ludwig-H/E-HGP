# Microbanc T2-d-A6 : chaîne de l'ordre K de la Session recouverte, avant / après sur G4

8 octobre 2026. Sur les grandes trames, la queue de la Session recouverte est faite du noyau de l'ordre 5 puis de son
historique et de son registre. Session M (`receipts/g4_fullm_20261008`, produit `957e9784f`, appareil, 48 fils) : sur
`kitti_ng_08_002119` (99 099 sites, mur médian 319,8 ms, queue 79,1 ms), le G de l'ordre 5 finit vers 101 ms, son
noyau vers 199 ms, son registre vers 228 ms et la tour vers 232 ms, quand le dernier calcul de G finit vers 156 ms.
Le diagnostic local (`CONCEPTION_A6.md` du chantier) montre que le noyau lui-même suit le nombre de représentants ; ce
qui retarde la chaîne est la numérotation de l'ordre (cohortes de même rang, superlinéaire), qui retient le départ du
noyau et le prive des feuilles calculées par G, puis l'historique séquentiel. Trois leviers, même objet :

- **N** : numérotation par morceaux de cohortes (`number_births_range`) ;
- **I** : indices de racine calculés en avance du noyau par des tâches d'aide (`hint_leaves`) ; le noyau, inchangé, lit
  des feuilles remplacées par leur racine dans un état antérieur de l'union-find, ce qui ne change aucune de ses
  décisions ;
- **H** : historique par morceaux (contrôle, profondeur par remontée des attaches), puis la CSR par survivant.

```text
phase=exploration_v12_hors_registre
backend=cuda_g4 (catalogue) ; cpu_reference (G, T, M, V, R)
objet=full_pi0
quantification=quantized_u21_input_only
public_status=not_claimed
```

[`pilote_t2d_a6.py`](pilote_t2d_a6.py) (bibliothèque standard, Python 3.10 nu, aucun `assert`, outils partagés
`microbancs/outils/banc_full.py` et `lecteur_full.py`) construit deux sondes `mhgp12_full_probe` au profil 21 avec
`MHGP12_ENABLE_CUDA=ON` : **avant** depuis l'archive épinglée de `bdfca8fb1`, **après** depuis le paquet (même base
et lot A6) ; **avant_bis** est le binaire d'avant, joué comme un bras distinct (A/A). Voie par défaut de la sonde
(Session recouverte, cache de blocs de 8 Gio par défaut depuis `72f622a55`, donc dans les deux bras), voie appareil,
48 fils ; chaque journal est lu par le lecteur strict partagé (schéma `recouvert`, ordre des fins par ordre exigé :
ouverture ≤ G ≤ noyau ≤ M ≤ R, V(k) ≥ M(k) et M(k−1), maximum des G égal à la fin de G ; A6 garde ces relations).

- **identité** (avant, après) : ng00-02 à K5 et K10, ng00 à K5 à un fil, uniformes de 8 000, 16 000 et 32 000 sites,
  les 37 trames `v12set` en Session ;
- **grandes** (décisif) : les trames `v12set` de plus de 60 000 sites en Session, 6 tours ; dans chaque tour, un
  processus neuf par bras (ordre tourné d'un cran par tour, sans inversion), qui joue les grandes trames deux fois ;
  le second tour du processus fait foi ;
- **ng** (garde) : ng00, ng01, ng02 à K5, 5 tours × 10 passes, la première écartée.

**Règle écrite d'avance (`REGLE_T2D_A6`, 8 octobre 2026, 09:11 UTC)** : **adopté** si toutes les empreintes FUL1 de
l'identité sont identiques, si la borne haute de l'IC 95 % (bootstrap sur les tours, 10 000 tirages, graine 20261008) de
la moyenne géométrique des rapports après / avant sur les grandes trames est sous **0,97**, et si celle de chacune de
ng00, ng01, ng02 est sous **1,02** ; **rejeté** sinon ; **refusé** si une prise manque (processus en échec ou journal
refusé par le lecteur strict), si un binaire change, si un journal manque ou a changé, si le GPU n'est pas vide, si
l'auto-test du juge échoue, ou si la moyenne géométrique avant_bis / avant sort de 1 ± 1,5 % sur les grandes trames ou
sur l'une de ng00-02 (veto A/A). Publiés sans jugement : par grande trame, mur, queue et retard du noyau de l'ordre K
sur la fin de G de cet ordre, par bras. **Amendements (8 octobre 2026, avant toute mesure G4 de ces bras ; seuils,
statistiques et veto inchangés)** : 09:55 UTC, base `72f622a55` au lieu de `86d7e39d8` et cache de blocs de 8 Gio
dans les deux bras ; 10:30 UTC, base `bdfca8fb1` (porte native de terminaison intégrée, lecteur partagé fermé).

Mode `--essai` (local, voie CPU, sans CUDA, moins de prises) : verdict forcé « essai », jamais une mesure.
