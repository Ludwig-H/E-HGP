# MorseHGP3D v3

MorseHGP3D v3 explore une construction exacte et industrielle de la hiérarchie
Morse/HGP en dimension trois, sans matérialiser la mosaïque de Delaunay d'ordre
supérieur.

Cadre courant :

```text
phase=exploration_v3_hors_registre
backend=cpu_reference_bounded_oracles_and_g4_diagnostic
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

La v3 n'est ni promue dans le registre officiel, ni qualifiée GPU, ni déclarée
exacte sur son domaine public. Une session G4 SPOT documentée au `HEAD=35fcea8`
a utilisé ses 48 processeurs logiques comme ressource CPU, sans kernel CUDA ;
le reçu certifie l'arrêt ciblé `TERMINATED` et ne qualifie aucun SLO.

## Verdict actuel

Le contrat n'est pas rempli. Pour `n=50000` et `K_max=10`, la cible principale
est `p95 warm_e2e<100 ms` et la cible secondaire `p95 warm_e2e<1 s` sur un G4,
sortie complète et synchronisation comprises. Aucun échantillon qualifiable ne
reçoit l'une ou l'autre.

Le pin historique `2b89ea1` introduit enfin une première tranche
`BallForm -> PrimitiveSphereKey -> census I_B/U_B -> SphereRun`. C'est le bon
ordre architectural, mais **l'étape 0A n'est pas reçue pour u16** : les
constructeurs q3/q4 rabattent des numérateurs de 67 à 81 bits vers `int64`, puis
créent des carrés jusqu'à environ 162 bits dans `i128` avant réduction. Les
huit CTests du pin, puis les dix du successeur, ne couvrent que `coord<=64` ;
leur juge de Gram dépasse lui-même 128 bits sur des fixtures u16, ne recertifie
ni la positivité ni la clé primitive, et le mutant de clé reste corrélé.

Le producteur au pin `3c11bc8`, inchangé au `HEAD=35fcea8`, ajoute un probe
nommé stage 0B. Il ne ferme pas 0B : il
compare Kruskal à Floyd--Warshall sur le même hypergraphe de `PointId`, dans
une unique DSU. Il n'émet ni dix forêts par ordre, ni lots, coverage,
verticales ou payload. Cette DSU est structurellement fausse dès `k=2` lorsque
deux générateurs distincts partagent un point mais moins de `k` identités.

Le verdict live est dans
[`audits/AUDIT_ETAT_COURANT.md`](audits/AUDIT_ETAT_COURANT.md). L'audit ciblé de
la tranche est
[`audits/AUDIT_BALL_EVENT_V0_2B89EA1_20260813.md`](audits/AUDIT_BALL_EVENT_V0_2B89EA1_20260813.md).
Le contre-rejeu du faux 0B est
[`audits/AUDIT_CONTRE_RECEPTION_STAGE_0B_3C11BC8_20260813.md`](audits/AUDIT_CONTRE_RECEPTION_STAGE_0B_3C11BC8_20260813.md).

## Route active

La prochaine chaîne à fermer est :

```text
0A  BallForm -> BallKey -> census -> BallEvent exact
0B  oracle exhaustif borné -> lots -> dix forêts -> verticales -> payload
1   CKPairTape q2 -> porteurs aigus -> OwnedCK-WST3/WST4, toujours factorisés
2   certifier rang/census et mesurer F2/F3/F4, M3/M4 et H
3   porter la même tranche sur device, puis mesurer warm_e2e sur G4
4   ouvrir séparément tout nouveau profil numérique
```

Une source incomplète peut être comparée dans le sink de référence, mais ne
publie jamais un succès. Il n'existe pas de watermark monotone par ancre : les
runs sont scellés, triés et mergés par niveau exact avant le premier commit
d'un lot. « Streamé » signifie mémoire résidente bornée, jamais fold en ligne
sur une source non scellée.

## Contrat d'identité

Les couches restent distinctes :

- `PrimitiveSphereKey` : cinq coefficients primitifs de
  `A||z||^2+B dot z+C`, avec `A>0`, avant census ;
- `BallKey` : identité de nuage, profil et schéma exact ajoutés à la clé
  primitive ;
- `SupportKey` : vrais `PointId` triés, jamais positions Morton ou indices
  d'un buffer ;
- `BallEvent` : `BallKey`, supports, owners, niveau exact, `I_B/U_B`, lanes,
  provenance, complétude du census et disposition transactionnelle.

Le fold contractuel ne doit dépendre ni de `__int128` natif, ni du nombre de
limbs du profil. Le probe courant viole encore cette frontière : son
comparateur lit directement `PrimitiveSphereKey` et effectue ses contrôles
après des multiplications signées susceptibles de déborder. Une fois la
frontière reçue, un futur profil binary64 pourra changer `ExactKernel` et le
codec sans réécrire le fold. La cardinalité seule ne motive pas binary64 : la
grille u16 3D contient $2^{48}$ sites distincts ; l'index dense et le `PointId`
sont des codecs séparés.

## Prochaines réparations P0

Avant d'appeler `0A` fermé :

1. construire directement les polynômes q3/q4 sans centre rabattu en `int64`
   et employer une autorité BigInt/rationnelle sur tout `[0,65535]^3` ;
2. juger indépendamment dépendance affine, positivité, clé primitive, niveau,
   census et owner sur des `PointId` non denses ;
3. ajouter epoch/profile/schema, statuts typés, marqueurs de complétude et
   `SupportRecord` atomique ;
4. appliquer `count -> preflight -> fill -> validate -> publish`, avec zéro
   payload sur cap moins un, erreur numérique ou dégénérescence non admise ;
5. borner les générateurs de fixtures et refuser leur capacité plus un ;
6. différencier toute la sortie de `0A`, puis fermer `0B` par générateurs et
   ordres jusqu'au `BenchmarkOutputContract-v1`.

## Source factorisée q2/q3/q4

La source retenue exploite Callahan--Kosaraju sans développer son produit
cartésien :

```text
CKPairTape(A,B)                          toutes les paires, exact-once
  -> OwnedCK-WST3(A,B,C)                carrier de l'arête maximale
  -> OwnedCK-WST4(A,B,C,D)              second carrier/apex
  -> BallKey/RLE -> rang/census -> fold
```

Pour chaque rectangle CK, une boule `B_R` contenant `A union B` fixe un niveau
Morton. Tout troisième ou quatrième sommet d'un support dont `ab` est l'arête
maximale appartient à `3B_R`. Les cellules non vides de ce niveau donnent donc
une extension ternaire complète, puis leurs couples non ordonnés une extension
quaternaire complète. L'owner longueur/`EdgeKey` rend les sorties exact-once.
Une paire n'est q2 propre que si `D=||b-a||^2>0` : les positions dupliquées
sont rejetées, quotientées ou filtrées exactement avant cette promotion.

q3 recertifie `E+X-D>0` et l'indépendance affine. q4 ne signifie pas « quatre
faces aiguës » : l'autorité est la stricte positivité des quatre
barycentriques du circumcentre. Une face aiguë adjacente à l'arête maximale
sert seulement à choisir un carrier géométrique primaire.

Le chemin q4 élimine d'abord les `CarrierBlock` sans face aiguë, avant de
former les couples de cellules. Pour une face exacte, les centres vivent sur
une droite et la puissance de chaque apex y est affine : une sweep 1D par lots
égaux remplace l'arrangement 2D comme candidat principal. Les comparaisons
rationnelles peuvent dépasser `i128` sous u16.

Les masques de rang restent indépendants. Une fixture u16 de 64 points possède
un q4 régulier de rang 4, alors que ses six arêtes q2 et ses quatre faces q3 ont
toutes rang 12. `OwnedCK-WST4` doit donc consommer la relation aiguë q3
**pré-rang**, jamais les événements q3 retenus.

Le nombre de blocs initiaux vaut conditionnellement `O(s^3 n)`,
`O(s^3*eta^-3*n)` et `O(s^3*eta^-6*n)` pour q2/q3/q4, avec `0<eta<=1` et une
vraie propriété fair/compressed-split. Ces bornes ne couvrent pas tous les
raffinements `MIXED`. Leur masse logique peut rester quadratique ou pire. Les
blocs sont paresseux jusqu'à un consommateur factorisé reçu ou au preflight
atomique d'une vraie sortie. Le rapport complet
et son contre-audit sont
[`audits/AUDIT_SOURCE_CK_WST_Q2_Q3_Q4_35FCEA8_20260814.md`](audits/AUDIT_SOURCE_CK_WST_Q2_Q3_Q4_35FCEA8_20260814.md)
et
[`audits/AUDIT_DEBLOCAGE_Q4_PORTEUR_AIGU_SOC64_LIVE_35FCEA8_20260814.md`](audits/AUDIT_DEBLOCAGE_Q4_PORTEUR_AIGU_SOC64_LIVE_35FCEA8_20260814.md).

## Déblocages mathématiques prêts après `0A`

Quatre pistes sont assez précises pour être implémentées dans les composants
existants, avec échec fail-open.

### `SOC64` et `CORNER512`

Pour `e=z-a`, `t=b-z`, `H=e dot t`, `E=||e||^2`, `X=||t||^2`, q3 exige
`H>0 && 4H^2>EX` et q4 `H>0 && 3H^2>EX`. Le domaine est séparément convexe en
`e,t` et en `a,b,z`.

- Si les 64 couples de coins de `(C-A)×(B-C)` passent, tout le rectangle est
  `ALL`. Un échec est `UNKNOWN`.
- Les 512 triples de coins de `A×B×C` caractérisent exactement `ALL` pour
  l'enveloppe AABB continue. Un coin fictif échouant ne vaut pas `NONE` pour les
  seuls `PointId` stockés.

Le `JungSpindleRect-v0` actuellement branché n'est pas `SOC64/CORNER512` : il
combine des extrema séparés de `D,V,T`. Son diagnostic `n=6000,s=8` gagne
environ trois centièmes de point seulement. Cela réfute cette combinaison sur
les boîtes grossières mesurées, pas les deux théorèmes corrélés. Leur primitive
et 16 CTests bornés ont été observés verts dans le worktree ; l'ablation WSPD
et son coût transitif restent non reçus.

La prochaine ablation recommandée est `SOC64-shadow-q4` sur un échantillon
déterministe de tâches `central-MIXED`, avec cap propre, early exits, masse
créditable et aucun changement de fate. Si le signal est positif, le brancher
**avant** le raffinement local : un certificat corrélé évite une scission,
alors qu'un certificat rejoué après la scission paie déjà son front.

Le premier raccord live du 14 août additionnait les crédits SOC d'un ancêtre
aux crédits baseline du même sous-arbre et comptait deux fois les mêmes IDs.
Le shadow courant corrige cette faute : il compare les ledgers baseline et
union combinée, place SOC après les fallbacks et arrête une branche combinée à
son premier `ALL`. Un replay borné mesure `41` fermetures de masse `95`, contre
`127` et `316` avec la somme fautive. Il doit encore être capé, recevoir une
porte CTest intégrée et tuer un mutant `sum_instead_of_union`. À `n=2000`, le
shadow non capé a déjà soumis `3 809 028` tâches et ajouté environ `2,1 s` à la
vague sur une machine partagée ; ne pas lancer sa rampe 50k. Un retour inférieur
à `floor=q4` signifie seulement `UNKNOWN_BELOW_FLOOR`, pas une lane exacte.

### `JungDiskDepth`, puis LP projectif

Pour une paire ponctuelle owner `ab`, les centres q3 et q4 ne parcourent pas
tout le plan médiateur : avec `y=2c`, ils restent dans les disques exacts de
Jung `||y-d||^2<=D/3` et `||y-d||^2<=D/2`. Dans ce plan 2D fixe, un groupe d'au
plus trois IDs peut certifier qu'au moins un témoin est intérieur pour tout
centre admissible. Neuf groupes disjoints ferment q3, huit ferment q4 avant la
création des carriers de cette paire.

Cette preuve ne ferme pas un rectangle CK : ses endpoints font varier le plan,
le disque et les demi-plans. Il faut scinder jusqu'à une paire/microtile rejoué,
ou prouver un futur `BlockJungDiskDepth` uniforme. Une fixture `2×2` ferme q4
sur la paire basse alors qu'aucun de ses huit témoins n'est même q2 intérieur
sur la paire haute ; tout transfert depuis un représentant reste interdit.

Le LP global reste un oracle utile, mais son échec ne prouve plus une pénurie
sur le disque Morse : une fixture à huit groupes ferme `JungDiskDepth8` alors
que le LP sur tout le plan échoue dès la profondeur un.

Pour `s_i=z_i-a`, `d=b-a`, `D=||d||^2`, `q_i=||s_i||^2`, poser :

$$\kappa_G(d)=\min\left\lbrace \sum_i\alpha_iq_i:\sum_i\alpha_is_i=d,\ \alpha_i\geq0\right\rbrace.$$

`G` crédite un intérieur sur toute sphère par `a,b` si et seulement si
`d` appartient au cône positif de `G` et `kappa_G(d)<D`. Un optimum basique
emploie au plus trois IDs. Huit extractions disjointes donnent un fast path q4;
un arbre de suppressions fournit un oracle complet de profondeur universelle
relativement au pool, jusqu'à 3280 appels LP pour q4. Cette propriété porte sur
toutes les sphères par la paire, pas seulement les supports Morse; un échec
reste fail-open pour la source. Ce dernier n'est pas un hot path.

### Pelages inversés collectifs

`OriginOnionDepth-h` inverse une banque autour de l'ancre, retire
successivement les sommets de `conv({0} union P)`, puis teste si la cible
inversée reste strictement dans la dernière coque sans chute de rang. Chaque
couche fournit alors un ID intérieur distinct dans toute sphère par la paire.
Une facette se rejoue sur un BNode par le test séparable
`v||d||^2-u dot d>=1`, sous environ 87 bits. `h=8/9/10` ferme q4/q3/q2.

Ce fast path reste universel. Sur la famille u16 à deux droites, tous les
certificats universels peuvent laisser `n^2/4` paires alors que tous les
triangles sont obtus et la vraie source q3/q4 est vide. La porte par carrier
aigu doit y rendre zéro sans développer de `PairId`.

### Cages de quatre à six sites

Une positive basis inclusion-minimale 3D peut avoir quatre, cinq ou six sites. Les cages
tétra-only sont donc incomplètes. Une cage de six facettes possède au plus huit
sommets de fleur. `SixRoleCageProposer` reste une ablation counter-only ; chaque
groupe doit être validé exactement, et réduire une cage impose de recalculer sa
fleur.

Les preuves, limites de largeur et contre-fixtures sont consolidées dans
[`audits/AUDIT_REPONSE_PLAN_VERTICAL_SOC64_LP_1AA487D_20260813.md`](audits/AUDIT_REPONSE_PLAN_VERTICAL_SOC64_LP_1AA487D_20260813.md).

## Raffinement local : signal utile, coût non reçu

Le raffinement des seuls terminaux q4 non fermés réduit réellement `E4` à
`n=3000`, `s=8` : `eight_clusters` passe de `4 045 644` à `2 597 699` arêtes
résiduelles et `uniform` de `1 027 538` à `464 599`. Mais à profondeur quatre,
les recertifications passent respectivement de `31 538 327` à `199 169 436`
et de `108 858 186` à `193 020 841`. Sans `M4`, BallKeys, census et fold, il
est faux de conclure que le levier « paie ».

La télémétrie de tête double-compte les parents ensuite scindés et imprime
jusqu'à `380,15 %` de masse q2 fermée. Le ledger terminal
`CLOSED/OPEN/PENDING` reste cohérent ; les compteurs de tentatives doivent être
séparés de l'objet final.

La réparation algorithmique proposée est `ProofCarryingLocalRefinement` : un
enfant hérite des CNodes témoins déjà `ALL`, des `NONE` et de leurs IDs ; seuls
les `MIXED` sont rejoués. Cela évite de repartir de la racine à chaque split et
se prête à `count--scan--fill` avec continuations persistantes.

La recette G4 a depuis été exécutée sur CPU. Les quarante processus rendent
zéro, mais `terrain` conserve des continuations q3/q4 à 25 000 et 50 000 : ses
`sum_E4` sont des surensembles, pas des fenêtres finales. Sur
`eight_clusters`, `pending=0` et les pentes restent proches de `1,9` après
profondeur quatre ; cela réfute la configuration centrale mesurée, pas tous les
certificateurs rectangle. Le rapport `1,62` contre `1,57` compare en outre des
unités différentes et ne mesure pas un prix. Détails et réponses :
[`audits/AUDIT_REPONSE_CRITERE_MORT_SOC64_LP_35FCEA8_20260813.md`](audits/AUDIT_REPONSE_CRITERE_MORT_SOC64_LP_35FCEA8_20260813.md)
et
[`audits/AUDIT_CONTRE_RECU_RAMPE_RAFFINEMENT_G4_35FCEA8_20260813.md`](audits/AUDIT_CONTRE_RECU_RAMPE_RAFFINEMENT_G4_35FCEA8_20260813.md).

## Dégénérescences et sortie lourde

Le profil de coordonnées u16 n'exclut pas les cosphères. Le domaine candidat
utilise une politique `RelevantGP` fail-closed : un extra-shell pertinent rend
`unsupported_degeneracy` tant qu'aucun quotient complet n'est reçu. Cette
fermeture du domaine reste elle-même à recevoir.

Un `SphereRun` interne conserve l'identité et le census pour garder la décision
réversible. Il n'autorise pas un plateau public. Un quotient saturé ne devient
valide qu'après reconstruction des lots, dix forêts, coverage et verticales.
Si le contrat exige chaque `SupportKey`, une cosphère lourde est une borne de
sortie ; ni RLE ni streaming ne suppriment ce travail.

## Porte de coût

Une pente `sum_E4` ne qualifie rien seule. Chaque campagne publie au minimum :

- masses exclusives `CLOSED/OPEN/PENDING`, avec `pending=0` pour une fenêtre
  finale ;
- `E3/E4`, maximum par ancre, `M3/M4=sum m_ab`, tâches, splits et visites ;
- `BallKey` brutes/uniques, supports, census et tailles de shell ;
- sorties `H`, octets, HWM, opérations larges et temps par phase ;
- commandes, seeds, commit, diff, binaire et codes de sortie.

Les diagnostics CPU existants ne sont pas des modèles G4. Aucun cutoff kNN
n'est exact : des supports positifs gardent un partenaire arbitrairement loin
en rang. Aucun arrangement global, aucune mosaïque Delaunay d'ordre supérieur
et aucun catalogue exhaustif ne deviennent le chemin produit.

Le raffinement local réduit effectivement `E4`, mais ses parents et enfants
sont encore mélangés dans plusieurs compteurs de tentative. Son coût doit être
jugé après séparation `AttemptStats/TerminalLedger` et avec héritage des preuves
`ALL/NONE`. La campagne CPU G4 a calculé ses pentes après coup, coupe les
métriques physiques et n'exige pas la finalité ; elle n'est pas une campagne de
qualification. Voir
[`audits/AUDIT_CONTRE_RAFFINEMENT_LOCAL_ET_SESSION_G4_3C11BC8_20260813.md`](audits/AUDIT_CONTRE_RAFFINEMENT_LOCAL_ET_SESSION_G4_3C11BC8_20260813.md)
et son contre-reçu au pin `35fcea8`.

## Construire et tester

Depuis la racine du dépôt :

```bash
cmake -S morsehgp3D_v3 -B build/v3 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v3 --parallel
ctest --test-dir build/v3 --output-on-failure
python3 tools/check_docs.py
```

CUDA reste opt-in avec `-DMHGP3V_ENABLE_CUDA=ON`. Une session GCP éventuelle
doit suivre exclusivement les scripts gardés et les coupe-circuits décrits par
`AGENTS.md`.

## Arborescence documentaire

- [`PROPOSITION.md`](PROPOSITION.md) : proposition technique et mathématique
  consolidée ;
- [`audits/AUDIT_ETAT_COURANT.md`](audits/AUDIT_ETAT_COURANT.md) : unique
  verdict mutable ;
- [`audits/README.md`](audits/README.md) : index court des audits actifs et des
  dépendances historiques encore citées par le logiciel ;
- `oracle/` : juges bornés indépendants ;
- `prototype/` : candidats et probes, sans autorité produit implicite ;
- `receipts/` : diagnostics et reçus, dont le statut est fixé par l'audit.
