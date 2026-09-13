# DualBlocks : chercher d'abord les témoins susceptibles de saturer

13 septembre 2026. Audit complémentaire, `exploration_v8_hors_registre`,
`cpu_reference`, `quantized_u16_input_only`,
`audit_independant_math_and_architecture`, `public_status=not_claimed`.

**Un changement du seul ordre de visite réduit de 188 910 à 82 les tâches
DualBlocks d'une fixture n2064, avec les mêmes comptes saturés et les
mêmes 36 candidates.** La construction de l'arbre reste payée. La variante
est une copie temporaire de l'audit, pas une modification du moteur.
Ce résultat répond à la demande du développeur sur les géométries qui
dégradent son parcours conjoint ; il motive une proposition ordonnée
vers le facteur opposé avant les descentes difficiles.

## 1. Une tige proche de la frontière q4, puis huit bons témoins

Prendre D=60000, m dans 64,128,256,512,1024, et deux facteurs :

$$A=\lbrace(i,i,i):0\leq i<m\rbrace\cup\lbrace(3m+j,0,0):0\leq j<8\rbrace.$$

$$B=\lbrace(D+i,i,i):0\leq i<m\rbrace\cup\lbrace(D-3m-j,0,0):0\leq j<8\rbrace.$$

Les indices sont entiers ; ces cinq instances rentrent dans u16 et passent
la séparation s12 réelle de la factory. Le nuage A∪B n'a pas de témoin
extérieur. On prend q4 et Kmax10, soit un besoin de huit.

Les sites d'une même tige ne peuvent pas se servir de témoins universels.
Pour a_i et z=a_j dans A, choisir b_i=(D+i,i,i) dans B. Avec t=j−i :

$$H=Dt-3t^2,\qquad\Xi=2D^2t^2.$$

Si t≤0, H≤0 ; si t>0 et H>0, alors 2H²<Ξ. La symétrie donne B.
Les huit points orientés, en revanche, témoignent pour chaque ancre de
leur tige sur toute la boîte opposée. Le [juge indépendant](dual_order_probe.cpp)
le vérifie par entiers multiprécision sur tous les couples ancre–site et
les coins, sans appeler le prédicat produit.

Les comptes saturés exacts sont huit sur la tige et 7−j sur le point
orienté d'indice j. Le résidu ne contient donc que 8+7+…+1=36 paires.
Les trois méthodes restent comparées au même juge : le pool retrouve
ces comptes sur cette famille, tandis que les tubes laissent un résidu
beaucoup plus grand. La fixture est plane ; aucune génération réelle
de supports q4 ou de tour FULL n'est revendiquée.

## 2. Ce qui change dans l'expérience

Le parcours initial visite toujours le fils gauche des témoins avant le
droit. Dans A, il examine la tige avant les huit témoins qui permettraient
de la saturer. La copie expérimentale inverse cet ordre lorsque la boîte
opposée est entièrement à droite de la boîte des témoins, sur l'axe x ;
sinon elle conserve l'ordre initial. Le [runner](dual_order_checks.py)
conserve les deux fragments exacts et les hashes dans le reçu.

Ce changement ne retire aucune tâche indécise et ne précharge aucun compte
du pool. Les deux enfants continuent de partitionner la population des
témoins ; seul leur ordre change. Une tâche déjà saturée peut être sautée,
comme dans le produit initial. La somme saturée d'ensembles disjoints est
indépendante de l'ordre ; aucun ID supplémentaire ne doit être transporté.

À m1024, n2064 :

| Travail | Ordre initial | Variante temporaire |
| --- | ---: | ---: |
| Tâches du parcours | 188 910 | 82 |
| Couples de feuilles | 47 828 | 16 |
| Visites de points pour construire les arbres | 47 248 | 47 248 |
| Candidates | 36 | 36 |

Aux cinq tailles de tige, la variante paie 82 tâches après construction
des arbres. C'est une observation bornée à ces instances, pas une borne
universelle ni une complexité constante de la préparation complète.
Le [reçu](DUAL_ORDER_CHECKS.json) privilégie les compteurs ; aucun gain
de temps de tour ou extrapolation industrielle n'est déduit.

## 3. Comparaison défavorable conservée et proposition au développeur

Sur la grille n256, q2, le nombre de tâches passe de 1 236 à 850, mais
les couples de feuilles et requêtes universelles passent **de 1 à 22**.
Les unités ne sont pas interchangeables : le changement n'améliore pas
tous les compteurs. Les 15 cas géométriques par ordre comprennent aussi
la tige réfléchie, une ligne et des grilles q2/q3/q4 ; les comptes littéraux
et candidates restent identiques entre ordres. Cinq cas de la variante
sont en outre exécutés sous UBSan.

La proposition utile est de comparer une priorité fondée sur la projection
vers B, avec les deux directions et tous les axes, plutôt que de figer ce
test spécifique à x. Payer ses évaluations, la construction, les cas où
l'ordre défavorise la saturation et le résidu. Un échantillon de témoins
peut aussi guider la priorité ; ajouter ses comptes au parcours exigerait
en revanche d'exclure les IDs déjà crédités pour éviter un double compte.

```bash
python3 -B audits/morsehgp3D_v8_complementaire/dual_order_checks.py --selftest
```

L'API et les bornes géométriques ne sont pas réécrites par ce test.
Les comptes sont jugés avec des propriétaires factory non mutés ; le
défaut d'affectation de propriétaire est suivi séparément par l'autre
auditeur. GCP non utilisé.
