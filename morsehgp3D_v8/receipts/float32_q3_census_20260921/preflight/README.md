# Préflight incomplet — attendu analytique erroné du selftest

Les trois fichiers `selftest.stdout`, `selftest.stderr` et `selftest.json`
reproduisent les octets du premier appel du lanceur local dans
`/tmp/mhgp8-q3-census-gate.hVM1YhrX/normal/`. Aucun fichier d'entrée :
le programme reçoit seulement `--selftest`. Ce sous-dossier est une trace
d'échec, **pas une qualification close ni une archive autonome**. Les
sources complètes de cet essai avant correction n'ont pas été archivées ;
il ne faut pas leur substituer les sources corrigées comme preuve d'entrée.

Le selftest utilisait cette ligne incorrecte :

```cpp
const std::array<double, 3> powers{-.5*scale*scale, 2.5*scale*scale, 0};
```

Pour a=(-1,0,0), b=(1,0,0), x=(0,2,0), le centre est (0,3/4,0)
et le rayon carré 25/16. Aux témoins (0,1,0) et (0,-1,0), les puissances
sont -3/2 et +3/2, et non -1/2 et +5/2 : le terme -1 de l'axe x avait
été oublié dans cet attendu manuel. La ligne corrigée est donc
`powers{-1.5*scale*scale, 1.5*scale*scale, 0}`. Le code produit n'a pas
été modifié pour faire passer ce contrôle. Les juges indépendants Fraction
sur les 612 appels avaient passé avant cette correction du selftest.

Une recompilation intermédiaire `probe_r2` ajoutait seulement un diagnostic
d'arrondi/FTZ ; une nouvelle sortie `probe_r3` contient l'attendu corrigé.
Le binaire initial `probe` et son dossier de flux n'ont pas été écrasés.
Les captures de qualification formelles ultérieures sont distinctes.
