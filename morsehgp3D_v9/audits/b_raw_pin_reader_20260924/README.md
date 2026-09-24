# Lecteur indépendant B des épingles brutes CPU de C

Ce lecteur audite le reçu publié `../c_raw_pins_20260924/` en lecture seule.
Depuis la racine du dépôt :

```sh
python3 -B morsehgp3D_v9/audits/b_raw_pin_reader_20260924/read_pins.py
python3 -B -O morsehgp3D_v9/audits/b_raw_pin_reader_20260924/read_pins.py
python3 -B morsehgp3D_v9/audits/b_raw_pin_reader_20260924/selftest.py
python3 -B -O morsehgp3D_v9/audits/b_raw_pin_reader_20260924/selftest.py
```

Les trois mutations du selftest portent sur des copies temporaires. Pour le
digest et le code de retour, la copie de `SHA256SUMS` est recalculée : le refus
vient bien du contrôle sémantique. Aucun `assert` Python n'est utilisé.

Le lecteur impose les 29 chemins et SHA-256 du manifeste, le SHA de
`PINS_RAW.json`, la base, la forme du marqueur SHA du binaire, les 12 commandes
et codes de retour GNU time, ainsi que les 12 JSON v26 `complete_relative`.
Il recalcule SHA-256 et FNV-1a sur les trois entrées `full.u32le` versionnées
dans le reçu v8. Il vérifie les six lignes de `PINS_RAW.json`, leurs douze
cas, les trois condensés et l'égalité intégrale des tableaux `orders`,
`catalogue` et `tower_work` des deux bras. Le contrôle d'Euler est exigé
jusqu'à `Kmax−2` seulement.

Les JSON de C portent `input.grid=unspecified` : la provenance des fichiers
d'entrée est la grille 1 mm, mais ce libellé absent n'est pas réécrit. Le SHA
du binaire est un marqueur déclaré, car l'exécutable et les journaux de
compilation ne sont pas archivés dans ce reçu. Les résultats sont des
références CPU empiriques pour ces trois trames de la seule séquence 08 ; le
lecteur ne transforme pas ce reçu en preuve GPU/G4, float32 par défaut ou
qualification multi-séquence.
