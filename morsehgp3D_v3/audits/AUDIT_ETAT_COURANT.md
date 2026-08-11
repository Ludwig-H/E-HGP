# Audit courant de MorseHGP3D v3

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Fraîcheur et portée

`HEAD` audité : `2e49dcf45d5136bbeb1e3c345f84fa5739b2f961`. Les
modifications locales de l'auditeur sont exclusivement documentaires dans
`README.md`, `PROPOSITION.md` et `audits/`; aucun octet de prototype, d'oracle,
de CMake ou de reçu n'a été modifié par l'auditeur.

| objet décisif | SHA-256 |
| --- | --- |
| `CMakeLists.txt` | `c98e08be22a45c7430a2cd6d19da4acc3580a6117ec873e51c9b68e67decd480` |
| `prototype/cloud_families.hpp` | `1a3e3027c2e0880e6ff381fc80b707b9ec88dbf573579aac535cfc80bb307b54` |
| `prototype/morton_lbvh.hpp` | `23ffc797c35e24823cf346be934643b0447f8d69a5c0843b4fd090ddc548b267` |
| `prototype/pair_yao48_source.cpp` | `68d7435f36af85987885a2b55702282728afaa879c214ebf54814506e8ef861b` |
| `prototype/yao48_source.hpp` | `59720b420052aeb889cc05afdf557a8006a606ef2129c3751114ce3bc51068bd` |
| `prototype/emst_boruvka.hpp` | `0e2ca1276fb5b53f9e43c7186021fca9258bf91ceee4c85679179a6d5f9e68f4` |
| `prototype/emst_boruvka_probe.cpp` | `caa0cacd3c9ec2a25688673e005d823be4cb809c5c8ac34e690086c397cc1467` |
| `prototype/warm_e2e_h0_diagnostic.cpp` | `24243f88c60e9383b6e9f718f17f6b63048a8f976d60975b7ee0f6aa80e79d64` |
| binaire Release q2 | `31f3a9a17a06aaf5f2d78ec84d6c49f1cfff526a3178a00ac607729f2c8d8334` |
| binaire Release EMST | `2cab9d52d74c700014cd9e6238347f5ab6b704f69c15646522a8efe1a621a605` |
| binaire Release horizontal partiel | `b968ac76a61e284936e4907768e722e3823b0b360c91d1b0b2a840de1496fa21` |
| reçu brut q2 12,5/25/50 k | `acf8e89248131cc7fdce3246f559d380acbee4ce67548ac9fb5e26efdd67d889` |
| dérivation des exposants q2 | `f2d9783211d884fef821a45961d428ee645bad656685d1520337957f54d2776f` |

Les sources q2 et le binaire sont restés identiques avant, pendant et après les
douze profils. Le reçu versionné est identique octet pour octet à la sortie
originale. Une modification d'un SHA de code ou de binaire rend les tests et
profils correspondants historiques; le présent fichier doit alors être
repincé avant tout verdict live.

## Verdict

Le contrat n'est pas rempli. Le SLO officiel à 50 000 points et `K=10` porte
sur `BenchmarkOutputContract-v1` : dix forêts, applications verticales, lots
atomiques, certificat minimal et copie hôte dans le même p95 `warm_e2e`. Aucun
exécutable v3 ne produit ce payload.

Trois décisions sont désormais nettes :

- la source q2 CPU ferme ses ledgers sur les douze profils sanctionnés, mais sa
  gate de compteurs est **NO-GO** sur `terrain` et les deux familles scanline;
  un port CUDA littéral est interdit;
- le nouveau Borůvka/LBVH est un candidat exact et prometteur pour `k=1`, mais
  il ne publie encore que l'EMST brut et son juge laisse passer une classe de
  fausses incidences métriques;
- `P15-HOCUDA-P1a` possède maintenant une spécification q4 entière auditée,
  mais toujours aucune implémentation v3 reçue.

Le harnais nommé `warm_e2e_h0_v3_diagnostic` relie LBVH, EMST brut et q2
count-only. Son nom de série et son avertissement de portée sont corrects; sa
sortie n'est ni un warmup sanctionné, ni un payload horizontal matérialisé, ni
un SLO officiel.

## État des tests locaux

La configuration Release CPU enregistre 376 CTests. Sur les octets pincés :

- les 15 tests `^mhgp3v_emst_` passent;
- les 3 tests `^mhgp3v_warm_e2e_` passent;
- les matrices indépendantes supplémentaires EMST passent 120/120 : quatre
  familles, dix graines, trois tailles de feuilles, `n=257`, oracle Prim et
  permutation actifs.

Ces verts qualifient uniquement les propriétés exercées. Ils ne ferment ni les
bords EMST `n=1,2,3`, ni un sanitizer CMake de cette lane, ni la gate 50 k, ni
CUDA/G4, ni le payload produit. La suite globale n'est pas revendiquée dans ce
pincement tant qu'elle n'a pas été rejouée après la dernière configuration.

## Source q2 Yao48/LBVH

### Réception exacte locale

Le prédicat terminal et les coupes Yao restent stricts et fail-open aux
égalités. Le probe possède un oracle exhaustif borné, un ledger par ancre, des
sorts par paire dans les modes bornés, des banques exactes ou par antichaîne,
une enveloppe radiale multi-chambre et un classifieur partagé par ancre. Les
douze profils à `12 500/25 000/50 000` finissent avec `rc=0` et ferment
`region_pruned_mass+point_tombstones+survivors=C(n,2)`.

Le reçu brut et sa dérivation sont dans
[`../receipts/yao48_scale_20260811/`](../receipts/yao48_scale_20260811/).
Les exposants sont `log2(C(2n)/C(n))`; les chronos sont ceux de la phase locale
mono-thread, hors génération et LBVH, donc jamais un `warm_e2e`.

| famille | phase locale 50 k | visites de coupe, pentes | survivantes, pentes | tests `Phi` totaux, pentes | tests boîte classifieur, pentes |
| --- | ---: | ---: | ---: | ---: | ---: |
| terrain | 72,536 s | 1,388 / 1,381 | 1,398 / 1,426 | 1,403 / 1,457 | 1,559 / 1,642 |
| scanline simple | 105,141 s | 1,455 / 1,549 | 1,513 / 1,686 | 1,575 / 1,593 | 1,724 / 1,834 |
| multiecho | 69,811 s | 1,452 / 1,421 | 1,453 / 1,476 | 1,461 / 1,488 | 1,694 / 1,650 |
| uniforme | 75,702 s | 1,220 / 1,147 | 1,105 / 1,076 | 1,111 / 1,106 | 1,279 / 1,094 |

Les tests `Phi` totaux additionnent la voie liste et la voie arbre. À 50 k, le
classifieur exécute selon la famille 634 à 786 millions de tests ponctuels et
992 millions à 1,562 milliard de tests de boîtes. Les banques croissent près du
linéaire et la famille uniforme reste sous 1,35 sur les compteurs de travail;
le verrou est l'aval de coupe/classification sur les trois familles structurées.
Deux pentes de travail successives au-dessus de 1,35 suffisent à refuser la
route entière avant GPU. La masse prunée, naturellement proche de `Theta(n^2)`,
n'est pas utilisée comme compteur de travail.

L'enveloppe radiale est un progrès substantiel : avec les tombstones
ponctuelles, la coupe ferme 96 à 98 % de la masse selon le reçu dérivé. Mais les
2 à 4 % restants représentent encore 23 à 50 millions de paires; leur
classifieur domine. Sharder ce même travail change la latence, pas ces
exposants.

### Défauts de réception et de télémétrie

1. Dans `pair_yao48_source.cpp`, deux branches successives portent le même test
   `fixture_name == "radial-straddle"`. La première reconstruit le nuage après
   l'exécution; la seconde, qui exige un reçu radial et vérifie les six paires,
   est inatteignable. Le CTest dédié peut donc être vert sans ses assertions.
2. `merge_receipts` omet `radial_prunes`, `radial_pruned_mass`,
   `antichain_nodes`, `classify_list_tests` et `classify_list_pairs`. Le ledger
   de masse reste juste parce que la masse radiale est déjà incluse dans
   `region_pruned_mass`, mais la télémétrie shardée est fausse.
3. `work_done()` omet les tests de la voie liste, les opérations de tas et
   jusqu'à 48 tests de chambres cachés derrière une seule
   `bank_cone_visits`; il additionne en outre des maxima de pile/tas comme du
   travail. Le contrôle n'a lieu qu'avant et après une ancre. `max_work` reste
   donc un coupe-circuit imparfait de probe, jamais une borne produit.
4. Le mode shardé ne porte ni sorts ni reçus. Il vérifie la masse par ancre,
   mais pas une omission compensée par un doublon de même masse dans cette
   ancre. Le reçu de profil ne scelle pas à lui seul commit, ELF, options,
   environnement ou mémoire; le tableau de fraîcheur ci-dessus complète cette
   provenance pour ce seul audit.

Le statut exact est donc : source CPU count-only utile comme différentiel et
falsificateur d'architecture; route produit/GPU refusée en l'état.

## Lane `k=1` : EMST

### Ce qui est prouvé

Sous les préconditions d'un nuage u16 et d'un `MortonLbvh` intact construit sur
ce même nuage, le cœur Borůvka nominal est exact. Pour chaque composante figée,
le parcours trouve l'unique minimum de la clé totale
`(distance_squared,min_PointId,max_PointId)` : la borne AABB est un minorant,
le prune est strict, les égalités descendent et seuls les sous-arbres purs de
la composante sont sautés. La propriété de coupe rend chaque choix sûr; les
fusions de fin de ronde produisent un EMST en au plus `ceil(log2(n))` rondes.
Les quatre fixtures gravées et leurs constantes ont été recalculées
indépendamment.

Les compteurs déterministes 50 k sont favorables :

| famille | rondes | visites LBVH | tests de distance |
| --- | ---: | ---: | ---: |
| terrain | 8 | 15 357 840 | 1 663 166 |
| uniforme | 8 | 16 847 730 | 3 515 480 |
| scanline simple | 7 | 11 007 750 | 1 500 995 |
| multiecho | 7 | 13 583 290 | 1 592 139 |

Les deux pentes de visites et de tests restent sous 1,35 dans les quatre
familles. Les chronos locaux ont coexisté avec une campagne q2 lourde et ne
sont donc pas publiés comme reçu de latence.

### Ce qui manque avant de dire « lane reçue »

1. `check_spanning` ne vérifie jamais que le niveau d'une arête est sa vraie
   distance carrée. Sur le carré `(0,0),(1,0),(0,1),(1,1)`, le faux arbre
   `{(0,3),(0,1),(0,2)}` étiqueté trois fois `1` passe cardinalité, connexité,
   multiensemble et partitions, alors que `(0,3)` a une distance carrée `2`.
   Il faut rejouer chaque incidence en `i128`; un Kruskal canonique indépendant
   borné est requis pour revendiquer le transcript canonique.
2. Le cœur pousse les arêtes dans l'ordre des racines et des rondes; il ne trie
   ni ne rejoue les lots produits. Avec les abscisses
   `{0,1,3,4,1000,1100}`, une arête de niveau 10 000 est publiée à la première
   ronde avant une arête de niveau 4 de la seconde. Tri lié des triplets,
   groupement par niveau, partitions strictes/fermées et payload restent à
   construire.
3. Le terme `best-first` est faux : la machine emploie une pile LIFO, donc un
   DFS branch-and-bound proche-d'abord. Le LBVH ne porte aucun token
   d'identité/immutabilité validé par l'API.
4. Le parseur accepte jusqu'à `10^12`, caste ensuite en `int`, puis valide.
   `--points 4294967300` devient 4 et sort avec le code 0; le même défaut existe
   pour `--coord` et dans le harnais horizontal.
5. Les bords `n=1,2,3`, la contraction d'au moins moitié par ronde, un mutant
   `bd>=best`, un mutant d'incidence métrique et une cible sanitizer CMake
   manquent. Avec 50 k `PointId` colocalisés, l'égalité empêche tout prune et la
   première ronde effectue `n(n-1)` tests; le profil produit distinct doit le
   refuser, ou une extension doit pré-unir les classes nulles.

Le probe est un excellent différentiel borné, pas encore le producteur
normalisé `k=1`.

### Réemploi Yao-1

Sur le profil à coordonnées distinctes, le voisin canonique le plus proche de
chaque point dans chacune des 48 chambres Yao forme un graphe dirigé d'au plus
`48n` arêtes dont l'union contient l'EMST canonique. Le théorème, le prior art
enregistré et le contrat de mutualisation sont dans
[`AUDIT_REEMPLOI_EMST_YAO48_LIGNE_ENREGISTREE_20260811.md`](AUDIT_REEMPLOI_EMST_YAO48_LIGNE_ENREGISTREE_20260811.md).

Cette voie ne peut consommer une banque q2 comme preuve de minimum que si elle
ferme toutes les bornes plus petites, les ex æquo canoniques et les chambres
vides. Une patience ou un budget épuisé ne certifie jamais `empty`. Le
prototype CPU enregistré est rejeté à 50 k; seuls le théorème, le transcript et
les motifs de réduction sparse sont réutilisables.

## Harnais horizontal partiel

`warm_e2e_h0_v3_diagnostic` ne montre aucune data race manifeste par inspection,
mais sa réception reste insuffisante :

- il vérifie `region_pruned_mass+point_tombstones+survivors=C(n,2)`, pas
  `classifier_tombstones+census_records=survivors`, ni `anchors=n`;
- il hérite de la fusion incomplète des cinq compteurs q2 et ne porte aucun
  sort/digest permettant de rejouer le ledger;
- son « p95 » est une interpolation type 7 sur cinq répétitions par défaut,
  sans warmup; le protocole final exige un nearest-rank sur 30 nuages avec
  bruts, maximum et MAD;
- la provenance omet `chamber_visits`, commit, ELF, options, environnement,
  RSS et capacités; la génération précède le chrono et l'affichage arrondit à
  la milliseconde;
- l'arène append-only du classifieur peut atteindre un pic
  `O(survivantes*noeuds)` par worker, chaque worker dupliquant ses buffers, et
  aucun high-water physique n'est publié;
- il n'existe ni test d'invariance 1-vs-N, ni TSAN, ni mutant de ledger final.

Les deux CTests smoke rejouent la même commande et leur regex ne vérifie que le
nom de série. Cette série doit rester explicitement inéligible au SLO.

## q3/q4 et P1a

Les self-joins et le cœur de Jung restent des oracles/falsificateurs : leurs
rescans par paire sont déjà refusés avant CUDA. Les certificats Jung--Yao et
Helly sont sûrs mais incomplets; ils ne ferment pas seuls l'univers implicite.

La note
[`NOTE_SOLUTION_P1A_CENTER_COVER_MASSONLY_20260811.md`](NOTE_SOLUTION_P1A_CENTER_COVER_MASSONLY_20260811.md)
spécifie désormais exactement la tranche q4-only, seuil huit : coins
rationnels à l'échelle seize, cover de Jung, range-query collective,
microtuile avant patches, ledger bijectif et juge indépendant. Aucun code P1a
v3 ni différentiel `n=32` n'est reçu. La prochaine session G4 P1a ne devient
justifiée qu'après fermeture locale Release, sanitizer et oracle; son protocole
va alors directement de `n=32` au profil 50 k et ne qualifie aucun SLO.

## Conseils mathématiques et d'implémentation

1. Fermer d'abord les trous de réception à coût faible : métrique des arêtes
   EMST, tri/lots, branches radiales réellement mordantes, identités finales du
   ledger shardé, casts CLI et compteurs fusionnés. Aucun de ces correctifs ne
   transforme à lui seul la complexité.
2. Pour q2, remplacer l'enveloppe radiale seule par une coupe multi-chambre
   exacte. Pour chaque intersection `AABB`--chambre non vide, borner en entiers
   les trois sommes canoniques `x`, `x+y`, `x+y+z` et exiger strictement les
   trois inégalités Yao contre la banque correspondante. Un masque 48 bits et
   les versions de banques forment un reçu compact; toute égalité descend.
3. Ne plus développer les dizaines de millions de survivantes dans le
   classifieur actuel. Pour une boîte cible `Q` et un nœud témoin `W`, le
   minimum de `(q-p) dot (w-p)-||w-p||^2` sur `Q times W` est séparable et se
   calcule aux quatre coins par axe. Une antichaîne disjointe de masse dix dont
   chaque minimum est strictement positif certifie tout `Q`; l'égalité descend.
   La prochaine expérience doit transmettre ces crédits sous raffinement dans
   une traversée duale persistante, sans rescan racine ni matrice de couples.
   Un reçu logique factorisé remplace une arène par région ou paire.
4. Extraire Yao-1 avec q2 seulement si le transcript de minimum/`empty` est
   complet; comparer son coût marginal au Borůvka actuel. Le bottleneck global
   reste q2, pas la réduction sparse de `k=1`.
5. Rejouer la même gate de compteurs sur toute nouvelle architecture device
   avant une latence G4. Aucun résultat GPU n'est utile tant que deux pentes de
   travail restent rouges ou que le payload officiel n'a pas de producteur.
6. Implémenter P1a q4 séparément et conserver sa décision mass-only séparée de
   q2, de P1 complet et du SLO.

## Commentaires de code périmés ou trop forts

L'auditeur ne modifie pas le code de Claude. Les commentaires suivants doivent
être corrigés côté code sans recréer d'anciennes autorités :

- `CMakeLists.txt:4` dit « uniquement M1 » malgré les nombreux prototypes;
- `CMakeLists.txt:90-95` et `prototype/scale_profile.cpp:1-7` présentent un
  ancien compte de sommets comme l'unique mesure décisive pour 100 ms;
- `CMakeLists.txt:201-203` dit que « l'ancien nom P1a » désignait un
  center-cover retiré, alors que P1a est précisément le center-cover actif;
- `prototype/morton_lbvh.hpp:1-10` parle d'une disposition résidente et d'une
  construction Karras portable device, tandis que cette classe CPU trie avec
  `std::sort`, construit récursivement et rescane chaque plage pour son AABB;
- `prototype/emst_boruvka.hpp` et son probe disent `best-first`, « lane du
  contrat » et « trie/rejoue », alors que le composant actuel est le cœur brut
  DFS décrit ci-dessus;
- `CMakeLists.txt:261-264` et le probe EMST citent des fixtures « vérifiées hors
  bande » dans un rapport de session non identifié;
- `CMakeLists.txt:270-272` dit que le ledger horizontal est rejoué; seules deux
  égalités de masse partielles sont aujourd'hui contrôlées;
- `mhgp3v_structural_scale_k1_emst` exécute l'ancien
  `mhgp3v_structural_scale_check`, pas le nouveau Borůvka.

## GCP

GCP non utilisé pour cet audit. Aucun état de VM n'a été modifié.
