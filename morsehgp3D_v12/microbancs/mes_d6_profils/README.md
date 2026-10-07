# Microbanc MES-D6 : coût des profils u24 et u32 face à u21

7 octobre 2026. Mesure publiée, **sans règle d'adoption**, de la décision D6 (au moins u21, puis u24 et u32) et du
constat `CST-0207` : ce que coûte un profil de coordonnées élargi **sur les mêmes trames réelles**, et non un rapport
original / translaté dans un même binaire.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference (produit v12 : sondes du catalogue et de l'étage G de la tour)
quantification=quantized_u21_input_only, u24 et u32 (profils compilés à part)
public_status=not_claimed
```

[`pilote_d6.py`](pilote_d6.py) (bibliothèque standard, Python 3.10 nu) construit le produit à chaque profil demandé
(`-DMHGP12_COORD_BITS=21|24|32`, cibles `mhgp12_catalogue_probe` et `mhgp12_tower_probe`), puis joue, par tours
alternés, pour chaque trame et chaque couple (profil, dilatation) admissible, la sonde du catalogue puis celle de
l'étage G, `P` passes dans un processus neuf. Dilatations : ×1, ×8 (grille huit fois plus fine) et ×2048, retenues
quand la plus grande coordonnée dilatée tient dans le profil (sur les trames sans sol de SemanticKITTI, ×8 tient déjà
dans 21 bits).

**Contrôles** : à ×1, résolution et catalogue identiques aux trois profils (empreinte de la résolution, en-tête exclu
par la sonde ; SHA-256 de l'export du catalogue privé de son en-tête de 64 octets, qui porte les bits du profil) ;
comptes de l'objet (boules, incidences, naissances, cellules, représentants par ordre) inchangés par dilatation (tous
les prédicats sont homogènes) ; une empreinte par prise. Codes : 0 rendu et contrôles conformes, 1 contrôle violé,
2 usage ou entrée illisible, 3 construction impossible.

```bash
python3 pilote_d6.py --src <morsehgp3D_v12> --racine <dossier de construction> --donnees <trames lidar_ngXX>
    --sortie <dossier> [--cas ng00,ng01,ng02] [--profils 21,24,32] [--k 5] [--fils 48] [--passes 5] [--tours 3]
```

Essai local du 7 octobre (ng00, K3, profils 21 et 24, trois fils, une passe chaude, codespace chargé) : contrôles
conformes ; temps non significatifs. Les temps de la VM G4 font foi.
