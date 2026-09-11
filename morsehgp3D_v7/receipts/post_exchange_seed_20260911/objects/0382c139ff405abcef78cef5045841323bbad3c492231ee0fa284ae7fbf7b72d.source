# Phase statique : panne SAN levée et semis après échange

11 septembre 2026, livraison constructeur `ce842a3f`, header `33e7d05e…`.
`phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

## Panne de worker : nouveau rejeu SAN réussi

Le [paquet constructeur](../../receipts/static_worker_failure_20260911/README.md)
conserve son succès O2 et son premier refus LSan/ptrace. Notre
[rejeu distinct](worker_san.json) exécute **le même binaire SAN `70379dd9…`**,
avec les mêmes entrées, sans compilation ; fuites activées, stderr vide,
code 0. Deux allocations scratch échouent après admission, sortie globale
vide et workers joints ; le plancher de deux MEB payées est conservé.
La réutilisation nominale passe ensuite 2 524 signatures/coupes et 1 506
images verticales. Le test ne prouve pas une égalité indépendante de tous
les compteurs des workers, ni l'absence générale de races ; aucun TSan ici.
Les nouveaux pins avant/après ne complètent pas rétroactivement le premier
run constructeur. [worker_replay.py](worker_replay.py) conserve le protocole.

## Travail résiduel et piste sans nouvelle table globale

Les [tours statiques 8k/16k/32k](../../receipts/static_resolution_scale_20260911/README.md)
et le [comptage R/U](../../receipts/initial_representatives_20260911/README.md)
sont contre-vérifiés par leurs lecteurs épinglés. Noter U les représentants
initiaux uniques, S ceux résolus par semis **à l'entrée**, D les échanges
d'intrus. Sur ce chemin réussi, chaque groupe non semé démarre une MEB,
puis chaque échange en ajoute une : `M = U−S+D`.

| n | MEB initiales évitées face au cache nominal | MEB de descente évitées | MEB de descente encore payées |
| ---: | ---: | ---: | ---: |
| 8 000 | 1 999 980 | 42 101 | 1 404 945 |
| 16 000 | 4 380 172 | 93 143 | 2 927 135 |
| 32 000 | 9 267 110 | 199 546 | 6 090 817 |

Environ 98 % de la baisse vient donc des départs, tandis que les descentes
pèsent encore environ un tiers des MEB statiques. Ce sont des différences
de compteurs issus des captures, pas des gains de latence appariés.

La table immuable des semis `I(B) union U(B)` est déjà construite par K.
`static_terminal()` ne la consulte pas après remplacement d'un site :
il trie la nouvelle facette puis calcule sa MEB. **Un lookup exact à cet
endroit pourrait éviter la dernière MEB lorsque la facette est un semis.**

## Lemme : un tel hit diminue strictement le rayon

Soit C la MEB de la facette F, v le site retiré et z un intrus strict,
avec $F'=(F\setminus\{v\})\cup\{z\}$. Supposons que la clé entière de F'
soit celle d'un semis certifié B : $F'=I(B)\cup U(B)$ et $K=p_B+u_B$.
Le support positif de B appartient à F', donc sa MEB est exactement B.
Comme F' est contenu dans C, son rayon ne peut croître.

L'égalité ferait de C une MEB de F', donc C=B par unicité. Or v appartient
au nuage et à C=B : il appartiendrait alors à sa population complète
I(B) union U(B)=F', contradiction, puisque v a été retiré et z est distinct.
Ainsi **le niveau de B est strictement inférieur à celui de C**, lui-même
strictement antérieur au consommateur. B est admise à K=p_B+u_B. Le chemin
actuel calculerait B puis retournerait immédiatement **la même BallId**,
pour le même choix de site retiré et d'intrus.

Cette preuve emploie la population entière du census, pas un support ni
une population partielle. Elle vaut avec intérieurs et coquilles
supplémentaires, sous les mêmes hypothèses de certification du catalogue.

## Fixtures exactes et raccord proposé

[seed_fixture.py](seed_fixture.py) vérifie en rationnels deux témoins, sans
moteur C++. Pour A=(0,3,0), B=(8,3,0), Z=(4,2,0), W=(6,0,0), E=(4,23,0),
le triangle ABE est Gabriel positif, de rayon carré 2704/25. À K2, son
représentant AB a pour MEB le diamètre AB, de rayon carré 16, contenant
deux intrus : cette boule n'est pas admise à K2. Après retrait de A, chacun
des choix Z ou W produit un semis entier, de rayon carré 17/4 ou 13/4.
Pour cette résolution et ce retrait, une MEB au lieu de deux suffit
algébriquement. Ce n'est pas un nouveau comptage du moteur exécuté.

La contre-fixture est un carré avec quatre points intérieurs : retirer
un sommet diagonal peut garder le rayon grâce à l'autre diagonale. La
population complète a huit sites, contre K4 pour la facette ; ce cas doit
manquer le lookup et garder la descente à rayon égal habituelle.

Raccord minimal : passer la table existante en lecture seule au helper,
chercher après le tri des sites et avant `meb(next)`, comparer la clé
entière liée au même index/K, conserver les gardes strictes et le rejeu
temporel actuel. Compter ces hits séparément des semis initiaux ; ne pas
attribuer une MEB payée au raccourci. Il ajoute un lookup par échange et
ne demande aucune table globale supplémentaire.

Avant activation, un compteur d'observation H pourrait garder la MEB
actuelle et vérifier chaque hit contre sa BallId/niveau calculés ; la
fixture ci-dessus doit exercer H>0. Puis comparer payload, parents,
contributions et verticales avec/sans raccourci. Au plus une dernière MEB
est supprimée par chaîne : `H ≤ min(D,U−S)` et `M_nouveau = M−H`, si les
trajectoires restent identiques. **H n'est pas mesuré ici** ; ni économie
de temps ni proportion de hits ne sont supposées.

## Réponse au constructeur : support survivant et rayon égal

Un support positif déjà certifié Q de C suffit **si Q est entièrement
conservé après le retrait** : Q impose le rayon de C et F' est contenu dans
C, donc sa MEB reste C. Aucune nouvelle vérification de contenance des
sites n'est nécessaire ; l'intrus est déjà certifié strictement intérieur.
Mais le code courant retire `support_slots[0]` : son support choisi est
toujours détruit. Cette condition ne lui procure donc aucun hit gratuit.

Le critère exact d'égalité est que le centre de C appartienne à l'enveloppe
convexe des sites de F restés sur sa coquille après retrait. L'intrus
strict n'appartient pas à cette coquille. Un autre support positif, de
cardinal au plus quatre, en constitue un témoin. La diagonale conservée
du carré de la contre-fixture fournit précisément ce témoin. Si la coquille
sélectionnée était exactement le support positif minimal choisi, retirer
un de ses sommets impose au contraire une diminution stricte du rayon.

Le code offre déjà `ShellTable::contains_center()` et `minimal_supports()`.
La table d'une coquille supplémentaire présente au catalogue est construite
même si sa boule n'est pas admise au K courant. Son masque sans v permet
donc de décider l'égalité. **Ses bits suivent les PointId triés, tandis que
les slots du resolver suivent les indices géométriques triés** : convertir
explicitement, puis vérifier le choix sous permutation des PointId.

Coûts à déclarer : sans masque de coquille mémorisé, identifier les sites
restants demande au plus K−1 puissances exactes dans la boule déjà connue.
Un témoin alternatif conservé et encore inclus suffit ; sinon il faut
certifier son existence parmi les sous-ensembles de deux à quatre sites de
cette coquille, avec les prédicats entiers qualifiés. Ne pas supposer que
le seul cardinal de coquille, le maintien de l'ancienne arité, ou un support
incluant l'intrus suffit. Une liste locale de témoins évite toute nouvelle
matérialisation globale, mais son coût de construction doit être mesuré.
Avec K≤10, la frontière restante a au plus neuf sites : au plus 246
sous-ensembles de tailles deux à quatre, si aucune table ni témoin utilisable
n'existe. `anchor_meb` classe déjà tous les sites lors de l'acceptation du
support ; conserver un masque en plus du compte éviterait de répéter cette
classification. Les prédicats de plateaux existants, dont l'arithmétique
signée 192 bits du triangle, restent l'autorité à réutiliser.

Pour garder la trajectoire du resolver à l'identique, choisir le premier
support valide dans l'ordre actuel q2/q3/q4 puis lexicographique. Chercher
seulement parmi les sites de coquille conserve cet ordre relatif : un site
strictement intérieur ne peut appartenir au support positif d'une MEB
égale à C. Prendre arbitrairement un autre témoin pourrait changer le
prochain sommet retiré ; ce serait une modification de politique à qualifier.
Les masques et compteurs doivent aussi refléter le nouveau tri des indices.

Ce raccourci à rayon égal est distinct du hit de **population complète**,
qui impose le rayon strictement décroissant prouvé plus haut. Les trois
tours uniformes épinglées comptent zéro `same_radius_steps` : aucune
économie sur ce triplet ne peut lui être attribuée. Ces conditions sont
mathématiques ; aucun nouveau backend de supports n'est implémenté ici.

## Reproduction

```bash
python3 -B morsehgp3D_v7/audits/receipts_static_followup_20260911/verify.py
python3 -B -O morsehgp3D_v7/audits/receipts_static_followup_20260911/verify.py
```

Le lecteur vérifie les sources et captures référencées par
[context_pins.json](context_pins.json), le nouveau rejeu SAN et les fixtures,
puis recalcule [review.json](review.json). Il n'exécute aucun binaire, ne
relance aucun benchmark et ne crée aucune variante globale de qualification.
Les demandes R/U, échelle statique et s8/10/12 sont satisfaites dans les
paquets constructeur ; leurs tableaux détaillés n'ont pas à être recopiés
dans les notes actives. GCP non utilisé.
