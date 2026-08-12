# Note d'implémentation — source sparse complète sous théorème de rétraction

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cette note décrit la voie d'implémentation rendue possible par la réparation du
K-graphe de Gabriel. Elle ne reçoit aucun producteur actuel et ne modifie aucun
contrat public. Sa portée est strictement
`normalized_horizontal_h0_orders_two_through_effective_maximum`. L'ordre un,
les verticales, `full_pi0`, les identités Gamma et le SLO officiel gardent leurs
propres obligations.

## 1. Le théorème consommé

Le graphe de Gabriel brut est insuffisant : E5 montre qu'une coface
non-Gabriel peut installer silencieusement une facette réutilisée plus tard.
L'oracle exact `G_k^+` répare ce défaut en ajoutant les étoiles silencieuses de
toutes les cofaces, mais reste exhaustif.

Sous une porte régulière globale ou une composition terminale équivalente, le
théorème de rétraction permet une source plus petite :

1. toutes les cofaces directes/Gabriel de rang utile ;
2. leurs facettes du cœur, dédupliquées par labels ;
3. tous les co-minimiseurs exacts de première incidence dans la branche
   `|J_F|=0` ;
4. l'unique coface égale dans la branche `|J_F|=1` ;
5. une gateway résolue vers l'apex strict commun dans la branche `|J_F|>=2` ;
6. les carriers latents, les naissances internes et l'atomicité des niveaux.

Ce sous-flot préserve les composantes non triviales, leur généalogie et leur
couverture en `PointId`. Il ne reproduit pas le transcript facetté de Gamma.

## 2. Pipeline industriel candidat

```text
points u16 + LBVH exact immuable
  -> source terminale de supports q2/q3/q4
  -> census fermé I/E + BallKey exacte + owner exact-once
  -> événements directs réguliers, RLE par identité sémantique
  -> facettes strictes du cœur, au plus quatre propositions par événement
  -> dédup FacetKey + requête exacte J_F
       |J_F|=0 : lambda(F) + tous les co-minimiseurs
       |J_F|=1 : coface directe égale
       |J_F|>=2 : deux témoins + cible stricte T_F
  -> resolver strict mémoïsé vers un carrier pré-lot
  -> arêtes de carriers pondérées par niveau exact
  -> MSF parallèle avec tie-break canonique
  -> reconstruction par lots (k,beta) atomiques
  -> journal normalized_horizontal_h0 + coverage
```

Les structures persistantes sont proportionnelles au nuage, aux événements
directs, aux facettes uniques effectivement proposées, aux gateways et à la
MSF. Aucun tableau global de paires, de cofaces, de cellules d'ordre supérieur
ou d'incidences Gamma n'est autorisé.

## 3. Records minimaux

### `DirectBallRecord`

- `cloud_epoch`, `owner`, `BallKey` exacte ;
- support positif canonique et liste des provenances équivalentes ;
- `I` strict complet et shell fermé `E` ou reçu de range-report rejouable ;
- niveau exact et rangs utiles ;
- état terminal de la source et digest de la tâche.

Dans la branche régulière `E=U`, l'événement direct est `Q=I union U`. Si une
extra-shell pertinente existe, `(U,B)` ne représente pas toutes les cofaces
directes de la boule et ne peut pas entrer dans la réduction régulière.

### `FacetRecord`

- clé scientifique `(cloud_epoch,k,sorted PointIds(F))` ;
- miniboule et `beta(F)` recalculées comme contrôles ;
- premier lot source, liste de provenances et handle stable latent ;
- disposition `rooted`, `latent` ou `unresolved`.

Le niveau ne fait pas partie de l'identité de la facette : il est une fonction
de ses labels. Deux valeurs proposées pour la même clé constituent une faute.

### `GatewayReceipt`

- `F`, `beta(F)`, support essentiel `u_F` ;
- deux intrus distincts `z_F,w_F` pour la branche `|J_F|>=2` ;
- cible stricte `T_F`, étapes de descente et preuves de baisse ;
- `terminal_carrier`, stamp strictement pré-lot et digest du resolver ;
- classe d'autorité couvrant la confluence et les autres intrus.

Deux intrus prouvent la branche et une baisse stricte. Ils ne prouvent pas à
eux seuls que le carrier terminal représente toute la composante Gamma : cette
dernière prémisse doit être rejouée séparément.

## 4. Source directe et coquilles dégénérées

Pour une boule `B`, posons `I=X cap interior(B)` et `E=X cap boundary(B)`. Les
cofaces directes portées par `B` sont

$$Q=I\cup A,\qquad A\subseteq E,\qquad c_B\in\mathrm{conv}(A).$$

Un record de support `(U,B)` ne couvre que `A=U`. Il n'est donc une source
directe complète que sous la porte `E=U`, après census global de `I` et
fermeture de l'owner.

La route exacte partage les boules en trois classes :

| classe | décision |
| --- | --- |
| `E=U` dans la fenêtre | réduction régulière par directes et gateways |
| `p+s>=K_eff+2` | bloc saturé H0-inerte par le théorème 4.2 |
| extra-shell/support multiple de rang pertinent | quotient de plateau reçu ou refus fermé |

Choisir un pivot dans l'union de plusieurs supports minimaux est interdit : un
autre support peut survivre à la suppression et conserver exactement le même
niveau.

## 5. Resolver strict sans circularité

La descente remplace à chaque étape un point essentiel par un intrus strict et
transporte un témoin de coface sous le cutoff. Le niveau baisse strictement. Un
cycle est donc mathématiquement impossible, mais aucune petite borne de
profondeur n'est connue; la borne universelle brute est combinatoire.

L'implémentation emploie :

- un dictionnaire du cœur construit inductivement par lots exacts ;
- des stamps historiques ;
- une worklist device/host, un cache `FacetKey -> terminal` et la compression
  des suffixes ;
- un ledger de chaque étape et un test exact `beta(next)<beta(current)` ;
- aucune limite scientifique configurable.

Pour une gateway, un terminal doit être déjà enraciné dans le snapshot strict.
Un hit sur une facette seulement latente ne suffit pas. Une insuffisance de
ressource physique rend la transaction `unresolved` et ne publie aucun préfixe.

## 6. Niveaux et MSF

Le chemin existant compare exactement les niveaux `Sphere` avec six limbs, soit
384 bits : sa borne de produit croisé est inférieure à `2^326`. Cette largeur
fixe est compatible avec un port GPU et ne dépend pas d'une multiprécision non
bornée.

Une représentation de Gram réduite permettrait des comparaisons u16 en 256
bits, mais c'est une migration distincte : nouvelle clé, pgcd, sérialisation,
comparateur et portes d'overflow. Elle ne justifie pas de raccourcir le layout
actuel.

Les arêtes sont totalement ordonnées par niveau exact puis clé sémantique pour
la reproductibilité. La sémantique de filtration groupe toutefois toutes les
arêtes d'un même `(k,beta)` : le tie-break ne les séquentialise jamais.

Un Boruvka, filter-Kruskal ou Kruskal parallèle peut calculer une MSF de ce
graphe de carriers. Toute MSF préserve les composantes à chaque seuil. Le
payload se reconstruit ensuite sur les seules arêtes retenues, groupées par
niveau, avec snapshot strict, multifusion atomique et sidecar de naissances et
de couverture. L'ordre des rondes Boruvka n'est pas l'ordre de filtration.

## 7. Pourquoi la voie reste sparse

La route évite :

- les `C(n,k+1)` cofaces de `G_k^+` ;
- les cliques de facettes de Gamma ;
- le complexe de Čech et la mosaïque de Delaunay d'ordre supérieur ;
- les snapshots DSU par niveau ;
- les facettes isolées qui ne deviennent jamais carriers du quotient réduit.

Elle paie seulement les supports proposés puis certifiés, les facettes du cœur,
les premières incidences requises, les descentes et la MSF. Cette taille est
sortie-dépendante, pas prouvée linéaire. Les compteurs indispensables sont
supports bruts, `BallKey`, cofaces régulières, bras proposés, facettes uniques,
branches `J_F`, profondeurs/hits du resolver, arêtes avant/après MSF, niveaux
distincts, taille des lots, octets et high-water.

## 8. Limite des générateurs saturés

La tour de boules saturées est exacte sans position générale : un générateur
`S` représente implicitement son bloc de Johnson et deux générateurs se
rencontrent à l'ordre `k` si leur intersection contient au moins `k` labels.
Elle fournit l'oracle naturel des plateaux.

Garder seulement `O(|S|)` facettes connectées qui couvrent `S` n'est pas une
compression relative exacte. Elles peuvent manquer une composante stricte
incidente ou une facette réutilisée plus tard, exactement comme E5. Un record
saturé compact exige encore :

- la famille complète de générateurs pertinents ;
- leurs memberships fermés ;
- le join historique complet `|S intersection T|>=k` ;
- un lookup futur complet `F subseteq S -> handle_closed(S)`.

L'énumération brute de ces supports et joins est combinatoire. Tant qu'une
version sparse de ces quatre autorités n'est pas prouvée, les plateaux de rang
pertinent restent un repli borné ou `unsupported_degeneracy`, jamais une
branche silencieusement acceptée.

## 9. Portes avant CUDA et avant une seconde

1. Fixture E5 et comparaison `G_k^+`--Gamma facette par facette.
2. Différentiel d'identités `BallKey`, `I`, `E`, cofaces et owners, pas seulement
   des comptes.
3. Extra-shell unique-support et support-multiple sans extra-shell, séparés.
4. Branches `|J_F|=0/1/>=2`, ties complets et cible extérieure au cœur.
5. Profondeur du resolver, partage des suffixes, invariance shards/threads.
6. Comparateur de niveaux, égalités inter-arités et lots atomiques.
7. MSF contre fold exhaustif borné, multifusions et naissances latentes.
8. Pentes `12 500/25 000/50 000`, quatre familles, compteurs complets et
   high-water avant tout port device.
9. Après ces gates seulement : parité hôte/device, Compute Sanitizer, puis une
   session G4 gardée et le vrai `BenchmarkOutputContract-v1`.

Le probe `certified_locality_probe` au `HEAD` observé ne réalise pas ce
pipeline. Son mode `sparse` utilise une fenêtre fixe de 48 pour les trois
arités, rescane le nuage pour `J_F` et ne construit ni gateway, ni resolver,
ni MSF, ni fold. Ses volumes bornés sont utiles pour dimensionner; ils ne
constituent pas une source complète.

GCP non utilisé.
