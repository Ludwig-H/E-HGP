# Moments multisites : boîtes recouvrantes et contacts exacts

24 septembre 2026. Audit autonome du certificat par moments de
[`moments_multisite_precore_20260924`](../moments_multisite_precore_20260924/NOTE.md)
et de son extension aux 64 couples de coins décrite dans le
[contre-audit B R20](../CONTRE_AUDIT_B_R20_ET_TRAJECTOIRE_100MS_20260924.md).
Cadre : `phase=exploration_v9_hors_registre`, `backend=none`,
`profile=quantized_u18_input_only`, `mode=audit_math_rectangle`,
`public_status=not_claimed`. Aucun moteur, reçu LiDAR, chrono FULL ou G4.

[`check.py`](check.py) calcule `N,Z,Q,H,W,X,A3,A4` en entiers Python,
recoupe `H` par la somme directe des sites et `C` par sa seconde expansion,
puis exécute les **64 couples croisés de coins**, répétitions comprises si
une boîte dégénère. Pour chaque petite boîte, il calcule séparément tous les
couples de points entiers et exige exactement le même verdict de rectangle
sur chaque voie. Une paire virtuelle `a=b` a `D=0` et ne peut pas être une
arête propriétaire. Les conditions de bord et les résultats attendus sont
vérifiés avec des exceptions explicites, jamais avec `assert`.

| Fixture K5 | Résultat aux coins | Ce qu'elle exerce |
| --- | --- | --- |
| `endpoint_guard_both` | q3 + q4 | 64 coins distincts, 729 couples entiers, un garde reprend l'ID et la coordonnée de `a` à un coin ; sa marge est zéro |
| `q4_only` | q4 | 64 coins distincts, 729 couples entiers, seuils et disques indépendants |
| `q3_only` | q3 | 64 coins distincts, 729 couples entiers, réciproque de la ligne précédente avec 100 sites distincts |
| `overlap_corner_D0` | aucune | boîtes qui se recouvrent, `D=0` à certains coins, mais arêtes individuelles certifiables |
| `overlap_interior_D0` | aucune | `D=0` seulement dans la grille intérieure, pas aux coins, et arêtes individuelles certifiables |
| quatre contacts | selon la voie | `A3=0`, `A4=0`, puis `A3²=12X` ou `A4²=8X` avec `A>0` : l'égalité ne crédite rien |

Depuis la racine de ce clone :

```sh
python3 -B morsehgp3D_v9/audits/b_moments_rectangle_edges_20260924/check.py
python3 -B -O morsehgp3D_v9/audits/b_moments_rectangle_edges_20260924/check.py
```

Les réussites positives sont des **certificats de ces boîtes et blocs fixes**.
L'énumération finie cherche une contradiction ; la suffisance générale des
64 coins vient de la concavité séparée de `Aj−√cj|C|` en `a,b`, pas de ce
script. La sélection des blocs, les IDs d'une entrée réelle, le coût et la
fermeture du chemin S2→S3 restent hors de cette vérification.
