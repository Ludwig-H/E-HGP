# Note de Claude — la traversée duale `Q--W` de la lane q2

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cette note documente la troisième voie recommandée par
[`AUDIT_REPONSES_G4_Q2_YAO1_20260811.md`](AUDIT_REPONSES_G4_Q2_YAO1_20260811.md)
(réponse 2), telle qu'elle est implémentée sur le worktree. Elle ne prononce
aucune admission : le statut appartient au verdict live.

## 1. Le prédicat et son équivalence

Pour une ancre `p`, une cible `q` et un témoin `w`, poser

$$A(p;q,w)=(q-p)\mathbin{\cdot}(w-p)-\left\Vert w-p\right\Vert^{2}.$$

Alors `A>0` équivaut exactement à `(w-p) dot (w-q)<0`, c'est-à-dire à
« `w` est strictement intérieur à la boule diamétrale de `(p,q)` ». Dix témoins
de `PointId` distincts satisfaisant `A>0` tombstonent la paire `(p,q)` au
contrat H0 `K=10`. L'égalité `A=0` est un contact de coquille et ne crédite
rien.

## 2. Séparabilité et minimum aux quatre coins

En posant `u=q-p` et `v=w-p`,

$$A=\sum_{d}\left(u_dv_d-v_d^{2}\right),$$

chaque terme ne dépend que de `(u_d,v_d)` : la forme est **séparable par
axe**. Sur un rectangle `[u_{lo},u_{hi}]\times[v_{lo},v_{hi}]`, la fonction
`u v-v^{2}` est linéaire en `u`, donc son minimum en `u` est à une extrémité,
et concave en `v`, donc son minimum en `v` est aussi à une extrémité. Le
minimum exact est donc atteint à l'un des **quatre coins** :

$$\min A=\sum_{d}\min_{u\in\lbrace u_{lo},u_{hi}\rbrace,\ v\in\lbrace v_{lo},v_{hi}\rbrace}\left(uv-v^{2}\right).$$

Si `min A>0` sur une boîte de cibles `Q` et un nœud témoin `W`, alors **toute**
feuille de `W` est strictement intérieure à la boule diamétrale de **toute**
cible de `Q`. Une antichaîne de nœuds `W` de plages disjointes et de masse
totale dix tombstone `Q` entier, sans chambre ni banque.

Le majorant symétrique sert au rejet. Le sommet de `u v-v^{2}` en `v` est
`v=u/2`, de valeur `u^{2}/4` ; hors de l'intervalle le maximum est aux bords.
Le majorant entier prend `\lceil u^{2}/4\rceil` au sommet. Si `max A\leq0`,
aucun point de `W` n'est témoin d'aucune cible de `Q` et le sous-arbre est
écarté définitivement.

## 3. L'héritage exact des deux verdicts

C'est la propriété qui supprime le rescan de racine que l'audit refuse. Pour
`Q'\subseteq Q`, l'intervalle `U'` est inclus dans `U` sur chaque axe, donc :

- un nœud **accepté** le reste : `\min A'\geq\min A>0` ;
- un nœud **rejeté** le reste : `\max A'\leq\max A\leq0`.

Seuls les nœuds **ambigus** du parent sont réexaminés par l'enfant. La
traversée porte donc, par nœud de cibles, une antichaîne héritée et une
frontière ambiguë héritée ; la file de travail descend jusqu'à ce que chaque
branche soit acceptée, rejetée, ou réduite à une feuille ambiguë, et seul ce
résidu est transmis. La frontière ne repart jamais de la racine.

Séparabilité, minimum aux quatre coins, majorant, héritage et l'équivalence
`A>0 \Leftrightarrow \Phi<0` ont été vérifiés hors bande en arithmétique
exacte avant gravure.

Bornes u16 : `\lvert u\rvert,\lvert v\rvert\leq65535`, donc
`\lvert uv-v^{2}\rvert<2^{33}` par axe et `\lvert A\rvert<2^{35}` — `i64`
avec marge.

## 4. Effet mesuré, à binaire et nuage identiques

À `12 500` points, famille `terrain`, graine `20260810`, même binaire, même
`leaf_size`, census de sortie **identique** (`253 129` records) et juge borné
vert :

| ordonnance | survivantes | visites témoins | masse coupée | phase locale |
| --- | ---: | ---: | ---: | ---: |
| chambres Yao48 (banques 48) | 4 543 219 | — | 94,2 % | 16,5 s |
| duale, rescan racine | 1 057 788 | 261 058 042 | 98,6 % | 22,9 s |
| duale, frontière persistante | 996 438 | 122 022 307 | 98,7 % | 11,3 s |

Ces trois ordonnances rendent les **mêmes sorts** : la porte d'invariance des
politiques compare, sur les mêmes octets, budgets minimal et ample, mode
antichaîne et mode dual, et exige l'égalité de tous les sorts de tombstone et
de tous les agrégats de census. Une politique de travail accélère ; elle ne
décide jamais.

## 5. Ce que cette note ne prétend pas

Elle ne revendique ni une pente admise, ni une architecture reçue, ni un
temps qualifiable. La matrice `12 500/25 000/50 000` sur binaire figé est en
cours ; seuls ses exposants peuvent décider de la gate `1,35`, et seul le
verdict live peut les recevoir. Le classifieur terminal, le census matérialisé,
les sources q3/q4, le resolver, le fold et le payload officiel restent hors de
cette lane.

GCP non utilisé pour cette note.
