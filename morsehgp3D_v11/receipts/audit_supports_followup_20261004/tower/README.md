# Suivi supports : contre-garde de la preuve D2

4 octobre 2026. Lecture et modèles stdlib/Fraction uniquement : aucun natif, build, fit ou cloud.
La capsule fige les rapports de conception avant lecture (`reports_before/`), les petits raccords publics S2 et les sources de forêt utiles (`s2_sources_before/`), puis le contrat mathématique et la descente au pin Git `257aabb9291f42eb38bc80561c49fb0b07115845` (`contract_pin/`). Les métadonnées distinguent les copies Git des rapports WIP. Le worktree S2 est propre : il déplace les déclarations MEB dans `meb.hpp` et rend `tower.hpp` public. Il n'implante ni `build_order`, ni le journal, ni l'attribution future S3. Les exécutions citées par `verif_s2.md` sont celles du développeur/contre-lecteur, jamais rejouées ni qualifiées par cette capsule.

## Résultat exact

À K=2, les cinq sites distincts unitaires sont :
A=(2,10,0), B=(18,10,0), C=(10,20,0), Z=(9,3,0), W=(11,3,0).
Ils satisfont tous le domaine u21, sans translation supplémentaire ni poids multiples.

La boule b=MEB(ABC) a centre `(10,59/5,0)`, β_b=`1681/25`, p=0, m=q_min=3. Elle appartient à Cat2 et à la fenêtre W2. Sa trace F=AB est stricte : β(F)=64<1681/25. Mais MEB(AB), centre `(10,10,0)`, a Z,W strictement intérieurs ; p=2 et q_min=2. Elle est donc exclue de Cat2 puisque p+q_min=4>K+1=3.

L'énumération exhaustive des 25 présentations q2..q4 donne les niveaux positifs distincts de Cat2 :
`1, 49/2, 65/2, 41, 1681/25, 145/2`.
Le précédent niveau du catalogue avant b est donc exactement41, et

`41 < 64 < 1681/25`.

La justification « β(F)≤ℓ(r_b−1) » est fausse dans `reports_before/SPECIFICATION_FINALE.md:300` (preuve du lemme D, point2). Il s'agit d'un défaut de preuve du futur raccord, pas d'un défaut du moteur.

Les dix paires et dix triples sont aussi jugés par MEB exhaustive Gram/Fraction. À41, Γ2 possède trois composantes et AB n'est pas encore un sommet. À64, AB apparaît et se rattache à la composante de ZW, sans nouvelle naissance ni fusion de composantes. Juste avant b, il reste trois composantes ; au seuil fermé1681/25 elles fusionnent toutes, avec huit sommets actifs. L'affectation proposée par le journal n'est donc pas réfutée.

## Correction de preuve

T5 certifie que la graine g appartient à la classe de F aux coupes fermées a≥β(F), ou ouvertes a>β(F). Il ne donne pas cette garantie aux coupes antérieures à β(F) : voir `contract_pin/morsehgp3D_v11/src/tower/descent.hpp:95–96`.

Soit a_- le dernier niveau de CatK avant λ_b. La naissance de g est un niveau du catalogue et précède λ_b ; elle est donc au plus a_-. Le balayage peut interroger g à a_-. Les composantes FULL ne naissent et ne fusionnent qu'aux événements de CatK (T3 et complétude du catalogue). Leur application de continuation entre a_- et λ_b est donc bijective sur H0, même si de nouveaux sommets de ΓK apparaissent à des niveaux non retenus.

Appliquer T5 à `max(a_-,β(F))<λ_b`, puis transporter cette classe par cette continuation : le nœud vivant obtenu à a_- représente bien la composante stricte de F juste avant λ_b. C'est cette identité de CLASSE, et non la présence de F dans ΓK(a_-), qui justifie D2. La fermeture du plateau donne ensuite son parent de rang r_b, ou le même nœud s'il n'a pas fusionné. Les garde-fous `g.rank<r_b`, ant(b) avant le plateau et att(b) après tout le plateau restent inchangés.

Sur le témoin, la descente peut prendre les deux intérieurs ZW, dont la MEB naît à1. La graine est active à41 et partage la classe d'AB à64 : cela réalise exactement cette correction.

## Portée de la revue de source

Les erreurs antérieures de `C(m,K-p)` et d'unions DSU ont été corrigées dans la spécification ; les comptes cofaces/strict_traces/branches y ont des définitions séparées. La fenêtre comprend les boules faibles, les continuations conservent leurs dates, et le §7.2 attribue les nœuds après plateau par un balayage sur les graines. Aucun raccord S3 n'étant implanté dans S2, la revue ne prétend pas avoir jugé ces sorties natives.

Aucune garde future « initial_level≤niveau précédent » n'est spécifiée ailleurs : c'est la phrase300 de preuve qui est fautive. Ne pas la porter comme un refus. Le produit existant conserve la bonne condition stricte `initial_level<λ_b` (`s2_sources_before/src/tower/forest_plateau.cpp:53`). Le balayage exige seulement que la naissance de la graine soit active (`forest_ancestor_sweep.hpp:60–62`). Aucun mode `births_only` ni journal n'est implanté dans cette capture ; le futur rattachement requiert la forêt achevée, pas les seules naissances.

## Rejeu et fermeture

```sh
python3 -B -S check_gap.py > gap_normal.json
python3 -B -O -S check_gap.py > gap_optimized.json
```

PASS : **77 gardes**, **25 présentations** de supports pour Cat2 et **20 MEB** des paires/cofaces de Γ2. Sorties identiques normal/−O. Aucun `assert` ne porte une garde. `EXPECTED.json` fixe les faits centraux ; `AFTER.json` donne les empreintes finales sans remplacer les copies initiales. `SHA256SUMS` inventorie tous les fichiers, sauf lui-même à la racine. Aucun chrono, profil numérique natif, qualification S3 ou garantie statistique n'en résulte.
