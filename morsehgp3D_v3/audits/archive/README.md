# Registre des pistes archivées — MorseHGP3D v3

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=registre_documentaire`,
`public_status=not_claimed`.

## Ce que ce dossier est

Trente et un textes déplacés depuis `audits/` le 15 août 2026. Chacun a été
retiré du dossier vivant parce qu'il n'est plus une autorité **et** qu'il n'est
plus cité par le logiciel, par un reçu, par `../../PROPOSITION.md` ni par
`../AUDIT_ETAT_COURANT.md`. C'est la règle 5 de l'index des audits — « mettre à
jour la proposition consolidée, puis supprimer la note absorbée » — appliquée
pour de bon : l'index l'affirmait depuis plusieurs sessions sans que le
déplacement ait eu lieu.

**Rien n'est supprimé et rien n'est réécrit.** L'historique de falsification est
la valeur de ce chantier, pas son encombrement. Le tableau ci-dessous dit en une
ligne ce que chaque texte proposait, ce qui l'a tué, et ce qui en survit ; le
fichier complet reste à côté pour qui veut la preuve.

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
| `AUDIT_VERROU_MATHEMATIQUE_FRONT_JUNG_H0_GPU_20260812.md` | *absorbé* — remplacer la source par cellules de centres par un front de Jung coalescé, une enveloppe top-9 du plan médiateur et un owner génératif exact-une-fois | Ses théorèmes sont devenus l'ordonnance suivie ensuite, mais l'audit réfute lui-même le producteur qui les portait : le dual-tree d'ancres passe de `4,85` à `23,84` millions de visites q3 | Théorème 1 `R_q(K) < Lambda_D(K,P_q)` ; front coalescé à `141,18 n` au lieu de `302,87 n` ; constantes de spindle |
| `NOTE_CLAUDE_REPRISE_LOCALITE_CHAMBRE_FRONT_JUNG_20260812.md` | *réfuté* — produire le front par une banque directionnelle de chambres Yao-48, coupure radiale au dixième plus proche point de la chambre | Le calcul angulaire tue sa propre banque : la chambre Yao-48 a un diamètre de `54,74` degrés quand un témoin de Jung en exige `< 35,26` en q3 et `< 31,13` en q4. La condition q4 délimite de plus un **anneau** en `D_i/D_j`, pas un préfixe radial | Lemme 1 : dans une même chambre, `3 D_i^2 < D_j^2` place `b_i` strictement dans la boule diamétrale de `(a,b_j)` — exact, et sa coupure radiale q2 |
| `AUDIT_PROBE_LOCALITE_8C00AB0_20260812.md` | *réfuté* — `certified_locality_probe` fournit une génération locale exacte des trois arités et une fermeture par cône | Faux vert de saturation : à `n=70` le probe rend `q2/q3/q4 = 681/795/174` là où le juge exhaustif donne `681/884/202` | Contre-fixture `A=(0,1,0), B=(2,1,0), C=(1,2,0)` séparant extra-shell et support minimal non unique |
| `AUDIT_CONTRE_AUDIT_SPINDLE_CONE_WORKTREE_20260813.md` | *réfuté* — un cône cible ouvert par endpoint, alimenté par une banque k-NN, ferme les paires lane par lane | La rampe mono-ELF (`n=500` à `4000`) ne ferme sur aucune série deux pentes `log2` successives `<= 1,35` ; à `n=2000` la banque 96 dépense déjà `39,2` M de tests témoin-nœud et `84,0` M de tests de coins | Le noyau ponctuel u16 : `C_3 = {H>0, 4H^2 > E_2 X_2}`, `C_4 = {H>0, 3H^2 > E_2 X_2}` par l'identité de Lagrange, et la porte `ALL` par les huit coins |
| `NOTE_CLAUDE_REPARATIONS_P0_CONE_20260813.md` | *absorbé* — réparations d'exactitude du probe de cône (domaine `smax`, juge isolé, trois lanes jugées séparément) | Les réparations sont gravées en portes (`39` CTests `mhgp3v_cone_*` contre `30`), le NO-GO est accepté, et la route bascule ailleurs | L'isolement du juge : `spindle_cone_oracle.cpp` n'inclut ni le sujet ni `mhgp/mhgp.hpp`, et réécrit son arithmétique 128 bits |

## 2. Cellules de centres

| pièce | idée | pourquoi elle n'a pas tenu | ce qui survit |
| --- | --- | --- | --- |
| `NOTE_SOLUTION_SOURCE_CELLULES_CENTRES_20260812.md` | *supersédé* — énumérer la Source S par listes imbriquées de cellules de centres, avec census global exact | La note ne spécifie qu'un snapshot historique et déclare elle-même qu'aucun de ses résultats ne se transfère au source live postérieur | **Le lemme profondeur–cellule** (`beta <= R_p(C)`, `I_B` union `U_B` inclus dans `A_p(C)`) et sa preuve dichotomique — toujours le cœur de l'architecture |
| `AUDIT_JUGE_CELLULES_INDEPENDANT_90C06B0_20260812.md` | *réfuté* — un juge rationnel indépendant reçoit la source par cellules comme oracle borné sensible aux mutations | **Porte vacueuse** : la porte lance le driver sans `--judge`, le sujet refuse en code 2, et `WILL_FAIL TRUE` transforme ce refus en vert. En soumettant le flux muté directement, le juge rend code 1 avec 6 vérités manquantes | La vérification mathématique du juge (Gram, positivité barycentrique stricte = centre dans `relint conv(U)`) et la contre-fixture `k=1` |
| `AUDIT_CONTRE_AUDIT_407D4D1_SENTINELLE_HORS_SUPPORT_20260812.md` | *réfuté* — census terminal par sentinelle top-`(12-q)` hors support, parallélisé par sous-arbres disjoints | Le parallélisme ne réduit ni le nombre de cellules ni les `839 582 666` occurrences à `n=50 000`, **et il casse la télémétrie** : `--threads=2` publie `7 012` occurrences contre `22 543` lifts, avec code de retour zéro | Le théorème de la sentinelle top-`(12-q)` hors support et sa minimalité |
| `NOTE_CLAUDE_ETAT_CELLULES_CENTRES_20260812.md` | *mesure datée* — la subdivision de l'espace des centres remplace le parcours d'arrangement | Tables issues d'une machine partagée à deux cœurs, sans épingler ensemble binaire, commande, graine et transcript | Le filtre droite–cellule est **exact et fail-open** mais annule son gain sur CPU |
| `NOTE_CLAUDE_PENTES_UNIFORM_VERTES_20260812.md` | *mesure datée* — la pente rouge des cellules est une propriété du générateur `terrain`, pas de l'ordonnance | Pentes vertes issues d'un binaire local non gelé, sans transcript ; les `wall_s` relevés sous charge concurrente ne bornent rien | L'explication géométrique de la superlinéarité de `terrain` : `coord = sqrt(25 n)` fait croître la boîte en `n^1,5` |

## 3. Source par ancre et lentille aiguë

| pièce | idée | pourquoi elle n'a pas tenu | ce qui survit |
| --- | --- | --- | --- |
| `AUDIT_CONTRE_AUDIT_PRODUCTEUR_ANCRE_LENTILLE_AIGUE_20260812.md` | *réfuté* — une coupure de lentille aiguë ferme collectivement des chambres de paires avant tout `PairId` | La lentille ne ferme pas la famille adversariale : une construction u16 explicite donne un carrier aigu à **toutes** les paires, et le diagnostic sur `eight_clusters,n=50000` en trouve un sur `300/300` paires échantillonnées | **Le théorème de face adjacente aiguë** (tout q4 positif a au moins une face `abx` ou `aby` strictement aiguë) — encore utilisé par la lane q4 — et l'identité entière `4 Q_ab(x) = ||2x-a-b||^2 - D^2` |
| `REPONSE_CLAUDE_CONTRE_AUDIT_LENTILLE_AIGUE_20260812.md` | *supersédé* — réponse actant les réfutations et paramétrant les seuils par `smax` | Le fichier porte lui-même un bandeau de statut historique ; ses verdicts « domaine fermé » et « high-water fait » sont rectifiés par le contre-audit | `lane_death_threshold(smax,q)` et `envelope_depth(smax) = smax-2`, qui redonnent `10/9/8` et `9` à `smax=11`, et le mutant `smax-fixed-thresholds` |
| `AUDIT_JUNG_ANCHOR_389A742.md` | *réfuté* — un couple de carriers chacun dans la lentille suffit à faire de `pq` une ancre diamétrale | Fixture u16 à cinq points : quatre distances à `97 <= D^2 = 100` mais `||x-y||^2 = 144 > 100`, le centre sort de l'ellipse et un témoin est mal classé | La fixture à cinq points, permanente, et la correction minimale `||x-y||^2 <= D^2` — **posée avant le produit dans la lane q4 courante** |

## 4. Ledger des causes et owner

| pièce | idée | pourquoi elle n'a pas tenu | ce qui survit |
| --- | --- | --- | --- |
| `AUDIT_LEDGER_CAUSES_LIFTS_238CF12_20260812.md` | *absorbé* — fermer la partition des lifts par arité avant toute conclusion causale ; `SupportKey` avant lift | Sa prédiction chiffrée a été vérifiée puis intégrée : les `130 033` occurrences `pending` non attribuées sont récupérées, l'identité ferme à écart nul | L'identité `lifts_q = dégénérés + owner + positivité + acceptés + rang` et la fixture `K_24` |
| `NOTE_CLAUDE_LEDGER_CAUSES_LIFTS_20260812.md` | *supersédé* — premier ledger : la cause dominante est le rejet owner tardif, multiplicités `42/55/510` | La table ne ferme pas : `18 048 + 97 825 + 14 160` occurrences restent sans attribution, et les quotients divisent des occurrences de trois populations par les seules acceptations | Les taux de rejet owner — `96,1 %` en q2, `91,7 %` en q3, `92,1 %` en q4 — arithmétiquement justes, qui motivent le groupement avant lift |
| `NOTE_CLAUDE_MULTIPLICITE_SUPPORTKEY_20260812.md` | *mesure datée* — histogramme de multiplicité par `SupportKey` sous `--multiplicity` | Exécution non épinglée, sans graine ni ELF ; l'histogramme est refusé parce que ses trois issues sont un **stade maximal**, pas des propriétés orthogonales | La fermeture à écart nul par arité, confirmant la prédiction du contre-audit au chiffre près |

## 5. Ordre k, Gabriel et route sparse

| pièce | idée | pourquoi elle n'a pas tenu | ce qui survit |
| --- | --- | --- | --- |
| `AUDIT_REPARATION_K_GABRIEL_K_MST_20260812.md` | *absorbé* — remplacer le K-graphe de Gabriel brut par un graphe complété d'étoiles silencieuses | La fixture `E5` tue définitivement le graphe brut ; la réparation est passée dans `docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md` | Le lemme de l'étoile silencieuse : au plus `|I(Q)| <= k-1` attaches au lieu d'une clique complète |
| `AUDIT_REPONSES_ROUTE_SPARSE_GABRIEL_20260812.md` | *dialogue clos* — six questions d'architecture sur la route sparse directes + gateways | Chaque point est fermé avec sa cause ; notamment le pivot choisi dans l'union des supports est **faux sur un carré cosphérique** | La clé de niveau canonique `beta = N/(4D)` et ses bornes u16 avant réduction, par arité |
| `QUESTIONS_CLAUDE_ROUTE_SPARSE_20260812.md` | *dialogue clos* — les activations locales certifiées sont-elles déjà la source directe complète ? | Les deux prémisses de tête sont invalidées : les `68,07` enregistrements par point sont des supports **proposés**, pas des cofaces directes reçues | L'identité `Q = U union I` avec `k = p+q-1`, et les mesures d'arité par point |
| `NOTE_CLAUDE_MESURE_VOLUME_ARRANGEMENT_20260812.md` | *dialogue clos* — mesurer le volume de l'arrangement d'ordre k pour discuter l'énumération de ses sommets de transit | Tranché : une rampe finie sur une famille n'établit pas `Theta(n)`, et **les sommets de transit du plein arrangement ne sont pas une sortie normative** | Le caractère contractuel de `K_max = 10` et des dix forêts |

## 6. Réemploi Yao48, SupportKey et architecture GPU

| pièce | idée | pourquoi elle n'a pas tenu | ce qui survit |
| --- | --- | --- | --- |
| `AUDIT_REEMPLOI_YAO48_P1A_LIGNE_ENREGISTREE_20260811.md` | *réfuté* — réemployer la frontière Yao48/LBVH et le prune center-cover `P1a` de la ligne enregistrée | **Aucun prune ne survit au portage** : une fixture u16 montre que le filtre radial `dist2 >= 3D` est faux comme preuve de dix intérieurs stricts (`D=25`, un témoin sur la coquille, il ne reste que neuf stricts) | Les deux fixtures q2 gravées, dont les dix contacts à distance cinq de `(10,10,10)` donnant profondeur stricte nulle |
| `AUDIT_DEBLOCAGE_GPU_SUPPORTKEY_TOP12_20260812.md` | *absorbé* — dédupliquer les `SupportKey` avant toute géométrie, rejouer l'owner par point-location | Le diagnostic qui le motive est mesuré et devenu la route : `21 395 212` supports pour `839 582 666` géométries à `n=50 000`, soit `39,24` géométries par support et `81,6 %` de rejets owner | Le théorème du minimum auto-centré, qui fonde le « q3 par droite » du producteur courant |
| `NOTE_CERTIFICAT_HELLY_DISQUE_JUNG_20260811.md` | *supersédé* — certifier par Helly qu'une couverture du disque de Jung se réduit à au plus trois `PointId` | Le certificat reste **ponctuel** : aucune borne uniforme sur un produit d'AABB, et son test exact déborde l'arithmétique visée (`F_k` autour de `180` bits sous u16, hors `i128`) | La marge exacte `mu_z(t)` et le demi-plan mauvais ; le sous-certificat de taille au plus trois par Helly |
| `NOTE_COEUR_UNIVERSEL_JUNG_ANCRES_Q3_Q4_20260811.md` | *absorbé* — prédicats entiers sans division certifiant un cœur universel sur une arête maximale | La note déclare elle-même n'être qu'une reformulation de filtres antérieurs et **ne borne ni le nombre d'ancres survivantes ni le coût** ; le pire cas reste quadratique en paires | Les deux prédicats entiers et leurs relaxations par boule de milieu `3||U||^2 < D^2` et `15||U||^2 <= 4D^2` — **exactement les lanes q3/q4 du pipeline courant** |
| `NOTE_CLAUDE_MESURE_PORTE_REGULIERE_20260812.md` | *mesure datée* — la porte régulière exige un support minimal unique sans label extérieur | Les fractions de records portant une extra-shell (`4,17 %` sur `terrain`, `11,48 %` sur `scanline_single_pass`) **falsifient** l'hypothèse « aucune extra-shell » | Les trois prédicats d'extra-shell exacts et entiers, un par arité, dont le déterminant in-sphere nul en q4 |

## 7. Dialogue clos

| pièce | idée | pourquoi elle n'a pas tenu | ce qui survit |
| --- | --- | --- | --- |
| `QUESTIONS_CLAUDE_CELLULES_CENTRES_20260812.md` | *dialogue clos* — deux lemmes et trois questions avant la réécriture du prototype de cellules | Réponses rendues et reprises par la note de solution : `L1` reçu sous l'invariant de pool, `L2` seulement comme identité sémantique post-census | La preuve par contraposée de `L1` |

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
