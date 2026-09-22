# P0 — identités du nuage, du rectangle et des permutations

13 septembre 2026. Cadre : `phase=exploration_v8_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Le partage annoncé `CloudOwner/RectangleView` peut conserver les garanties
actuelles avec deux compatibilités distinctes : même nuage pour l'index Z
et sa requête ; même contexte de rectangle pour une restriction locale
et son plan axial. Les trois petites fixtures ci-dessous passent avec les
API actuelles. Elles réfutent des **modèles explicitement faux de migration**,
sans alléguer un défaut du produit actuel ni simuler un futur produit déjà
implémenté. Aucun moteur WSPD global, gain ou borne globale n'est qualifié.

## Ce qui est vérifié sur les API actuelles

Le [juge C++](cloud_rectangle_identity_probe.cpp), le
[runner](cloud_rectangle_identity_checks.py) et le
[reçu autonome](CLOUD_RECTANGLE_IDENTITY_CHECKS.json) capturent dix sources
produit et le juge dans un répertoire temporaire privé. Le HEAD de contexte
est `1c523fbe6e3ebb4a6b0aefdcb809961891aef713` ; la preuve porte sur les
contenus embarqués, dont le census en chantier `b1ca5edd575f39bc…` et les
bornes préparées `7bb46b4b7af3beed…`, avec hashes complets dans le reçu.
Les sources sont restées identiques pendant la capture et jusqu'à la
clôture du run normal. Ce contrôle d'identités ne requalifie pas à lui seul
la nouvelle arithmétique des bornes préparées.

1. **Le facteur opposé change les crédits : quatre sites.** Les coordonnées
   sont `(100,0,0)`, `(101,0,0)`, `(200,0,0)`, `(0,0,0)`, d'IDs originaux
   0, 1, 2, 3. Les deux entrées utilisent ce même ordre, `A={0,1}`,
   `B1={2}`, `B2={3}`, `Kmax=1`, `s=12`, sans cœur. Les factories actuelles
   produisent deux propriétaires privés. Pour chacune des stratégies Pool,
   DualBlocks et Tubes, les vrais crédits A sont `[1,0]` sur R1 et `[0,1]`
   sur R2, avec crédit B nul. Les deux modes census rendent respectivement
   les seuls supports `(1,2)` et `(0,3)`. Le faux modèle réutilisant les
   crédits R1 par rang local dans R2 supprime `(0,3)`, dont la profondeur
   stricte réelle est 0, et conserve `(1,3)`, de profondeur 1. La seule
   égalité du nuage, des tailles et de A ne suffit donc pas. Le vrai
   constructeur axial refuse la restriction R1 sur R2 dans les trois
   stratégies ; le vrai census refuse aussi l'index R1 avec le plan R2.
2. **Le seuil appartient à la requête : trois sites.** Les points sont
   `(0,0,0)`, `(100,0,0)`, `(50,0,0)`, avec `A={0}`, `B={1}`, sans cœur.
   Le même support a profondeur stricte 1. Les vrais plans conservent une
   candidate à Kmax 1 comme à Kmax 2 ; le census la rejette à 1 et la rend
   à 2. Le faux modèle prenant le seuil dans le premier index Kmax 1 la
   perd pour la requête Kmax 2. Aujourd'hui l'index et le plan de ces deux
   propriétaires différents sont bien refusés. Ce cas précise le raccord
   nécessaire si un futur index peut être réemployé entre seuils.
3. **Un ensemble B ne détermine pas son ordre : cinq sites.** Les points
   sont `(0,0,0)`, `(0,1,0)`, `(0,0,1)`, `(100,2,2)`, `(100,2,1)`, avec
   `A={0,1,2}`, `B={3,4}`, `Kmax=2`, `s=12`, sans cœur. Sur le même
   propriétaire, le vrai plan Independent conserve `b_order=[3,4]` sans
   arbre B ; Additive construit son arbre et produit `[4,3]`. Les deux
   modes census des deux plans donnent les mêmes cinq supports. Le
   descripteur Additive d'ancre 0, plage B `[0,1)`, désigne le site 4,
   de profondeur 1. Le faux modèle reprenant la boîte singleton du rang
   0 d'Independent utilise le site 3, de profondeur 3, et rejette cette
   plage valide. Un cache de boîtes B fondé seulement sur l'ensemble des
   sites serait donc insuffisant, même pour un unique rectangle et seuil.

GCC 13.3, C++20, `-O1 -Wall -Wextra -Wpedantic -Werror`, UBSan sans
récupération : 20 appels census, 34 supports émis contrôlés, 191 tests
scalaires de sites et cinq rejets `invalid_argument`. Le juge compare les
supports entre Pairwise et SharedBlocks, vérifie leur unicité, les comptes
de candidates/acceptées/rejetées, la cardinalité intérieure contre une
formule scalaire indépendante et la présence des extrémités dans la
coquille. Les trois faux transferts R1→R2, le faux seuil et le faux cache
de permutation sont tous réfutés. Le rejeu `python3 -O` donne les mêmes
résultats et reste effectif sans `assert` Python. Aucune mesure de temps
ni grosse fixture n'entre dans cette porte.

```bash
python3 audits/morsehgp3D_v8_complementaire/cloud_rectangle_identity_checks.py --replay audits/morsehgp3D_v8_complementaire/CLOUD_RECTANGLE_IDENTITY_CHECKS.json --output /tmp/mhgp8_cloud_identity_replay.json
python3 -O audits/morsehgp3D_v8_complementaire/cloud_rectangle_identity_checks.py --replay audits/morsehgp3D_v8_complementaire/CLOUD_RECTANGLE_IDENTITY_CHECKS.json --output /tmp/mhgp8_cloud_identity_replay_optimized.json
```

Le reçu conserve également l'échec initial du juge : le mauvais site de
la troisième fixture avait été annoté profondeur 2 au lieu de 3. La
formule scalaire l'a réfuté ; seule cette attente du juge a été corrigée.
Les sources produit de l'essai échoué sont identiques à celles du reçu
final ; son ancien juge et ses commandes restent embarqués.

## Contrat minimal proposé pour la migration

| Objet | Identité et garantie nécessaires |
| --- | --- |
| Nuage | Stockage privé immuable, profil et espace des IDs originaux. La factory valide les sites une fois avant publication. Un transfert de vecteur du client ne révoque pas ses anciens alias mutables. |
| Facteur | Référence au nuage et à une permutation immuable certifiée, plage ou nœud de cette permutation, cardinal et boîte calculés par le producteur de l'index. Un ID de nœud ou une boîte fournis librement ne constituent pas un certificat. |
| Rectangle | Deux facteurs **ordonnés**, même nuage, non vides et disjoints comme ensembles d'IDs, séparation certifiée sur leurs boîtes. Ses preuves de cœur restent liées à ces facteurs. |
| Contexte de plan | Rectangle, lane, seuil et politique de cœur immuables. Si Kmax reste dans le rectangle comme aujourd'hui, l'identité de ce rectangle couvre déjà le seuil. S'il en sort, cette partie du contrôle doit devenir explicite. |
| Plans et plages | Le plan garde son contexte et ses tableaux cohérents. Une plage désigne des rangs dans **sa** permutation ; une autre permutation exige un raccord déclaré vers les IDs originaux. |
| Index Z | Référence au nuage et identité propre de sa disposition de nœuds. La requête prend son seuil dans le contexte du plan ; une continuation de parcours appartient à cet index précis. |

Une restriction et son plan axial doivent partager le contexte rectangle,
lane et seuil, même si l'index Z accepte plusieurs contextes du même nuage.
Le résidu construit à Kmax 1 n'est pas un préfiltre complet pour Kmax 2 :
dans R2 ci-dessus, `(1,3)` est éliminé à 1 mais devient admissible à 2.
Des minorants numériques plafonnés peuvent éventuellement être réemployés
après un raccord prouvé et reconstruction du résidu au nouveau seuil ;
cela n'autorise pas à réutiliser les anciens descripteurs. La proposition
minimale garde une égalité de contexte stricte.

Pour des facteurs qui sont des intervalles d'une même permutation globale,
une permutation bijective et son inverse donnent `original_id(rang)` et
`rang(id_original)` sans table de taille n par rectangle. Deux plages
disjointes prouvent alors la disjonction des facteurs. Deux plages dans
des permutations différentes ne donnent pas cette preuve. Les rangs
physiques ne doivent pas devenir des IDs de sortie. Une factory limitée
à des nœuds certifiés permet de consulter cardinal et boîte sans rescanner
les sites ; accepter des listes d'IDs arbitraires requiert de payer et de
compter leur validation et leur correspondance.

Le cache de requêtes B peut partager une permutation canonique déclarée,
ou être lié à une instance immuable de permutation de plan. L'ensemble B
ne suffit pas, comme le montre la troisième fixture. De même, deux index Z
sur le même nuage peuvent numéroter leurs nœuds différemment : une frontière
de parcours ne migre pas entre eux sous la seule garantie « même nuage ».

Le cœur conserve ses obligations : sites distincts en IDs originaux,
extérieurs aux deux facteurs et certification stricte universelle pour
le rectangle et la lane effectivement utilisés. Une restriction prouvée
`A'⊆A`, `B'⊆B` peut hériter d'un minorant de cœur du parent : les anciens
sites restent extérieurs et la propriété universelle se restreint ; le
plafond est adapté au nouveau seuil. Le cas général exige sa propre preuve.
Un scalaire de cœur ne permet ni union sans doublons ni exclusion d'IDs
du census. Le census actuel repart à zéro et compte tous les sites du
nuage, politique qui évite cette obligation de soustraction.

Le graphe de durée de vie doit rester sans cycle de possession forte.
Par exemple, données du nuage immuables sans référence forte de retour ;
index, arbre de facteurs et rectangles possèdent ces données, puis plans
possèdent leur contexte. Un objet de session extérieur peut posséder le
nuage et ses index. Insérer un index possédant le nuage dans un cache lui-même
possédé fortement par ce nuage créerait un cycle `shared_ptr`. Publier
uniquement les objets complètement construits évite également qu'un
`bad_alloc` laisse une entrée de cache partielle. Les garanties déjà testées
sur les plans restent à recertifier au nouveau graphe de possession.

## Raccords précis dans les sources capturées et coût restant

Les contrôles actuels à préserver se trouvent dans la factory
[`prepare_rectangle`](../../morsehgp3D_v8/src/pipeline/local_credits.cpp)
(lignes 277–349 du snapshot), puis dans
[`AxisQ2Plan`](../../morsehgp3D_v8/src/pipeline/axis_q2.cpp)
(217–239 : propriétaire/lane avant copie des restrictions). La déclaration
[`Range`](../../morsehgp3D_v8/src/pipeline/local_credits.hpp) vise encore
l'ordre original. Les expressions `original.first+i` et `a_id-a.first`
dans les lignes 270 et 461 de `local_credits.cpp` demandent un raccord
explicite si les facteurs deviennent des plages spatiales.

Dans [`q2_census.cpp`](../../morsehgp3D_v8/src/pipeline/q2_census.cpp), le
snapshot lit actuellement le seuil dans `input_index.rectangle().kmax()`
(180) ; le contrôle de propriétaire strict précède le travail (446–448).
L'arbre de requêtes respecte déjà la permutation du plan (185–203).
Relâcher le premier contrôle vers le nuage implique de changer la source
du seuil, sans relâcher celui des restrictions ni celui des permutations.
Ces numéros décrivent les contenus embarqués ; les sources en chantier
peuvent ensuite évoluer.

Les postes à compter séparément sont la copie/validation globale,
permutation et inverse, arbre de facteurs, index Z, préparation et plans
par rectangle, éventuels raccords de permutations et sorties. Le partage
du nuage ne borne pas la somme des tailles de facteurs ni les visites
census sur la WSPD. Enfin, certifier chaque rectangle ne prouve pas que
le producteur WSPD couvre chaque paire non ordonnée exactement une fois :
couverture et orientation canonique restent un contrat global distinct.

Ce texte complète les portées de cache discutées dans
[census partagé et seuils](P0_CENSUS_PARTAGE_ET_SEUILS.md), ainsi que les
[garanties d'exceptions du census](P0_CENSUS_Q2_ET_EXCEPTIONS.md).
GCP non utilisé.
