# GAPP2 : proposition, issue exacte et contrat

8 octobre 2026. Source Git `9815c19b9a69b00cc927dc61837ef7c5dac63615`, quatorze fichiers
épinglés dans `capture.json`. Contrelecture de source et preuve conditionnelle ; aucun moteur,
build, GPU ni payload de données exécuté ou lu. Le commit ne modifie ni `src/` ni le contrat
de tour. Les résultats de campagne sont jugés séparément dans
[`gapp2_admission`](../gapp2_admission/README.md) ; ce reçu ne les réadmet pas.

## Proposition approximative, décision exacte

D1 propose avec L4 binaire64 sur les deux voies ; D2 compare la proposition produit binaire64
hôte à L4 binaire32 appareil. Les classes L4 proviennent des substitutions épinglées du bras
T2-d-B : le rejeu Python retrouve exactement le bloc généré dans `noyau_g.hpp`. Cette identité
textuelle n'est pas une qualification des compilateurs ni de leurs résultats flottants.

Dans `proposer_l4_flottant`, les coordonnées `u32` sont converties en `i64` **avant** soustraction,
puis converties en `float` ou `double`. À u21, ces différences entières sont représentables en
binaire32 ; leurs produits, centres et rayons demeurent approximatifs. `classer` ne consulte
pas le centre ni le rayon proposés : seulement le statut, l'arité et les indices du support.
Il applique `lem_t1`, sinon `certify_part`, puis le repli `exact_support` et le même certificat.
La voie entière elle-même écrit des centre/rayon nuls de remplacement : la proposition n'est
donc jamais un objet géométrique certifié directement consommable.

**Pourquoi l'issue peut rester identique.** Pour une partie valide F, un support S contenu
dans F, tous ses points sur une sphère de centre c, avec c dans conv(S), et F contenu dans
la boule fermée, certifient la plus petite boule de F. En effet, pour des poids positifs de
barycentre c et tout centre z,
`sum(lambda_i * ||s_i-z||²) = R² + ||c-z||²` : toute boule contenant F a rayon au moins R.
L'unicité de cette plus petite boule exclut deux réponses certifiées différentes. Les poids,
les côtés et la canonisation sont ici décidés exactement par le produit.

La canonisation dans `supports.cpp` choisit l'arité minimale puis le support minimal par
positions parmi la coquille de F. Si le support canonique global d'une boule du catalogue
est dans F, il est aussi ce minimum local ; `lem_t1` ou la table donnent donc la même boule.
Sinon, les propositions certifiées donnent le même support canonique local et la même issue
« census ». Des présentations internes de `CertifiedBall` peuvent néanmoins différer ; on
ne prouve pas l'identité de leurs octets. Cette preuve suppose catalogue valide, partie de
sites distincts, domaine numérique admis et appels exacts réussis.

`memes_issues` compare la boule pour « table », ou arité/support pour « census » ; il ne
compare volontairement **pas** le mécanisme T1/certificat/repli. Le mutant sans test du
diamètre peut ainsi être rattrapé par le certificat et le repli exact : son rejet par trop
de replis est une porte de coût/contrôle, pas nécessairement une sortie géométrique fausse.

## Frontières que le microbanc ne franchit pas

- Les propositions portent les parties ayant raté la **première** sonde, pas tous les pas
  des chaînes G. Le lot de requêtes census est toujours récolté par la politique p64.
  On ne rejoue donc pas ici toutes les chaînes et tous leurs censuses issus de l4f32.
- `LotIssues` et sa certification exacte sont exécutés sur CPU **après** toutes les passes
  chronométrées (`mes_g_app.cpp:944`). Les temps L4 appareil comprennent l'initialisation
  du compteur, le noyau entier, la compaction et le noyau flottant ; allocations,
  chargements initiaux, rapatriement et certification sont hors de cette fenêtre.
  Ils ne mesurent ni G intégré ni FULL, ni un coût total de repli sur GPU.
- L'identité compare les buffers finaux aux références ; elle ne certifie pas séparément
  chaque buffer de chaque passe chronométrée. L'égalité des issues n'est ni une nouvelle
  empreinte FULL ni une preuve de l'identité de tous les compteurs de mécanismes.
- `classer` n'est pas un remplacement direct de `locate` : à la ligne 444, un
  `certify_part` en erreur est absorbé dans le repli ; le produit (`resolve.cpp:135`)
  propage ce refus typé. Un indice proposé hors F est aussi remplacé par le premier site,
  alors que le produit contrôle son appartenance. La certification reste la barrière
  géométrique, mais les domaines et refus ne sont pas qualifiés identiques. Pour intégrer,
  conserver les refus du produit et rendre explicites les propositions invalides.

Le chemin entier L4 annonce **B ≤ 30**. Ses distances et produits scalaires additionnent
trois produits de différences de sites : leur valeur absolue est bornée par
`3*(2^B-1)^2`, inférieure à `2^63` pour B=30, mais pas pour B=31 ou 32. L'ancrage local
n'élargit pas les différences entre deux sites. La campagne u21 ne qualifie donc pas ce
corps `i64` à u32 ; il faut un domaine contrôlé ou une arithmétique plus large avant ce port.

## Règle nouvelle et portée du § 8

L'étape 2 remplace les critères du premier microbanc par une règle déclarée distincte.
Pour D2, bornes hautes des IC : somme census+sondes+proposition appareil / référence hôte
≤ 0,10 et proposition seule ≤ 0,20. Pour D1 : somme ≤ 0,15, proposition ≤ 0,50 et L4 hôte /
proposition hôte de référence ≤ 1,05. Les identités exactes, taux de repli/non résolu,
A/A, isolation et provenance restent des conditions supplémentaires. Ces rapports de
microbanc ne sont pas des rapports FULL. Ils ne réhabilitent pas rétroactivement le
premier résultat rejeté ; le jugement séparé de GAPP2 rejette également D1 et D2.

Le § 6 autorise déjà une proposition flottante qui ne décide rien ; passer en binaire32
n'exige donc pas d'affaiblir l'exactitude géométrique. Le § 8 demande des compteurs de
travail identiques entre hôte et appareil **pour une même politique**, dont les routes
T1/certificat/repli. La preuve d'issue ci-dessus ne prouve pas cette égalité de routes.
Une intégration doit déclarer précisément sa politique et ses compteurs : soit politiques
distinctes, avec identité exigée au sein de chacune, soit modification explicite de la
classification des compteurs si des mécanismes différents relèvent d'une même politique.
Le texte d'amendement de l'étude est une proposition, pas une modification acquise du
contrat. Avec D1/D2 rejetés, aucune adoption ni décision d'amendement n'est à déduire.

Pour un futur essai intégré, garder certification/replis et refus dans la source produit,
le census complet canonique et les sauts déterministes ; vérifier toutes les chaînes,
les empreintes FULL et les compteurs de la politique déclarée. Les buffers microbanc hors
`MemoryBudget`, leurs durées de vie, les transferts et la certification doivent entrer dans
le coût et le budget de ce chemin. Les scénarios de temps extrapolés de l'étude ne
remplacent pas cette mesure, particulièrement quand G recouvre la forêt.

## Rejeu borné

Depuis ce dossier, avec les objets Git accessibles :

```sh
python check.py /workspaces/E-HGP
python -O check.py /workspaces/E-HGP
sha256sum -c SHA256SUMS
```

Les deux sorties sont identiques au champ `result` de `capture.json` : quatorze hashes,
contrat inchangé, absence de changement produit dans ce commit, génération exacte des
deux classes et borne entière B30. Le vérificateur n'exécute pas le C++, ne teste pas la
sémantique GPU et ne remplace pas la preuve conditionnelle exposée ci-dessus.
