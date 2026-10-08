# MES-B : mémoire par étage et refus du mur nul

Contrelecture des commits `902041f66` et **`9feadf92725190e14bf2a56fe98c761ba5afda1a`**,
depuis les octets Git, sans modification produit. Suite de [mes_b_livraison](../mes_b_livraison/README.md),
dont le constat historique reste inchangé. La session L1, épinglée à **`403736300`**, n'hérite
ni de ce nouveau schéma ni de ces nouvelles mesures mémoire.

Le résidu `log(0)` est corrigé : le lecteur classe désormais un mur nul et un nombre de sites
nul comme `illisible`, avant toute statistique. La garde supplémentaire de `slope` rend `None`
si une abscisse ou une durée n'est pas strictement positive. Le contre-exemple synthétique
historique n'atteint donc plus B3. Aucun mur nul natif n'avait été observé.

## Qualification Python rejouée

Les portes officielles normale et `-O` rendent code 0 et exactement la même sortie :
`lecture=28 issues=7 verdicts=9 empreintes=3 etiquettes=12 pilote=2`.
Le runner des mutants est exécuté avec observation de ses journaux enfants, sans changer leurs
sources ni leurs arguments : **24 substitutions valides, 24 codes 1**. Vingt-trois rendent un
diagnostic explicite de la porte ; `pente_sans_garde` provoque le `ValueError: math domain error`
attendu. Aucun échec de syntaxe ou d'import n'explique ces morts. Les deux nouveaux mutants mémoire
s'ajoutent aux vingt anciens, puis viennent le mur nul et la garde de pente. Les quatre corruptions
de bloc mémoire ne sont donc pas quatre nouveaux mutants distincts.

La lecture préalable confirme que ces portes emploient des sondes **Python fictives** ; leur mode
essai relève seulement `cmake --version`. Aucun moteur, compilation, GPU, GCP ou payload réel.
La porte `full_probe_check.py` est lue et épinglée, mais pas exécutée : elle lancerait des moteurs.
Les heures exactes de ces nouveaux rejeux figurent dans la capture.

## Sens et limites de la mémoire publiée

`memoire_octets` exige les cinq clés P/C/G/raccord/TMVR, chacune portant deux u64 : usage du
**MemoryBudget de la Session** en fin d'étage et pic pendant l'étage. Les booléens sont exclus,
usage ≤ pic et maximum des cinq pics = `pic_octets` sont vérifiés. Ce n'est ni un RSS par étage
ni une mesure du pic VRAM physique. Les entrées résidentes participent au budget.

Dans FULL, `open(budget)` partage le budget hôte/appareil et publie `pic_appareil_octets=0`.
Avec `--budget-appareil`, le pic appareil est séparé. **MES-B exige cette seconde forme** sur la
voie appareil ; sa voie CPU exige les trois champs appareil/épinglé/pic appareil nuls. Les durées
des étages restent celles des fenêtres existantes ; les marques mémoire intermédiaires restent
dans le mur englobant, la marque finale après son arrêt. Aucun impact temporel n'est mesuré ici.

## Trois cohérences encore non vérifiées

La fixture officielle sert de JSON conforme au lecteur, pas de sortie native géométriquement
qualifiée. Chaque mutation isolée ci-dessous reste admise `ok` au pin livré :

| Mutation | Invariant justifié par les sources |
| --- | --- |
| capacité appareil 7, pic appareil 6 | Avec budget séparé, le pic couvre les réservations des tableaux encore résidents. |
| épinglé 11, pic Session 10 | `stage` réserve le transit dans le budget hôte avant allocation ; il reste résident en fin de C. |
| usage fin C = 4, pic G = 3 | `restart_peak()` initialise le pic suivant à l'usage courant. |

Références épinglées : `device_cuda.cu` réserve avant `cudaMalloc`/`cudaMallocHost`, puis publie
les capacités ; `buffer.hpp::restart_peak` échange le pic avec `used()` entre tâches ; les marques
de `full_probe.cpp` suivent P/C/G/raccord/TMVR. La dernière inégalité n'impose **pas** que l'usage
reste croissant : des libérations peuvent réduire l'usage final sans effacer le pic initial.
Un témoin avec usage C=4 puis usage G=1/pic G=4 demeure admis.

`coherence_memoire.patch` propose ces trois gardes dans le lecteur MES-B seulement. Application
vérifiée en copie temporaire ; les trois contre-JSON deviennent `illisible`, les témoins positifs
CPU/appareil et celui avec libération restent admis. Les autres refus restent acquis.
Ce correctif partiel n'est pas intégré et ne certifie pas toute cohérence physique possible.
Aucune mesure réelle incorrecte ni défaut du calcul mémoire C++ n'est déduit de ces contre-JSON.

```sh
python3 -B check.py --repo DEPOT
python3 -B -O check.py --repo DEPOT
python3 -B check.py --repo DEPOT --run-gates
```

Le lecteur extrait les neuf sources Git épinglées, rejoue uniquement les JSON ciblés et applique
le correctif sur une copie temporaire. `--run-gates` ajoute les trois commandes Python officielles.
Les sorties ciblées normal/−O sont identiques au résultat capturé ; aucun fichier produit vivant
n'est lu comme autorité ou modifié.
