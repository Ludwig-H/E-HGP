# Lecteur FULL recouvert — gardes des horloges et dépendances

8 octobre 2026. Défaut du **lecteur**, rattaché à CST-0018, au commit livré **86d7e39d834cae86768d49a23d83bf53454ec2a3** : `lecteur_full.py` SHA `c3e9e0f4…`. La capture initialement non commise et les quatre fichiers finalement livrés sont identiques. Aucune compilation, exécution moteur, donnée de coordonnées ni GCP ; seulement lecture des sources et relecture de JSON existants/synthétiques.

**Neuf corruptions de diagnostics sont admises par le lecteur livré, puis refusées par le correctif proposé.** Le contrôle positif est un vrai processus ng00 après de la [session A déjà admise](../session_t2da_admission/README.md), conservé intégralement hors dépôt. Une seule première ligne FULL est altérée par cas :

| Altération | Écart à la source |
|---|---|
| `tour_ns = wall_ns + 1` | l'appel tour sort du mur englobant |
| ouverture recouvrement mise à 0 | les deux publications de `d.open_ns` divergent |
| toutes les fins G mises à 0 | elles précèdent l'ouverture |
| fin noyau k1 mise à 0 | elle précède sa fin G |
| fin M k1 mise à 0 | elle précède la clôture du noyau |
| fin R k1 mise à 0 | elle précède sa fin M |
| fin V k2 mise à 0 | elle précède sa fin M |
| fin V k4 abaissée à M(k4) | M(k3)>M(k4) dans cette passe : seule la dépendance à l'ordre inférieur est violée |
| toutes les fins G abaissées à l'ouverture | le maximum déclaré G ne correspond plus aux fins par ordre |

À chaque fois : `parse_output(0, texte, attendu)` rend `ok` avant, `illisible` avec la proposition. Ces contre-flux ne sont pas des prises réellement mesurées et n'invalident pas les temps de la session A.

## Gardes proposées et justification

[proposition.patch](proposition.patch) modifie uniquement `check_overlapped`, sans toucher au moteur ni au lecteur séquentiel :

- P+C+tour ≤ mur ; ouverture recouvrement = ouverture G.
- Pour chaque ordre, ouverture ≤ fin G ≤ fin noyau ≤ fin M ≤ fin R.
- Pour k≥2, fin V(k) ≥ max(fin M(k), fin M(k−1)). V(k1)=0 reste la convention existante.
- Le maximum des fins G par ordre doit correspondre à la fin G globale, avec la seule exception exacte ci-dessous.

`full_probe.cpp` mesure P, C et `build_tower` par fenêtres successives du mur ; les deux ouvertures sont le même `d.open_ns`. `pipeline_run.cpp` publie la fin d'une tranche G avant son drapeau consommable ; le noyau clôt après toutes ses tranches. `step_dependencies` ordonne noyau→contraction M→R, et M(k),M(k−1)→verticales V(k). Les horodatages M/V/R précèdent la libération de leurs successeurs. `pipeline.cpp` ajoute la même ouverture à toutes les fins, puis publie après le join.

**Aucun ordre entre V et R n'est imposé.** Les traces positives contiennent effectivement 435 fins R<V et 1 268 fins V<R (sur les ordres k≥2 des passes recouvertes). Aucun total de fenêtres chevauchantes n'est ajouté au mur, ni interprété en temps CPU.

**Exception d'un nanoseconde :** `note_g_end` publie `max(1, g_max)` globalement, mais les maxima bruts par ordre. Si tous les offsets G mesurés sont nuls, le maximum des fins locales vaut ouverture et la fin globale peut valoir ouverture+1. La garde accepte exactement ce cas ; sinon l'égalité est stricte. Un positif synthétique dérivé de la prise réelle conserve cette convention. La voie sans aucune tranche garde un offset global nul, également accepté. Cela évite de rejeter un format permis par la source, même si cette limite n'est pas observée dans les mesures A.

## Vérification bornée

Le patch s'applique dans un temporaire et produit SHA **15437e5f09408d5fbdc6288e33a2452490489f13bf9a3b3a24c99f5d109b6554**. Les **61 journaux / 850 passes** de A, dont les 36 processus de chronos/Sessions demandés, restent admis avec réponses identiques : 29 processus/422 passes recouverts et 32 processus/428 passes séquentiels. La spécification vient de la prise déjà admise et épinglée ; il s'agit ici de compatibilité du correctif, pas d'une nouvelle admission de campagne indépendante.

Quatre mutants syntaxiquement valides retirent séparément l'enveloppe tour, l'égalité des ouvertures, la dépendance M(k−1) et le maximum G. Le positif passe toujours, mais chaque mutant réadmet sa corruption dédiée : la causalité ne repose ni sur un échec d'import ni sur une erreur de syntaxe.

```sh
python3 -B check.py CAPTURE_LF RETOUR_A DEPOT > lecture.json
python3 -B -O check.py CAPTURE_LF RETOUR_A DEPOT > lecture_O.json
cmp lecture.json lecture_O.json
```

Normal/−O correspondent à `results.json`. Le lecteur contrôle les hashes des sources et des 61 journaux, leur identité avec Git, l'application du patch, puis les contre-flux. Les sources et JSONL complets restent dans les captures extérieures ; aucune copie intégrale n'alourdit ce reçu.

**Portée limitée :** ces gardes ne ferment pas tout le lecteur. Notamment le résidu historique `code=False` ou `0.0`, encore admis via une comparaison à 0, n'est pas corrigé par ce patch d'horloges. Ni ce reçu ni la compatibilité A ne qualifient une future campagne, les nouvelles commandes après changement de défaut de la sonde, ou tous les invariants possibles du schéma. Le correctif reste proposé tant qu'il n'est pas intégré.
