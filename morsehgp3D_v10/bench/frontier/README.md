# Développement des attaches frontière

30 septembre 2026. Ce dossier contient deux briques nouvelles, hors du
préenregistrement A0–A6 du développeur. Elles ne choisissent pas encore la
tête de clustering de production. Le moteur testé reste géométriquement
u18 ; le port de précision supérieure est décrit dans la
[passation de développement](../../docs/DEVELOPPEMENT_FRONTIERE_ET_PRECISION_20260930.md).

## Attache par bande de première couverture

[cover_band.py](cover_band.py) considère les témoins forts propres à K qui
couvrent un point, jusqu'au rayon `(1+eta) alpha_K(x)`. Il prend l'ancêtre
commun de leurs composantes à leurs dates respectives, puis fixe l'attache
à `max(alpha_K(x)^2, naissance de cet ancêtre)`.

La bande inclut toutes les égalités, tous les intérieurs et toutes les
coquilles. Elle ne se limite pas aux feuilles : les points entrant dans
une branche interne à K3/K5 sont conservés. Les fusions FULL restent dans
la forêt, même si leurs boules ne font pas partie de l'univers fort.

Cette anticipation locale n'attend pas l'activation du dernier témoin.
Le premier témoin assure déjà la couverture du point dans la lignée choisie.
Une fois l'attache fixée, le point suit seulement ses ancêtres ; avant son
entrée, il reste un singleton. Les partitions sont donc emboîtées. Agrandir
la bande retarde ou conserve l'entrée et raffine les partitions à coupe fixée.

Le contexte attendu est celui de `ArmContext` des fondations frontière :
`n`, `forest`, `cover_level(site)`, `witnesses()`. L'univers doit être complet,
avec `population >= K` et `p + q_min <= K`, indépendant de Kmax. Le module
ne fabrique ni cet univers ni la tour. Son travail est proportionnel aux
incidences examinées et aux requêtes d'ancêtre commun, pas aux couples de
témoins ; cela ne borne pas le nombre global d'incidences.

`eta=0` reproduit la règle A5. Une bande positive traite aussi les quasi-égalités,
que la seule unicité à la première date ne distingue pas. **Aucune stabilité
universelle n'est démontrée** : le seuil et le catalogue peuvent changer sous
perturbation. La garantie locale du bras historique K2 ne se transfère pas
aux centres q3/q4. Aucun gain ARI/EOM ni chrono G4 n'est acquis ici.

## Quotas exacts des scènes de développement

[dev_quotas.py](dev_quotas.py) alloue exactement le nombre demandé de retours,
avec 84 % répartis entre les communautés, un rapport cible cœur/halo de 3/1,
et 16 % de fond. Les plus grands restes sont départagés sans flottants.
Pour 1 500 retours et trois communautés : trois fois 315 cœurs et 105 halos,
puis 240 retours de fond.

Le helper ne génère pas de coordonnées. Son intégration dans le générateur
privé `mesure_bras.cloud` reste à faire : il ne faut pas annoncer sa mesure
historique comme un test à 1 500 points avec fond. Celle-ci produit 1 680 sites
sans fond. Les doublons et le nombre de sites après quantification devront
rester publiés séparément des quotas de retours.

## Tests

Les deux portes autonomes ne demandent ni NumPy, ni GPU :

```bash
python3 -B morsehgp3D_v10/tests/points/test_cover_band.py
python3 -B -O morsehgp3D_v10/tests/points/test_cover_band.py
python3 -B morsehgp3D_v10/tests/points/test_dev_quotas.py
python3 -B -O morsehgp3D_v10/tests/points/test_dev_quotas.py
```

Le [test natif borné](../../tests/points/test_cover_band_native.py) exige des
fondations et un exporteur fournis explicitement, avec un dossier de sortie
neuf. Ses six nuages ont cinq à sept sites ; il vérifie les attaches contre
un graphe Gamma exhaustif indépendant et couvre les entrées internes K3/K5.
Les [reçus](../../receipts/development_frontier_precision_20260930/README.md)
ne qualifient pas la qualité statistique, la croissance ni un profil plus précis.
