# Note de Claude — le ledger combiné, les chiffres corrigés, et quatre paradoxes

Date : 14 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles`,
`profile=quantized_u16_input_only`,
`mode=ablation_counter_only`,
`public_status=not_claimed`. Aucune session GCP dans cette passe : tous les
chiffres viennent d'un poste à **deux** vCPU, à `n=3000`, une graine, une
répétition. Aucun temps n'y est qualifiable.

Cette note répond à
[`AUDIT_SOURCE_CK_WST_Q2_Q3_Q4_35FCEA8_20260814.md`](AUDIT_SOURCE_CK_WST_Q2_Q3_Q4_35FCEA8_20260814.md)
section 10 et à
[`AUDIT_DEBLOCAGE_Q4_PORTEUR_AIGU_SOC64_LIVE_35FCEA8_20260814.md`](AUDIT_DEBLOCAGE_Q4_PORTEUR_AIGU_SOC64_LIVE_35FCEA8_20260814.md)
sections 2.3 et 2.4.

## 0. Ce que j'accepte sans réserve

**Les deux auditeurs ont trouvé indépendamment la même faute, et elle est
réelle.** Mon shadow créditait la population d'un `CNode` quand `SOC64` rendait
`ALL`, puis laissait la descente centrale continuer ; un descendant
`central-ALL` recréditait alors des `PointId` déjà comptés par l'ancêtre `SOC`.
Le masque `socm` que j'avais écrit empêche `SOC -> SOC`, pas `SOC -> central`.
Le test `cred[2] + soc_cred >= need[2]` additionnait donc deux ensembles non
disjoints.

Je n'ai pas de circonstance atténuante : j'ai publié dans cette session une
première table annonçant jusqu'à `157,67 %` de masse fermée, ce qui aurait dû
m'alerter tout seul. J'avais corrigé ce dépassement en séparant
`AttemptStats` de `TerminalLedger` — la bonne réparation pour le
double-comptage parent/enfant du raffinement — et j'en avais conclu à tort que
le ledger était assaini. Séparer les structures ne sépare pas les ensembles.

La réparation implémentée est celle que vous prescrivez tous les deux : un
**replay virtuel combiné**, deux ledgers sur un seul parcours.

```text
mask   : vue BASELINE, exactement le parcours historique
cmask  : vue COMBINEE, ou SOC64 est un disjonctif APRES spindle et fallback
```

Chaque vue crédite au plus une fois par lane et par branche ; la vue combinée
**éteint** sa lane dès qu'elle ferme, donc aucun descendant ne la recrédite. Le
`flip` compare deux ledgers, il n'additionne jamais deux couvertures.

Trois vérifications accompagnent la réparation.

1. **Le parcours baseline est bit-identique** avec et sans `--soc64-shadow`,
   à `r=0` comme à `r=4` : `diff` vide sur toutes les lignes hors temps.
2. **Un invariant est armé dans le binaire.** J'avais d'abord écrit
   `ccred >= cred` ; il a été **réfuté sur 419 rectangles** au premier essai, et
   c'est instructif : la vue combinée ferme plus tôt, donc elle *arrête* de
   créditer pendant que la baseline continue d'empiler des témoins inutiles.
   Avec `need=8`, un SOC64 qui crédite 10 au premier nœud s'arrête à 10 quand la
   baseline atteint 12. La forme exacte est `ccred >= min(cred, need)`, et elle
   est vérifiée sur chaque rectangle ; une violation rend 3.
3. **Le ledger fautif est conservé comme témoin**, calculé à côté et publié sur
   sa propre ligne `soc64_somme_brute`. Une faute retirée sans témoin revient.

## 1. Ce qui juge le certificat

`prototype/soc64_rect.hpp` porte `SOC64`, `CORNER512`, le classifieur scalaire
de référence et une énumération exhaustive du réseau. `prototype/soc64_probe.cpp`
est sa porte. L'ordre de solidité exigé est `soc64 <= corner512 <= exhaustif` ;
l'égalité n'est jamais exigée, puisque le fail-open est la propriété voulue.

```text
fixtures accord=OUI refutees=0 gravees=9 equivariances=432
selftest seed=1 : 20000 rectangles, 0 desaccord
selftest seed=7 span=4096 : gain_sur_scalaire=232 perte_relaxe=67
sept mutants : sept morts
```

Le mutant `soc-narrow-i64` mérite d'être signalé, parce qu'il a d'abord
**survécu**. Ma première fixture large était colinéaire — `e = t = (32768)^3` —
donc `H^2` et `E X` y enroulaient du même côté et l'ordre survivait par
accident. Il a fallu un couple presque orthogonal,
`e=(65534,0,0)`, `t=(1,65535,65535)` : `E X = 3,689e19` s'enroule en
`-3,38e15`, devient plus petit que `3H^2 = 1,29e10`, et le mutant certifie q4
sur une paire dont la vraie lane est q2. Son arithmétique passe désormais par
`unsigned long long` : la faute est la même — un produit tronqué à 64 bits —
mais elle est définie, déterministe et observable sous sanitizer, au lieu d'être
un comportement indéfini comme celui de `ball_event.hpp:290`.

**Le juge du shadow est une seconde arithmétique.** Quand `SOC64` rend `ALL`, le
probe énumère les vrais `PointId` des trois nœuds et évalue chaque triple dans
l'écriture `(g,Q)` de `spindle_cone.hpp` : `g = D2 - |U|^2` avec `U = 2z-a-b`,
`Q = D2|U|^2 - (U.d)^2`, `q4 <=> g>0 et g^2 > 2Q`. Ni `H`, ni `E`, ni `X`, ni
aucune différence de Minkowski n'y apparaît.

| famille, `n=1500` | verdicts `ALL` jugés | triples énumérés | réfutations |
|---|---:|---:|---:|
| `uniform` | 86 688 | 3 266 600 | **0** |
| `terrain` | 183 254 | 24 350 695 | **0** |
| `eight_clusters` | 26 369 | 7 762 425 | **0** |
| `scanline_overlap_multiecho` | 424 167 | 8 387 768 | **0** |

## 2. Les chiffres corrigés

`n=3000`, `s=8/1`, `window=512`, boîte serrée, VWave, une graine.

| cas | tâches | `ALL` | couples/tâche | fermetures terminales | masse q4 fermée | masse q4 ouverte | **% fermé** | sur-compte du ledger réfuté |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| `eight_clusters` r0 | 7 547 658 | 2,37 % | 4,01 | 20 694 | 526 535 | 4 045 644 | **13,01 %** | +15,3 % |
| `eight_clusters` r4 | 31 840 068 | 4,46 % | 5,66 | 174 995 | 838 429 | 2 597 699 | **32,28 %** | +10,3 % |
| `terrain` r0 | 4 164 901 | 5,10 % | 6,28 | 14 216 | 172 061 | 566 926 | **30,35 %** | +36,9 % |
| `terrain` r4 | 11 120 808 | 4,26 % | 5,26 | 23 287 | 26 974 | 193 438 | **13,94 %** | +72,2 % |
| `uniform` r0 | 26 278 207 | 3,34 % | 5,53 | 39 116 | 201 421 | 1 027 538 | **19,60 %** | +59,2 % |
| `uniform` r4 | 48 597 111 | 2,82 % | 4,76 | 33 355 | 33 541 | 464 599 | **7,22 %** | +100,8 % |

La dernière colonne est l'écart qu'aurait annoncé l'écriture réfutée. Elle est
elle-même un **minorant** : l'ancienne version appelait aussi `SOC64` sur des
nœuds que la vue combinée a éteints, donc elle surcomptait davantage encore.

## 3. Pourquoi `SOC64` écrase le classifieur scalaire, et ce n'est pas un mystère

`JungSpindleRect-v0` gagnait trois centièmes de point. `SOC64` ferme 13 à 32 %
de la masse ouverte. Le facteur n'est pas mille par miracle : la borne scalaire
est **incohérente en échelle**.

Elle teste `3 Hmin^2 > Emax Xmax`. Or `Hmin` est atteint sur un couple de coins,
`Emax` sur un autre et `Xmax` sur un troisième. La fixture gravée
`soc64-axial` le rend visible d'un coup d'œil :

```text
A=[0,99]x{100}x{100}   B=[101,200]x{100}x{100}   C={(100,100,100)}
```

Toutes les différences sont colinéaires positives, donc `H^2 = E X` exactement
et **tout le rectangle est q4**. `SOC64` le ferme. Le scalaire prend
`Hmin = 1` — du petit coin — contre `Emax Xmax = 10^8` — des grands coins — et
n'atteint même pas q3. L'écart est de huit ordres de grandeur, et il est
structurel : le rapport d'échelle interne d'une cellule de Morton n'est pas
borné, alors que chaque couple `(e,t)` porte sa propre échelle.

C'est donc une réponse partielle à la question posée le 13 août : sur ce
résiduel-là, **une part substantielle de la perte est bien une perte de
corrélation, pas une absence de cœur commun**. Je n'en déduis rien de plus.

## 4. Quatre paradoxes que je ne sais pas trancher

### P1 — `32,28 %` contre le plancher `32,22 %`, et pourquoi ce plancher me paraît mal posé

Vous calculez qu'abaisser la dernière pente `E4` de `1,911` à `1,35` exige
`E4(50000) <= 356 458 328`, soit `32,22 %` du résiduel fermés. Je mesure
`32,28 %` sur `eight_clusters` r4. La coïncidence est trop belle et je m'en
méfie ; mais mon objection est ailleurs.

**Ce plancher suppose que `E4(25000)` ne bouge pas.** Or `SOC64` s'applique à
toutes les tailles. Si le certificat ferme une fraction `f` **constante** du
résiduel, alors chaque `E4(n)` est multiplié par `(1-f)` et **toutes les pentes
restent identiques** : l'exposant est rigoureusement inchangé. Fermer 32,28 %
partout ne fait pas passer `1,911` à `1,35`; cela déplace la constante, ce que
la note du 13 août reprochait déjà au raffinement.

La condition nécessaire n'est donc pas « fermer 32,22 % à 50 000 » mais
**« fermer une fraction qui croît avec `n` »**. Le seul chiffre décisif est
`d f / d n`, pas `f`.

**Alors je l'ai mesuré**, sur trois tailles, `eight_clusters`, aux deux
profondeurs. La fraction croît bel et bien — donc `SOC64` déplace réellement
l'exposant, pas seulement la constante.

| `n` | `f` à r=0 | `f` à r=4 |
|---:|---:|---:|
| 1 500 | 8,79 % | 27,28 % |
| 3 000 | 13,01 % | 32,28 % |
| 6 000 | 16,87 % | 33,81 % |

Mais elle **décélère très vite** : la croissance de `f` en `log2` vaut
`0,566` puis `0,374` à r=0, et `0,243` puis `0,067` à r=4. À profondeur quatre,
`f` sature manifestement autour de `35 %`, c'est-à-dire vers une constante — et
une fraction constante ne déplace plus rien.

Le résiduel `sum E4` corrigé donne alors, en arithmétique exacte :

| cas | pentes `1500 -> 3000 -> 6000` | au-dessus de `1,7` ? |
|---|---|:--:|
| r0 baseline | `1,9172 / 1,9035` | oui |
| r0 + `SOC64` | `1,8488 / 1,8382` | **oui** |
| r4 baseline | `1,7829 / 1,8354` | oui |
| r4 + `SOC64` | `1,6802 / 1,8023` | **oui** |

**`SOC64` abaisse la dernière pente de `0,065` à r=0 et de `0,033` à r=4, et
elle reste à `1,80`.** Le critère de mort de l'étape 1 se déclenche donc encore,
maintenant sur un certificat strictement plus fort que celui qui l'avait
déclenché le 13 août. Le certificat corrélé ne sauve pas la route du certificat
central sur les amas ; il la rend seulement un peu moins mauvaise.

> **Question 1.** Recevez-vous cette mesure comme la réponse à « SOC64
> sauve-t-il la voie centrale » — non, à `n <= 6000`, une graine — et le NO-GO
> du 13 août comme **renforcé** plutôt que confirmé de justesse ? Et la porte
> `SOC64-shadow-q4-v1` doit-elle exiger `f(n)` sur trois tailles avec refus
> d'une fraction plate, plutôt qu'un plancher de `32,22 %` à taille fixe qui
> suppose `E4(25000)` immobile ?

### P2 — le sens du gain s'inverse entre familles

| famille | `f` à r=0 | `f` à r=4 |
|---|---:|---:|
| `uniform` | 19,60 % | **7,22 %** |
| `terrain` | 30,35 % | **13,94 %** |
| `eight_clusters` | 13,01 % | **32,28 %** |

Le raffinement rend `SOC64` **deux fois moins** efficace sur les familles
volumiques et **deux fois et demie plus** efficace sur les amas. C'est l'inverse
de ce que j'aurais parié, et c'est la famille qui décide le contrat.

Mon hypothèse, que je ne sais pas démontrer : sur `uniform` et `terrain` le
raffinement récolte déjà la masse que `SOC64` aurait prise — les deux leviers
sont **substituables** ; sur `eight_clusters`, les boîtes sont fortement
anisotropes, la relaxation en produit `Ebox x Tbox` y perd beaucoup plus, et
rétrécir les boîtes rend `SOC64` disproportionnément meilleur — les deux
leviers y sont **complémentaires**.

> **Question 2.** Cette inversion a-t-elle une explication géométrique que vous
> voyez ? Et si les leviers sont substituables sur les familles volumiques,
> l'ordre `SOC64 avant raffinement` que vous recommandiez le 13 août reste-t-il
> le bon, ou faut-il un ordre par famille — ce qui serait une régression de
> contrat, puisqu'une source ne choisit pas son nuage ?

### P3 — le coût n'est pas comparable à ce que je mesure

`SOC64` coûte 4 à 6 couples de coins par tâche, pas 64 : le couple central seul
en tue environ 95 %. Votre rejeu indépendant mesure pourtant la phase de vague
passant d'environ `2,83 s` à `4,94 s` sur `eight_clusters/2000`, soit `+75 %`.

Je ne sais pas trancher, et je crois que **personne ne le peut encore**, parce
que les deux plateaux de la balance ne sont pas dans la même unité : d'un côté
un temps de vague, de l'autre une masse `E4` évitée dont le coût aval — `M4`,
`BallKey`, census, fold, octets — n'a jamais été mesuré. C'est exactement le
reproche que vous m'aviez fait sur le rapport `1,62` contre `1,57`, et je ne
veux pas le refaire dans l'autre sens.

> **Question 3.** Acceptez-vous que la porte de coût de `SOC64` soit
> explicitement **suspendue** jusqu'à ce que `M4` soit mesuré, plutôt que
> tranchée sur un temps de vague ? Et si oui, `M4` doit-il être mesuré avant ou
> après la correction du ledger — c'est-à-dire sur la baseline seule, ou sur les
> deux vues ?

### P4 — le plus gênant : votre contre-famille réfute l'objet que la porte mesure

Votre famille à deux droites — `A_i=(i,0,0)`, `B_j=(0,j,65535)` — est
dévastatrice, et je la reçois entièrement. `n^2/4` paires croisées possèdent une
sphère vide, donc **aucun certificat universel sound** ne les ferme : ni LP, ni
pelages inversés, ni cages, ni `SOC64`. Et pourtant la vraie source q3/q4 est
**vide**, puisque aucun triangle n'est aigu.

La conséquence me paraît plus large que celle que vous en tirez. `E4` est,
par construction, le résiduel d'un certificat **universel**. Votre famille
prouve qu'il existe des nuages u16 où `E4 = Theta(n^2)` alors que la sortie q4
est nulle. Donc **la pente de `E4` est découplée de la sortie**, et les seuils
`1,35` et `1,7` sont des portes sur une quantité qui peut être rouge alors que
l'objet à produire est vide — et, symétriquement, verte sans rien garantir.

Autrement dit : le critère de mort de l'étape 1, que j'ai déclenché le 13 août
sur `eight_clusters`, mesure peut-être le mauvais objet.

> **Question 4.** Faut-il retirer `sum E4` du rôle de porte décisionnelle et le
> remplacer par les compteurs du porteur aigu — blocs `RectKey`, faces aiguës
> exactes, événements de sweep, touches site-face ? Si oui, le NO-GO que j'ai
> prononcé sur la configuration centrale doit-il être **rétracté** comme
> non pertinent, ou conservé comme un no-go d'ingénierie sur un objet qui
> n'était pas le bon ?
>
> **Question 4 bis.** Vos deux propositions — `OwnedCK-WST3/WST4` depuis les
> cellules rencontrant `3B_R`, et `AcuteCarrierGateway-q4` avec sweep 1D — me
> semblent être la même route vue de deux côtés : le `CarrierBlock(A,B,C)` de
> l'une est l'`AcuteCarrierBlock` de l'autre, et la sweep 1D remplace
> l'intersection de lentilles. Est-ce exact ? Et quel est le point d'entrée que
> vous voulez que j'implémente en premier — le `CKPairTape-v0` avec son oracle
> `PairId -> RectId`, ou directement la porte de blocs aiguë ?

## 5. Ce que j'ai fait de vos autres remarques

- **Sémantique du seuil (2.4).** Vous avez raison : sous `floor > q2`, un retour
  inférieur au seuil n'est pas une lane exacte. Votre contre-fixture
  `A={(0,0,0)}, C={(1,1,1)}, B=[0,4]x{1}x{1}` est gravée sous le nom
  `soc64-seuil-sous-plancher`. L'API rend désormais `kUnknownBelowFloor = -1`
  dans tous les cas sauf deux : énumération complète, ou valeur `kLaneNone` —
  qui est le minimum du treillis, donc exacte. Le contrat est vérifié aux deux
  seuils par la porte des fixtures.
- **Cascade d'arités (section 8, point 7).** Le binaire tuait déjà le mutant
  `arity-cascade` sur `arite4`, mais aucune porte ne l'exigeait : la couverture
  reposait sur un accident. `mhgp3v_centre_cell_kill_arity_cascade_arite4` est
  ajoutée, code exact 4.
- **Analyseur de rampe.** `audits/check_rampe_pentes.py`, schéma
  `RampSlopeAnalyzer-v1`. Il décide `pente <= p/q` par la comparaison
  **entière** `m2^q n1^p <= m1^q n2^p` : aucun logarithme flottant n'est évalué,
  et une pente exactement au seuil est décidée sans ambiguïté. Rejoué sur le
  reçu `35fcea8`, il reproduit vos pentes à la sixième décimale
  (`1,898146 / 1,909154 / 1,911063`), refuse les trois fenêtres `terrain` à
  `pending>0`, et signale en outre que les deux familles `scanline` ont des
  pentes de `3,04` à `5,79` sur un intervalle — ce que le reçu ne disait pas.
  Il vérifie aussi l'identité exclusive `fermee+pendante+ouverte = C(n,2)` par
  lane, seul cas où il rend 1.
- **Ce que je n'ai pas fait.** Pas d'échantillon scellé pondéré par `|A||B|`,
  pas de cap déterministe à 4096 tâches, pas de temps ni de HWM dans le probe
  autonome, `SocStats.wide` compte toujours deux au lieu de trois ou quatre.
  Ce sont des dettes reconnues, pas des oublis.

## 6. `M4` existe maintenant, et c'est le chiffre qui décide

Vous exigez tous les deux `M4` avant tout claim, et il n'existait **nulle
part** dans le code — le symbole n'apparaissait que dans `PROPOSITION.md`.
Je l'ai implémenté sous la forme que vous demandez : `EdgeAcuteCarrierSample-v0`,
échantillon **scellé, déterministe, pondéré par la masse `|A||B|`**, par tirage
systématique sur la masse cumulée. Aucun générateur pseudo-aléatoire.

Ce qui est compté, pour une arête ouverte `ab` : le nombre de sites `x` tels que
`ab` soit l'arête maximale faible de `abx` **et** que `abx` soit strictement
aigu, c'est-à-dire `D>=E`, `D>=X`, `V.V>D`. Ce n'est pas un choix : par votre
lemme du porteur aigu, ce sont **exactement** les candidats du premier étage
d'une source q4 owner-edge.

L'estimateur est stable : `486,2 / 492,0 / 489,6 / 490,9` pour
`K = 512 / 2048 / 4096 / 16384` sur `eight_clusters` à `n=3000`, soit `1,2 %`
d'écart sur un facteur trente-deux d'échantillon.

| famille | `n` | `E4` ouverte | porteurs/arête | `M4` estimé |
|---|---:|---:|---:|---:|
| `uniform` | 1 500 | 443 392 | 111,8 | `4,96e7` |
| `uniform` | 3 000 | 1 027 538 | 128,7 | `1,32e8` |
| `uniform` | 6 000 | 2 394 081 | 133,4 | `3,19e8` |
| `eight_clusters` | 1 500 | 1 071 162 | 236,0 | `2,53e8` |
| `eight_clusters` | 3 000 | 4 045 644 | 489,6 | `1,98e9` |
| `eight_clusters` | 6 000 | 15 135 764 | 1 023,4 | `1,55e10` |

| famille | pente `E4` | pente porteurs/arête | **pente `M4`** |
|---|---|---|---|
| `uniform` | 1,2125 / 1,2203 | 0,2031 / 0,0514 | **1,4157 / 1,2717** |
| `eight_clusters` | 1,9172 / 1,9035 | 1,0530 / 1,0638 | **2,9702 / 2,9673** |

**Sur `eight_clusters`, `M4` est cubique.** Et la cause est structurelle, pas
constante : le nombre de porteurs admissibles par arête ouverte croît
**linéairement en `n`** (pente `1,06`). La raison tient en une ligne — la
maximalité faible `D>=E` confine `x` à la boule de rayon `||ab||` autour de `a` ;
or les arêtes q4 encore ouvertes sur un nuage en amas sont précisément les
**longues arêtes inter-amas**, dont la boule de maximalité avale un amas entier.
Le facteur de factorisation owner-edge x carrier y est donc `Theta(n)` par arête.

Sur `uniform` le même compteur sature — `111,8 -> 128,7 -> 133,4`, pente
`0,05` — et `M4` reste à `1,27`. Mais la constante est déjà écrasante :
`3,19e8` incidences à `n=6000`, soit environ `53 000` par point ; l'extrapolation
naïve à `50 000` donne `4,9e9`. Une seconde de bout en bout à ce niveau
suppose plus de cinq milliards d'incidences traitées, et ce n'est que le
**premier** facteur, avant l'apex.

Autrement dit : `E4` à `1,22` sur `uniform` avait l'air vert, et il masquait un
`M4` de `1,27` avec une constante de cinq mille par point.

### La contre-famille, mesurée

J'ai gravé votre famille à deux droites comme famille de nuage `two_lines`,
déterministe et sans graine. À `n=400`, donc `m=200` :

```text
fenetre q4 : masse_ouverte=48114 (60,293 % de C(n,2))
porteurs q4 : echantillon=4096 moyen=0.000 max=0 sans_porteur=4096 (100,000 %)
M4_estime=0
```

À `n=800` : `176 714` de masse ouverte, `55,3 %` de `C(n,2)`, et de nouveau
**zéro porteur sur la totalité de l'échantillon**. `SOC64` n'y ferme que
quelques paires intra-droite, et son juge `(g,Q)` reste d'accord.

La démonstration est donc complète et elle est expérimentale : sur ce nuage le
résiduel universel est quadratique et la source est vide. **Le compteur de
porteurs le voit instantanément ; la porte sur `sum E4` ne le voit pas.** Cinq
CTests gravent ce couple de nombres.

## 7. Ce que je propose de faire ensuite, et ce que j'attends de vous

Ma lecture, en une phrase : **`SOC64` est reçu comme un prune exact et bon
marché, et il est réfuté comme sauvetage de la voie centrale.** Il entre dans
l'union des certificats ; il ne change pas l'architecture.

Les points 2 à 4 de la liste que j'avais écrite sont faits dans cette même
passe : le compteur de porteurs, la contre-famille gravée, et `M4`. Reste :

```text
1. figer SOC64 en OR de prune, sans rampe 50k : la pente est mesuree et rouge
2. decider si `sum E4` reste une porte, ou si `M4` la remplace  -> Question 4
3. si M4 remplace E4 : mesurer M4 a 12500/25000/50000 sur G4, avec l'echantillon
   scelle, et gater la PENTE de M4 et non celle de E4
4. attaquer la cause structurelle : la boule de maximalite d'une longue arete
   inter-amas avale un amas entier                              -> Question 5
5. CKPairTape-v0 ou AcuteCarrierGateway selon votre reponse 4 bis
```

> **Question 5, née de la mesure `M4`.** Sur `eight_clusters`, les porteurs par
> arête ouverte croissent en `Theta(n)` parce que la maximalité faible confine
> `x` à la boule de rayon `||ab||` autour de `a`, et qu'une longue arête
> inter-amas y enferme un amas entier. Ce n'est pas une constante à optimiser,
> c'est la géométrie de la famille. Deux issues me semblent possibles et je ne
> sais pas choisir :
>
> - **soit** la profondeur doit tuer ces arêtes AVANT l'énumération des
>   porteurs — mais `SOC64` n'en ferme que 32 %, et votre contre-famille montre
>   qu'aucun certificat universel ne peut garantir de les tuer ;
> - **soit** l'ordre `owner-edge x carrier` est le mauvais ordre pour les amas,
>   et il faut un owner qui ne soit pas l'arête maximale — au prix de perdre
>   l'exact-once que l'arête maximale garantit.
>
> Voyez-vous une troisième issue ? Et le `3B_R` de votre section 3.1 change-t-il
> quelque chose à ce compte, ou ne fait-il que le borner par en haut sans
> réduire le nombre réel de porteurs admissibles ?

Je ne lance aucune session G4 tant que la question 4 n'est pas tranchée : une
rampe à 50 000 sur `sum E4` coûterait une session pour mesurer un objet dont
votre contre-famille montre qu'il peut être découplé de la sortie. Si vous
tranchez pour `M4`, la session est prête à partir — l'échantillon scellé coûte
`K x n` opérations, donc rien.

## 7. Non-claims

Rien ici ne reçoit une source q4, ne mesure `M4`, ne produit un `BallKey`, ni
n'approche le contrat `50000/G4`. Les pourcentages de la section 2 sont des
estimations **à ordre de visite constant** : un vrai branchement de `SOC64`
arrêterait la descente sur un nœud fermé et rendrait son budget à d'autres
sous-arbres, ce que le shadow ne simule pas. Une graine, une taille, une
répétition, deux vCPU. Le seul résultat solide de cette passe est négatif et il
est à mon débit : la première table que j'ai publiée était fausse, et ce sont
vos deux audits qui l'ont établie.
