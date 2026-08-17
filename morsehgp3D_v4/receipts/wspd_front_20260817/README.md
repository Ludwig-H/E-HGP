# Reçu — campagne front WSPD v4, 36 configurations

Date : 17 août 2026 UTC. Machine : conteneur CPU 4 cœurs (pas un chiffre G4).
HEAD au moment du run : voir le commit qui introduit ce reçu.

Commande (boucle) :

```bash
./build/v4/mhgp4_wspd_scaling_probe --family=$fam --n=$n --s=$s --seed=3 --min-rect=1000
```

`fam` dans {uniform, terrain, eight_clusters, scanline_single_pass},
`n` dans {8000, 16000, 32000}, `s` dans {6, 8, 10}.

Résultat : `masse=EXACTE` et `permutation=OK` sur les 36 lignes.
Rectangles par point à n=32000 : uniform 357/612/928 (s=6/8/10),
eight_clusters 224/346/484, terrain 63/96/136, scanline 44/68/96.
Front ~13–15 % plus petit que les comptes v3 à configuration comparable
(scission par diamètre + séparation sur boîtes serrées), régime linéaire non
atteint sur uniform/eight_clusters — mêmes constats que la v3, aucune pente
publiée (trois exposants successifs exigés par le plan de test).
Temps single-thread indicatifs : t_front de 13 ms (scanline 8k s=6) à
1193 ms (uniform 32k s=10).
