# K : budget de temps sous recouvrement parfait à coûts inchangés

**Diagnostic conditionnel, pas une mesure de A ni une borne sur un nouvel algorithme.**
Les JSON sont ceux de la [session K admise](../session_k_full/README.md), source
`c9ac60f20`. Calcul séparé pour chaque passe GPU K5, puis mêmes agrégations que le
contrat : 135 passes chaudes ng00–02 et 185 passes chaudes des 37 trames.

Pour les coûts séquentiels observés, on pose
`L = P + C + raccord + max(G, TMVR)`.
Cela superpose parfaitement les deux enveloppes G et TMVR, garde P/C/raccord et
supprime même le petit résidu `delta = mur − (P+C+G+raccord+TMVR)`.
La médiane de cohorte est celle des médianes par trame.
Une deuxième variante garde ce résidu : `L + delta = mur − min(G,TMVR)`.
Aucune médiane d'étage n'est additionnée à une autre.

| Cohorte | Mur mesuré, médiane / maximum contractuel | L, médiane / maximum contractuel |
| --- | ---: | ---: |
| ng00–02 | 159,303 / 164,636 ms | 102,132 / 112,661 ms |
| 37 trames, six séquences | 241,309 / 467,920 ms | **169,498 / 311,049 ms** |

Sous ce scénario, **29/37 médianes et 31/37 maxima contractuels restent >100 ms**.
Les trois médianes individuelles ng00/ng01/ng02 deviennent
102,132 / 83,865 / 111,673 ms. Garder `delta` donne les mêmes nombres de dépassements.
La préparation+catalogue seule reste sous 100 ms pour ces prises, mais atteint
96,865 ms au maximum sur la cohorte de 37 trames : la marge pour l'aval peut être très courte.

« Maximum contractuel » garde la définition de K : maximum des médianes chaudes
par processus/trame. Sur ng, chaque processus a neuf passes chaudes ; sur la
cohorte de 37 trames, une seule par processus/trame, cinq processus. Le maximum brut des
45 passes ng ne le remplace pas. Les deux champs restent séparés dans
[results.json](results.json) et les statistiques originales sont recoupées
exactement avec le reçu K avant de publier le calcul.

**Interprétation pour le développeur.** À coûts d'étages conservés, le seul
recouvrement G/TMVR ne suffit pas pour le contrat des 37 trames. Mesurer aussi la
réduction du travail C/T et les dépendances critiques par ordre. Ce n'est pas
une impossibilité pour A : son ordonnanceur peut changer le travail, les attentes,
les allocations et les effets de cache ; la concurrence peut aussi introduire de
la contention. Inversement, les dépendances par ordre rendent le chevauchement
parfait supposé ici potentiellement irréalisable. Ce scénario ne prédit donc
aucun chrono de A, aucune mémoire de coexistence ni gain qualifié.

[check.py](check.py) vérifie le hash de l'archive et des 20 journaux déjà admis,
la cohorte, les prises chaudes et l'identité algébrique avec le résidu. Rejeu
normal/−O identique ; aucune sonde, donnée de scène ou commande cloud exécutée.

```sh
python3 -B check.py --archive /chemin/session-K/results/results.tar.gz
python3 -B -O check.py --archive /chemin/session-K/results/results.tar.gz
```
