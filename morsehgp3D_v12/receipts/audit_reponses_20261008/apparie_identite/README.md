# Pilote apparié : preuves d'identité et tableaux non relus

8 octobre 2026. **Prototype non commis** capturé de façon stable sur HEAD `67b6e6d57` : pilote `fe22a6813ebb4f80…`, porte `65a658d851072628…`, lecteur LF `c3e9e0f4…`. Sources intégrales conservées hors dépôt. Périmètre CST-0018 : admission et publication des mesures, aucun résultat moteur faux démontré.

`reread` ne relit que les journaux de campagne et seulement leur mur chaud. `judge` fonde l'identité sur l'union des résumés `empreintes`, sans exiger une empreinte propre à chaque bras. `tables` utilise ensuite les résumés d'étages, CPU et pic qui n'ont pas été confrontés aux bruts.

Les quatre témoins partent d'une campagne de la **fausse sonde Python officielle**, en mode essai. La référence et l'A/A sont identiques ; le bras cache donne le rapport 0,9, le séquentiel 1,1. Le jugement calculé adopte cache et rejette séquentiel. Chaque altération suivante conserve exactement ce jugement avant correction :

| Témoin | Preuve devenue absente ou incohérente |
|---|---|
| Suppression des douze journaux d'identité | aucun brut ne reste pour les trois trames × quatre bras |
| FUL1 ng00/cache changé puis journal re-haché | bruts `cd…`, résumé toujours `ab…` |
| Liste d'empreintes ng00/cache vidée | l'union des autres bras reste de cardinal 1 |
| Résumés P/CPU/pic de ng00/cache falsifiés | mur intact et bruts inchangés ; les tableaux publient les autres champs inventés |

Dans le dernier cas, une ligne auparavant à 0,9 ms de mur affiche toujours 0,9 ms, mais désormais **P=999,0 ms, CPU=888,0 ms et pic=777 Mio**. Le juge ne refuse pas. Cela ne décrit aucune prise réelle ; c'est un contre-exemple causal de relecture incomplète.

## Correction ciblée proposée

[relecture.patch](relecture.patch) remplace `reread` seulement : chemins dérivés de la place attendue, présence et SHA du journal, code entier 0, attente de deux passes avec digest pour chaque identité, puis lecture stricte. Il recalcule `take_summary` et les empreintes de **toutes** les prises d'identité/campagne, et confronte chaque champ aux résumés publiés. La comparaison JSON conserve les types numériques ; un booléen ne vaut pas un entier.

Les quatre témoins deviennent `refuse` (contrôle incohérent/manquant), et le jugement du positif reste identique. Le patch est appliqué dans un temporaire, postimage SHA **7ca0a8456d3bb66ef742eab8af017fc678254df630cb56296863ac611da25f87**. Aucune source du développeur modifiée. La règle statistique et ses seuils ne changent pas.

Cette correction ne ferme pas les cohortes/tours supplémentaires, la cohérence du protocole global ou la relecture des informations v12set ; ces points sont distincts. Elle doit être composée avec les autres gardes en cours. Aucun changement de verdict rétroactif d'une campagne réelle n'est revendiqué.

## Reproduction et limites

```sh
python3 -B check.py SNAPSHOT > lecture.json
python3 -B -O check.py SNAPSHOT > lecture_O.json
cmp lecture.json lecture_O.json
```

Normal/−O identiques à `results.json`. Le harness vérifie les pins, produit la fixture avec la porte officielle, altère/restaure ses fichiers temporaires, et compare l'ancien juge au patch. Seuls des processus Python fictifs sont lancés : pas de moteur natif, compilation, GCP ni lecture de données sous licence.

Le « positif » désigne ici l'admission par LF `c3e9e0f4`, pas une sortie native qualifiée. La fixture officielle contient encore V(k2)=5<M(k1)=6 ; les nouvelles [gardes LF proposées](../lf_recouvert_gardes/README.md) la refusent à juste titre. Sa correction de fixture doit accompagner l'intégration de ces gardes ; elle ne change pas la causalité des quatre témoins d'identité/relecture au pin actuel. Les 850 passes natives A ne sont pas concernées par cette limite synthétique.
