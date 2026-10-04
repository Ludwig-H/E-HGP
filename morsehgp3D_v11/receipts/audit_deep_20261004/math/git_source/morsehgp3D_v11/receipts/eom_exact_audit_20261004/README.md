# Aide exacte pour les scores EOM

4 octobre 2026. Audit borné, profil de travail u21/CPU, `not_claimed`.
Aucune construction native, import du produit, fit, donnée LiDAR ou session G4.

Pour une date **positive** e=√t+√M−√q, t,M,q rationnels non négatifs,
poser d=t+M−q et Δ=d²−4tM. Si Δ≠0 :

```
1/e = [(t−M−q)√t + (M−t−q)√M + d√q − 2√(tMq)] / Δ.
```

Le numérateur est (√t+√M+√q)(d−2√(tM)). En multipliant par e,
on obtient (d+2√(tM))(d−2√(tM))=Δ. Si Δ=0, la branche
q=(√t+√M)² donne e=0 et doit être refusée. Pour e>0, nécessairement
q=(√t−√M)², donc e=2√min(t,M) et 1/e=√min(t,M)/(2min(t,M)).
Le cas tM=0 et Δ=0 donne aussi e=0. Une API générale doit certifier
e>0 avant d'utiliser la branche singulière ; le helper teste un domaine
positif déclaré, pas le domaine de dates négatives.

Cela donne au plus quatre classes carrées rationnelles par réciproque ;
le cube reste dans les quatre classes {t,M,q,tMq} (dépendances possibles).
Addition, produit et puissances entières s'effectuent dans des sommes de
racines rationnelles. Deux termes partagent une classe si leur quotient
est un carré rationnel, testable par deux racines carrées entières, sans
factorisation. Les racines de classes distinctes sont indépendantes sur Q
(caractères du groupe de Galois multiquadratique). Ainsi, toutes les sommes
de coefficients nulles certifient **exactement** un score nul.

La somme d'un score peut avoir bien plus de quatre classes : la borne est
par date, pas par arbre. Le signe non nul reste à séparer par intervalles
avec budget et refus. Le regroupement naïf du helper est quadratique en
nombre de termes ; ce n'est ni une architecture native rapide proposée,
ni un budget u21 complet, ni une qualification du contrat 100 ms. La
variante λ=−log r exige un autre contrat, non traité ici.

## Témoin géométrique et prototype privé

Le nuage collinéaire entier A={0,6,12}, B={22,28,34}, D={52,58,64}
du [reçu précédent](../flat_selection_math_r2_20261004/README.md), plongé
par x↦(x,x,0), a ses rayons multipliés par √2. Pour H qualifié k=2/m=3,
les entrées valent 6√2, A/B fusionnent à 8√2 puis avec D à 12√2.
À mcs=3 et λ=1/r, racine exclue :

```
S(A∪B) = 6[1/(8√2)−1/(12√2)] = √2/8
S(A)+S(B) = 6[1/(6√2)−1/(8√2)] = √2/8.
```

L'égalité est donc certifiée malgré des scores individuels irrationnels.
Les trois fonctions privées figées `_phi_interval`, `stability_interval`,
`eom_select` sont extraites par AST ; aucun import ni fit. Au budget
128 bits, elles choisissent bien parent+D, mais signalent un arbitrage
forcé. Ce témoin n'est **pas une erreur de labels** : il montre comment
éviter un refus/indicateur inutile sur une égalité vraie. Auparavant, un
autre reçu montrait un mauvais gagnant flottant ; ne pas confondre les deux.

La formule a été contre-relue indépendamment par l'auditeur mathématique.
`SOURCE.json` épingle la copie privée, inchangée pendant ces tests ; les
lectures futures testent cette copie et ne dépendent plus du chemin privé.

## Rejouer

```
python check.py
python -O check.py
sha256sum -c SHA256SUMS
```

**888 gardes** concordantes : 292 dates positives (dont 13 Δ=0), identités
e/e³ avec leurs réciproques, borne de quatre classes, refus des dates
nulles, égalité géométrique et extraction AST. Une mutation qui omet le
facteur 2 devant √(tMq) est tuée par l'identité exacte, sans crash.
Les sorties ne qualifient aucune décision native, aucun grand nuage ni G4.
