# Rectificatif à `b6f9715` — le codec confond nœuds et bornes de plage

Date : 16 août 2026 UTC.  
Dossier : `morsehgp3D_v3/`.  
Audit rectifié : [`AUDIT_Q2_5EEFE_972C_DECISION_ORACLE_VS_GENERATEUR_20260816.md`](AUDIT_Q2_5EEFE_972C_DECISION_ORACLE_VS_GENERATEUR_20260816.md), commit `b6f97157e631d2319e9a8724158d56d2709261a6`.  
Audit concurrent reçu : [`AUDIT_RECEPTION_Q2_COREDEPTH_CENSUS_RELATION_20260816.md`](AUDIT_RECEPTION_Q2_COREDEPTH_CENSUS_RELATION_20260816.md), commit `77320f47501b817b48d89afbbe6c5833a8fee9c9`.  
Dernier commit fonctionnel commun : `972c20bd2ee3fb46a7a7fe74a9e96bb895084c16`.

Cadre :

```text
phase=exploration_v3_hors_registre
backend=math_reference_and_cpu_scheduler
profile=quantized_u16_input_only
mode=pairframe_codec_type_rectification
public_status=not_claimed
```

> [!IMPORTANT]
> **Rectification.** La section 5.1 de mon audit `b6f9715` qualifie :
>
> ```cpp
> b_node > dom_ep
> ```
>
> de simple off-by-one et demande `b_node >= dom_ep`.
>
> Cette conclusion est **trop rapide pour le probe abstrait actuel**. Dans
> `pair_frame_probe.cpp`, Claude sérialise :
>
> ```cpp
> k.a_node = e.p0;
> k.b_node = e.p1;
> ```
>
> où `[p0,p1)` est une plage de paires et `p1` est une borne exclusive. Ainsi :
>
> ```text
> p1 == g_nP
> ```
>
> est valide. Le test `b_node > dom_ep` est donc cohérent avec cette sémantique
> de **RangeEnd**.
>
> Le vrai défaut est plus profond : `CoreContinuation` appelle ces champs
> `a_node/b_node` et les type comme `NodeHandle`, tandis que le probe les emploie
> comme `PairRange.begin/end`. Un même codec prétend donc valider deux ABI
> incompatibles. Il faut séparer les types et les schémas, pas changer un signe
> sans regarder ce que l’entier représente.

---

## 1. Ce que je retire explicitement

Je retire comme assertion générale :

```text
b_node == endpoint_domain_size est toujours hors domaine.
```

Elle est vraie pour :

```text
EndpointNodeHandle,
```

mais fausse pour :

```text
PairRangeEnd exclusif.
```

La gate `b_node == dom_ep -> invalid_input` demandée dans G24 de `b6f9715` doit
donc être remplacée par une gate **typée**.

Cette correction ne change pas les autres conclusions de `b6f9715` : le q2
courant reste un oracle de décision, la source physique reste quadratique, les
vrais `PointId` ne sont pas émis, le payload HGP est absent et le codec ne lie
pas encore suffisamment un record à son propriétaire.

---

## 2. Le défaut réel : une union non taguée de deux identités

### 2.1 Sémantique annoncée par `PairFrame`

Le header déclare :

```cpp
using NodeHandle = int;

struct PairFrame {
  NodeHandle a_node;
  NodeHandle b_node;
};
```

Le contrat naturel est alors :

```text
0 <= a_node < endpoint_node_count
0 <= b_node < endpoint_node_count.
```

### 2.2 Sémantique employée par le probe abstrait

Le probe emploie les mêmes champs comme :

```text
p_begin = p0
p_end   = p1
```

avec :

```text
0 <= p_begin < p_end <= pair_count.
```

Les deux contrats diffèrent précisément sur la borne supérieure de la seconde
coordonnée, mais pas seulement : un nœud d’arbre et une borne de tableau ne
possèdent ni la même identité, ni la même fonction de population, ni la même
preuve de `pair_mass`.

### 2.3 Risque

Un record valide du probe abstrait peut être syntaxiquement accepté par un
consommateur croyant lire deux handles de nœuds. Réciproquement, recomputer :

```text
pair_mass
```

requiert :

```text
p_end-p_begin
```

pour une plage abstraite, mais :

```text
population(a_node)*population(b_node)
```

pour un vrai `PairFrame` géométrique.

Aucun `ContexteDecodage` unique ne peut inférer cette différence depuis deux
entiers non tagués.

---

## 3. Réparation d’ABI recommandée

### 3.1 Deux types, idéalement deux schémas

```cpp
struct PairNodeOwner {
  EndpointNodeHandle a_node;
  EndpointNodeHandle b_node;
};

struct AbstractPairRangeOwner {
  uint32_t begin;
  uint32_t end_exclusive;
};
```

Puis :

```text
CoreContinuationNode-v1
CoreContinuationRangeProbe-v1
```

Le second reste un type de test et ne doit pas devenir une ABI produit.

Une union taguée est possible :

```cpp
enum class OwnerKind : uint8_t {
  kEndpointNodes,
  kAbstractPairRange
};
```

mais deux schémas séparés sont plus difficiles à confondre accidentellement et
plus simples à auditer.

### 3.2 Validations propres

Pour `EndpointNodes` :

```text
0 <= a_node < endpoint_node_count
0 <= b_node < endpoint_node_count
pair_mass == population(a_node)*population(b_node)
rect_id owns exactly this product
```

Pour `AbstractPairRange` :

```text
0 <= begin < end_exclusive <= pair_count
pair_mass == end_exclusive-begin
```

### 3.3 Gates

```text
node record decoded as range  -> SCHEMA/OWNER_KIND error
range record decoded as node  -> SCHEMA/OWNER_KIND error
node handle == node_count     -> invalid_input
range end == pair_count       -> valid
range end > pair_count        -> invalid_input
wrong recomputed pair_mass    -> invalid_input
```

Le codec produit q2 devra employer exclusivement `EndpointNodes`.

---

## 4. Deuxième rectification : estimation du nombre de blocs q2

Mon audit donne « environ 39 millions » de `PairFrame` initiaux pour :

```text
n=50000, cap_rect=64.
```

Cette valeur est plausible si la masse moyenne d’un bloc terminal est proche de
`32`, mais elle n’est pas démontrée par le code relu.

La masse totale vaut exactement :

```text
C(50000,2) = 1 249 975 000.
```

Si tous les blocs portaient la capacité maximale `64`, le plancher serait :

```text
ceil(C(n,2)/64) = 19 530 860 blocs.
```

Les blocs réels peuvent être moins remplis, donc leur nombre peut être plus
grand. La formulation honnête est :

```text
des dizaines de millions de blocs sont attendus sous cap 64 ;
la constante doit être mesurée ;
le pire cas reste quadratique.
```

Le diagnostic d’architecture ne change pas : cette prépartition ne peut pas être
la source 50k. Mais le facteur deux imaginaire est retiré avant qu’il ne devienne
un graphique, habitat naturel des constantes non prouvées.

---

## 5. Réception de l’audit concurrent `77320f4`

L’audit concurrent converge avec `b6f9715` sur le point principal :

```text
5eefe084 reçoit q2 CoreDepth sur géométrie réelle ;
il ne reçoit pas encore le générateur HGP q2 complet.
```

Je reçois notamment ses compléments suivants.

### 5.1 Le théorème de non-crédit s’étend à q3/q4

Tout prédicat ponctuel `Wq`, `q in {2,3,4}`, exige d’abord :

```text
H(a,b,z)>0.
```

Si `C` recouvre `A`, le triplet `a=z=p` appartient au produit et donne `H=0`.
Aucun certificat sûr `ALL_Wq` sur tout le produit ne peut donc réussir.

Condition d’implémentation essentielle : le certificat q3/q4 doit vérifier le
**signe** avant de mettre `H` au carré. Un test purement quadratique pourrait
certifier le cône opposé.

Le non-crédit reste un théorème ; la conservation et le replay restent des
mécanismes.

### 5.2 La masse relationnelle doit appartenir au majorant

Si `relation_frontier` est une liste disjointe de `mixed_spans`, alors :

```text
upper = lower + mixed_mass + relation_mass_upper.
```

Si le tag `RELATION` est porté par un span déjà présent dans `mixed_spans`, il ne
faut pas le compter deux fois.

L’ABI doit choisir l’une de ces deux représentations. Le codec courant recalcule
la masse sur `mixed_spans` seulement ; une troisième convention implicite serait
une source directe de faux `CORE_CLEAR`.

### 5.3 Le census reste dû pour q2

La paire q2 détermine sa boule, mais le bit :

```text
|I_B| < h2
```

ne fournit ni `|I_B|`, ni `I_B`, ni `U_B`, ni le niveau exact. La conservation
locale de `shell_spans` ne constitue pas encore leur consommation par un
`BallCensusLedger`.

Ce point est entièrement compatible avec la section 2 de `b6f9715`.

---

## 6. Statut rectifié

| Élément | Verdict rectifié |
|---|---|
| `b_node > dom_ep` dans le probe de plage | cohérent avec `end_exclusive` |
| même test pour un vrai NodeHandle | insuffisant ; `>=` requis |
| codec unique pour plage et nœuds | refusé |
| type/schéma propriétaire tagué | requis |
| estimation fixe « 39 millions » | retirée |
| ordre de grandeur « dizaines de millions » | maintenu, à mesurer |
| conclusion de parcimonie | inchangée |
| audit concurrent `77320f4` | reçu positivement |
| théorème endpoint q3/q4 | reçu avec test de signe préalable |
| masse relationnelle dans `upper` | à typer explicitement |
| q2 CoreDepth | reçu |
| générateur HGP q2 complet | encore ouvert |

---

## 7. Message direct à Claude

Le codec a désormais deux utilisateurs sémantiquement différents :

```text
le probe abstrait sérialise une plage [p0,p1) ;
le futur produit sérialisera deux nœuds endpoint.
```

Il faut les séparer maintenant. Sinon les portes négatives continueront de
« prouver » tantôt une borne exclusive, tantôt un handle, selon le commentaire
que le lecteur a vu en dernier.

Après ce découplage, l’ordre reste celui donné par les deux audits convergents :

```text
1. vrais PointId et D=0 ;
2. CKPairTape/WSPD paresseux ;
3. L_open + U_closed / shell ;
4. BallKey + I_B/U_B + BallEvent ;
5. continuation q2 réellement sérialisée ;
6. seulement ensuite carrier q3 canonique.
```

Le fait de corriger publiquement l’audit ne change pas cette route. Il évite
simplement d’introduire un bug réel en « réparant » un test qui validait en fait
une autre donnée que celle annoncée, variante informatique de l’opération
chirurgicale parfaitement réussie sur le mauvais patient.
