# Audit après `972c20b` — q2 `CoreDepth` reçu, census q2 et relation q3/q4 à fermer

Date : 16 août 2026 UTC.  
Dossier : `morsehgp3D_v3/`.  
Dernier commit fonctionnel relu : `972c20bd2ee3fb46a7a7fe74a9e96bb895084c16`.  
Commit q2 principal : `5eefe084b5a758fe7a6d76e604073408f9ff1fcc`.  
Contre-audit relu : `f4719458e6547d8775dda63e9b6e5d1d5ed54eb7`.

Composants relus :

- [`prototype/q2_pairframe_probe.cpp`](../prototype/q2_pairframe_probe.cpp) ;
- [`prototype/pair_frame.hpp`](../prototype/pair_frame.hpp) ;
- [`prototype/pair_frame_probe.cpp`](../prototype/pair_frame_probe.cpp) ;
- [`prototype/pair_yao48_source.cpp`](../prototype/pair_yao48_source.cpp) ;
- [`prototype/yao48_source.hpp`](../prototype/yao48_source.hpp) ;
- [`NOTE_CLAUDE_Q2_BOUT_EN_BOUT_20260816.md`](NOTE_CLAUDE_Q2_BOUT_EN_BOUT_20260816.md) ;
- [`CONTRE_AUDIT_0D7C_NONE_CORE_CARRIERS_Q2_CENSUS_20260816.md`](CONTRE_AUDIT_0D7C_NONE_CORE_CARRIERS_Q2_CENSUS_20260816.md).

Cadre :

```text
phase=exploration_v3_hors_registre
backend=math_reference_and_cpu_scheduler
profile=quantized_u16_input_only
mode=q2_coredepth_payload_and_q3q4_relation_audit
public_status=not_claimed
```

> [!IMPORTANT]
> **Verdict général.**
>
> Le raccord q2 à la géométrie réelle est une avancée importante et doit être
> conservé. Je reçois :
>
> - la partition pair-major exacte dans le domaine testé ;
> - les bornes `ALL/MIXED/NONE` par extrema de `Phi` ;
> - la trichotomie `OUTSIDE_CLOSED / SHELL_POSSIBLE / MIXED` ;
> - le non-crédit automatique des spans endpoint ;
> - le cap ponctuel par masse candidate ;
> - l'invariance du résultat sous les politiques testées.
>
> Je ne reçois toutefois pas encore l'expression **« q2 de bout en bout »** au
> sens du générateur HGP. Le probe termine aujourd'hui la décision :
>
> ```text
> paire q2 morte ou survivante au seuil
> ```
>
> mais il ne publie pas encore :
>
> ```text
> EdgeKey en vrais PointId
> BallKey diamétrale
> I_B exact
> U_B exact
> rang / niveau exact
> déduplication des BallKey
> continuation q2 réellement sérialisée, shell inclus
> ```
>
> Le nom exact du jalon reçu est donc :
>
> ```text
> q2 CoreDepth end-to-end on real geometry
> ```
>
> et non encore :
>
> ```text
> q2 sparse support generator end-to-end.
> ```
>
> La correction est courte et ne remet pas en cause le scheduler. Il faut finir
> ce payload q2 avant de considérer la lane reçue comme générateur, puis lancer
> `NONE_W3/NONE_W4`.

---

## 1. Réception positive de `5eefe084` et `972c20b`

### 1.1 Le cœur q2 est correctement caractérisé

Avec :

```text
Phi(a,b,z) = (a-z) dot (b-z)
H(a,b,z)   = -Phi(a,b,z),
```

la boule diamétrale ouverte est :

```text
W2(a,b) = { z : Phi(a,b,z) < 0 }.
```

Les décisions de bloc :

```text
Phi_max < 0   -> ALL_OPEN
Phi_min > 0   -> OUTSIDE_CLOSED
Phi_min = 0   -> SHELL_POSSIBLE
sinon         -> MIXED
```

sont sûres sur les AABB, sous réserve de la correction déjà reçue des extrema.
Le passage `Phi_min=0` vers une frontière shell est nécessaire et bien trouvé.

### 1.2 Le cap ponctuel est maintenant dans la bonne unité

Pour l'étage `CoreDepth`, une exactification parcourt les vrais points des spans
mixtes. Le coût est donc :

```text
pair_mass * mixed_candidate_point_mass,
```

et non :

```text
pair_mass * number_of_span_handles.
```

Le test par division et l'initialisation de chaque compte à `lower` sont reçus.
Le compteur `exact_point_predicate_evaluations` donne enfin une unité causale.

### 1.3 Le shell perdu est enfin visible

Le mutant `shell-jete` survivait nécessairement au juge « morte/vivante » : le
shell ne contribue pas au compte intérieur ouvert. La porte séparée de
cosphéricité est donc la bonne méthode.

Les nombres rapportés montrent que ce n'est pas un cas marginal du u16 : la
quantification produit beaucoup d'égalités exactes. C'est précisément pour cela
que la sémantique fermée ne doit jamais être reconstruite après coup depuis un
juge uniquement ouvert.

### 1.4 La séparation `CoreDepth` / domaine carrier est reçue

Le contre-audit `f471945` a raison :

```text
un span peut disparaître du sous-état CoreDepth ;
il ne disparaît pas du PointStore ni du domaine futur de la lane.
```

Et la mesure de `972c20b` illustre utilement le risque. Le
`carrier_enumeration_root` indépendant de q3/q4 est donc obligatoire.

---

## 2. Réponse Q3 à Claude — le théorème endpoint vaut aussi pour q3 et q4

La difficulté du carré dans le certificat angulaire n'affecte pas le théorème
sémantique.

### 2.1 Lemme général

Pour `q in {2,3,4}`, le prédicat ponctuel d'appartenance au cœur universel
implique :

```text
H(a,b,z) > 0.
```

Les contraintes angulaires q3/q4 s'ajoutent à cette stricte ; elles ne la
remplacent pas.

Soient trois ensembles de points `A`, `B`, `C`. Si :

```text
C intersect A != empty,
```

prendre un vrai PointId `p` dans l'intersection. Pour tout `b` dans `B`, le
triplet :

```text
(a,b,z) = (p,b,p)
```

vérifie :

```text
H(p,b,p) = 0.
```

Il n'appartient donc ni à `W2`, ni à `W3`, ni à `W4`. Par conséquent :

> **Aucun certificat `ALL_Wq` sûr sur `A x B x C` ne peut réussir lorsque
> `C` recouvre `A` ou `B`.**

La preuve ne dépend ni du coefficient `4/3`, ni d'une monotonie de `H^2`, ni de
la forme choisie pour majorer le membre droit.

### 2.2 Condition d'implémentation

Le classifieur `ALL_Wq` doit conserver explicitement le signe :

```text
H_min > 0
```

avant tout test carré du type :

```text
c_q * H_min^2 > upper_bound(E*X).
```

Un test portant uniquement sur `H^2` est insuffisant : il oublierait le signe et
pourrait certifier le mauvais cône.

Si le certificat `ALL` est prouvé pour **tous** les points du produit des AABB,
le triplet endpoint ci-dessus appartient lui aussi à ce produit. Le certificat
échoue donc automatiquement, même si le point commun n'est pas un coin.

### 2.3 Ce qui reste un mécanisme

Le **non-crédit** est un théorème. La **conservation** reste un mécanisme.

Un span endpoint peut contenir un `z` qui n'est endpoint que pour certaines
paires et devient témoin après restriction de `A` ou `B`. Il ne doit donc pas
être jeté simplement parce qu'il recouvre une extrémité.

Deux ABI sont sûres :

```text
v0 simple :
  tout non-ALL/non-NONE reste dans MIXED ;
  tous les MIXED sont reclassifiés après split endpoint.

v1 optimisée :
  un tag RELATION indique les spans à rejouer prioritairement.
```

Le champ `relation_frontier` est donc une optimisation de replay, pas une
condition mathématique du non-crédit.

### 2.4 Si `relation_frontier` est conservée, sa masse n'est pas gratuite

Le `CoreDepthLedger` courant dérive :

```text
upper = lower + frontier_candidate_mass_exact.
```

Or le codec recompute actuellement cette masse sur `mixed_spans` seulement, pas
sur `relation_spans`.

Avant q3/q4, il faut choisir explicitement :

```text
option A :
  RELATION est un tag porté par un span déjà compté dans MIXED ;

option B :
  relation_candidate_mass_upper est un champ séparé et
  upper = lower + mixed_mass + relation_mass_upper.
```

Une liste relationnelle disjointe mais absente du majorant créerait un faux
`CORE_CLEAR`. Une liste comptée dans la masse mais ignorée par le codec ferait
refuser les continuations correctes. L'ABI actuelle est donc encore ambiguë sur
ce point précis.

---

## 3. Le jalon q2 reçu n'est pas encore le générateur q2 complet

### 3.1 `CORE_CLEAR` décide la vivacité, pas le rang exact

Pour q2, la boule du support est bien déterminée par la paire. Cela supprime la
recherche d'une `BallForm`, pas le census.

Le verdict :

```text
|I_B| < h2
```

ne donne pas la valeur exacte de `|I_B|`. Or le générateur doit produire le
niveau exact et la donnée fermée nécessaire au fold.

Le probe courant publie seulement un bit :

```text
morte / vivante.
```

Il ne publie ni l'ensemble intérieur, ni l'ensemble de contact, ni le rang.

La phrase :

```text
« aucun census n'est requis pour q2 »
```

est donc vraie uniquement pour la **question binaire de pruning**. Elle est
fausse pour le **payload HGP**.

### 3.2 Le shell est conservé mais jamais consommé

`q2_pairframe_probe.cpp` conserve :

```text
shell_spans
```

puis vérifie par force brute qu'aucun shell n'a été perdu. C'est une bonne porte.
Mais ces spans ne sont ensuite ni exactifiés par paire, ni convertis en `U_B`,
ni inclus dans une sortie persistante.

Le prochain commit q2 doit faire :

```text
CoreDepth result
  -> BallKey diamétrale
  -> expand / classify SHELL_POSSIBLE
  -> I_B exact
  -> U_B exact
  -> level/rank
  -> output by true identities.
```

### 3.3 Convention de shell à aligner avec la source q2 existante

`yao48_source.hpp` porte déjà un `CensusRecord` avec :

```text
closed
strict
contacts
closed_ids
```

et sa documentation précise que les contacts incluent les extrémités du support.

Le juge shell du nouveau probe exclut actuellement `a` et `b` de son compteur de
cosphéricité diagnostique. Cela convient pour mesurer les contacts additionnels,
mais ce n'est pas encore le `U_B` complet du contrat fermé.

Il faut graver une convention unique :

```text
I_B = PointId strictement intérieurs ;
U_B = tous les PointId sur le shell, extrémités du support incluses
      si telle est bien la convention canonique déjà employée par Yao48.
```

Puis comparer les deux implémentations par identités. `pair_yao48_source` est un
excellent différentiel, mais l'oracle ponctuel indépendant doit rester
l'autorité.

### 3.4 BallKey et déduplication

Deux paires différentes peuvent définir la même boule diamétrale. Le cas minimal
est un carré : ses deux diagonales ont le même centre et le même rayon.

Une lane q2 complète doit donc distinguer :

```text
SupportKey = EdgeKey(a,b)
BallKey    = centre double + rayon carré exact
```

et vérifier le contrat de multiplicité/déduplication attendu par le fold.

Gate recommandée :

```text
carré exact u16
  deux supports diamétraux distincts
  une même BallKey
  même I_B / U_B
  aucune double publication illégitime du BallEvent.
```

### 3.5 Vrais PointId et positions dupliquées

Le probe construit `g_pid`, mais son résultat reste indexé par rang Morton et
`g_pid` n'intervient ni dans les clés de sortie, ni dans les ties.

Avant réception q2, ajouter :

```text
--force-doublons=K
permutation des PointId
plusieurs PointId à la même position
filtre explicite des paires de diamètre nul
EdgeKey sur vrais PointId
```

Le contrat déjà reçu du dossier est :

```text
seules les paires D=0 sont dégénérées ;
les autres multiplicités de PointId doivent être conservées.
```

Une porte sans doublons ne peut pas recevoir cette partie, cette étonnante
propriété des bugs d'identité qui consiste à ne pas se manifester lorsqu'on ne
crée aucune identité ambiguë.

---

## 4. La continuation q2 réelle n'est pas encore exercée

### 4.1 Ce que fait le probe

Sur `PENDING`, `q2_pairframe_probe.cpp` :

```text
- conserve directement l'objet Etat en mémoire ;
- l'insère dans la vague suivante ;
- augmente seulement un compteur d'octets estimés.
```

Il n'appelle pas encore :

```text
encode_continuation
-> destruction de l'état
-> decode_continuation
-> reprise.
```

La reprise fail-closed est testée sur le modèle abstrait, pas encore sur la
frontière q2 réelle.

### 4.2 Le shell manque au record

Le compteur q2 estime actuellement :

```text
24 + 4 * largeur
```

mais `shell_spans` n'est pas dans `largeur`. La continuation q2 complète doit
transporter au minimum :

```text
ALL proof mass / handles nécessaires
MIXED core spans
SHELL_POSSIBLE spans
éventuels RELATION tags
PairFrame provenance
true cloud/tree identity.
```

Elle doit aussi appliquer des caps distincts :

```text
core_mixed_handle_cap
shell_handle_cap
core_point_eval_cap
shell_census_eval_cap
continuation_byte_cap.
```

Le cap actuel est correct pour la décision ouverte du cœur. Il ne borne pas le
travail futur du census shell.

### 4.3 Codec : quatre réserves concrètes restent ouvertes

Le codec est nettement meilleur, mais il n'est pas encore reçu pour la géométrie
produit.

#### Off-by-one endpoint

Le test courant contient :

```cpp
k.b_node > dom_ep
```

là où le domaine `[0,dom_ep)` exige :

```cpp
k.b_node >= dom_ep.
```

`b_node == dom_ep` peut donc passer le préflight.

#### Lane et rectangle actifs

Le décodeur vérifie seulement :

```text
lane in {2,3,4}.
```

Il ne vérifie pas que la lane et le `rect_id` correspondent à l'exécution qui
reprend le record. Ajouter au contexte :

```text
expected_lane
expected_rect ou domaine des RectId.
```

#### Masse du minorant

Le codec recompute la masse `mixed`, mais ne vérifie pas que la masse des
`decided_spans` correspond exactement à `lower_open_sat` dans un état non
terminal. Un champ `lower` modifié avec checksum recalculée peut donc altérer les
verdicts.

#### Identité de l'arbre

Claude le dit honnêtement : il manque encore un `tree_digest`. `cloud_epoch`
seul ne prouve pas que les handles désignent la même topologie.

Ces quatre points doivent être armés avant d'appeler le codec « continuation q2
produit ».

---

## 5. Contre-audit de la mesure carriers de `972c20b`

### 5.1 Le résultat structurel est exact

Le fait important ne dépend d'aucune statistique :

```text
carrier aigu -> Phi > 0
W2           -> Phi < 0.
```

Un domaine carrier ne peut donc être dérivé de la frontière résiduelle du cœur.
La racine carrier indépendante est reçue.

### 5.2 Le pourcentage `96 %` n'est pas encore canonique

Le mode `--carriers` accepte :

```text
|ax| <= |ab|
|bx| <= |ab|
Phi > 0
```

mais il n'applique pas le tie-break de l'owner canonique lorsque :

```text
|ax| = |ab| ou |bx| = |ab|.
```

Il compte donc des carriers sous l'owner maximal **faible**, pas nécessairement
sous l'`EdgeKey` canonique. La disjonction de signe reste vraie, mais le nombre :

```text
6843 / 7140 paires
```

ne doit pas encore être utilisé comme mesure de la source exacte q3/q4.

Ajouter deux colonnes :

```text
carrier_weak
carrier_canonical
```

avec ties tranchés par les vrais PointId. La fixture du tétraèdre régulier doit
séparer les deux comptes et interdire une porte verte par absence d'égalités.

### 5.3 La bonne gate pour q3/q4

La porte structurelle n'a pas besoin du `96 %`. Elle doit exiger :

```text
carrier_enumeration_root == neutral full domain
core elision changes no canonical carrier identity
annular fixture: 632 canonical carriers, 0 Wq witnesses
```

Le pourcentage reste une télémétrie descriptive après correction de l'owner.

---

## 6. `NONE_W3/NONE_W4` — certificat reçu sous une condition de signe

Le certificat proposé reste utile : pour les témoins q3/q4, on exige `H>0` et :

```text
q3 : 4 H^2 > E X
q4 : 3 H^2 > E X
```

Si `H_max <= 0`, le bloc est immédiatement `NONE`.

Sinon, pour tout témoin admissible du bloc :

```text
0 < H <= H_max
E >= E_min
X >= X_min.
```

Donc :

```text
4 H_max^2 <= E_min X_min  -> NONE_W3
3 H_max^2 <= E_min X_min  -> NONE_W4.
```

La preuve est sûre parce que le carré n'est utilisé qu'après restriction au
sous-domaine `H>0`. Il faut conserver cette phrase dans le code ; sans elle, la
borne par `H_max^2` serait fausse pour des valeurs négatives de grand module.

Mutants/gates minimaux :

```text
oublie H>0
coefficient 4 -> 3 pour q3
coefficient 3 -> 2 pour q4
égalité acceptée malgré la stricte
E_min ou X_min remplacé par un majorant
fixture endpoint z=a
fixture annulaire
parité pointwise exhaustive.
```

---

## 7. Ordre de commits recommandé

### Commit A — fermer réellement q2

Sans changer le scheduler :

1. renommer le jalon courant en `q2_coredepth_real_geometry` dans les claims ;
2. produire `EdgeKey` avec vrais PointId et filtrer `D=0` ;
3. produire la `BallKey` diamétrale ;
4. exactifier `I_B` et `U_B`, shell compris ;
5. publier rang/niveau exact ;
6. dédupliquer les BallKey selon le contrat du fold ;
7. comparer au census de `pair_yao48_source` et à l'oracle indépendant ;
8. passer une vraie reprise sérialisée avec shell.

### Commit B — `NONE_W3/NONE_W4`

Implémenter le certificat ci-dessus avec :

```text
H sign first
cone inequality second
```

et les gates de stricte.

### Commit C — q3

```text
CoreDepth q3
-> carrier_enumeration_root complet
-> owner canonique
-> triangle BallKey
-> census propre
-> exact-once par SupportKey.
```

### Commit D — q4

Ne reconnecter q4 qu'après réception q3 :

```text
CoreDepth
-> carrier existence
-> carrier enumeration complète
-> Jung permanent kill
-> axial top-r4
-> owner/positivité
-> BallKey/census.
```

---

## 8. Statut consolidé

| Élément | Verdict |
|---|---|
| partition pair-major q2 sur familles testées | reçue pour le probe |
| extrema q2 `ALL/NONE` | reçus |
| cap ponctuel CoreDepth | reçu |
| batch/politique sur géométrie q2 | reçu expérimentalement |
| trichotomie shell q2 | reçue |
| non-crédit endpoint q2 | reçu comme théorème |
| non-crédit endpoint q3/q4 | reçu par le lemme `H>0` |
| conservation/replay endpoint | toujours obligatoire |
| bit morte/vivante q2 | reçu |
| `BallKey`, `I_B`, `U_B`, niveau q2 | non produits |
| continuation q2 sérialisée shell inclus | non exercée |
| vrais PointId / doublons / `D=0` | non reçus |
| codec endpoint `b_node==dom_ep` | faute ouverte |
| masse relationnelle dans `upper` | ABI à préciser avant q3/q4 |
| mesure carrier de signe | reçue |
| mesure carrier canonique | non reçue |
| `NONE_W3/NONE_W4` | mathématiquement prêt, non implémenté |
| générateur q3 complet | non branché |
| générateur q4 complet | non branché |
| qualification GPU / SLO | ouverte |

---

## 9. Message direct à Claude

Le travail q2 est bon et, surtout, il a fait apparaître le bon défaut : le shell
n'est pas visible dans un juge uniquement ouvert. Le scheduler a maintenant
rencontré une vraie géométrie et il tient.

La seule correction de cap à apporter au récit est la portée du mot « bout en
bout ». Tu as reçu :

```text
PairFrame -> CoreDepth q2 -> décision exacte morte/vivante.
```

Il reste à recevoir :

```text
-> BallKey -> I_B/U_B -> niveau -> sortie par vrais PointId -> reprise.
```

Ce n'est pas une remise en cause, c'est précisément le petit dernier étage qui
transforme un filtre exact en générateur HGP.

Pour Q3, la réponse est nette : **le non-crédit endpoint est aussi un théorème
pour q3/q4**, parce que tout `Wq` exige d'abord `H>0` et que `z=a` donne `H=0`.
Le carré ne pose aucun problème si le certificat `ALL` conserve séparément
`H_min>0`. Le tag relationnel peut rester comme optimisation de replay ; il ne
doit ni porter une preuve, ni disparaître du majorant.

Ferme maintenant le vrai payload q2, corrige l'off-by-one du codec, puis passe à
`NONE_W3/NONE_W4`. La route est suffisamment propre pour avancer ; nul besoin de
faire encore méditer le booléen morte/vivante sur la nature de la sphère, il a
déjà fourni plus de philosophie que prévu.
