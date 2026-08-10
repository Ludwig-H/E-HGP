# Solution constructive — recevoir le transcript par témoins de composante

Date : 10 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, CPU de vérité,
`profile=quantized_u16_input_only`, aucun statut public. Cette note répond au
dernier verrou observé après l'intégration du marquage `q_min` : comparer trois
comptes par niveau ne distingue pas les composantes qui portent ces événements.
Elle ne modifie aucun prototype.

## Résultat utilisable

Une composante du graphe de générateurs à l'ordre `k` possède un identifiant
canonique de coût `O(k)` : la plus petite `k`-face portée par l'un de ses
générateurs. Deux composantes distinctes ne peuvent jamais partager ce témoin,
car deux générateurs contenant la même `k`-face ont une intersection de taille
au moins `k` et sont donc adjacents.

Le transcript exact d'un événement de composante peut ainsi être reçu par :

```text
(ordre, niveau exact, témoin fermé, témoins stricts absorbés, type)
```

Il n'est nécessaire de matérialiser ni toutes les `k`-faces, ni un graphe de
Johnson, ni une mosaïque globale.

## 1. Témoin canonique d'une racine

Pour un générateur saturé `M` de capacité au moins `k`, soit `first_k(M)` la
liste triée de ses `k` plus petits `PointId`. Pour une racine `R` du DSU de
générateurs, définir :

$$\omega_k(R)=\min_{M\in R}\mathrm{first}_k(M),$$

où le minimum est lexicographique.

**Lemme d'unicité.** Si deux racines portent le même témoin `F`, chacune
contient un générateur qui contient `F`. Ces deux générateurs ont une
intersection de cardinalité au moins `k`; le seuil du join les relie. Les deux
racines sont donc identiques.

**Lemme de validité.** `first_k(M)` est une `k`-face de `Gamma_k` dès que `M`
est actif, puisque sa miniboule a un niveau au plus égal à celui de `M`. Le
témoin appartient donc réellement à la composante qu'il identifie.

**Lemme d'accord avec l'oracle.** Sous source complète pour l'ordre `k`, ce
témoin est exactement la plus petite `k`-face de la composante Gamma. D'un
côté, chaque `first_k(M)` est une face de cette composante, donc le minimum de
l'oracle lui est inférieur ou égal. De l'autre, pour la face minimale `F`, le
générateur saturé `Sat(F)` appartient à la source et à la même racine;
`first_k(Sat(F))` est inférieur ou égal à `F`. Les deux minima sont égaux. Sans
complétude certifiée, le record reste celui de la sous-famille et ne peut pas
être comparé comme un témoin autoritatif de Gamma.

À l'activation d'un générateur, initialiser son témoin par `first_k(M)`. À une
union, conserver le minimum lexicographique des deux témoins. Le coût est
`O(k)` par union et la mémoire `O(kG)` dans la forme simple; un interning des
petits vecteurs peut la réduire sans changer le contrat.

Le témoin utilise ici les `PointId` et doit être lié au digest canonique de
l'entrée. Si le résultat doit être invariant à une renumérotation du nuage,
remplacer l'ordre des identifiants par l'ordre géométrique canonique déjà
employé pour les supports, puis sérialiser les coordonnées exactes.

## 2. Record exact d'un événement

Avant le lot de niveau `a`, chaque composante stricte possède son témoin. Lors
du premier contact, capturer ce témoin avec son handle strict. Après fermeture
atomique du lot, pour chaque racine finale marquée par `q_min<=k+1`, publier :

- `order=k`;
- niveau rationnel exact `a`;
- `closed_witness=omega_k(R)`;
- liste triée et sans doublon des témoins stricts absorbés;
- type dérivé de la longueur de cette liste : `birth`, `continuation` ou
  `multifusion`;
- liste ou digest canonique des générateurs d'événement qui ont marqué la
  racine;
- `coverage_delta` séparé, s'il est utile au produit.

La liste des générateurs marquants doit identifier chaque boule par son niveau
exact, son centre exact, le digest de son saturé et `q_min`. Elle reçoit le
prédicat local même lorsque l'ajout ou l'oubli d'un générateur se produit dans
une racine déjà marquée et ne change pas le type de composante.

**Lemme de clé compacte.** Sur un catalogue valide et un nuage fixé, le saturé
`M` suffit en réalité à identifier la boule génératrice `B`. En effet, `B`
contient `M`, donc la miniboule de `M` a un rayon au plus égal à celui de `B`;
mais un support `U` de `B` est inclus dans `M`, donc toute boule contenant `M`
contient `U` et a un rayon au moins égal à celui de `B`. Par unicité de la
miniboule euclidienne, la miniboule de `M` est exactement `B`. Le payload peut
donc transporter un handle interné ou un digest canonique de `M`, lié au digest
du nuage, plutôt que recopier centre, rayon et tous les membres dans chaque
record. `q_min` reste un champ de provenance à certifier, pas une partie
nécessaire de l'identité géométrique.

Le record de composante reçoit ensuite la bijection des racines. Deux erreurs
de même type sur deux composantes ne peuvent plus se compenser dans un simple
total par niveau.

## 3. Construction indépendante dans l'oracle Gamma

L'oracle exhaustif possède déjà les `k`-faces explicites et leur DSU. À chaque
niveau, il peut calculer sans convention produit :

1. le témoin lexicographique de chaque composante stricte;
2. les faces et cofaces exactes du lot;
3. les composantes fermées touchées;
4. pour chacune, son témoin fermé et l'ensemble des témoins stricts qu'elle
   absorbe;
5. le type `0/1/>=2` et les boules exactes responsables.

Canonicaliser la collection de records par
`(order,level,closed_witness)`, puis comparer le vecteur entier. Les numéros de
nœuds internes, l'ordre des unions et les indices catalogue ne participent
jamais au verdict.

Cette comparaison est plus forte que les couvertures d'observations. Deux
composantes de `k`-faces différentes peuvent avoir la même union de `PointId`;
une couverture n'est donc pas un identifiant de composante.

## 4. Fixture de niveaux égaux disjoints

À `k=1`, utiliser deux triples disjoints :

```text
A=(5,0,0), B=(-3,4,0), C=(-3,-4,0)
D=(100,0,-5), E=(100,0,0), F=(100,0,5)
```

La boule du triangle `ABC` et celle supportée par la paire `DF` ont toutes deux
le rayon cinq, donc le niveau carré 25. Le triangle a `q_min=3` et est redondant
à l'ordre un; ses arêtes sont strictement antérieures. Le saturé `DEF` a
`q_min=2` et porte un vrai événement dans une composante disjointe. Cette
composante est déjà connectée strictement par `DE` et `EF`, de niveau carré
`25/4`; son événement est donc lui aussi une continuation.

Un mutant qui oublie le marqueur vrai de `DEF` et marque à tort `ABC` conserve
l'ensemble des niveaux, les partitions et le triple de comptes par niveau : une
continuation dans les deux cas. Il change seulement le témoin de la composante
touchée. La comparaison par records le refuse immédiatement; c'est la porte
minimale contre les erreurs compensées.

## 5. Cas `q_min=k+1`

Un générateur coface-only ne porte pas de nouvelle `k`-face, mais ses
`k`-facettes sont strictement antérieures. Son record doit donc avoir une liste
de témoins stricts non vide. La garde fail-closed devient :

```text
q_min=k+1 et strict_witnesses vide => refus du lot
```

Pour recevoir la provenance du générateur lui-même, choisir comme témoin de
coface son support canonique de taille `k+1`. Pour identifier la composante,
continuer d'utiliser `omega_k(R)`; les deux notions ne doivent pas être
confondues.

Le triangle régulier à `k=2` fournit la fixture positive : ses trois paires
existent strictement avant, puis la coface les réunit au niveau du cercle. Une
source partielle qui ne contient que le triangle viole la garde; elle ne doit
pas être présentée comme une naissance Gamma.

## 6. Mutants qui mordent le chemin sujet

Le décalage `q_min+1` dans l'oracle prouve que la comparaison de niveaux mord;
il ne mute pas le marquage du fold. Ajouter au moins :

- `skip_first_event_marker`;
- `mark_first_redundant_generator` avec `q_min>k+1`;
- `drop_first_strict_witness`;
- `duplicate_first_component_event`;
- `classify_one_strict_as_silent`;
- `accept_q_k_plus_1_birth`;
- `stale_root_witness_after_union`;
- `use_coverage_as_component_identity`.

Chaque mutant doit mourir par un message propre au transcript, pas seulement
par la provenance de `n_support` ou le prédicat des ensembles de niveaux. Un
plancher `transcript_records_compared>0` et un plancher par type empêchent une
porte vide.

### Réception live et mutant compensé exact

Le delta live postérieur à `bc2dafa` implémente déjà le témoin fermé et les
témoins stricts dans G², postings par lots, postings global et dans l'oracle de
`k`-faces. Les campagnes bornées concordent, et les mutants témoin strict perdu
et témoin périmé meurent par les records seuls. C'est un résultat positif.

La combinaison actuelle `skip_first_event_marker + mark_first_redundant` n'est
toutefois pas le mutant compensé de la section 4 : « first » sélectionne deux
niveaux différents, et les triples divergent déjà. Le mutant décisif doit
opérer dans le premier lot qui contient deux racines finales distinctes de même
type, l'une portant un marqueur vrai et l'autre un générateur redondant. Il
échange ces deux décisions dans ce lot, puis affirme avant la comparaison des
records que les triples sont inchangés. Sur la fixture `ABC/DEF` au niveau 25,
le triple reste une continuation; seul le témoin fermé change.

Claude a maintenant introduit ce mutant atomique. La commande live
`--force-swap-marking 1` sur la fixture rend le code 1 avec uniquement
`RECORDS REFUTES : temoin ferme divergent` et aucun `TRANSCRIPT REFUTE` : la
valeur ajoutée de l'identité de composante est donc positivement démontrée.

Deux obligations orthogonales restent utiles : un mutant qui retire ou ajoute
un marqueur **dans une racine déjà marquée**, reçu seulement par le digest des
boules marquantes; et une fixture à `PointId` clairsemés, par exemple
`{10,INT_MAX}`, qui reçoit la retraduction des identifiants denses avant le
payload public.

La sonde hors dépôt correspondante passe déjà sous ASan/UBSan sur les trois
générateurs `{10,INT_MAX}`, `{10,1000,INT_MAX}` et
`{1000,INT_MAX}` : les trois joins publient le même record brut, avec témoin
fermé `{10,1000}` et témoin strict `{10,INT_MAX}` à `k=2`. Cette preuve
positive est prête à être transformée en fixture permanente.

Le mutant `extra_marker_in_marked_root` demande une autre fixture : il survit
sur la fixture disjointe, où aucune racine marquée ne contient le redondant
choisi. À `k=1`, prendre
`A=(15,10,10)`, `B=(7,14,10)`, `C=(7,6,10)` et
`D=(15,10,20)`. Le triangle `ABC`, de centre `(10,10,10)` et rayon cinq, a
`q_min=3`; la paire `AD`, de centre `(15,10,15)` et même rayon, a `q_min=2`.
Leurs saturés partagent `A`, donc appartiennent à la même racine au niveau 25.
Avant ce niveau, `ABC` est déjà connecté par ses arêtes, tandis que `D` est
isolé; `AD` porte la vraie fusion et `ABC` est redondant. Ajouter `ABC` aux
marqueurs ne change ni triple, ni témoin fermé, ni témoins stricts : seule la
liste `marking_saturations` diverge. Ces quatre points ont une enveloppe affine
de dimension trois; c'est la porte minimale du nouveau champ.

## 7. Provenance et source partielle

Avant de lire `n_support` comme `q_min`, vérifier au minimum
`1<=n_support<=min(4,rank)` et un bit `q_min_certified`. L'accord borné du juge
sur des campagnes ne crée pas ce bit au runtime.

De même, `smax>=n` exclut seulement la censure par `smax`; il ne certifie pas la
complétude de la famille saturée. Les gardes qui supposent la source complète
doivent recevoir `source_complete_for_order[k]`, pas déduire cette propriété
d'une comparaison de deux entiers.

Sans ces certificats, les records peuvent rester utiles avec la sémantique
`relative_to_certified_subfamily`, mais ce suffixe décrit une preuve réellement
présente; il ne doit pas être imprimé par défaut sur une sous-famille sans
certificat.

## Décision proposée

Le triple de comptes par niveau est un excellent premier falsificateur et doit
être conservé comme reçu de masse. La promotion au transcript exact demande le
vecteur canonique de records par témoin, une provenance runtime et au moins un
mutant du marquage sujet. Cette extension reste locale aux racines touchées et
respecte entièrement l'architecture légère.

GCP non utilisé pour cette note.
