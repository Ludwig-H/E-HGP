# État courant audité de MorseHGP3D v5 — 30 août 2026

Cadre : `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Dernier pin Claude relu : `495b234f`, documentaire seulement. Dernier pin
source fonctionnel relu : `351faccc`. Le worktree partagé contenait pendant la
revue un candidat fonctionnel `s>=8` non commité et d'attribution mélangée ; il
est traité comme proposition à contre-relire, jamais comme livraison ni comme
autorité.

## Verdict

Le travail de génération q3/q4 est utile et peut continuer, mais l'objet qu'il
alimente doit être requalifié avant toute promotion sémantique : la v5 actuelle
émet les événements du **sous-flot Gabriel horizontal**, puis les deltas d'un
K-MST sur ses facettes. Ce n'est ni le `hgp_reduced` exact défini par le contrat
actif Gamma, ni encore une hiérarchie partielle munie d'une projection prouvée.

Ce verdict bloque un claim d'exactitude, pas l'exploration : les événements
Gabriel restent une source positive et les optimisations de Claude restent
réutilisables. La réparation constructive consiste à nommer ce sous-flot,
refuser qu'il satisfasse `require_exact`, puis ajouter la source d'incidences
silencieuses et son oracle indépendant au lieu de jeter le chantier.

## P0 — la sémantique de forêt est surqualifiée

Les autorités racine sont explicites :

- `docs/SPECIFICATION_MORSEHGP3D.md` § 1 et § 17 réserve au flot Gabriel brut
  les rôles de proposition, connectivité positive ou compression et exige les
  cofaces Gamma, y compris les incidences silencieuses ;
- `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` enregistre la réduction brute
  comme fausse en général ;
- `docs/TEST_PLAN_MORSEHGP3D.md` exige `forest_semantics=partial_refinement`
  quand la source n'est pas complète et un échec explicite si
  `require_exact=true`.

À l'inverse, `morsehgp3D_v5/README.md`,
`morsehgp3D_v5/docs/ARCHITECTURE.md`,
`morsehgp3D_v5/docs/MATHEMATIQUES.md` § 7 et
`src/forest/fold.hpp` appellent encore le K-MST du graphe de Gabriel la forêt
HGP exacte. `ForestResult::deltas` est décrit comme « payload hiérarchique
complet ». `print_run` publie simultanément
`tower_scope=profile_complete_k10` et `vertical_maps=none`, sans exposer
`proof_basis`, `forest_semantics`, `reconstruction_contract_id` ni
`require_exact`.

Le juge `mhgp5_forest_judge` est bien indépendant pour les calculs rationnels,
les niveaux et les rôles. Il ne l'est pas pour l'objet : son étape 3 filtre les
cofaces Gabriel, puis son étape 5 construit le graphe à partir de ce même
univers. Il requalifie donc correctement le sous-flot v4/Gabriel, mais ne peut
pas prouver sa complétude Gamma.

### Fixture obligatoire `gabriel-point-set-counterexample-5-points-v1`

Le nom court « E5 » est ambigu avec l'exemple planaire E5 de
`docs/contracts/EXEMPLES_CONTRACTUELS.md`. Employer l'identifiant canonique
ci-dessus, ou à défaut « E5 tridimensionnelle ».

La fixture contractuelle est :

```text
A=(0,0,7)  B=(0,9,6)  C=(1,4,0)  D=(0,0,1)  E=(4,1,2)
```

À l'ordre 2, les cofaces non-Gabriel `ACD` et `ACE` attachent silencieusement
`AC` au niveau carré `33/2`. La coface Gabriel future `ABC` réutilise `AC` au
niveau `83886/3563`. Gamma possède alors une seule composante couvrant
`ABCDE`, tandis que le flot Gabriel brut conserve `ABC` séparé de `ACDE` et ne
les réunit qu'au niveau `24` par `BCE`. Ce n'est donc pas
seulement une différence de représentation facettée : la généalogie et le
temps de fusion des unions de points divergent. `BCE` reste une vraie coface
Gamma : c'est sa promotion tardive en nœud de fusion, pas la coface elle-même,
qui est artificielle relativement à la généalogie Gamma.

Un recalcul rationnel indépendant retrouve aussi `CDE=162/25` et
`ADE=189/17`, puis les cinq niveaux ci-dessus. La porte racine
`tests/oracle/test_gabriel_counterexample.py` passe `4/4`, et un probe borné
contre le worktree v5 émet `CDE,ADE,ABC,BCE,BCD`, jamais `ACD/ACE`. Le
diagnostic n'est donc pas seulement documentaire.

### Fermeture minimale attendue

1. Renommer la sortie actuelle avec
   `proof_basis=gabriel_positive_connectivity` et provisoirement
   `forest_semantics=verified_events_only`; remplacer `tower_scope` par une
   portée horizontale explicite tant que `vertical_maps=none`. Le statut
   `partial_refinement` exige d'abord une projection réduite et un
   `PartialScope` démontré : les `ComponentDelta` actuels comptent encore les
   facettes isolées implicites comme parents.
2. Ajouter `reconstruction_contract_id` et `require_exact`; refuser
   atomiquement la combinaison `require_exact=true` avec cette source.
3. Graver `gabriel-point-set-counterexample-5-points-v1` dans une porte v5 qui
   compare un oracle Gamma exhaustif borné au sujet. Le mode Gabriel doit y
   produire la divergence attendue, jamais un faux accord.
4. Ensuite seulement, raccorder une source sparse d'incidences silencieuses et
   comparer à Gamma aux coupes ouvertes et fermées. L'oracle reste borné et ne
   devient pas l'architecture produit.
5. Requalifier le rendu actuel comme rendu Gabriel borné. Il n'implémente pas
   encore le payload Gamma complet de la section 9.1.

## P1 — corrections au contre-audit d'optimisation

### `s>=8` est le plancher normatif du profil produit, pas encore livré

Décision utilisateur du 30 août : `s=1` n'a pas de sens pour le produit v5 et
la séparation entière minimale est 8 pour toute la voie produit, q2 comprise.
Au pin `495b234f`, cette décision est seulement documentée : la version commitée
de `validate_run_options` et des deux CLI refuse encore uniquement `s<1`, et
`alive_rectangles` n'a aucune garde interne. `s=1` et `s=7` restent donc
acceptés contrairement au contrat demandé ; `s=8` n'est pas encore une
frontière exécutable reçue.

La justification continue correcte se place sur un rectangle terminal : poser
$M=\max(r_A,r_B)$ et $g=d-r_A-r_B\geq sM$. Pour $M>0$, le rayon de décision
continu vérifie $R_{\mathrm{dec},q}\geq(\kappa_qs-1)M$. Exiger uniformément la
marge forte $R_{\mathrm{dec},q}>M$ donne $s>2/\kappa_q$ ; la lane limitante q4
impose $s>2/\sin(15^\circ)\simeq7{,}727$, d'où le plus petit entier 8. Le cas
$M=0$ se traite séparément et garde un rayon positif pour deux singletons
distincts. Cette dérivation ne prouve pas automatiquement que l'arrondi entier
dirigé de `core_ball().radius4` reste supérieur à $M$ ; le seuil produit est
normatif et ce transfert logiciel demande sa propre porte. Il ne faut surtout
pas en déduire que tout citron particulier est vide sous 8. Les primitives WSPD
et des contre-fixtures de génération peuvent encore exercer une petite
séparation par un opt-in test-only explicite, sans produire un résultat
`run_pipeline` recevable.

Le candidat du worktree va dans la bonne direction : garde globale, refus
interne explicite, champ haut niveau borné par `MHGP5_TESTING`, propagation à la
fixture q2 et comparaison large sans overflow. Le booléen de bypass de la
primitive basse reste toutefois compilé dans le produit, et l'opt-in n'est pas
propagé aux lanes batch/device. Comme ce candidat n'est ni commité ni
attribuable à un seul auteur dans le worktree partagé, l'audit ne le reçoit
pas. Claude doit porter et épingler son patch cohérent avant que ces points
puissent être déclarés fermés.

La réception devra graver `s=0,1,7` en rejet, `s=8` en limite positive, le code
2 et le texte exact aux deux CLI, ainsi que le refus API avant génération. Elle
doit aussi rejouer le mutant q2 test-only, les mutants post-séparation et la
frontière arithmétique large. Accepter `s=INT64_MAX` peut être sûr
arithmétiquement tout en forçant une WSPD quadratique : toute revendication
sous-quadratique suppose s fixé indépendamment de n, ou une borne supérieure de
profil.

#### Contre-relecture du nouveau `PROFIL_SEPARATION.md`

Le seuil produit reste reçu, mais le document candidat réintroduit trois erreurs
que le code venait justement de corriger :

- $(\kappa_q-2/s)d_{\min}$ n'est pas la demi-largeur exacte du citron commun.
  La notation n'est valide ici que si $d_{\min}$ désigne le gap de boules
  $g=d-r_A-r_B$, pas une autre notion de distance minimale. Sous les majorations
  utilisées, $(\kappa_q-2/s)g$ borne le **surplus de marge continu**
  $R_{\mathrm{dec},q}-M$, tandis que le rayon lui-même vérifie
  $R_{\mathrm{dec},q}\geq(\kappa_qs-1)M$. À $s=4$, $6{,}93$ ou $7{,}73$, la
  garantie uniforme $R_{\mathrm{dec},q}>M$ disparaît ; le citron réel ne
  « s'annule » pas en général.
- `core` n'est pas vide par construction pour tout `s<8`. Contre-fixture
  immédiate : deux boîtes singletons distinctes ont des rayons de boîte nuls et
  `core_ball(q,A,B).radius4 > 0` en q3/q4, quelle que soit la valeur de `s` qui
  a conduit à ce rectangle. La justification correcte est donc : `s=8` est le
  plus petit entier donnant **uniformément** la marge voulue aux trois lanes,
  non une nécessité rectangle par rectangle.
- Le tableau `alive_rectangles` ne décrit aucune révision cohérente : au pin
  commité il n'existe pas encore de garde sous 8 ; dans le candidat du worktree,
  l'absence d'opt-in lève `invalid_argument` au lieu de rendre zéro rectangle.

Le statut de preuve est lui aussi mélangé. Être mesuré à `s=8` rend un chiffre
admissible au domaine ; cela ne lui donne ni reçu ni dénominateur. En
particulier, le marginal `80,9--99,8 %` et les facteurs `18,69/15,98` restent
des diagnostics non reproductibles tant que probe, commande, stdout, seed,
dénominateur et hash ne sont pas versionnés. Enfin, le document commité en
`495b234f` se dit écrit au pin `f83fd184`, alors que ce pin acceptait encore
`s>=1` dans `run_pipeline` et les CLI. Il doit nommer le futur commit
fonctionnel, ou se déclarer relatif au worktree candidat.

#### La marge continue n'est pas une marge discrète stricte

Le commentaire candidat de `wavefront.hpp` va encore trop loin en affirmant
que le `CoreBall` calculé a uniformément un rayon strictement supérieur à
$M=\max(r_A,r_B)$ à `s=8`. Prendre les boîtes plates
`A=[0,1]x{0}x{0}` et `B=[5,6]x{0}x{0}`. Le prédicat est exactement sur sa
frontière : `D2=100=(8+2)^2 W2`. Pourtant, en q3 comme en q4, les arrondis
dirigés de `core_ball` donnent `radius4=2`, tandis que `4M=2`. Le rayon entier
sous-approché vaut donc $M$, pas strictement plus.

Cela ne crée aucune fausse mort et ne remet pas en cause le plancher produit.
La formulation reçue est plus étroite : 8 est le premier entier au-dessus du
seuil **continu** q4 ; le `CoreBall` discrétisé reste une sous-approximation
sûre, sans marge stricte uniforme revendiquée après arrondi. Toute propriété
qui aurait réellement besoin de `radius4>4M` exige un lemme et une porte
distincts.

#### La borne basse de `s` n'est pas un contrat de coût

Le cas accepté et testé `s=INT64_MAX` n'est pas seulement « susceptible »
d'être mauvais. Dans la grille u16, son membre droit dépasse le `D2` maximal
dès qu'au moins un facteur a `W2>0`; seules deux feuilles de diamètre nul sont
séparées. La WSPD émet donc exactement une paire par couple de positions, soit
$\binom{n_u}{2}$ rectangles. Accepter ce cas comme profil produit bénit un mode
quadratique déterministe.

Deux décisions seulement sont cohérentes : borner le profil opérationnel aux
valeurs qualifiées, actuellement 8/10, tout en gardant la primitive large pour
ses tests arithmétiques ; ou conserver tout i64 mais annoncer explicitement que
ces valeurs n'ont aucun claim de temps, mémoire ou sous-quadraticité. Dans les
deux cas, la borne $O(s^3n)$ de la WSPD fair-split classique ne doit pas être
attribuée à la variante Morton-radix actuelle : `MATHEMATIQUES.md` la déclare
correctement ouverte, tandis que l'en-tête de `wavefront.hpp`,
`cloud_index.hpp` et `ARCHITECTURE.md` l'affirment encore. Les vagues et le
vecteur terminal sont donc output-sensitive et peuvent eux-mêmes être
quadratiques.

Le contournement bas niveau n'est pas non plus réellement compilé test-only :
le booléen `alive_rectangles(...,allow_subprofile_separation)` existe dans tous
les builds. Seul le champ correspondant de `GenerateOptions` disparaît sans
`MHGP5_TESTING`. Soit compiler un tag/overload de diagnostic uniquement dans
les cibles de test, soit déclarer honnêtement qu'une primitive interne possède
un bypass explicite non exposé par `run_pipeline`; l'architecture ne peut pas
promettre les deux.

### V151--V153 — aucune répétition exacte, utiliser d'abord l'index existant

La colonne nommée `|B|` par V151 est `|A||B|` moyen. La reconstruction des
trois cohortes trouve toutes les clés exactes `(endpoint,opposite_node)`
uniques : `130969/130969`, `377199/377199` et `361120/361120`. Les 65--189
rectangles incidents ne sont donc pas 65--189 réemplois du même crédit.
L'amortissement exact à l'intérieur d'un rectangle vaut en moyenne seulement
`1,43`, `2,53`, `1,29` partenaires côté A et presque autant côté B.

Le catalogue directionnel n'est pas reçu : 32 cônes ont déjà une demi-largeur
idéale minimale de `20,36 deg`, les 480 Mo ne stockent aucune portée radiale et
le remplissage global reste naïvement quadratique. La P0 constructive emploie
la primitive existante `count_universal_witnesses(leaf(a),B)` pour calculer le
minorant certifié `H_A^cert`, puis `H_B^cert` seulement sur les colonnes encore
requises. Le test aux boîtes et aux coins est suffisant, pas exact relativement
aux seuls points réels des facteurs. La borne sûre reste
`max(core+h_a+h_b,H_A^cert+h_b,H_B^cert+h_a)` et l'intersection des bitsets est
émise en ordre croissant. Aucun cache avant la mesure `requests/unique`, des
visites, du mur et du HWM.

### V154 — le gain de placement n'est pas déjà pris

L'ordre relu dans `src/pipeline/generate.hpp` est, pour q3 comme q4 :

```text
corner_histograms
rect_cover_handles
rect_diametral_candidates éventuelle
porte histogramme par ancre
cover/scan par ancre
```

La réponse V154 conclut donc trop tôt. Préclassifier les survivants de la porte
histogramme **avant** handles et requête peut encore :

- éviter handles et requête si tout le rectangle est mort ;
- éviter cover et scans pour chaque ancre morte dans un rectangle partiellement
  vivant ;
- construire la requête dense paresseusement au premier survivant qui en a
  besoin.

Le grand-livre historique ne change pas : `anchors` reste la masse complète
`|A||B|`, partitionnée en morts par ligne, morts par seuil et survivants.
L'extension doit ajouter des compteurs causaux sans rebaptiser les mêmes morts.

Le cover actuel classe radialement dans 32 classes ; la route dense n'est pas
un tri radial exact. Les chiffres V151/V154 n'ont pas de probe, commande,
stdout et hash suivis. La proposition directionnelle reste donc une hypothèse,
pas une piste fermée.

L'attribution demande trois bras : ordre actuel ; seule porte historique
déplacée avant handles ; puis ce même ordre avec `H_A^cert/H_B^cert`. Le deuxième
bras retire le gain de simple placement du gain propre au nouveau crédit. Une
seconde partition ferme :

```text
hist_survivors = oneside_dead_A + oneside_dead_B_after_A + oneside_survivors
```

Elle ne change pas le sens historique de `anchors_killed_hist`. Comparer les
candidats bruts et, en q3, l'identité
`w3_killed_OFF = oneside_dead_ON + w3_killed_ON`.

### V155 — q4 est la priorité mesurée, sans loi asymptotique acquise

La décomposition des reçus existants confirme que q4 est le premier mur sur
`terrain` et `scanline`. La lecture appariée des graines 3, 4 et 5 à 32 000
points donne :

| cohorte | part q4 | pente sécante q4, 2k vers 32k | médiane |
|---|---:|---:|---:|
| `terrain` | 40,5–53,6 % | 1,746–1,977 | 1,842 |
| `scanline_single_pass` | 42,0–57,8 % | 1,503–1,808 | 1,589 |

Cela justifie de porter ensuite la cascade à q4. Cela ne justifie pas encore
« q4 est quadratique », « WSPD est sain », « census/fold sont linéaires » ou
« uniform n'a aucune pathologie » : ce sont des pentes sécantes locales sur
une campagne bornée. Les trois graines doivent être rapportées, pas seulement
la graine 3 la plus rouge.

La cascade `h_core`, `h_a`, `h_b` se transfère telle quelle **à la porte
d'ancre q4** : les identités A, B et hors `A union B`, les seuils propres à q4
et la sortie anticipée ne dépendent ni des seeds ni des complétions. Une ancre
prouvée morte autorise à sauter lentille, seeds, cœur de Jung, corde et
complétions. La sélection axiale v3/v4 est fermée et absente de la v5 ; ne pas
la réintroduire implicitement dans ce raccord.
En revanche, ne pas sommer ensuite ces crédits scalaires avec des témoins de
seed, corde ou complétion sans IDs ou partition de provenance : ces sources
peuvent se recouvrir.

Correction géométrique : l'ouverture directionnelle maximale depuis une
extrémité vaut environ `54,736 deg` en q4 contre `60 deg` en q3, soit une
ouverture complète `109,47 deg` contre `120 deg`. Le `125,26 deg` de V155 est
le mauvais supplément.

La porte par lignes existe déjà en q4 au pin source. Le simple raccord
`EndpointCredit` à W4 déplace des morts que W4 trouvait avant les seeds ; il ne
peut donc pas, seul, réduire le mur cœur/corde des ancres W4 survivantes. Le
replay local de `mhgp5_q4_stage_probe`, un fil, politique produit, `n=8000`,
ferme ses identités et mesure sur `terrain/scanline` `2258/2410 ms` de
cœur+corde contre `1057/939 ms` de complétions. Diagnostic seulement, pas reçu.

La suite constructive est un `WitnessTape` par ancre W4 survivante : l'union
triée et dédupliquée des IDs déjà certifiés par `h_coeur`, `h_a`, `h_b`, puis
par le scan W4 ; `H_A/H_B` pourront ensuite ajouter leurs **IDs**, jamais leurs
sommes scalaires. À huit IDs l'ancre meurt ; une survivante en porte donc au
plus sept. Le profil public refusant les positions dupliquées, le format juste
est un index de position unique de poids implicite 1, pas un couple
`(représentant,poids)`.

Ces IDs peuvent initialiser directement le cœur de Jung et la profondeur, en
excluant respectivement `{a,b,x}` et `{a,b,x,y}`, puis en sautant ces mêmes IDs
dans le scan. Ils ne peuvent pas préremplir aveuglément les quatre compteurs de
`ChordPieces` : leurs extrémités `muhat` élargissent la corde exacte hors du
disque de Jung. Chaque ID de la tape doit d'abord passer par le même
`ChordPieces::update` strict et ne créditer que les morceaux effectivement
certifiés. Les routes query, cover, batch et device transportent les IDs
canoniques, jamais des offsets ; tout mapping absent ou dupliqué désactive la
précharge en fail-open. Séparer les shadows `oneside_gate` et `w4_tape`, sinon
le placement et la propagation aval restent confondus.

### V156 — fermeture rectangle : ne pas réinventer le raffinement exhaustif

Le récit `99,766 %`, `93 195` ancres et `8,7e5` seeds mélange des bras ou des
dénominateurs. Au reçu seed 3, `terrain n=2000`, `93 195/C(2000,2)=4,662 %`
survivent, donc `95,338 %` sont retirés, et le compteur q3 vaut `420 699`
seeds. Le taux varie avec n et V156 ne joint ni probe, commande, stdout ni
hash. Les pentes `1,19--1,36` restent des exposants sécants de temps.

Le certificat demandé existe déjà en partie dans `postsep_refine` : compter le
cœur de petites cellules radix `A_i x B_j`, puis prendre leur minimum, donne un
`depth_lb` valide pour tout le rectangle. Il absorbe cœur, siblings de A/B et
témoins extérieurs sans addition. Sa version exhaustive est toutefois déjà
mesurée négative : elle retire beaucoup de masse q4 mais augmente le mur, dont
environ `+34 %` sur `scanline 100k`, L=3.

La suite recevable est donc strictement bornée et transactionnelle :

1. déplacer avant handles le plancher déjà calculé
   `core+min(h_a)+min(h_b)` et compter `rect_hist_all_dead` ;
2. en counter-only, mesurer combien de rollbacks actuels auraient tous leurs
   enfants morts ;
3. seulement si non-vide, essayer une partition proof-only de profondeur et de
   visites fixes. Pour chaque cellule, prendre
   `max(depth_lb_parent,fresh_cell)` ; tuer le parent seulement si toutes
   atteignent `h_q`, sinon rendre exactement le parent.

Les cellules n'ont pas besoin d'être séparées quand elles servent uniquement à
prouver la mort totale et ne sont jamais publiées. Si des survivantes sont
émises, séparation et ledger exact redeviennent obligatoires. Ne pas rebaptiser
ce minimum `h_coeur`, car les ensembles témoins peuvent varier par cellule. Le
certificateur budgeté conserve seulement, sous la borne WSPD encore à recevoir
et pour s fixé, un surcoût borné par état ; il ne prouve pas à lui seul une
complexité sous-quadratique.

## P1 — coutures d'implémentation encore ouvertes

- `src/core/parse.hpp` est encore non suivi alors que trois cibles l'incluent :
  un commit qui l'oublie casse immédiatement le clean build.
- Ce parseur strict ne couvre que `--s`; `n`, `smax`, `seed`, `threads` et les
  autres entiers utilisent encore `atoi/atoll`. La provenance doit dire
  « parsing exact de `--s` », pas « des entiers de CLI ».
- Les mutants `postsep-core-without-corners` et `postsep-refine-q2` sont
  désormais tués dans le helper test-only avant `run_pipeline`. Ils ne gardent
  plus les refus `kInvariantViolated` de `run.hpp` contre une régression. Ajouter
  une injection de ledger à `s=8` ou une porte dédiée à la validation produit.
- L'opt-in sous-profil de `GenerateOptions` n'est pas propagé aux lanes batch
  q3/q4 : le même objet test-only passe en intégré et lève en batch/device.
  Propager sous macro ou déclarer l'opt-in limité à la lane intégrée.
- `wspd_wavefront` retourne sur un index sans nœud avant de vérifier `p,q` ; sa
  promesse de rationnel strictement positif n'est donc pas fail-closed sur le
  singleton.
- Les campagnes `s=6` encore citées dans `ECHELLE.md` et `GPU.md` ne sont plus
  rejouables par une commande produit. Nommer leur cible `MHGP5_TESTING` exacte
  ou les requalifier comme mesures historiques hors profil.
- `EndpointCredit` n'est exercé directement par aucune fixture. Ajouter une
  activation, une frontière `h-1`, une exclusion de tout `A union B` et un
  mutant qui ignore le crédit.
- Ajouter `w3_sites_visited` et `w3_early_by_endpoint`; l'union intégrée ne crée
  pas de nouveaux morts finaux, elle avance seulement certains arrêts.
- Les nouveaux compteurs `hist_killed_rows`, `hist_killed_thresh` et
  `hist_survivors` sont alimentés sur la lane intégrée, mais restent à zéro sur
  les builders batch/device et ne sont pas comparés par leurs portes.
- q4, batch et device ne portent pas encore le même budget typé. Ne pas partager
  les comptes q3/q4 : les seuils, clés et ledgers restent propres à chaque lane.
- Pour le prochain reçu, mesurer séparément rectangles entièrement morts,
  handles/requêtes/nœuds/candidats évités, sites W3/W4 visités, seeds,
  tests cœur/corde, complétions et puissances évités, mur et HWM. Comparer
  sorties, fates et ordre canonique.

## Ordre de travail conseillé à Claude

1. Patch de vérité contractuelle et fixture
   `gabriel-point-set-counterexample-5-points-v1` ; cela sécurise la
   signification des mesures sans bloquer le sous-flot.
2. Fermer le raccord `s>=8` : marge uniforme correctement nommée, refus interne
   explicite, bypass réellement test-only, validation produit postsep restaurée
   et mutants rejoués dans un build non concurrent.
3. Helper commun de préclassification histogramme avant handles ; mesurer le
   plancher rectangle puis `rollback_children_all_dead` avant toute nouvelle
   preuve-partition.
4. q4 en premier : `oneside_gate`, puis `w4_tape`; q3 sert de garde de
   non-régression. Garder cœur/corde/complétions séparés après la porte d'ancre
   et tuer un mutant qui crédite les quatre morceaux sans `ChordPieces::update`.
5. Campagne appariée trois graines avec reçus bruts et hashes. Ne construire un
   cache direction–rayon qu'après avoir mesuré les clés uniques
   `(endpoint,opposite_node,lane)` et le HWM.

## Vérification indépendante

Lecture statique du pin `495b234f` :

```text
docs/PROFIL_SEPARATION.md ajouté                         OUI
validate_run_options refuse s<8                         NON (refuse s<1)
CLI CPU/CUDA refusent s<8                               NON (refusent s<1)
alive_rectangles possède une garde interne sous 8       NON
```

Le commit reçoit donc la décision de domaine et le retrait des mesures sous 8,
mais pas encore son application exécutable. Le candidat non commité n'est pas
substitué à ce constat.

Diagnostic seulement sur ce candidat, dans un build CPU propre hors arbre :

```text
configure Release sans CUDA/sanitizers                         PASS
6 cibles ciblées compilées et liées avec -Werror              6/6 PASS
CLI/API/WSPD large/fold/separation ciblés                     20/20 PASS
mhgp5_postsep_refine nominal                                  SANS VERDICT (interrompu)
```

Les portes CUDA ne sont ni générées ni exécutées. Les CTests CUDA candidats
lancent d'ailleurs le binaire sans `--gpu` : ils vérifieraient le parser du
pilote compilé avec nvcc, pas la branche device. Plusieurs suites lancées en
parallèle dans `build/v5` ne constituent pas un résultat de suite complète ;
leurs portes de temps sont contaminées par la concurrence.

Sur le pin source `351faccc` :

```text
cmake -S morsehgp3D_v5 -B build/v5 -DCMAKE_BUILD_TYPE=Release   PASS
cmake --build build/v5 --parallel                              PASS
ctest --test-dir build/v5 --output-on-failure -R '^mhgp5_(q3_lane_batched.*|q4_lane_batched.*|anchor.*|sector.*|cell_grid.*)$'
53/53 PASS, 167,49 s
```

Cette campagne reçoit les chemins actuels ciblés. Elle ne reçoit ni la
complétude Gamma, ni les mesures V151/V154, ni une loi d'échelle. Aucun résultat
GPU n'est revendiqué. GCP non utilisé.

Ce fichier est le seul verdict mutable. Toute modification fonctionnelle
postérieure à `351faccc` le rend périmé jusqu'à relecture.
