# `mes_g_profil` : profil de l'étage G par poste sur G4 (information)

8 octobre 2026. Banc d'**information**, sans règle ni verdict : il ne décide rien, il sert à choisir le levier suivant
de l'étage G sur des parts mesurées à 48 fils sur G4, plutôt que sur le profil local à 3 fils sous charge de T2-d-B2.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference (sonde de G profilée, hors produit)
objet=full_pi0 (étage G de la tour K5)
quantification=quantized_u21_input_only
public_status=not_claimed
```

[`profil_g.py`](profil_g.py) construit la sonde de G ([`bench/tower_probe.cpp`](../../bench/tower_probe.cpp),
`resolve_tower`, voie CPU) avec `MHGP12_TOWER_PROFILE` ([`src/tower/profile.hpp`](../../src/tower/profile.hpp)), puis la
joue à 48 fils et à un fil sur ng00, la trame médiane du `v12set` (`kitti_ng_02_001606`, 64 740 sites) et la plus
grande (`kitti_ng_08_002119`, 99 099 sites). Par trame et par nombre de fils, il publie le mur de la sonde, l'index
des naissances, la résolution et, par poste (trace, sonde, proposition, LEM-T1, certificat, repli, census saturé,
census complet, pas, arrêt, reste), le temps-fil, la part des cycles, les occurrences et le coût par occurrence.

Limites, dites d'avance :

- les cycles d'une section sont sommés sur les fils : à 48 fils, ils comptent aussi l'attente de la mémoire et la
  contention. Le rapport des temps-fils à 48 fils et à un fil mesure la perte d'une section au passage à l'échelle ;
- le profil ajoute deux lectures encadrées par `lfence` à chaque occurrence. Il gonfle les petites sections ; le biais
  mesuré est publié, il n'est pas retranché ;
- ce n'est pas le produit. La Session recouverte n'est pas jouée, et les murs publiés ici sont ceux de la sonde
  profilée.

Auto-test (Python nu, sous `-O`) : `python3 -S -O profil_g.py --auto-test`.
