# B3-K adopté : identité du produit avec le bras mesuré

10 octobre 2026. Le développeur a adopté les clés seules dans `2aaed1847`.
Reconstruction indépendante des huit fichiers du bras `cles` déclaré dans
`81b0883d1`, depuis la base A6c `aa6338ee8` : **tous les fichiers de src/ du
produit sont identiques octet pour octet au bras mesuré**, sans ajout ni retrait.
Le resolveur est revenu exactement à la base ; seul le mutant spécifique au
balayage est retiré du manifeste de tour, de plancher74 à73. Catalogue inchangé.

Cette comparaison rattache le code adopté aux [statistiques B3b](../b3b_stats/README.md).
Elle ne transfère pas les mesures du lot complet au bras clés, ne constitue
pas une nouvelle exécution native et ne lève pas les limites d’[admission](../session_b3b_admission/README.md).
Aucun nouveau chrono CPU, 37 trames ou massif. Empreintes et comptes dans
`capture.json`, sans duplication des sources.

```sh
python3 -B check.py /chemin/du/depot
```
