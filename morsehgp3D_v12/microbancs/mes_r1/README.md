# Microbanc R1 : raccourci du registre R pour les classes à cellule unique, avant / après sur G4

8 octobre 2026. Sur les grandes trames, l'étage R de l'ordre 5 est le dernier maillon de la chaîne de la Session
recouverte : session M (`receipts/g4_fullm_20261008`, produit `957e9784f`, appareil, 48 fils), sur les 21 trames
`v12set` de plus de 60 000 sites, médianes : R(5) − V(5) = 11,3 ms, fin de la tour − R(5) = 2,3 ms (19,5 ms et 3,8 ms
sur la trame de 99 099 sites). R relit chaque représentant de chaque cellule retenue (`component_at` à la coupe ouverte),
trie et déduplique. Le critère prouvé par l'auditeur Codex (reçu `registre_classe_unique`) dit quand ce travail est
évitable : pour une cellule retenue `t` de `d` événements dont la classe M `z = cell_node[t]` a `q` enfants, `t` est la
seule contributrice de sa classe si et seulement si `q = d + 1`, et alors ses branches sont exactement les enfants de
`z`, déjà triés. Mesure locale sur l'arbre de `30a69104a` : 99,9 % des lignes de R(5) sont directes sur les grandes
trames (0 écart au lemme sur toutes leurs lignes).

```text
phase=exploration_v12_hors_registre
backend=cuda_g4 (catalogue) ; cpu_reference (G, T, M, V, R)
objet=full_pi0
quantification=quantized_u21_input_only
public_status=not_claimed
```

Levier **R1** (`src/tower/registry_branches.cpp`, port du patch proposé par l'auditeur sur les étapes factorisées de R) :
`plan_row` calcule pour chaque ligne la longueur de son travail, nulle pour une ligne directe ; la première passe ne
relit aucun représentant d'une ligne directe (son nombre de branches est l'arité de la classe), la seconde copie les
enfants de la classe ; `branch_nodes` n'est alloué que si une ligne générale existe ; la première admission de R passe
de `4 Q_R` à `4 Q_g` octets de travail ; `branch_reads` compte les représentants réellement relus (`Q_g`). Même objet,
même registre, mêmes branches.

[`pilote_r1.py`](pilote_r1.py) (bibliothèque standard, Python 3.10 nu, aucun `assert`, outils partagés
`microbancs/outils/banc_full.py` et `lecteur_full.py`) construit deux sondes `mhgp12_full_probe` au profil 21 avec
`MHGP12_ENABLE_CUDA=ON` : **avant** depuis l'archive épinglée de la tête de `main` juste avant R1 (`--base`,
`--avant-archive`, `--avant-sha256` : trois valeurs à repointer dans le plan ; depuis le retrait d'A6 (13:33 UTC), la
tête de `main` après ce retrait, sources de `bdfca8fb1`, plus le lot T2-d-B2 s'il est intégré avant ; le juge exige
la même base à la construction et au jugement),
**après** depuis le paquet ; **avant_bis**
est le binaire d'avant, joué comme un bras distinct (A/A). Voie par défaut de la sonde (Session recouverte, cache de
blocs de 8 Gio par défaut dans les deux bras), voie appareil, 48 fils ; chaque journal est lu par le lecteur strict
partagé (schéma `recouvert`, ordre des fins par ordre exigé).

- **identité** (avant, après) : ng00-02 à K5 et K10, ng00 à K5 à un fil, uniformes de 8 000, 16 000 et 32 000 sites,
  les 37 trames `v12set` en Session. FUL1 n'écrit pas les tableaux du registre : leur identité mot pour mot sur ces
  mêmes entrées (registre, branches, historique, événements, cibles) est établie hors chrono avant la session ;
- **grandes** (décisif) : les trames `v12set` de plus de 60 000 sites en Session, 6 tours ; dans chaque tour, un
  processus neuf par bras (ordre tourné d'un cran par tour, sans inversion), qui joue les grandes trames deux fois ;
  le second tour du processus fait foi ;
- **ng** (garde) : ng00, ng01, ng02 à K5, 5 tours × 10 passes, la première écartée ;
- **k10** (information, sans jugement) : ng00, ng01, ng02 à K10, 3 tours × 5 passes.

**Règle écrite d'avance (`REGLE_R1`, 8 octobre 2026, 12:31 UTC)** : **adopté** si toutes les empreintes FUL1 de
l'identité sont identiques, si la borne haute de l'IC 95 % (bootstrap sur les tours, 10 000 tirages, graine 20261008)
de la moyenne géométrique des rapports après / avant sur les grandes trames est sous **0,99**, et si celle de chacune de
ng00, ng01, ng02 est sous **1,02** ; **rejeté** sinon ; **refusé** si une prise manque (hors étape k10), si un binaire
change, si un journal manque ou a changé, si le GPU n'est pas vide, si l'auto-test du juge échoue, ou si la moyenne
géométrique avant_bis / avant sort de 1 ± 1,5 % sur les grandes trames ou sur l'une de ng00-02 (veto A/A). Publiés sans
jugement : par grande trame, mur, queue et R(K) − max(M(K), V(K)) par bras ; K10. **Amendements (avant toute mesure ; seuils,
statistiques et veto inchangés)** : 12:32 UTC, base `fd84039c0` (30a69104a + correctif de deux mutants du manifeste,
produit identique) ; 13:11 UTC, base `6497ed3b5` (pont de publication CST-0242, sortie identique) ; 13:23 UTC, base
en paramètre ; 13:33 UTC, A6 rejeté et retiré de `main` : R1 rebasé sur `bdfca8fb1` (+ B2 s'il est intégré avant).
13:57 UTC, base fixée par le coordinateur : `4171b2653` (A6 retiré, lot T2-d-B2 intégré, non encore adopté).
14:09 UTC, base fixée par le coordinateur : `5f8e777cf` (`4171b2653` + T1-d, catalogue en flux ; module du catalogue
seulement).

Le juge **ferme la cohorte avant toute statistique** : le plan attendu vient de la commande et du seul manifeste
`v12set` (clés d'identité, bras, trames et ordre tourné des grandes, nombres de tours et de passes, fils, K,
binaires `avant_bis` = `avant`, fermés après ng et k10), port au pilote R1 de la fermeture proposée par l'auditeur
Codex pour A6 (reçu `a6_pilote_admission`). [`test_pilote_r1.py`](test_pilote_r1.py) (147 journaux synthétiques admis
par le vrai lecteur strict, jugés avec rejeu) : nominal adopté ; identité réduite, tours manquants, ordre réduit,
A/A sur deux binaires, chronos à un fil et K10 tronqué refusés ; une prise K10 en échec sans effet sur le verdict ;
mode essai et rapport sans manifeste.

Mode `--essai` (local, voie CPU, sans CUDA, moins de prises) : verdict forcé « essai » avant les tableaux, jamais une
mesure.
