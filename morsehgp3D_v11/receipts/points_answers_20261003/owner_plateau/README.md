# Propriétaire à un plateau exact : contre-garde de l'oracle Decimal

La correction `localcontext(120)` du fichier LIVE `bench/points_reference.py`, SHA-256 `ab200babb0aa066139725a830e4c95c9eba5d05bbc904fe08d03c0907c8c0333`, conserve un défaut causal de propriétaire fermé. Ce défaut est reproduit sur la fonction réellement extraite par AST, sans moteur exécuté.

À k=2,m=1, prendre X={(0,0,0),(2,2,0),(−4,−4,0),(4,4,0)}. La copie pure exacte de `Definition` construit les naissances/parents FULL₂ aux niveaux {2,2,8,8,18}. Pour le site 0 : première lignée {0,1} à t²=2 ; rival {0,2} à q²=8, rencontre à m²=18. La date de pendaison exacte est

e=√2+√18−√8=2√2=√8.

À β=8, le parent de la première lignée couvre {0,1,3} ; la coupe fermée a déjà remplacé l'enfant {0,1}. L'oracle ab200 calcule cependant `e=rad(2)+(rad(18)−rad(8))` **un ulp Decimal sous** `rad(8)` : écart `−1E−119` à précision 120. Dans sa boucle `if rad(parent.level)<=e` (`points_reference.py:204–210`), il garde donc l'enfant mort : signature `(2,{0,1})` au lieu de `(8,{0,1,3})`.

Le même résultat est reproduit après la translation commune +4 à chaque coordonnée. Cette version {(4,4,4),(6,6,4),(0,0,4),(8,8,4)} est directement admissible u21 ; aucune conversion ou géométrie hors domaine ne crée le défaut.

La copie LIVE `points_radius.py` SHA `457b997fa7eecea92dd0f27cd29f514655dbd0ff716ce050825fc65aaf5783d1` tranche correctement : son `RValue(2,18,8).cmp_sqrt(8)` et son `cmp_level` réel donnent exactement 0. Son `ancestor_at_radius` réel, avec un premier tableau valide de remontée et sa boucle terminale, prend le parent. Il n'y a donc ici **aucun défaut démontré du helper exact**.

Le contrôle Π₃ du même nuage entre directement à t²=8, sans rival qualifié, et l'oracle Decimal rend alors le bon parent. Le défaut est établi pour l'API brute m=1, pas pour toutes les scènes de la règle retenue H^r_{k+1}.

La porte LIVE `points_gate.py` SHA `10246322812b39e206461cf2d1c576948046c422583d2c6dac3c7c5699d02f0c` compare m∈{1,k+1,k+2} (`:90`) et les signatures de propriétaires (`:108–116`). L'ajout de cette fixture à k2,m1 donnerait donc `same_date=True`, `same_owner=False`. Nous n'avons exécuté ni cette porte native ni une campagne G4.

Correction proposée : conserver le triple rationnel (t,m,q) comme autorité de la date et comparer symboliquement cette date à chaque rayon de naissance pour choisir le propriétaire. Pour préserver l'indépendance de l'oracle, l'égalité peut être certifiée par regroupement indépendant des classes de carrés, puis le signe non nul par intervalles rationnels avec refus explicite. Une augmentation fixe de précision Decimal ou une tolérance « proche du plateau » ne certifie ni les égalités ni les dates strictement voisines. La sélection du rival maximal doit obéir à la même doctrine exacte.

`check_owner.py` extrait exclusivement les fonctions/classes demandées depuis les snapshots hachés, puis recoupe la date et le parent avec Gamma en Fraction. Aucune fonction de fit ni d'export natif n'est importée. Les guards explicites restent actifs sous −O. Les deux cas (origine et translation) donnent les mêmes signatures intrinsèques et le même écart Decimal. Les copies des sources sont figées ; les modifications LIVE éventuelles sont consignées séparément dans `after.json`.

Rejeu depuis ce répertoire :

```sh
python3 -B check_owner.py > normal.json 2> normal.stderr
python3 -B -O check_owner.py > optimized.json 2> optimized.stderr
cmp normal.json optimized.json
sha256sum -c SHA256SUMS
```

Normal et −O : 42 gardes PASS, statut `PASS_EXPECTED_COUNTEREXAMPLE`, sorties identiques. C'est une preuve Python de défaut d'oracle, aucune qualification native ou matérielle nouvelle.
