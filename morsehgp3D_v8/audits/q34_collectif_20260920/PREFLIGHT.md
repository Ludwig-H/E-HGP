# Préflight de la porte de corde

Avant la capture finale, la commande réellement exécutée
`python3 morsehgp3D_v8/audits/q34_collectif_20260920/chord_bound_gate.py`
a quitté avec code1, stdout vide. Extrait de l’erreur transmis par son
auteur à l’auditeur principal :

```text
File ".../chord_bound_gate.py", line 257, in main
  require((old, new) == (28, 26), "collective fixture bounds changed")
File ".../chord_bound_gate.py", line 23, in require
  raise RuntimeError(message)
RuntimeError: collective fixture bounds changed
```

L’attendu26 était erroné : q=ceil(1904/407)=5, qS=595 et
ceil_sqrt(595)=25. Seul l’attendu est devenu(28,25), pas la formule.
Ce document préserve le signalement et l’extrait disponibles ; ce n’est
pas une capture intégrale du premier processus ni un résultat qualifié.
Les sorties finales normal/−O sont conservées séparément dans `capture/`.
