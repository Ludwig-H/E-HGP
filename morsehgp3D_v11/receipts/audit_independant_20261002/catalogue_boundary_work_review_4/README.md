# Coquilles exactes : travail local et refus de capacité

Source figée `f391bf13e`, produit catalogue `643fe47d7`. Calculs autonomes
entiers, aucun import du produit/oracle, build, CTest, benchmark ou GCP.
Les nombres ci-dessous sont des conséquences de la géométrie et des boucles
publiées ; aucune durée native ni borne globale n'en est déduite.

Prendre tous les triplets entiers x²+y²+z²=R², translatés par (R,R,R).
Ils sont distincts, admissibles u18, sans fusion et tous sur une même coquille.
Toute boîte fermée contenant le centre donne des distances égales en ce
centre : aucun site ne domine strictement un autre sur cette boîte.
Le réservoir et les masques de préfixes ne retirent donc aucun de ces sites
dans la feuille propriétaire du centre. Cette feuille est atteignable ; la
preuve ne suppose pas qu'une boîte artificielle soit fournie au générateur.

| R | Sites conservés | Feuille propriétaire avec les défauts | Préfixes par passe si énumérée |
| --- | ---: | --- | ---: |
| 5 | 30 | Racine, leaf_size=32 | 31 930 |
| 15 | 150 | Boîte unité après quinze subdivisions | 20 822 900 |
| 35 | 270 | Boîte unité, dépasse max_leaf=256 | Refus avant énumération |

Sur R5, LA boule centrale présente quinze supports q2, 120 q3 et 1 560 q4
strictement positifs. Pour K5 et K10, chacun passe le census complet : zéro
intérieur, trente sites de coquille. Cela donne **50 850 tests side par passe**
pour cette seule identité géométrique. Le support canonique q2 émet une fois ;
1 694 présentations sont rejetées après census/canonicalisation. Les deux
passes paient deux fois ce travail. Les autres boules ne sont pas comptées
dans ce nombre ; ne pas réduire le catalogue entier à cette unique boule.

R15 ne réclame pas de simulation des vingt millions de préfixes : tous les
masques sont nuls dans sa feuille, et la boucle parcourt chaque combinaison
d'arité 1..4. R35 prouve qu'une entrée u18 valide peut exiger un refus avec la
capacité par défaut. Si un autre refus survient avant la feuille centrale,
il interdit déjà une sortie complète ; il ne supprime pas cette obstruction.
Augmenter max_leaf peut permettre cette géométrie tout en aggravant le coût
combinatoire. Les limites sont honnêtement déclarées par le produit.

## Piste sûre à mesurer

Si une portion de coquille révèle déjà un support strictement positif de
plus petite arité que la présentation courante, celle-ci ne peut être S*.
Son rejet anticipé est correct sans achever SON census : la boule émise par
son vrai support doit encore publier toute I/U. Un support minimal non
canonique peut également être rejeté si un support de même arité
lexicographiquement antérieur est certifié. Cette preuve ne fournit pas
automatiquement une détection rentable ; borner/comparer son coût sur G4.

Le ledger actuel compte les census, sans détailler les prédicats de
canonicalisation. Au prochain diagnostic utile, distinguer arités des
présentations, recherches de support, rejets non canoniques et coût de
collecte des coquilles. Préserver les trente sites de frontière et le refus
transactionnel ; aucune réduction arbitraire de coquille n'est proposée.

[Calcul autonome](check.py), [sources figées](SOURCE_BEFORE.json),
sorties normal/−O identiques et hashes de fermeture conservés.
