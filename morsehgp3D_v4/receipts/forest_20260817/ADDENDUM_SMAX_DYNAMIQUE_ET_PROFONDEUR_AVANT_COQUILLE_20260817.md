# Addendum — smax dynamique dans l'aval + profondeur avant coquille

Date : 17 août 2026. Exécution conjointe des deux audits ciblés
`AUDIT_CIBLE_E7E4D5_SMAX_DYNAMIQUE_DANS_LE_FOLD` et
`AUDIT_CIBLE_E7E4D5_DEPTH_PREFLIGHT_AVANT_SHELL` (leur volet deux-passes
était déjà couvert par l'exécution de l'audit `ec683b` ; restaient le
paramètre dynamique et le point transactionnel).

## 1. Le paramètre amont existe en aval (`kmax_eff`)

L'aval revenait silencieusement à `K_max=10` (census cap 9, expansion
11, dix forêts, totaux sur dix) alors que les filtres WSPD étaient
calibrés par `smax_eff` : `--smax=6` filtrait une hiérarchie et en
repliait une autre — les ordres `K > smax_eff−1` étaient des sous-flux
accidentels, jamais garantis complets. Corrigé partout
(`forest_probe.cpp`) :

- passe 1 : mort à `|I_B| >= smax_eff + 1 − q_min` ;
- passe 2 : `interior_cap = smax_eff − q_min` (au profil K_max=5 :
  caps 4/3/2 — inutile de chercher un neuvième intérieur quand le
  troisième déclasse déjà la boule) ;
- expansion `smax_eff`, folds et totaux `1..kmax_eff`, juge aligné
  (plafond `smax_caps − 2`) ; les tableaux statiques de taille 11
  restent, seule la tranche `1..kmax_eff` est publiée.

**Fixture de frontière gravée** (`--kmax-gate`, posée sur
`BallData -> expansion -> fold` sans dépendre d'un certificat WSPD) :
les sept points de l'audit — boule diamétrale de `ab` (R²=100) à CINQ
intérieurs, événement exactement d'ordre `K=6`. `smax=6` : la boule
meurt au préfiltre (5 >= 5), `balls_dead_depth == 1`, aucune sortie
K=6 ; `smax=7` : l'événement K=6 est présent, niveau exact 100 vérifié
par `compare_exact_level`. Mutant `fold-hardcodes-kmax10` (les
constantes 9/11/10 rétablies) : K=6 apparaît sous `smax=6` — TUÉ.

**Le profil secondaire `K_max=5` devient un produit mesurable** :
`--smax=6 --judge` à n=120 — 0 désaccord de bout en bout, et les coûts
suivent enfin le profil (116 495 boules uniques au lieu de 439 283 ;
census 15 ms ; 5 105 événements). Régression du profil maximal : toutes
les portes `smax=11` existantes, inchangées (88 vertes).

## 2. Profondeur avant coquille (transactionnel)

Défaut de l'ancien census à une passe : au DOUBLE débordement
(`|I_B| > cap` ET `|U_B| > shell_cap`), le statut dépendait de l'ordre
des sous-arbres (`dead_depth` ou `resource_exhausted` selon la branche
rencontrée d'abord). La structure deux-passes le corrige par
construction : la profondeur est décidée en passe 1, la coquille n'est
matérialisée que pour une survivante.

**Fixture gravée** (depth-gate) : boule R²=50 centrée (100,100,100)
avec 10 intérieurs en Morton BAS et 15 points de coquille en Morton
HAUT (le DFS droite-d'abord du census visite les clés décroissantes :
la 13e coquille arrive avant le 10e intérieur). Verdict exigé :
`dead_depth` par la passe 1, jamais `resource_exhausted`. Mutant
`shell-cap-before-depth` (l'ancien census appelé d'abord) : rend
`resource_exhausted` à tort — TUÉ. Variante gravée : profondeur 9
(survivante) + coquille 15 > cap 12 → `resource_exhausted`, le statut
légitime.

Compteur ajouté : `prefilter_nodes` (avec `prefilter_leaf_tests`,
`prefilter_range_add_mass`, `full_census_keys` déjà publiés).

**88 portes CTest vertes.** Prochaine étape inchangée : le filtre de
candidats q4 à la génération (question aux auditeurs pendante), puis
les campagnes n = 8000/16000/32000 — désormais mesurables sur les DEUX
profils contractuels (K=10 <100 ms G4, K=5 <1 s).
