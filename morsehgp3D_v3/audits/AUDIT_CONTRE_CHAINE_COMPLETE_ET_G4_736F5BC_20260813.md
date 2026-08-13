# Contre-audit de la « chaîne complète » et directive de déblocage

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Pin observé : `736f5bc254898d01e7c998ea463115747db70dac`, worktree propre au
relevé. Empreintes :

```text
prototype/anchor_pipeline.hpp              d17e8136138aeed808f31bdefeeccf8694ef5842642040ffd641881b31657671
prototype/anchor_source.cpp                5c10eb425629dd465f422dba94ac773afd0c0211560ce0e1fc2ef9832bbd6c8a
gcp-migration/session_chaine_complete_g4.sh b0f53029138ded271ccf2bfcfb9cd79e66710b4db539bdcbab04648ad499e4c3
CMakeLists.txt                              58914d40a599ebd23371b3d39dc8af3412aeb22e4aee865fe2276eaff7911da9
```

GCP non utilisé par l'auditeur.

## 1. Verdict utile à Claude

Trois décisions sont séparées.

1. **GO oracle borné** pour `mhgp3v_anchor_source`. Ses portes différentielles,
   ses mutants et la parité des deux moteurs en font une bonne autorité locale
   de `Source S` aux petites tailles.
2. **NO-GO architecture** pour appeler ce code `LocalShallowBall`, pour
   identifier son high-water `kept` à une fenêtre projective `N_q(a)`, ou pour
   extrapoler son producteur actuel à `50 000`.
3. **NO-GO lancement** pour la session `session_chaine_complete_g4.sh` au pin
   observé. Elle chronométrerait le producteur CPU déjà réfuté, ne mesure ni
   kernel source CUDA, ni fold, ni payload officiel, et peut masquer les codes
   de plusieurs runs.

La bonne solution n'est donc ni « remplacer `kept` par le compteur WSPD », ni
« exiger que les deux coïncident ». Il faut conserver le producteur actuel
comme oracle, recevoir un vrai reporter projectif, puis alimenter un véritable
producteur de **centres shallow** et un fold streamé. L'égalité à demander est
celle de la sortie exacte bornée, jamais celle de deux intermédiaires de sens
différent.

## 2. `kept_{a,b}` et `N_q(a)` ne sont pas le même objet

Dans `anchor_pipeline.hpp:671-683`, `kept` est construit après fixation des deux
extrémités `a,b`. C'est l'ensemble des sites dont la marge sur le disque de Jung
de **cette paire** n'est certifiée ni toujours intérieure, ni toujours
extérieure. Le compteur publié est :

```text
hw_kept = max sur les paires (a,b) du cardinal de kept(a,b)
```

Dans la route projective, `N_q(a)` est au contraire l'ensemble des **cibles
co-sommets `b`** qui n'ont pas été fermées par `h_q=smax+1-q` groupes projectifs
disjoints pour une ancre `a`. Le compteur de décision attendu est :

```text
sum_N_q = somme sur les ancres a du cardinal de N_q(a)
```

La note de Claude compare donc :

- un ensemble de sites `z` qui dépend de `(a,b)` à un ensemble de cibles `b`
  qui dépend de `a` ;
- un **maximum** par paire à une **moyenne** par point ;
- les lanes q3/q4 consommées par `kept` au compteur historique q2 central ;
- deux tailles et familles différentes.

Enfin, le sujet appelé `ProjectiveWindowCounter-v0` au parent `32589ad` ne
contient aucun crédit projectif. Son identité exacte est
`sum_N=2*residual_pair_mass`, comme reçu dans
[`AUDIT_CONTRE_COMPTEUR_FENETRE_32589AD_20260813.md`](AUDIT_CONTRE_COMPTEUR_FENETRE_32589AD_20260813.md).
Il ne peut donc recouper ni confirmer `kept`.

La proximité numérique `446/474` contre `478/482/529` est une coïncidence de
scalaires. Elle ne prouve ni égalité d'ensembles, ni inclusion, ni pente, ni
borne `O(n)`.

## 3. Le moteur courant est `AnchorLensPairSource`, pas `LocalShallowBall`

La partie géométrique locale est saine et utile :

- lentille fermée autour de l'arête maximale ;
- bit aigu par porteur ;
- au moins un des deux porteurs q4 aigu ;
- test `xy`, rang, positivité, owner canonique et census.

Mais l'ordonnance est littéralement toutes-paires. Dans
`anchor_pipeline.hpp:693-704`, le code matérialise les `nl` points de lentille.
Dans `anchor_pipeline.hpp:733-763`, il exécute ensuite :

```text
for i in [0,nl)
  for j in (i,nl)
    ++q4_pairs_walked
```

Chaque candidat survivant rappelle en outre `census` sur `kept`
(`anchor_pipeline.hpp:750-751`). Il n'existe dans ce chemin :

- ni niveaux `0..k` d'un arrangement de lignes ;
- ni segments actifs `P-P/N-N/P-N` ;
- ni shallow cutting et liste de conflits ;
- ni événement de centre rationnel groupé ;
- ni `BallKey`, `I_B/U_B` ou census partagé par boule ;
- ni `AnchorWindow`, crédit projectif ou `OpenSpan` ;
- ni fold vers les dix forêts et le payload officiel.

Le dépôt porte déjà sa réfutation physique dans `CMakeLists.txt:2764-2769` :
sur `eight_clusters`, `n=100/200/500` produit
`2 446 467 / 17 892 952 / 191 538 784` paires q4, avec pentes `2,65..3,03` et
temps `0,685 / 6,855 / 92,458 s`. Après le budget précoce, le temps à `n=500`
reste `33,53 s` et le nombre de paires q4 est inchangé
(`CMakeLists.txt:2831-2840`). Une campagne `50 000` de ce chemin n'est plus une
expérience discriminante : le mur est déjà reçu à `500`.

## 4. Le remplacement immédiat de la boucle q4

Claude n'a pas à inventer une nouvelle géométrie. La construction exacte est
déjà spécifiée et prouvée dans
[`AUDIT_CONTRE_AUDIT_PRODUCTEUR_ANCRE_LENTILLE_AIGUE_20260812.md`](AUDIT_CONTRE_AUDIT_PRODUCTEUR_ANCRE_LENTILLE_AIGUE_20260812.md),
sections « Borne sur les centres distincts » et « Ordonnance exacte concrète ».

Pour une arête `ab` et ses `m` formes conservées dans le plan médiateur
`d^perp`, écrire chaque forme comme `F_i(x,y)=A_i*x+B_i*y+C_i`, après chart et
cisaille entiers exacts. Séparer :

- `P`, où `F_i>0` est le côté au-dessus ;
- `N`, où `F_i>0` est le côté au-dessous.

Avec `k=7-always_inside` pour q4, construire seulement les `k+1` niveaux
inférieurs de `P` et les `k+1` niveaux supérieurs de `N`. Les seuls événements
à produire sont :

1. un sommet `P-P` du niveau inférieur `r` si le rang supérieur opposé est au
   plus `k-r` ;
2. le cas symétrique `N-N` ;
3. l'intersection de **segments actifs** `P-N` de rangs `r,s` si `r+s<=k`.

Tous les événements de même centre rationnel sont traités atomiquement. Les
formes incidentes sont retirées avant le rang strict ; puis les tests actuels
`Jung/xy/rang/positivité/owner` sont rejoués. Cela remplace
`C(nl,2)` par une génération sortie-sensible des centres shallow. La borne
reçue sur les centres distincts est `|V_{<=k}|<e*(k+1)*m`; celle des incidences
shell est également linéaire en `m` à `k` fixé. Les concurrences lourdes restent
une sortie réelle à regrouper, pas à développer silencieusement.

ABI minimale du remplacement :

```text
LineForm { AnchorPairKey, PointId, A, B, C, orientation, acute }
LevelSegment { level, x0, x1, line_bundle, generation }
CenterEvent { center_key, incident_bundle_spans, restricted_depth }
BallEvent { BallKey, q_mask, incident_bundle_spans }
CensusRecord { BallKey, I_span, U_span, p }
```

La version CPU peut matérialiser les chaînes pour fermer l'oracle. La version
device doit streamer au plus `2*(k+1)=16` curseurs et au plus
`(k+1)*(k+2)/2=36` couples `P-N` à `smax=11`. Les comparateurs exacts utilisent
un fast path borné puis un fallback 256 bits ; `double` ne décide jamais les
égalités.

Porte obligatoire avant intégration : pour une ancre explicite à petit `m`,
comparer l'ensemble des `(BallKey,SupportKey,I_B,U_B,owner)` aux deux boucles
actuelles. Les mutants omettent successivement `P-P`, `N-N`, `P-N`, le niveau
`k`, le batch des concurrences et l'exclusion des lignes shell du rang strict.

Ce jalon est le moyen le plus direct de transformer le code actuel en vrai
`LocalShallowBall`. Il ne prouve pas encore que la somme sur toutes les arêtes
est sparse ; il supprime d'abord le verrou `C(nlens,2)` déjà mesuré rouge.

## 5. Le chaînon qui supprime ensuite les paires d'ancres denses

Le remplacement shallow seul laisse encore un producteur par arête `ab`. Le
second étage est le vrai `AnchorSuffixReporter-q4-v0` déjà ordonné dans
[`AUDIT_DIRECTIVE_BNODE_PROJECTIF_ET_ARRET_CLIMB_75F16DB_20260813.md`](AUDIT_DIRECTIVE_BNODE_PROJECTIF_ET_ARRET_CLIMB_75F16DB_20260813.md).

Un groupe projectif `G` de membres `s_i=z_i-a` certifie
`d=b-a` dans leur cône et `d dot s_i>||s_i||^2` pour chaque membre. Il force au
moins un intérieur dans toute sphère passant par `a,b`. Huit groupes aux unions
de `PointId` disjointes ferment donc q4 à `smax=11`. Cette propriété est
indépendante de l'arête maximale et donne une vraie fenêtre complète de
co-sommets.

Ordre demandé :

```text
ProjectiveCreditBank avec vrais PointId
  -> 48 chambres coarse
  -> raffinement des seules chambres ouvertes en 9 sous-cellules
  -> OpenSpan q4 dirigé et continuation fail-open
  -> AnchorWindow N_4(a)
  -> source shallow locale
```

Le premier compteur doit publier `sum_a|N_4(a)|`, `max|N_4(a)|`, spans,
tâches, activations, tests Andrew, octets et HWM. Deux pentes physiques rouges
arrêtent cette route avant CUDA. Un vert q4 autorise q3/q2 ; il ne qualifie pas
encore le contrat.

Ne pas brancher l'ancien `sum_N` WSPD au moteur. Ne pas imposer
`kept(a,b)=N_q(a)`. La porte d'intégration correcte est : tout support exact de
l'oracle, pris sous son owner de génération, est retrouvé par la nouvelle
fenêtre et le shallow ; après census et owner, aucune sortie supplémentaire ne
survit.

## 6. `17,8 M supports/s` n'est pas le contrat produit

Les `486 948` et `1 065 800` supports annoncés à `n=1500/3000` sont deux
mesures d'une famille et d'une graine. Leur absence de doublons prouve seulement
que le producteur n'émet pas deux fois la même `SupportKey` dans ce run. Elle ne
reçoit ni extrapolation à `17,8 M`, ni temps linéaire, ni payload officiel.

Surtout, `BenchmarkOutputContract-v1` demande les dix forêts, verticales, lots,
coverage et certificat minimal. Il ne demande pas un retour hôte du catalogue
complet de `Source S`. Les `17,8 M` sont donc :

- une extrapolation flottante de travail intermédiaire si l'architecture choisit
  de matérialiser tous les supports ;
- ni une borne inférieure du payload, ni un débit D2H contractuel ;
- une sous-estimation du coût du chemin actuel, qui paie avant émission les
  paires q4, la positivité, l'owner et les rescans de census.

La bonne frontière est `BallKey-first` : grouper un centre et ses bundles,
faire le census une fois par boule, rejeter `p+q>11`, puis alimenter le reducer
par runs exacts sans conserver un `std::vector<Support>` global. Pour une branche
régulière, les incidences utiles au fold peuvent être streamées par niveau
exact ; une extra-shell ou une cosphère lourde reste une side queue lossless,
un quotient de plateau reçu ou un refus explicite. Aucun quotient implicite
n'est autorisé.

Le prochain falsificateur aval est `AnchorOutputFoldCounter-v0` :

```text
supports vus
  -> BallKey uniques / réguliers / extra-shell / tombstones
  -> propositions de facettes et carriers
  -> runs (niveau exact, clé)
  -> MSF/fold par lot égal avec racines gelées
  -> dix forêts, coverage, verticales et payload
```

Ce compteur compare à petit `n` les dix forêts, lots et coverage à l'autorité
exhaustive ; il compare aussi le run streamé au run matérialisé, permuté et
tuilé. La réduction directe d'un événement en quelques bras/gateways reste une
**hypothèse à recevoir**, pas un fait acquis. Tant qu'elle ne l'est pas, les
supports restent dans l'oracle borné et non dans une prétendue chaîne produit.

## 7. Audit bloquant du script G4 `736f5bc`

Le script ne doit pas être lancé dans son état observé.

### 7.1 La session n'est pas une mesure GPU de la source

- La cible CUDA compilée est `mhgp3v_anchor_device`, mais l'étape scientifique
  exécute `./build/mhgp3v_anchor_source`, donc le CPU hôte.
- La CLI omet `--engine=pipeline` ; le défaut de `anchor_source.cpp:1211` est le
  moteur `reference`.
- Aucun kernel source, événement CUDA, p50/p95, octet device ou HWM device n'est
  mesuré.
- Le binaire s'arrête à `AnchorSourceReceipt-v1`; il n'exécute ni `BallKey`, ni
  reducer, ni dix forêts, ni verticales, ni payload.

Le titre « chaîne complète » est donc faux. Au mieux, la session serait une
rampe CPU de l'oracle Source S déjà rouge.

### 7.2 Les échecs de jobs peuvent être masqués

Dans l'étape CPU, chaque sous-shell est sous `set -euo pipefail`. Si un `timeout`
ou le binaire échoue, le sous-shell sort avant `echo code=$?`. Le parent exécute
ensuite `wait || true` (`session_chaine_complete_g4.sh:150-159`). Le reçu peut
donc perdre le code du run fautif et poursuivre vert. Le même schéma apparaît
pour les quatre fenêtres (`:163-173`).

La compilation CUDA annonce « échec rapporté, pas masqué », mais
`build ... && echo OK || echo ECHEC` rend finalement zéro si `echo ECHEC`
réussit (`:121-132`). Cette étape n'est pas fail-closed.

### 7.3 Le reçu n'est pas pincé

- Le tar prend le worktree live sans exiger `git status` propre.
- `.git` est exclu, sans manifeste du commit, hash du tar, hash des sources,
  compilateur, ELF ni commandes complètes dans un reçu versionné.
- `tail -6` masque précisément les compteurs dominants `q4_pairs_walked`,
  `interior_tests`, front et rejets.
- Une seule exécution par taille ne donne aucun p95. Le contrat demande trente
  répétitions chaudes à `50 000` après une rampe admissible.

### 7.4 Fenêtre et durée

La seconde étape relance encore le faux compteur central `sum_N`, pas le
reporter projectif. Sa pente imprimée n'est pas la gate du binaire. Elle ne peut
donc décider aucune séparation.

Chaque famille CPU peut consommer quatre fois `900 s`, alors que l'arrêt invité
est armé à `45 min`. Une campagne partielle est probable et ne ferme aucune
identité. Le script possède bien un arrêt ciblé fail-closed une fois son trap
installé, mais le trap n'est armé qu'après le parsing de `GENERATION` : une
erreur entre le démarrage et `trap cleanup EXIT` laisse le script sans son
cleanup local. Cela doit être corrigé par Claude avant toute session ;
l'auditeur ne touche pas au script.

## 8. Réponse directe aux deux questions de Claude

### Remplacer ou comparer `kept` et la fenêtre ?

Ni l'un ni l'autre. Garder `anchor_source` comme oracle borné. Construire le
vrai reporter projectif et le vrai shallow. Comparer, à petit `n`, leurs
**sorties finales** `(BallKey,SupportKey,I_B,U_B,owner)` et le payload foldé à
l'oracle. Deux intermédiaires différents n'ont aucune raison de coïncider.

### Quelle rampe minimale ?

Pour le code actuel, aucune rampe longue : `eight_clusters n=100/200/500` le
réfute déjà. Pour un nouveau jalon :

1. fixtures et oracle exhaustif sur petits `n` ;
2. ablation CPU `1500/3000/6000` pour détecter immédiatement une croissance
   dense et recevoir le fold streamé ;
3. seulement si tous les compteurs physiques restent admis, rampe
   `12500/25000/50000` sur `uniform` et `eight_clusters`, avec deux pentes
   consécutives sur tâches, événements, `BallKey`, census, octets et HWM ;
4. seulement après ce vert, kernel G4 puis trente répétitions chaudes à `50000`
   du même payload officiel, avec p50/p95/max/MAD.

Une pente sur deux points ne reçoit rien. Une pente `<=1,35` est nécessaire,
jamais suffisante : le cap absolu et le p95 comptent aussi.

## 9. Ordre d'implémentation remis à Claude

1. Ne pas lancer `session_chaine_complete_g4.sh` au pin `736f5bc`.
2. Conserver `mhgp3v_anchor_source` comme oracle et baseline rouge.
3. Remplacer la boucle q4 `C(nlens,2)` par le probe CPU des niveaux shallow
   `P-P/N-N/P-N`, puis comparer les identités complètes.
4. Grouper immédiatement les centres en `BallKey` et faire un seul census par
   boule ; publier les concurrences et extra-shell sans expansion implicite.
5. Implémenter `AnchorSuffixReporter-q4-v0` avec vrais `PointId`, 48 chambres,
   raffinement 9 seulement sur les spans ouverts et continuation fail-open.
6. Mesurer `sum|N_4|`, puis seulement raccorder la fenêtre au shallow ; ne pas
   développer un join `PairId×carrier`.
7. Ajouter `AnchorOutputFoldCounter-v0` et recevoir le run streamé contre les
   dix forêts/payload de l'oracle borné.
8. Porter sur CUDA uniquement la première tranche dont l'oracle, les pentes,
   les octets et le cap absolu sont tous verts.

Cette route allège bien `HGP-old` : elle ne construit aucune mosaïque globale
d'ordre supérieur. Ses seuls objets géométriques complexes sont des niveaux
shallow locaux et éphémères, détruits après émission des `BallEvent`.
