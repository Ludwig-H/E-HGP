# Contre-audit live de la borne supérieure de crédits

Date : 14 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## 0. Snapshot et verdict

Le parent stable est
`HEAD=a58d0207d8e0482ced4b0207144fa311193c0388`, commit
`le premier certificat qui retire du travail, et non qui en ajoute`. Le delta
logiciel mobile de Claude ajoute `--borne-sup` à
`prototype/wspd_wavefront_probe.cpp`. Le snapshot falsifié ci-dessous a le
SHA-256
`906408854e4592de6649d3854d6cfc6c57c61936655fd8c93ddcf572f59f9d44` ;
le binaire Release rejoué a le SHA-256
`cd808bfdfce6cf280ba6836086126755a6e02229ab0c79ad3f6547bb9c7ba324`.
Le worktree est mobile et ce pin de contenu prime sur toute mention de
« version courante ». Claude a ensuite produit la réparation mobile
`ec5ec3d447bf94efb51bb34e88010d224561d3ff6920556e2a2872e091b0df84`,
qui réintroduit `pop(cg)+pop(cd)` par bit `MIXED`. Son binaire Release rejoué a
le SHA-256
`1e26cc2d4f82c6883e2d742ced1ff2a49f01ef46f876f60a99172d98264fbf6c`.

Verdict : le lemme visé est utile et potentiellement très bon pour le coût,
mais sa première implémentation est **fausse**. Elle retire la population
d'un parent `MIXED` sans remettre celle de ses deux enfants. La prétendue borne
supérieure tombe donc artificiellement à zéro, les trois lanes sont déclarées
mortes et une sortie entièrement ouverte est publiée avec `pending=0` et
`fenetre_finale=OUI`. La révision `ec5ec3d4` répare cette faute précise et
retrouve les mêmes fates/masses que la baseline sur l'ablation ci-dessous ;
elle reste non reçue tant que la parité n'est pas une CTest et que les ledgers
combinés et BJD ne sont pas traités. La borne est paritaire sur l'antichaîne de
`--climb`, mais cette source omet indépendamment une feuille et n'est donc pas
une source complète.

L'auditeur n'a modifié aucun logiciel. GCP non utilisé.

## 1. Lemme exact, par ledger

Pour une lane `q` et un ledger logique `L`, noter `cred[L,q]` le crédit déjà
authentifié et `stack[L,q]` les tâches encore susceptibles de produire des
identités. Si les tâches forment une antichaîne de populations disjointes, la
quantité suivante majore le nombre de crédits singleton encore atteignable par
ce ledger :

```text
upper[L,q] = cred[L,q] + sum(pop(t) : t dans stack et q dans mask_L(t))
```

À chaque transition, l'invariant est :

```text
pop parent = pop enfant_gauche + pop enfant_droit
ALL   : retirer pop(parent) du reste, ajouter pop(parent) au crédit
NONE  : retirer pop(parent) du reste
MIXED : remplacer pop(parent) par les deux populations enfants
```

Ainsi `upper[L,q] < need[q]` prouve que **ce ledger et ce pool de tâches** ne
peuvent plus fermer la lane. Cette borne doit être maintenue séparément pour la
baseline et la vue combinée : `cred` et `ccred` diffèrent, tout comme `mask` et
`cmask`. Utiliser la borne baseline pour éteindre la vue combinée peut perdre un
succès SOC déjà proche du seuil.

La portée ne doit pas être sur-vendue. Un `NONE` du certificateur central, un
cap ou un proposant non invoqué peuvent cacher une preuve géométrique dans un
raffinement ultérieur. La borne prouve donc `OPEN_FOR_THIS_LEDGER`, jamais
« aucun événement Morse n'existe ». Elle ne devient une preuve d'ouverture
géométrique qu'après réception d'une source complète et de tous les
certificateurs collectifs applicables. Si des groupes Jung sont encore
possibles, chaque crédit de groupe doit consommer au moins un vrai `PointId`
distinct du même reste authentifié ; sinon `cred+reste` ne les majore pas.

## 2. Faute exacte du snapshot `90640885`

Au dépilement, le code soustrait bien `pop_of(tk.node)` de chaque lane de
`tk.mask`. Lorsqu'un nœud interne est `MIXED`, il pousse ensuite ses deux
enfants, mais n'ajoute ni `pop_of(cg)` ni `pop_of(cd)` à `reste`. Comme la pile
initiale contient la racine par défaut, le premier `MIXED` produit alors :

```text
reste avant pop = n
reste après pop = 0
deux enfants encore vivants dans la pile
cred + reste < need
lane déclarée morte à tort
```

Ce n'est pas une simple perte de complétude sous cap : la sortie affirme une
finalité et efface des fermetures qu'une exécution sans l'option trouve.

Commandes appariées :

```bash
build/v3/mhgp3v_wspd_wavefront_probe --family=eight_clusters --points=1500 --sep-euclid=8/1 --tight --vwave --window=512 --window-ledger
build/v3/mhgp3v_wspd_wavefront_probe --family=eight_clusters --points=1500 --sep-euclid=8/1 --tight --vwave --window=512 --window-ledger --borne-sup
```

Résultat déterministe :

```text
baseline : lectures=9 570 325 ; fermetures q2/q3/q4=90 735/18 609/11 853
borne-sup: lectures=129 764   ; fermetures q2/q3/q4=0/0/0
baseline : masses ouvertes q2/q3/q4=106 809/1 013 842/1 071 162
borne-sup: masses ouvertes q2/q3/q4=1 124 250/1 124 250/1 124 250
borne-sup: lanes_mortes=389 292 ; pending=0 ; fenetre_finale=OUI
```

Le facteur de temps apparent n'est donc aucun gain : il provient d'un abandon
incorrect de presque toute la vague.

Sur la réparation `ec5ec3d4`, la même commande donne les mêmes fermetures et
masses q2/q3/q4 que la baseline, avec `lectures 9 570 325 -> 9 567 705`,
`lanes_mortes=268 095` et `visites_evitees=385`. Ce rejeu reçoit la conservation
sur une configuration, pas le contrat général ni un gain temporel.

## 3. Réparation et portes exigées

Deux écritures sûres sont possibles : ne soustraire définitivement la
population qu'après un verdict `ALL/NONE`, ou la soustraire au pop puis la
réajouter par bit `MIXED` lors du push. Dans la seconde forme, vérifier
explicitement `pop(cg)+pop(cd)=pop(parent)` et garder deux tableaux de reste,
un par ledger.

La forme la plus simple maintient directement `upper=cred+reste` : `ALL` et un
split `MIXED` le laissent inchangé ; seul un `NONE` consommé ou un `MIXED`
terminal sans continuation retire sa population. Elle évite le couple
soustraction/réinsertion, rend le mutant initial structurellement impossible et
réduit les mises à jour du hot path. Elle exige toujours un `upper` par ledger
et un potentiel séparé pour tout proposant post-boucle.

Deux interactions propres à la borne restent bloquantes dans `ec5ec3d4` :

- la mortalité est calculée avec `cred/mask`, puis éteint aussi `cmask`, alors
  que la vue SOC combinée peut avoir `ccred>cred` ;
- BJD crédite après la boucle à partir d'une banque de feuilles déjà retirées
  de `reste`, donc `cred+reste` ne majore pas son crédit collectif futur ;

Elles produisent déjà des divergences sans troncature :

```text
terrain,n=16,coord=64,seed=3,SOC64-shadow : fermetures combinees 1 -> 0
terrain,n=64,coord=64,seed=3,BJD8          : fermetures q4 84 -> 82
                                              groupes BJD 338 -> 319
```

Pour BJD, la borne composée doit inclure un potentiel de banque. Avec les
groupes binaires disjoints actuels, une majoration simple est
`cred+reste+min(cap_groupes,floor(nfeuilles_libres/2))`; les membres déjà
dépilés vivent dans `feuilles_vues`, donc précisément hors de `reste`.

Ces combinaisons doivent être refusées jusqu'à l'existence d'un potentiel par
ledger incluant la banque BJD. `--climb` est un défaut distinct : ses frères
successifs forment bien une antichaîne disjointe et la borne OFF/ON est paritaire
sur ce parcours, mais leur union omet `pos0`. Empiler aussi cette feuille est
nécessaire avant de qualifier la source de finale.

La contradiction BJD est exécutable au même pin : sur `uniform,n=200`, avec
`--tight --vwave --window=512 --window-ledger --bjd-groupes=8`, l'ajout de
`--borne-sup` change les fermetures q4 de `2510` à `2509`, la masse fermée de
`4309` à `4308` et les groupes couvrants de `8151` à `8132`, tout en gardant
code zéro. Une autre fixture `eight_clusters,n=200,seed=8` passe de 151 à 150
fermetures q4. Le refus de la combinaison est donc P0, pas une précaution
théorique.

Avant toute mesure de performance :

- comparer option OFF/ON sur les mêmes nuages et exiger des fates et masses
  q2/q3/q4 bit-identiques, `pending` et troncatures compris ;
- graver un mutant `forget-mixed-children` tué par une fixture dont la racine
  est `MIXED` mais dont un enfant contient assez de témoins pour fermer ;
- exercer séparément baseline et vue combinée avec `cred != ccred` ;
- exercer `--climb`, `--none-descend`, Midball, SOC64 et BJD, car ils changent
  les masks ou les familles de preuves ;
- publier morts par lane et localement par taille. Le compteur actuel
  `visites_evitees` ne compte que des racines dépilées déjà mortes : à `n=16`,
  `994` lectures disparaissent mais il n'affiche que `2`. Le renommer
  `racines_elaguees` et publier le delta apparié de lectures ;
- propager le masque `mort` au statut final. Sous `window=8`, dix preuves de
  mort sont encore toutes imprimées `pending`, car seul `mort==7` évite la
  troncature globale ;
- cap ou ledger incomplet donne `PARTIAL/UNKNOWN`, jamais ouverture exacte.

Après ces portes, ce critère est prometteur : il peut arrêter causalement une
descente témoin dont la masse restante est insuffisante, contrairement au
packing BJD tardif. Le GO performance exige néanmoins une baisse appariée des
recertifications **et** du temps, sur uniforme et huit amas, sans changement de
fate. Aucun résultat 50 000/G4 n'en découle encore.

## 4. Contre-audit croisé

Le contre-audit de l'autre flux a correctement retiré deux raccourcis :

- `central NONE => Midball ALL impossible` est faux ; la fixture
  `A=[0,8], B=[10,100], C={9}` impose d'essayer Midball sur tout verdict
  non-`ALL` ;
- le coût de 72 multiplications au callsite Midball était un compte source,
  pas le bloc Release : le désassemblage montre que l'inlining élimine déjà le
  maximum inutilisé et conserve 24 multiplications.

Aucune identité indépendante n'est déduite de ces flux. Les conclusions sont
reçues par redérivation, fixture et snapshot, pas par autorité personnelle.
