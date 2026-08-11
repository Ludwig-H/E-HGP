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
`A>0 \Leftrightarrow \Phi<0` découlent des arguments ci-dessus. Leur réception
logicielle et leurs fixtures appartiennent au verdict live; une vérification
hors bande non archivée n'est pas une autorité.

Bornes u16 : `\lvert u\rvert,\lvert v\rvert\leq65535`, donc
`\lvert uv-v^{2}\rvert<2^{33}` par axe et `\lvert A\rvert<2^{35}` — `i64`
avec marge.

## 4. Reçus d'échelle, binaire figé

L'objection de provenance de l'audit est reçue : le tableau précédent
mélangeait deux binaires et n'attachait aucun journal. Il est remplacé par une
matrice `12 500/25 000/50 000` exécutée sur un **binaire figé**, dont la sortie
brute et les exposants dérivés sont archivés :

| objet | SHA-256 |
| --- | --- |
| [`dual_scale_counters_raw.txt`](../receipts/yao48_dual_20260811/dual_scale_counters_raw.txt) | `a19ac56290e3262f9f1fc9b05e37952688f3a26db1f80fb989325a53292ce1b1` |
| [`dual_exponents_derived.txt`](../receipts/yao48_dual_20260811/dual_exponents_derived.txt) | `264dd91eeb96a4243558e3f84322fe1db7d004e1d3e15156ee0af5e973c8b349` |
| binaire figé de cette matrice | `0fce8ec7c91152d2b6b1bb4ca6e8401f2081528bbbcc42c61987a7a49260b071` |

Sur les trois familles structurées, tous les compteurs de **sortie** et de
**classification** passent la gate `1,35` :

| famille | survivantes | boîtes | tests | census | coupe à 50 k |
| --- | --- | --- | --- | --- | ---: |
| terrain | 1,05 puis 1,03 | 1,10 puis 1,16 | 1,06 puis 1,05 | 1,02 puis 1,01 | 99,66 % |
| scanline simple | 1,04 puis 1,03 | 1,16 puis 1,13 | 1,07 puis 1,07 | 1,01 puis 1,01 | 99,70 % |
| multiécho | 1,12 puis 1,11 | 1,24 puis 1,20 | 1,17 puis 1,17 | 1,01 puis 1,01 | 99,53 % |

L'ordonnance par chambres du même binaire donnait `1,40` à `1,83` sur ces
mêmes compteurs. **Un compteur reste rouge** : les visites de la frontière
ambiguë (`1,50` puis `1,93` sur `terrain`), c'est-à-dire le TRAVAIL de
recherche et non la sortie. La gate n'est donc pas entièrement verte.

Deux politiques de travail ont été mesurées puis tranchées sur les mêmes
octets, à sorts et census identiques : un ordre best-first par majorant est
**rejeté** (173 millions de visites contre 122, et trois fois le temps) ;
l'exploitation ponctuelle des feuilles ambiguës est **conservée** (survivantes
`996 438` puis `674 986`, visites `122` puis `95,5` millions à `12 500`
terrain). Une matrice figée de cette dernière est en cours et remplacera les
chiffres ci-dessus lorsqu'elle sera complète.

## 5. Ce que cette note ne prétend pas

Elle ne revendique ni une architecture reçue, ni un temps qualifiable. La
rampe du commit `c70974e` contient les trois tailles pour les trois familles
structurées, mais `uniform` est incomplète. Les compteurs de sortie et du
classifieur passent sous `1,35`; `dual_witness_visits` reste rouge deux fois
dans chacune des trois familles. La gate de travail globale est donc **NO-GO**,
même si le résiduel est nettement meilleur. La portée et la provenance sont
auditées dans
[`AUDIT_RECU_YAO48_DUAL_C70974E_20260811.md`](AUDIT_RECU_YAO48_DUAL_C70974E_20260811.md).

Le census matérialisé, les sources q3/q4, le resolver, le fold et le payload
officiel restent hors de cette lane.

GCP non utilisé pour cette note.
