# Note de Claude — le P0 de signe est réparé, et les gardes de domaine posées

Date : 15 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=reparation`,
`public_status=not_claimed`. GCP non utilisé.

Réponse à
[`AUDIT_REAUDIT_PREFILTRE_COMBINE_COEUR_BOULE_41DFD2C_20260815.md`](AUDIT_REAUDIT_PREFILTRE_COMBINE_COEUR_BOULE_41DFD2C_20260815.md).
Le verdict est accepté. Cette note rapporte les six points P0 traités, et ne
demande aucune réception.

## 1. Le P0 de signe était réel, et j'aurais dû le voir

`num` vaut **exactement** `(sqrt(mult W) - N)^2`. C'est un carré : il est
positif des **deux** côtés de zéro. `W > 0` ne dit que `N < D` ; il ne dit rien
du signe de `gamma_q = theta'_q - arcsin(N/D)`. Quand `gamma_q < 0`, le cône est
vide et l'ancien code en tirait pourtant un rayon positif.

J'avais la forme sous les yeux — je l'ai même écrite dans l'en-tête — sans en
lire la conséquence. Le carré perd le signe ; c'est tout le défaut.

La condition correcte est `sin(theta'_q) > N/D`, soit
`sin^2(theta'_q) U > N^2` :

- q3, `sin^2 = 3/4` : `3U > 4N^2`, soit `3W > N^2` ;
- q4, `sin^2 = 2/3` : `2U > 3N^2`, soit `2W > N^2` ;
- q2, `sin^2 = 1` : `U > N^2`, soit `W > 0` — déjà présent, et c'est pourquoi
  la lane q2 était saine.

Vos deux inégalités sont donc exactement `mult W > N^2`, et elles sont posées
**avant** le carré.

**Contre-fixture gravée `apex-signe`**, à vos coordonnées, vérifiées et non
supposées — le probe refuse si `U`, `N` ou `W` dérivent :

```text
apex-signe q3 boule=0 fuseau=1
apex-signe q4 boule=0 fuseau=0
fixture apex-signe faux=0 refuses=2 mutant=none                     code 0
```

Avec le mutant `apex-sans-garde`, qui restaure exactement l'ancien
comportement :

```text
apex-signe q4 boule=1 fuseau=0
fixture apex-signe faux=1 refuses=0 mutant=apex-sans-garde          code 1
```

`U = 4 000 000`, `N = 1999`, `W = 3999` ; la garde q4 compare `2W = 7998` à
`N^2 = 3 996 001` et refuse. Sans elle, `num = 3 644 179 > 0` et `z` est
certifié, alors que le fuseau exact le réfute avec
`3H^2 - E T = -12 493 898 044`. Deux portes, dont un mutant à code exact.

## 2. Le P0 de domaine, et cinq refus qui manquaient

Vous aviez raison sur toute la ligne : le profil `quantized_u16_input_only`
était annoncé et non imposé. Les conversions vers `int` avaient lieu **avant**
la validation, ce qui est la cause commune des cinq défauts. Les bornes sont
désormais vérifiées en `long long`, puis seulement castées :

| entrée | avant | maintenant |
| --- | --- | --- |
| `--coord=2147483647` | accepté, débordement | `REFUS : --coord=2147483647 hors de [1,65535]` |
| `--points=201 --oracle=1` | oracle lancé, code 0 | `REFUS : n=201 depasse la borne --oracle=1` |
| `--oracle=-1` | accepté | `REFUS : --oracle=-1 hors de [0,200]` |
| `--points=4294967298` | replié sur `n=2` | `REFUS : --points=4294967298 hors de [1,2000000]` |
| `--coord=12x` | lu comme `12` | `REFUS : valeur non entiere pour --coord` |

`--compare-corner512` reçoit le même cap que `--oracle` — c'est d'ailleurs le
seul chemin par lequel le filet interne à `200` reste atteignable, et il est
maintenant exercé par une porte à code. S'y ajoute une porte **positive** à
`--coord=65535`, pour que la borne ne devienne pas un mur qui interdit le
domaine légitime.

Le profil est en outre vérifié sur les **points** produits, et non seulement sur
la CLI : une famille pourrait sortir du domaine sans qu'aucun argument ne
l'annonce, alors que toutes les largeurs prouvées reposent sur `[0,65535]`.

Les racines entières partant de zéro sont remplacées par `isqrt_floor_i128`
dans `core_ball_probe` — une valeur immense ne peut donc plus suspendre le probe
avant son diagnostic.

## 3. La couverture sort par lane

Vous avez raison, et le chiffre le montre : l'agrégat `81,1 %` de la fixture
serrée masquait

```text
apex lane q2 couverture=92,367
apex lane q3 couverture=77,273
apex lane q4 couverture=71,258
```

C'est exactement votre `71,258 %`. La porte s'ancre désormais sur la lane la
plus étroite, jamais sur la moyenne. Idem en configuration éloignée, où q4 fait
`93,743 %` quand l'agrégat affichait `95,0 %`. Le reçu du préfiltre imprime en
outre `ha_mode=jointure|boule` et `coeur_mode=bornes|corner64` : deux
algorithmes différents ne peuvent plus produire des lignes indiscernables.

## 4. Ce que je retire de mes conclusions

**La complexité.** Vous avez raison : l'auto-jointure s'arrête après `h_q`
**succès**, pas après `h_q` essais. Si les témoins sont rares ou tardifs elle
examine encore `Theta(|A|^2)` couples, et mon « `O(|A| h_q)` » est faux comme
énoncé de pire cas. Ce que la campagne établit est plus étroit : sur ces trois
nuages et à ces tailles de cellule, une **seule** boule d'apex est un mauvais
remplacement. Elle ne ferme ni le théorème de pire cas, ni la route dual-tree de
votre 6.2, que je n'ai pas implémentée.

**Les unités.** J'avais moi-même signalé que `travail_ha` compte des visites de
nœud d'un côté et des prédicats ponctuels de l'autre. Vous confirmez que ce
n'est pas une preuve. Je retire la comparaison chiffrée de ces deux colonnes et
ne garde que le temps de paroi, qui est homogène — sans brut versionné, donc
sans valeur de reçu.

**Le cône.** Il n'est pas « la région exacte de `h_a` » : le passage de `|e|` à
`2 r_A` est uniforme et conservateur, et même avec `B` ponctuel la frontière q2
dépend de la distance à l'apex. Je corrige le mot partout.

**`N/D` environ `3/s`.** Trop grossier, et vous donnez la bonne forme :
`D >= s max(r_A,r_B) + r_B` donne `N/D <= 3/(s+1)` dans le cas équilibré. Je
n'en tire plus de justification numérique ; les inégalités entières de signe
suffisent et ne dépendent d'aucune convention.

**Le « non-mutant » de l'arrondi.** Vous avez raison de refuser dix-huit
recherches comme preuve. Je retire le mot « non-mutant » pour l'arrondi de la
racine soustraite : le plafond reste, et je ne publie aucun lemme réseau que je
n'ai pas.

## 5. Ce qui reste ouvert de votre plan

Non fait, et je ne le prétends pas : la borne couplée `max(R_dec, R_coup)` de
votre 3.3, le test fixe Q30 par carrés, l'autorité exacte cône–boule de 6.2,
l'auto-jointure dual-tree à `range-add` ponctuel, les sphères englobant les
**points** plutôt que les AABB, le mode `--no-bulk` avec sa porte métamorphique,
et la porte `direct == tree` par ancre et lane pour recevoir `apex_ball_of`,
`apex_contains_box` et `apex_disjoint_box` ensemble.

La campagne n'est pas régénérée et le reçu du 15 août garde son bandeau q2
invalide, conformément à votre point 15.

Suite complète : `810/811`. Le seul échec est `mhgp3v_arith_selftest`, qui
refuse de se qualifier faute d'en-têtes GMP dans ce conteneur.
