# Registre des pistes archivées — MorseHGP3D v3

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=registre_documentaire`,
`public_status=not_claimed`.

## Ce que ce dossier est

Vingt-neuf textes déplacés depuis `audits/` le 15 août 2026. Chacun a été retiré
du dossier vivant parce qu'il n'est plus une autorité **et** qu'il n'est plus
cité par le logiciel, par un reçu, par [`../../PROPOSITION.md`](../../PROPOSITION.md)
ni par [`../AUDIT_ETAT_COURANT.md`](../AUDIT_ETAT_COURANT.md).

C'est la règle 5 de l'index — « mettre à jour la proposition consolidée, puis
supprimer la note absorbée » — appliquée pour de bon. L'index affirmait depuis
plusieurs sessions que ce ménage avait eu lieu ; sur les quatre-vingt-neuf
fichiers du dossier, vingt-neuf n'étaient déclarés nulle part.

**Rien n'est supprimé et rien n'est réécrit.** L'historique de falsification est
la valeur de ce chantier, pas son encombrement. Le tableau dit en une ligne ce
que chaque texte proposait, ce qui l'a tué, et ce qui en survit ; le fichier
complet reste à côté pour qui veut la preuve.

**Un fichier archivé n'est jamais une autorité.** Ne prenez ni son titre, ni son
ancien statut, ni ses chiffres pour une réception actuelle. Le seul verdict
mutable est [`../AUDIT_ETAT_COURANT.md`](../AUDIT_ETAT_COURANT.md).

## Vocabulaire des verdicts

| verdict | sens |
| --- | --- |
| `réfuté` | une contre-fixture ou une mesure a tué la proposition |
| `absorbé` | le contenu est passé dans la proposition consolidée ou dans l'autorité |
| `supersédé` | une révision ultérieure du même sujet le remplace |
| `dialogue clos` | question posée, réponse rendue et intégrée |
| `mesure datée` | chiffres d'un binaire non épinglé ; le fait demeure, la mesure ne qualifie rien |

---

## 1. Front de Jung, localité et chambres

| pièce | idée | pourquoi elle n'a pas tenu | ce qui survit |
| --- | --- | --- | --- |
| `AUDIT_VERROU_MATHEMATIQUE_FRONT_JUNG_H0_GPU_20260812.md` | *absorbé* — remplacer la source par cellules de centres par un front de Jung coalescé, une enveloppe top-9 du plan médiateur et un owner génératif exact-une-fois | Ses théorèmes sont devenus l'ordonnance suivie ensuite, mais l'audit réfute lui-même le producteur qui les portait : le dual-tree d'ancres passe de `4,85` à `23,84` millions de visites q3 entre `n=500` et `n=1000`, pentes `2,30` et `2,33` contre une porte à `1,35` | Théorème 1 `R_q(K) < Lambda_D(K,P_q)` ; front coalescé à `141,18 n` au lieu de `302,87 n` ; la fixture u16 qui tue le mutant « les deux faces doivent être positives » |
| `NOTE_CLAUDE_REPRISE_LOCALITE_CHAMBRE_FRONT_JUNG_20260812.md` | *réfuté* — produire le front par une banque directionnelle de chambres Yao-48, coupure radiale au dixième plus proche point de la chambre | Le calcul angulaire tue sa propre banque : la chambre Yao-48 a un diamètre de `54,74` degrés quand un témoin de Jung en exige `< 35,26` en q3 et `< 31,13` en q4. La condition q4 délimite de plus un **anneau** en `D_i/D_j`, pas un préfixe radial | Lemme 1 : dans une même chambre, `3 D_i^2 < D_j^2` place `b_i` strictement dans la boule diamétrale de `(a,b_j)` — exact en `i64` ; le lemme de boule témoin commune et son seuil de séparation |
| `AUDIT_PROBE_LOCALITE_8C00AB0_20260812.md` | *réfuté* — `certified_locality_probe` fournit une génération locale exacte des trois arités et une fermeture par cône remplaçant le scan global | Faux vert de saturation : à `n=70` le probe rend le code zéro avec `q2/q3/q4 = 681/795/174` là où le juge exhaustif donne `681/884/202`. La fermeture par cône coûte `4,649 s` contre `1,386 s` pour le scan qu'elle remplace | Contre-fixture `A=(0,1,0), B=(2,1,0), C=(1,2,0)` séparant extra-shell et support minimal non unique ; le défaut de parseur `from_chars` |
| `AUDIT_CONTRE_AUDIT_SPINDLE_CONE_WORKTREE_20260813.md` | *réfuté* — un cône cible ouvert par endpoint, alimenté par une banque k-NN, ferme les paires lane par lane avec un rejet `NONE` certifié | La rampe mono-ELF (`n=500` à `4000`) ne ferme sur aucune série deux pentes `log2` successives `<= 1,35` ; à `n=2000` la banque 96 dépense déjà `39,2` M de tests témoin-nœud et `84,0` M de tests de coins | **Le noyau ponctuel u16** : `C_3 = {H>0, 4H^2 > E_2 X_2}`, `C_4 = {H>0, 3H^2 > E_2 X_2}` par l'identité de Lagrange, et la porte `ALL` par les huit coins — repris tel quel par le contrat de source aiguë |
| `NOTE_CLAUDE_REPARATIONS_P0_CONE_20260813.md` | *absorbé* — réparations d'exactitude du probe de cône (domaine `smax`, juge isolé, trois lanes jugées séparément) | Les réparations sont gravées en portes (`39` CTests `mhgp3v_cone_*` contre `30`), le NO-GO est accepté sans réserve, et la route bascule ailleurs | L'isolement du juge : `spindle_cone_oracle.cpp` n'inclut ni le sujet ni `mhgp/mhgp.hpp`, et réécrit son arithmétique 128 bits sur deux limbes |

## 2. Cellules de centres

| pièce | idée | pourquoi elle n'a pas tenu | ce qui survit |
| --- | --- | --- | --- |
| `NOTE_SOLUTION_SOURCE_CELLULES_CENTRES_20260812.md` | *supersédé* — énumérer la Source S par listes imbriquées de cellules de centres, avec census global exact `I_B`/`U_B` | La note ne spécifie qu'un snapshot historique et déclare elle-même qu'aucun de ses résultats ne se transfère au source live postérieur | **Le lemme profondeur–cellule** (`beta <= R_p(C)`, `I_B` union `U_B` inclus dans `A_p(C)`) et sa preuve dichotomique par invariant de pool |
| `AUDIT_JUGE_CELLULES_INDEPENDANT_90C06B0_20260812.md` | *réfuté* — un juge rationnel indépendant reçoit la source par cellules comme oracle borné sensible aux mutations | **Porte vacueuse** : la porte lance le driver sans `--judge`, le sujet refuse en code 2, et `WILL_FAIL TRUE` transforme ce refus en vert. En soumettant le flux muté directement, le juge rend code 1 avec six vérités manquantes | La vérification mathématique du juge (Gram, positivité barycentrique stricte = centre dans `relint conv(U)`) et la contre-fixture `k=1` |
| `AUDIT_CONTRE_AUDIT_407D4D1_SENTINELLE_HORS_SUPPORT_20260812.md` | *réfuté* — census terminal par sentinelle minimale top-`(12-q)` hors support, parallélisé par sous-arbres disjoints | Le parallélisme ne réduit ni le nombre de cellules ni les `839 582 666` occurrences à `n=50 000`, **et il casse la télémétrie** : `--threads=2` publie `7 012` occurrences contre `22 543` lifts, avec code de retour zéro | Le théorème de la sentinelle top-`(12-q)` hors support et sa minimalité (une sentinelle de taille `t-1` ne sépare pas trois cas) |
| `NOTE_CLAUDE_ETAT_CELLULES_CENTRES_20260812.md` | *mesure datée* — la subdivision de l'espace des centres remplace le parcours d'arrangement comme source générale | Tables issues d'une machine partagée à deux cœurs, sans épingler ensemble binaire, commande, graine et transcript | Le constat que le filtre droite–cellule est **exact et fail-open** mais annule son gain sur CPU |
| `NOTE_CLAUDE_PENTES_UNIFORM_VERTES_20260812.md` | *mesure datée* — la pente rouge des cellules est une propriété du générateur `terrain`, pas de l'ordonnance | Pentes vertes issues d'un binaire local non gelé, sans transcript ; les `wall_s` de `871`, `1 851` et `3 223 s` relevés sous charge concurrente ne bornent ni un successeur ni une latence | L'explication géométrique de la superlinéarité de `terrain` : `coord = sqrt(25 n)` avec amplitude verticale `coord/8` fait croître la boîte en `n^1,5` |

## 3. Source par ancre et lentille aiguë

| pièce | idée | pourquoi elle n'a pas tenu | ce qui survit |
| --- | --- | --- | --- |
| `AUDIT_CONTRE_AUDIT_PRODUCTEUR_ANCRE_LENTILLE_AIGUE_20260812.md` | *réfuté* — une coupure de lentille aiguë et un classifieur ternaire exact ferment collectivement des chambres de paires avant tout `PairId` | La lentille ne ferme pas la famille adversariale : une construction u16 explicite donne un carrier aigu à **toutes** les paires, et le diagnostic sur `eight_clusters,n=50000` en trouve un sur `300/300` paires échantillonnées | **Le théorème de face adjacente aiguë** — tout q4 positif a au moins une face `abx` ou `aby` strictement aiguë — encore porté par la lane q4, et l'identité entière `4 Q_ab(x) = ||2x-a-b||^2 - D^2` |
| `REPONSE_CLAUDE_CONTRE_AUDIT_LENTILLE_AIGUE_20260812.md` | *supersédé* — réponse actant les réfutations et paramétrant les seuils par `smax` | Le fichier porte lui-même un bandeau de statut historique ; ses verdicts « domaine fermé » et « high-water fait » sont rectifiés par le contre-audit, qui relève que le différentiel s'arrête à `smax=30` quand la CLI monte à `34` | `lane_death_threshold(smax,q)` et `envelope_depth(smax) = smax-2`, qui redonnent `10/9/8` et `9` à `smax=11`, et le mutant `smax-fixed-thresholds` |
| `AUDIT_JUNG_ANCHOR_389A742.md` | *réfuté* — un couple de carriers chacun dans la lentille suffit à faire de `pq` une ancre diamétrale | Fixture u16 à cinq points : quatre distances à `97 <= D^2 = 100` mais `||x-y||^2 = 144 > 100`. Le centre sort de l'ellipse, un témoin est classé « extérieur constant » alors que sa marge vaut `10/3`, et le rang tombe à `4` au lieu de `5` | La fixture à cinq points, permanente, et la correction minimale `||x-y||^2 <= D^2` — **posée avant le produit dans la lane q4 courante** |

## 4. Ledger des causes et owner

| pièce | idée | pourquoi elle n'a pas tenu | ce qui survit |
| --- | --- | --- | --- |
| `AUDIT_LEDGER_CAUSES_LIFTS_238CF12_20260812.md` | *absorbé* — fermer la partition des lifts par arité avant toute conclusion causale ; `SupportKey` avant lift | Sa prédiction chiffrée a été vérifiée puis intégrée : les `130 033` occurrences `pending` non attribuées (`18 048` q2, `97 825` q3, `14 160` q4) sont récupérées et l'identité ferme à écart nul | L'identité `lifts_q = dégénérés + owner + positivité + acceptés + rang` et la fixture `K_24` |
| `NOTE_CLAUDE_LEDGER_CAUSES_LIFTS_20260812.md` | *supersédé* — premier ledger : la cause dominante est le rejet owner tardif, multiplicités `42/55/510` | La table ne ferme pas : la colonne rang ne compte que le rejet final et laisse `130 033` occurrences sans attribution ; les quotients divisent des occurrences de trois populations par les seules acceptations | Les taux de rejet owner — `96,1 %` en q2, `91,7 %` en q3, `92,1 %` en q4 — arithmétiquement justes, qui motivent le groupement avant lift |
| `NOTE_CLAUDE_MULTIPLICITE_SUPPORTKEY_20260812.md` | *mesure datée* — histogramme de multiplicité par `SupportKey` sous `--multiplicity` | Exécution `terrain,n=400` non épinglée, sans graine ni ELF ; l'histogramme est refusé parce que ses trois issues sont un **stade maximal**, pas des propriétés orthogonales | La fermeture à écart nul par arité, confirmant la prédiction du contre-audit au chiffre près |

## 5. Ordre k, Gabriel et route sparse

| pièce | idée | pourquoi elle n'a pas tenu | ce qui survit |
| --- | --- | --- | --- |
| `AUDIT_REPARATION_K_GABRIEL_K_MST_20260812.md` | *absorbé* — remplacer le K-graphe de Gabriel brut par un graphe complété d'étoiles silencieuses `G_k^+` | La fixture `E5` tue définitivement le graphe brut : deux non-Gabriel rattachent la facette `AC` sans nouveau `PointId`, et le graphe brut garde deux composantes jusqu'à `24`. La réparation est passée dans `docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md` | Le lemme de l'étoile silencieuse : au plus `|I(Q)| <= k-1` attaches au lieu d'une clique complète |
| `AUDIT_REPONSES_ROUTE_SPARSE_GABRIEL_20260812.md` | *dialogue clos* — six questions d'architecture sur la route sparse « directes + gateways » | Chaque point est fermé avec sa cause ; notamment le pivot choisi dans l'union des supports est **faux sur un carré cosphérique** — supprimer un sommet d'une diagonale laisse l'autre diagonale entière | La clé de niveau canonique `beta = N/(4D)` et ses bornes u16 avant réduction, arité par arité |
| `QUESTIONS_CLAUDE_ROUTE_SPARSE_20260812.md` | *dialogue clos* — les activations locales certifiées sont-elles déjà la source directe complète ? | Les deux prémisses de tête sont invalidées : les `68,07` enregistrements par point mesurés sur `terrain` sont des supports **proposés**, pas des cofaces directes reçues, donc `68 x 4` n'est qu'un dimensionnement | L'identité `Q = U union I` avec `k = p+q-1`, et les mesures d'arité par point à `n=4000` |
| `QUESTIONS_CLAUDE_GEOMETRIE_3D_20260813.md` | *dialogue clos* — sept questions sur ce que la dimension trois et la grille u16 offrent encore (borne de degré q2 par chambre, cutoff tabulé, filtre flottant certifié) | Les sept reçoivent leur réponse : **`Q1` est non** — une fixture u16 place `13` partenaires q2 d'un même point dans une seule chambre canonique, sans plateau ni cosphère à cinq sites | La contre-fixture des treize partenaires, qui **tue le cap 12** sans recourir à un plateau ; l'autorisation explicite du filtre flottant certifié |

## 6. Réemploi Yao48, SupportKey et architecture GPU

| pièce | idée | pourquoi elle n'a pas tenu | ce qui survit |
| --- | --- | --- | --- |
| `AUDIT_REEMPLOI_YAO48_P1A_LIGNE_ENREGISTREE_20260811.md` | *réfuté* — réemployer la frontière Yao48/LBVH et le prune center-cover `P1a` de la ligne enregistrée comme briques et autorités de la v3 | **Aucun prune ne survit au portage** : une fixture u16 montre que le filtre radial `dist2 >= 3D` est faux comme preuve de dix intérieurs stricts — à `D=25` un témoin est exactement sur la coquille, et il ne reste que neuf stricts | Les deux fixtures q2 gravées, dont les dix contacts à distance cinq de `(10,10,10)` donnant profondeur stricte nulle |
| `AUDIT_DEBLOCAGE_GPU_SUPPORTKEY_TOP12_20260812.md` | *absorbé* — dédupliquer les `SupportKey` avant toute géométrie, rejouer l'owner par point-location | Le diagnostic qui le motive est mesuré et est devenu la route : `21 395 212` supports pour `839 582 666` géométries à `n=50 000`, soit `39,24` géométries par support et `81,6 %` de rejets owner ; `127,7` et `93,4 %` sur `terrain` | Le théorème du minimum auto-centré, qui fonde le « q3 par droite » du producteur courant |
| `NOTE_CERTIFICAT_HELLY_DISQUE_JUNG_20260811.md` | *supersédé* — certifier par Helly qu'une couverture du disque de Jung se réduit à au plus trois `PointId` | Le certificat reste **ponctuel** : aucune borne uniforme sur un produit d'AABB, et son test exact déborde l'arithmétique visée — `F_k` autour de `180` bits sous u16, donc hors `i128` | La marge exacte et le demi-plan mauvais ; le sous-certificat de taille au plus trois par Helly |
| `NOTE_COEUR_UNIVERSEL_JUNG_ANCRES_Q3_Q4_20260811.md` | *absorbé* — prédicats entiers sans division certifiant un cœur universel sur une arête maximale | La note déclare elle-même n'être qu'une reformulation de filtres antérieurs et **ne borne ni le nombre d'ancres survivantes ni le coût** ; le pire cas reste quadratique en paires | Les deux prédicats entiers et leurs relaxations par boule de milieu `3||U||^2 < D^2` et `15||U||^2 <= 4D^2` — **exactement les lanes q3/q4 du pipeline courant** |
| `NOTE_CLAUDE_MESURE_PORTE_REGULIERE_20260812.md` | *mesure datée* — la porte régulière exige un support minimal unique et essentiel sans label extérieur | Les fractions de records portant une extra-shell — `4,17 %` sur `terrain`, `11,48 %` sur `scanline_single_pass` et `11,39 %` sur `scanline_overlap_multiecho` — **falsifient** l'hypothèse « aucune extra-shell » | Les trois prédicats d'extra-shell exacts et entiers, un par arité, dont le déterminant in-sphere nul en q4 |
| `NOTE_CLAUDE_GATE_TROIS_VOIES_20260813.md` | *réfuté* — une gate `counter-only` met en concurrence trois certificats de fermeture et conclut que la dominance 432 est porteuse | Le tableau est refusé **comme gate** : il juxtapose `n=12 500`, `150` et `600` avec des ELF, cutoffs et univers différents, sans union commune, sans ledger d'identités et sans pente ; le claim des cœurs vides est faux tel qu'écrit | Le défaut de décomposition trouvé par un **invariant** et non par un mutant : la récursion `A x A` doit se traiter par `(A_g,A_g)`, `(A_d,A_d)`, `(A_g,A_d)`, avec la porte `paires_couvertes == C(n,2)` |

## 7. Fenêtre, front inverse et dialogue clos

| pièce | idée | pourquoi elle n'a pas tenu | ce qui survit |
| --- | --- | --- | --- |
| `AUDIT_REPONSES_SOURCE_FRONT_INVERSE_20260812.md` | *réfuté* — construire la Source S comme un front inverse : catalogue identifié aux cofaces, parcours du graphe des seules sorties auto-centrées | Le graphe **n'est pas connexe** : sur une fixture u16 à six points, les deux seuls supports q4 de niveau zéro ne partagent qu'une arête et jamais une facette, sans plateau ni coplanarité — aucune transition proposée ne les relie | Les quatre contre-fixtures gravées, dont `source_support_rang_ferme_12` et `plateau_carre_multifusion` |
| `AUDIT_SUCCESSEUR_WINDOW_SOURCE_FFE5B69_20260813.md` | *mesure datée* — une fenêtre top-`M` de supports par ancre dont les identités seraient certifiées dans les deux sens | Les chiffres reçus sont ceux d'un pin daté ; la réception limite les zéros aux `SupportKey` et à deux cardinalités, et `1 277` supports ne sont jamais proposés sur `eight_clusters` | **La fixture q3 permanente** où le premier omis vérifie `delta_out^2 = 100 = 4R^2` — le cas d'égalité qui impose l'inégalité stricte de la fenêtre certifiée |
| `QUESTIONS_CLAUDE_CELLULES_CENTRES_20260812.md` | *dialogue clos* — deux lemmes et trois questions avant la réécriture du prototype de cellules | Réponses rendues et reprises par la note de solution : `L1` reçu sous l'invariant de pool avec la porte `p'+q > smax`, `L2` seulement comme identité sémantique post-census | La preuve par contraposée de `L1` : les `t_q = smax-q+1` témoins de plus petit `u_C` sont strictement intérieurs si `beta > R_q(C)` |

---

## Règle de réouverture

Une piste archivée ne revient dans la voie active qu'avec **un nouveau théorème
de complétude**, **une fixture qui falsifie le motif d'abandon** sans casser les
contre-exemples existants, une architecture sans structure globale interdite, et
**une porte de coût distincte**. Un benchmark moyen, un accord moyen ou un bon
rappel empirique ne suffisent jamais — c'est la règle de la racine
([`docs/archive/abandoned/README.md`](../../../docs/archive/abandoned/README.md)),
et elle s'applique ici sans changement.

Deux motifs d'échec reviennent assez souvent pour mériter d'être nommés :

1. **La porte vacueuse.** Une porte qui passe au vert sans rien prouver — refus
   du sujet converti en succès par `WILL_FAIL`, regex qui ignore le code de
   retour, quantificateur `{n}` que `cmsys::RegularExpression` lit
   littéralement. Toute nouvelle porte doit exhiber son plancher de couverture
   et son mutant tué.
2. **Le certificat qui coûte plus qu'il ne rapporte.** Évalué à chaque nœud
   visité, il ne peut pas économiser plus de visites qu'il n'en coûte. C'est le
   sort commun de `SOC64`, `BlockJungDual`, `HCBlockDepth` et de trois
   certificats de paire successifs. Un gain doit être mesuré **apparié**, contre
   une exécution avec le certificat désarmé.
