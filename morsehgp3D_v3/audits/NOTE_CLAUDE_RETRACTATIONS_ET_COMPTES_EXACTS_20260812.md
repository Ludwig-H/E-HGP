# Note de Claude — rétractations, rampe gelée et comptes exacts du graphe

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## 1. Faute de méthode reconnue : rampe sur binaire mouvant

L'audit a raison et la faute est entière. J'ai reconstruit
`mhgp3v_centre_cell` **pendant** la campagne d'échelle. Le cas `25 000` s'est
terminé sur un inode supprimé et le cas `50 000` a démarré sur un ELF différent
de celui annoncé dans l'en-tête du reçu. Le fichier
`receipts/centre_cell_scale_20260812/scale_counters_raw.txt` est donc
**irrecevable**. Le contre-audit confirme ce verdict, mais pas sa suppression :
un artefact réfuté doit être renommé `invalid_mixed` et conservé avec son
diagnostic, car il portait les seules sorties brutes 12 500/25 000. Le commit
`64cf6fe` ne conserve que l'objet historique de 34 lignes arrêté après le
12 500; la sortie 25 000 observée ensuite n'est plus reconstructible depuis
Git. Les comptes recopiés dans l'audit restent des observations, pas un
substitut au transcript détruit.

La campagne est relancée sur un binaire **gelé hors de l'arbre de build**, avec
l'empreinte de l'ELF revalidée **avant et après chaque cas** et un champ
`identique=oui|NON` par cas. Le nouveau reçu est
`scale_counters_frozen.txt`, contrat `CentreCellScaleReceipt-v2`.

Au pin de cette note, ce fichier ne contient encore que douze lignes : en-tête,
commande 12 500 et `elf_before`. Il est **ouvert**, sans stdout, code, hash aval
ni footer, et a été commité avant la fin. Le driver temporaire n'exerce que
`terrain`, `uniform` et `scanline_single_pass`; il omet notamment
`eight_clusters` et n'est donc pas une rampe contractuelle. Il utilise `>>`,
n'a ni mode shell fail-closed, ni trap `ABORTED`, ni timeout, ni arrêt sur
code/hash, et n'est pas archivé. Le manifeste doit ajouter SHA CMake,
commande/type/flags de build, SHA du driver, état/diff worktree, session, ainsi
que `probe_factor`, `probe_top_cap` et `batch_records`. Cette exécution est en
outre concurrente au CTest sur un hôte deux-cœurs : ses compteurs peuvent être
diagnostiques, jamais son temps.

## 2. Rétractation : la déduplication par lot n'est pas prouvée locale

Mon commit `64cf6fe` conclut que le RLE `SupportKey` n'a pas besoin d'être
global. **Je retire ce titre.** Il s'appuyait sur un seul point de laboratoire,
`terrain, n=400`, sans transcript archivé et sur un ELF depuis remplacé.

L'audit exhibe deux contre-mesures qui suffisent à interdire l'extrapolation :
sur `uniform, n=5`, le gain local à profondeur quatre vaut `1,007`; sur
`uniform, n=25`, le gain global vaut `3,960` mais les gains locaux tombent à
`2,276/1,575/1,130` aux profondeurs `1/2/3`. Il donne aussi la raison
géométrique : le plan bissecteur d'une paire diamétrale traverse un nombre
quadratique de cubes par niveau dyadique, et rien ne le retire des listes quand
`n<smax-1`.

L'énoncé défendable est donc seulement : **pour une antichaîne fixée de
sous-arbres, la déduplication locale reste exacte** — le lot qui contient la
feuille propriétaire conserve le support pertinent, les autres constatent zéro
propriétaire — mais elle ne possède **aucune borne de parcimonie**, et son gain
dépend de la famille et de la taille.

Je retire également la borne « facteur 42 ». Le tableau contient
`263 825` clés distinctes non dégénérées, donc une géométrie par `SupportKey`
ramènerait `2 220 024` occurrences à `263 826..268 632` solves : un facteur
`8,26..8,41`.

## 3. Observation différée sans attribution causale du coût

Le lift différé est implémenté et **jugé** : il passe l'accord d'identités sur
`uniform`, `terrain` et la grille gravée, y compris avec un lot minuscule de
`1 024` enregistrements qui force de nombreuses vidanges.

Sur `terrain, n=1 500`, il divise les lifts par `5,345` — `7 820 379` vers
`1 462 984` — et rend exactement les mêmes `98 752` supports. Il coûte pourtant
`13 %` de temps `user` de plus : `4,345 s` contre `3,832 s`.

Ces nombres n'ont ni transcript autonome, ni commande et pins complets; ils
restent historiques. Même reçus, ils démontreraient seulement que **cette
implémentation différée réduit les lifts sans réduire le temps utilisateur sur
ce cas**. Elle change simultanément tri, copies de contextes/CSR, allocations,
ordre positivité--owner et calendrier du census : on ne peut en déduire ni que
le lift n'est pas dominant, ni qu'il vaut « un dixième » du parcours. Une
attribution causale exige une ablation à flot identique ou des compteurs
matériels autour de la primitive. La variante reste donc `--deferred-lift`,
désactivée par défaut, et son intérêt device demeure à mesurer.

## 4. Comptes exacts du graphe de bissecteurs

Le critère de split employait le potentiel d'intervalles, majorant grossier d'un
surgraphe. C'est lui qui rendait la pente des cellules rouge : près de la racine
il explose et force à découper là où le vrai graphe est creux.

Dans la branche de sonde opt-in, les compteurs du graphe sont exacts et
emploient chacun sa coupe de lane : en général `E_2` sur `D_(smax-2)`, `T_3`
sur `D_(smax-3)`, `T_4` et `Q_4` sur `D_(smax-4)`; les noms
`D_9/D_8/D_7` ne valent qu'à `smax=11`. Les bitsets sont orientés par position
dans `mine` triée, pas par `PointId`; cet ordre total suffit pour que
`popcount(N+(i) inter N+(j))` compte chaque triangle une fois et
`popcount(N+(i) inter N+(j) inter N+(k))` chaque `K4` une fois. Le plafond de
sonde `top<=96` donne seulement une borne diagnostique finie. Avec
`W=ceil(top/64)`, le compte dense coûte notamment
`Theta(E_3 W+T_4 W+E_2+T_3+Q_4)`; l'admission
`E_2+3T_3+6Q_4` conserve des poids de coût heuristiques et ne borne ni temps,
ni contextes, ni octets, census, owner, groupement ou tri.

La borne d'incidence de l'audit, `4 Q_4 <= (m_4-3) T_4`, n'est pas encore
implémentée exactement comme contrôle : le code calcule
`floor((m_4-3)T_4/4)` puis tolère `Q_4=borne+1`. Le test correct est directement
`4Q_4>(m_4-3)T_4`, en largeur vérifiée et sans exclure `T_4=0`. Le code trois
emploie aussi le libellé inadapté « lemme de profondeur » pour une éventuelle
violation d'incidence. L'affirmation « jamais violée » n'a pas de transcript et
est retirée jusqu'à une porte non vide avec mutant d'une unité.

## 5. Ce que le sondage change, et ce qu'il ne change pas

Le sondage du vrai graphe est appliqué dans une bande d'indécision et sous un
plafond de largeur. Sur `terrain, n=1 500` :

| `probe_factor` | cellules | lifts | supports |
| ---: | ---: | ---: | ---: |
| `1` (désactivé) | `208 705` | `7 820 379` | `98 752` |
| `8` | `70 121` | `12 005 776` | `98 752` |
| `16` | `47 161` | `16 107 213` | `98 752` |

Ces trois lignes ne possèdent pas de transcript autonome; elles constituent une
observation historique. Elles suggèrent ici un échange cellules--lifts, pas un
réglage optimal général. Avec `probe_factor=1`, la sonde est algébriquement
désactivée : `!terminal` implique `work>work_cap`, incompatible avec
`work<=work_cap*1`. Un A/B device devra la recalibrer, et son reçu devra imprimer
le facteur, le plafond 96, `probe_tests>0`, les octets et les identités.

## 6. Portes

Sur le couple source
`dbaa2e0128c5be30e2f7c75784e38758a45c7bb938fba5d8ab4a87c71d5ad764`,
CMake `0f64c1c60afbf4af51339807b758e49ec0312d4be69f7dcda8303d251616c865`
et ELF Release
`423797e9964538f42701660d8baaf492b302f801a4aeb4b0df1b183986a5a037`,
les vingt-huit portes `mhgp3v_centre_cell_*` passent en `202,12 s`. La sortie
CTest a le SHA-256
`ac8063615912a8272c1e781f3b1baf8381ecc056180abbb2cd9c266d7861cd58`
et `LastTest.log`
`ac5774d57f40e1e785f62baf666f477a34388b0ac1723f1cb35c1c8c6e61e750`.
Le temps est contaminé par la rampe concurrente et ne qualifie aucun débit.
Cette gate reçoit les accords/fixtures enregistrés, dont quatre variantes
différées et `strata-stop`; elle ne reçoit ni mutant différé sémantique,
`owner_multiple` fail-closed, HWM d'octets, ni la modification source
postérieure au pin. Surtout, les quinze sorties normales du log impriment toutes
`probe_tests=0` : les vingt-huit verts ne reçoivent pas le nouveau compte
`E2/T3/T4/Q4`. Il faut une commande `--probe-factor>1`, des planchers
`probe_tests/probe_accepted`, une fixture K4 et un mutant d'incidence.

GCP non utilisé.
