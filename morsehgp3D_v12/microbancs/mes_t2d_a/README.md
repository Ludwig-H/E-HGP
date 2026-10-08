# Microbanc T2-d-A : Session recouverte (décision D-F2), avant / après sur G4

8 octobre 2026. Mesure de la tranche T2-d-A : l'étage G et les étages T, M, V, R de la tour dans **une seule région du
Pool** (`build_tower`, `src/tower/pipeline.hpp`), contre la base (`main` à `902041f66` : `resolve_tower` puis
`build_forests` ; base reportée de `8dc5d6b16` à `902041f66` à la reprise du 8 octobre, même produit que `8da450ab7`
plus la mémoire par étage de la sonde, règle inchangée). Même sortie : vidage FUL1 identique à l'octet ; seul le mur
change.

```text
phase=exploration_v12_hors_registre
backend=cuda_g4 (catalogue) ; cpu_reference (G, T, M, V, R)
objet=full_pi0
quantification=quantized_u21_input_only
public_status=not_claimed
```

[`pilote_t2d_a.py`](pilote_t2d_a.py) (bibliothèque standard, Python 3.10 nu, aucun `assert`) construit deux sondes
`mhgp12_full_probe` au profil 21 avec `MHGP12_ENABLE_CUDA=ON` : **avant** depuis l'archive épinglée des sources de la
base (empreinte SHA-256 vérifiée avant déballage), **après** depuis le paquet, jouée avec `--recouvert` (sans cette
option, la sonde garde la voie séquentielle et le schéma de `902041f66`, que lisent strictement `MES-FULL` et `MES-B`).
Journaux de construction, empreintes des binaires et extrait du `CMakeCache` de chaque bras sont archivés. Puis :

- **identité** : processus `--digest` (empreinte FUL1 hors du mur) par bras sur ng00–02 à K5 (deux passes) et à K10,
  sur ng00 à K5 à **un fil** (déterminisme 1 contre 48 fils), sur les uniformes de 8 000, 16 000 et 32 000 sites à K5,
  et sur les 37 trames `v12set` (une passe par trame) ; en plus, ng00–02 à K5 par la sonde après **sans**
  `--recouvert` (voie séquentielle refondue) ;
- **campagne** : ng00–02 à K5, 48 fils, voie appareil, sans empreinte (le coût de l'empreinte ne pèse pas sur la durée
  de la session) ; 5 tours ; dans chaque tour, trames en ordre tournant et, pour chacune, un processus neuf par bras,
  ordre des bras alterné d'un tour à l'autre ; 10 passes par processus, la première à froid écartée ;
- **session** : les 37 trames `v12set` enchaînées dans une Session, K5, voie appareil, 3 processus par bras, deux tours
  (le second fait foi) ; médiane et maximum publiés, non jugés.

**Règle écrite d'avance (`REGLE_T2D_A`)** : par processus, mur FULL = médiane des passes 2 à 10 ; par tour et par
trame, rapport après / avant ; moyenne géométrique et IC 95 % par bootstrap sur les tours (10 000 tirages, graine
20261008). **Adopté** si toutes les empreintes FUL1 des processus d'identité sont identiques d'une prise et d'une passe
à l'autre **et** si la borne haute de l'IC est sous 1 sur chacune de ng00, ng01, ng02 ; **rejeté** sinon ; **refusé** si
une prise manque, si un binaire change, si un journal manque ou a changé, si le GPU n'est pas isolé, ou si l'auto-test
du juge échoue (cinq cas synthétiques : adopté, rejeté par une trame, rejeté par une empreinte, refusé, rejeté par le
bruit). Le juge relit chaque journal brut (empreinte du fichier, admission stricte, résumé recalculé).

**Schéma recouvert** (lignes `etapes_schema` = `"recouvert"`, exigé du bras après et refusé du bras avant ; en-tête de
`bench/full_probe.cpp`) : `etapes_ns` ne garde que la **partition murale** — P, C, G (du début de `build_tower` à la fin
du **dernier calcul** de G ; la pré-passe des feuilles qui le suit est un travail de la forêt), raccord (nul), TMVR (la
**queue**, intervalle mural de cette fin à la tour complète : forêt non recouverte et clôture) —, de somme au plus le
mur ; `fenetres_ns` porte les **sommes de fenêtres murales des tâches** (G, forêt, forêt après la fin de G, T, M, V, R :
temps-fils, ni murs ni temps CPU, elles peuvent dépasser la queue comme le mur) ; `recouvrement` porte les instants et
les comptes (ouverture, fin de G, fin, queue, durée de l'appel vue de la sonde, reprises et arrêts du noyau, octets
admis) ; `fins_par_ordre_ns` les fins par ordre (dernier calcul de G, noyau, M, V, R) ; `memoire_octets` = P, C, tour.
Les gardes séquentielles des lecteurs de la session K et de `MES-B` (`T + M + V + R <= TMVR`,
`tables + resolution <= G`) ne s'appliquent pas à ce schéma.

Publié en plus, sans jugement, pour les deux bras : temps CPU du processus pendant le mur (`cpu_ns` de la sonde, tous
les fils ; les fils sans travail de la Session patientent par pauses, cessions puis sommeils de 20 µs, et ce temps y
figure) et pic du budget de la Session.

Mode `--essai` (local, voie CPU, sans CUDA, moins de prises, deux trames `v12set`) : verdict forcé « essai », jamais
une mesure.
