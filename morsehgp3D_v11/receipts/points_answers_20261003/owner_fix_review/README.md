# Revue du correctif de propriétaire fermé — 3 octobre 2026

Le correctif Python LIVE `points_reference.py` SHA-256 `2f05ceb420913e2bb42569c5da193609395b3812ea253d22a1d7d2d1281e9ccb` ferme le défaut causal conservé dans `../owner_plateau`. Le rival maximal et le propriétaire à la date passent maintenant par la routine rationnelle exacte `two_roots_sign` ; Decimal ne décide plus ces deux opérations.

Les sources ont été copiées avant lecture/exécution. Le HEAD développeur reste `8df2025ab0074403f9dd94a303d5f367424b8561` ; ces fichiers sont des états LIVE séparés. `points_gate.py` est figé à `d45bc9db8336038b8fc2a4b0aa81869be0eb6a7d9302de8b3d6801d83eb0089a`, `points_radius.py` à `457b997fa7eecea92dd0f27cd29f514655dbd0ff716ce050825fc65aaf5783d1`. Ils ne reçoivent aucune qualification native/G4 par cette revue.

Le témoin X={(0,0,0),(2,2,0),(−4,−4,0),(4,4,0)}, k2,m1, a la date exacte e=√2+√18−√8=√8. L'oracle corrigé choisit bien le parent vivant de niveau 8 couvrant {0,1,3}. Le même résultat est vérifié après translation commune +4 (admissibilité u21), inversion des indices et homothétie 3. Les profils m=1,3,4 et les précisions Decimal 8,120,200 donnent les mêmes propriétaires exacts. Pour tous les sites, le rival/date retenu et l'ancêtre sont recoupés par un calcul indépendant à classes de carrés rationnelles, puis intervalles isqrt ; ni la routine du banc ni celle de l'oracle ne décide l'attendu.

La routine nouvelle compare A=√a₁+√a₂ et B=√b₁+√b₂, pour rationnels non négatifs. Comme A,B≥0, leur différence a le signe de A²−B²=c+2(√p−√q), avec c=a₁+a₂−b₁−b₂, p=a₁a₂, q=b₁b₂. Si les deux termes s'opposent, le signe revient à celui de c fois le signe de c²−4(√p−√q)²=g+8√(pq), où g=c²−4(p+q). Pour g<0, comparer 64pq à g² est légitime ; le cas g=0,pq=0 conserve l'égalité. C'est bien ce que fait le code. Dans la sélection, `(meet,old_q,new_q,old_meet)` compare les deux différences de racines ; dans la remontée, `(t,meet,q,birth)` compare e au rayon de naissance avec `>=0`, donc une égalité prend le parent fermé.

Contrôles indépendants bornés : 4 349 comparaisons de deux racines contre deux (exhaustif a,b,c,d∈{0..7}, 250 rationnelles, et l'égalité du témoin avec voisins stricts ±2⁻¹⁸⁰). Les trois derniers scalaires sont dans 192 bits ; ce ne sont pas trois nouvelles fixtures géométriques natives. Tous les propriétaires et dates des quatre transformations du nuage sont recoupés à trois précisions. Total : **20 831 gardes PASS**, sorties normales et −O identiques. Aucun nouvel échec n'est observé dans ce périmètre.

La porte figée confronte déjà les dates symboliques et signatures de propriétaire, ainsi que les comparateurs indépendants. Cette revue n'exécute pas la porte native ni une campagne. Les tableaux/degrés de liberté d'une entrée, un prototype Python et un port natif restent des périmètres distincts.

Rejeu depuis ce répertoire :

```sh
python3 -B check_fix.py > normal.json 2> normal.stderr
python3 -B -O check_fix.py > optimized.json 2> optimized.stderr
cmp normal.json optimized.json
sha256sum -c SHA256SUMS
```

Le script extrait les fonctions réellement hachées par AST, importe uniquement le petit oracle `Definition` figé et reconstruit ses attendus en Fraction/isqrt. Les guards explicites restent actifs sous −O. `after.json` consigne les empreintes LIVE finales et éventuelles dérives sans changer les snapshots. Aucune compilation, exécution native, fit ou opération GCP.
