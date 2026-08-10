# Audit live — transcript `q_min`, préflight et join postings global

Date : 10 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_and_bounded_oracles`,
`profile=quantized_u16_input_only`, `mode=audit_independant`, aucun statut
public. Cet audit ne modifie aucun prototype.

## Snapshot reçu

Le snapshot est un worktree non committé au-dessus de
`HEAD=origin/main=651e47f804060a864c463387d541d982f93e1554` :

| fichier | SHA-256 |
| --- | --- |
| `CMakeLists.txt` | `40c6707fa5c44b65b773ab3a6f0ce15885ead010aeb34d4a8a761c405caf8e2a` |
| `prototype/saturated_fold.hpp` | `39cf76edea86847753eec263207ab9e257dcb9f08c6420a80b205b840561cdd6` |
| `prototype/saturated_fold_global.hpp` | `63e57476b8de8860a0da32a1f9ad50b5dda3e1f976d888df93e2b10dab2fc68e` |
| `prototype/saturated_pipeline.cpp` | `317346d2142adb7f6b7f73eb62eb3b8a77ceedab7ac5b2349c991a6d6b1724a3` |
| `prototype/postings_join_gate.cpp` | `60ce8340d1dbc2f358b37cedcc854e27b3d17beba7fef64e0b7ef56862f98a3d` |
| `oracle/gamma_forest_judge.cpp` | `4fd1fa2b34b5185debefa47249824923473ce073254603a76e586c4b24ec8be8` |

`saturated_fold_global.hpp` est encore non suivi par Git dans ce snapshot.
Tout résultat ci-dessous est donc attaché à ces empreintes, pas au seul HEAD.

### Correctif live immédiatement postérieur

Claude a répondu pendant l'audit. Le fold global passe à
`f71954a355f6159a0ef3e594665fe630bd116d775a0444ccb17a9457c7e0f830`
et le pipeline à
`bb16b8ce79261c56f1110c3c38a40bebe1de8787348c2324012c7c07d13a1cad`;
les quatre autres empreintes restent celles du tableau. La fonction globale
accepte désormais un budget, calcule un modèle de pic sur `P_post` et publie le
manifeste avant son refus. Ce correctif est positivement crédité. Un rebuild
Release frais de ce second snapshot et sa sélection élargie passent 24/24 en
33,12 s.

Deux raccords restaient ouverts sur ce snapshot intermédiaire : le pipeline ne
transmettait pas le budget et `P_post` passait par un `long long` avant la
garde.

Claude les ferme dans le delta live suivant : global
`2556aa42f3de24e15bd07340c559e45bd645e2573b642a89d98e90dea78f0e12`,
pipeline
`1f172eaf0e1acee8c7610d2ada3482d61010c5229cc2768ee6fd313f844892b2`
et CMake
`caa983806e6f370f5fff9808fee023c253ff299086ad7fb55b49a83563cb0e65`.
La séquence est maintenant `degrés u128 -> manifeste -> budget -> CSR ->
émission`, le pipeline transmet le budget et un CTest global l'exige. Un build
frais et la sélection courante complète du delta passent 25/25 en 12,49 s. À
1 Mio, la voie globale rend le code 3 et publie `P_post=6 889 344`, pic estimé
214,0 Mio et lot global 6 889 344. Ces deux corrections sont reçues; restent la
qualité de la borne mémoire et la matérialisation intégrale.

### Réception live postérieure : records de composante par témoin

Après le commit `bc2dafaff96edbca6c5fff455b5071730d95437d`, Claude a
implémenté la solution par témoin proposée plus bas. Le snapshot non committé
reçu ici est épinglé à `saturated_fold.hpp=8603da95`,
`saturated_fold_global.hpp=788492ca`, `postings_join_gate.cpp=db10d35e`,
`gamma_forest_judge.cpp=3f6d9333` et `CMakeLists.txt=586c075a`.

Le résultat positif est substantiel : G², postings par lots et postings global
propagent le minimum lexicographique des `k` plus petits `PointId`; les deux
joins compressés retraduisent bien leurs identifiants denses avant publication.
Le juge construit indépendamment, depuis son DSU de `k`-faces, le témoin fermé,
les témoins stricts absorbés et le type de chaque composante. Il compare ensuite
ces records au niveau rationnel exact. Les campagnes directes donnent 1 309
records concordants sur 30/30 ordres génériques et 2 308 sur 60/60 ordres
saturés; la fixture dédiée donne 41 records sur trois ordres. Les mutants
`drop-strict-witness` et `stale-witness` rendent le code 1 uniquement par
`RECORDS REFUTES` : ils établissent donc une vraie valeur ajoutée au-delà des
triples par niveau.

La preuve d'accord avec le témoin minimal de l'oracle a une hypothèse précise :
elle est autoritative seulement sous source complète pour l'ordre `k`. Toute
`first_k(M)` du produit est une face de l'oracle, tandis que la face minimale
`F` de l'oracle possède alors `Sat(F)` dans la source et
`first_k(Sat(F))<=F`; les deux minima coïncident. Sans certificat de complétude,
le record reste exact relativement à la sous-famille, pas à Gamma.

Une dernière porte annoncée comme « compensation » n'établit pas encore ce
qu'indique son nom. Le CTest courant combine `skip_first_event_marker` et
`mark_first_redundant`, mais le premier frappe un singleton au premier niveau
et le second un niveau ultérieur. La sortie contient donc aussi
`TRANSCRIPT REFUTE : types divergents`; les triples suffisent déjà à tuer le
mutant. La correction constructive est un mutant **atomique dans un même lot** :
au niveau 25 de la fixture, retirer le marqueur vrai de la composante `DEF` et
marquer à tort la composante redondante `ABC`, toutes deux continuations. La
porte doit d'abord constater l'égalité des triples, puis exiger
`RECORDS REFUTES`. Cela reçoit exactement l'erreur compensée visée.

Claude a ensuite remplacé cette combinaison par le mutant atomique proposé.
Sur la fixture, `--force-swap-marking 1` rend maintenant le code 1 avec
uniquement `RECORDS REFUTES : temoin ferme divergent`, sans
`TRANSCRIPT REFUTE`. Cette fermeture est positivement reçue sur le live. Le
second mutant `--force-extra-marker 1` survit encore sur la fixture disjointe :
il n'y trouve aucun redondant dans une racine déjà marquée. La fixture minimale
qui le fait mordre est donnée dans la note témoin : triangle `ABC` et paire
`AD`, de même rayon cinq et partageant `A`; le vrai marqueur fusionne la racine,
le triangle redondant ne change que `marking_saturations`.

Le snapshot épinglé ne contient pas encore la liste ou le digest des boules
marquantes. Il peut donc manquer un marqueur dans une racine déjà marquée sans
changer le record de composante. Claude a commencé à ajouter immédiatement
après ce pin un multiensemble `marking_saturations`. Cette clé est
mathématiquement suffisante sur un catalogue valide et un nuage fixé : si `B`
est la miniboule d'un support `U` inclus dans son saturé `M`, alors `B` contient
`M`, tandis que toute boule contenant `M` contient `U`; les deux inégalités de
rayon et l'unicité de la miniboule imposent `B=miniball(M)`. Un handle interné
ou digest canonique de `M`, lié au digest d'entrée, reçoit donc la boule sans
copier tous ses membres dans chaque record. Le champ
`level_representative` est par ailleurs un indice catalogue brut : le juge le
retraduit correctement en rationnel exact, mais le payload public devrait
porter le niveau exact ou l'ordinal canonique de sa classe. Enfin, le pipeline
ne hache ni ne compare encore `gamma_records`.

Une sonde hors dépôt ASan/UBSan sur le catalogue abstrait
`{10,INT_MAX}`, `{10,1000,INT_MAX}`, `{1000,INT_MAX}` est positive : G²,
postings par lots et global à deux threads rendent les mêmes partitions et les
mêmes records; à `k=2`, le record observé publie
`closed={10,1000}` et `strict={10,INT_MAX}`. La retraduction dense vers
`PointId` brut est donc fonctionnelle sur ce cas extrême. Il reste utile de
graver cette sonde en fixture permanente afin que la propriété ne repose pas
sur les seuls nuages aux identifiants contigus.

Le coût mémoire doit suivre cette avancée : `witness` et `staged_witness` sont
actuellement deux tableaux de petits vecteurs par générateur et par ordre, soit
environ `48*G*K` octets d'objets `vector` avant leurs payloads et allocations.
Les capacités des témoins et de leurs copies d'époque ajoutent au pire environ
`4*G*K*(K+1)` octets. Cela représente déjà environ 14,4 Mio pour
`G=40 007,K=5`, avant allocateur, alors que le préflight réserve 8,0 Mio pour
l'ensemble des états; à `G=50 000,K=10`, ce seul poste approche 46 Mio contre
20 Mio annoncés. Les `marking_saturations` recopient ensuite les membres par
record et par ordre. Le modèle de pic antérieur ne les couvre donc pas. Une
représentation en petit tableau fixe de taille `K`, ou des handles internés dont
le stade ne copie qu'un handle, conserve la preuve en réduisant fortement ce
poste.

Le snapshot stabilisé de cette intégration est : CMake `f96ea587`, fold G² et
par lots `d510780d`, global `7209ee4a`, gate `953722f7`, juge `b1a409e3` et
pipeline `1f172eaf`. Un build frais réussit; la sélection élargie passe 27/27
en 27,60 s, dont onze portes Gamma/records, les campagnes et six mutants
postings, les refus, les deux comparaisons pipeline et les deux budgets. Les campagnes comparent 1 309 puis 2 308 records,
marqueurs inclus. Le mutant d'échange ne produit que
`temoin ferme divergent`; le marqueur superflu, sur cinq nuages saturés, ne
produit que dix `boules marquantes divergentes`. La fixture permanente des
identifiants clairsemés fait passer les trois joins, à un et deux threads.

## Verdict constructif

Le noyau mathématique du join est maintenant en bonne voie : aucune erreur de
connectivité n'a été trouvée, les trois calculs G², postings par lots et
postings global rendent le même fold sur les campagnes bornées, et la porte
compare désormais chaque poids exact `(M,N)->|M intersection N|` à un oracle
d'intersections directes. C'est un vrai gain de falsifiabilité.

Le prédicat d'événement `q_min<=k+1` est correctement employé pour marquer les
racines. Les histogrammes, témoins fermés, témoins stricts, types et saturés
marquants concordent à chaque niveau sur 30/30 ordres génériques et 60/60
ordres saturés. L'identité des composantes du transcript est donc reçue sur ces
campagnes bornées. Le statut Gamma autoritatif reste conditionné à une source
complète et à `q_min` certifié au runtime.

Le préflight par lots calcule maintenant exactement `P_post` et la masse du
plus gros lot avant émission, remet le reçu à zéro et laisse son manifeste
observable lors d'un refus de budget. La forme globale est, elle, une bonne
troisième vérité parallèle, mais pas encore la forme d'échelle annoncée : elle
conserve tous les buffers locaux, les concatène dans un second vecteur de
`P_post` occurrences, puis effectue un tri global. Ses « chunks » ne bornent
donc encore ni la mémoire ni le travail d'un posting lourd.

La décision utile pour Claude est ainsi : **GO pour conserver les trois voies,
le rejeu DSU commun et les records par témoin; prochain palier = interning des
témoins/marqueurs, certificat de source et runs externes bornés.** Aucun graphe
de Johnson, sous-simplexe ou mosaïque d'ordre supérieur n'est nécessaire.

## Résultats positifs reproduits

Un configure/build Release frais des trois exécutables modifiés réussit. Deux
sélections CTest, 15 puis 6 tests sans recouvrement utile au delta, passent
21/21 en 13,62 s cumulées. Elles couvrent les deux campagnes postings, les six
mutants existants, les refus CLI, les comparaisons in-process par lots/global,
le budget et les portes `q_min`.

Les campagnes du join donnent :

| campagne | générateurs | `P_post=poids` | unions réussies | niveaux |
| --- | ---: | ---: | ---: | ---: |
| 30 nuages génériques | 1 950 | 110 390 | 4 916 | 4 782 |
| 20 nuages saturés | 1 623 | 129 661 | 4 200 | 2 386 |

Pour chaque catalogue de ces campagnes, la porte rejoue aussi la forme globale
à un puis deux threads, exige le même fold que G² et le même reçu champ à champ
que la forme par lots. La table complète des poids est reconstruite par
intersections directes indépendantes.

Sur `--points 32 --smax 11 --max-order 3 --seed 20260810`, la forme globale à
deux threads et G² rendent le même digest diagnostique
`13583866067985804659`, les mêmes 6 628 niveaux et le même histogramme Gamma
`215/3666/161`. Le reçu global porte `P_post=6 889 344`, 2 220 704 paires
réduites et 6 980 unions réussies. Sur cette petite entrée, le join global
n'apporte pas encore de gain temporel : 6,13 s de fold contre 5,11 s pour G²;
c'est un diagnostic, pas une régression scientifique.

Les deux campagnes du juge donnent :

| campagne | prédicat | histogrammes par niveau | niveaux prédits | erreurs `q_min` |
| --- | ---: | ---: | ---: | ---: |
| générique | 30/30 | 30/30 | 1 234 | 0 |
| saturée dégénérée | 60/60 | 60/60 | 1 704 | 0 |

Ce crédit est renforcé par un résultat négatif instructif : le mutant actuel
`--force-qmin-shift 1` rend bien le code 1 et 124 désaccords, mais le sous-bilan
du transcript reste 4/4 en accord. Il mute l'oracle de niveaux et la
provenance, pas le marquage du sujet. La prochaine porte de transcript doit
donc muter le sujet lui-même.

## Verrou mathématique résolu : identifier une composante sans la matérialiser

Les trois comptes par niveau laissent passer deux erreurs compensées entre
composantes de même type. La solution légère est le témoin canonique
`omega_k(R)`, minimum lexicographique des `k` plus petits membres d'un
générateur de la racine `R`. Deux racines distinctes ne peuvent partager ce
témoin : deux générateurs qui contiennent la même `k`-face ont une intersection
d'au moins `k` points et le join les relie.

Le record à comparer avec l'oracle est donc :

```text
(ordre, niveau exact, témoin fermé, témoins stricts absorbés,
 type, identités des générateurs marquants)
```

Le témoin se maintient par un minimum à chaque union; son coût est `O(k)` et il
reste local aux racines touchées. La preuve, la fixture compensée de deux
continuations au niveau 25 et les mutants nécessaires sont détaillés dans
[`NOTE_SOLUTION_RECU_TRANSCRIPT_PAR_TEMOIN_20260810.md`](NOTE_SOLUTION_RECU_TRANSCRIPT_PAR_TEMOIN_20260810.md).

La porte tue maintenant séparément marqueur vrai omis, générateur redondant
marqué, témoin strict perdu, témoin périmé, échange compensé et marqueur
superflu dans une racine déjà marquée. Elle publie un plancher de records.
Restent une fixture permanente `q_min=k+1`/absence de naissance, des planchers
par type, la canonicalité autonome du niveau et le certificat de source; c'est
la frontière exacte entre « records relatifs reçus » et « transcript Gamma
public exact ».

## Verrou d'échelle résolu architecturalement : des runs réellement bornés

La forme globale actuelle répartit les points entre workers, mais ne découpe
pas l'intérieur d'un posting lourd. Elle garde ensuite simultanément les
buffers locaux et leur concaténation, puis les arêtes uniques et leur ordre de
rejeu. Son pic est donc en `O(P_post+U)`, avec jusqu'à deux copies des
occurrences pendant la concaténation. Le budget global est désormais raccordé
et refuse avant le CSR; il protège d'un OOM prévisible selon le modèle courant,
mais ne transforme pas cette allocation proportionnelle à `P_post` en runs
bornés.

La transformation exacte proposée est la suivante :

1. calculer en entier vérifié les degrés, `L_sat`, `P_post` et la masse de
   chaque domaine triangulaire avant toute émission;
2. numéroter chaque paire interne d'un posting par son indice triangulaire et
   découper aussi les postings lourds en intervalles de taille bornée;
3. donner à chaque worker un buffer de capacité fixe `C`, trier et réduire ce
   buffer, puis sceller un run par `(domaine, intervalle, masse entrée, somme
   des poids, première/dernière clef, digest)`;
4. effectuer un merge déterministe des runs par clef `(M,N)` en addition
   vérifiée; le poids complet obtenu est indépendant du découpage et du nombre
   de threads;
5. calculer alors le lot d'activation `max(batch(M),batch(N))` et écrire
   l'arête réduite dans des runs de rejeu bornés, triés par `(lot,M,N)`;
6. rejouer les lots séquentiellement avec le DSU actuel, sans conserver la
   table globale des arêtes.

Le pic devient une fonction du nombre de workers, de `C`, des buffers de merge,
du CSR et des DSU, et non de `P_post`. `P_post` reste le coupe-circuit de travail
et de volume de spill. Cette même décomposition fournit les domaines GPU : le
GPU produit et trie des runs bornés; le merge et le rejeu restent d'abord CPU,
jusqu'à ce que leurs reçus soient fermés.

Les mutants spécifiques sont : frontière triangulaire sautée ou doublée,
dernier run omis, poids partagé entre deux runs non additionné, overflow au
merge, arête envoyée au mauvais lot et dépendance au nombre de threads. Une
fixture avec un seul posting dominant doit exiger qu'il soit effectivement
partagé entre workers; le découpage actuel par point ne le garantit pas.

### Deux réductions exactes avant même le spill

À l'ordre un, la clique d'un posting n'a jamais besoin d'être émise. Relier
chaque générateur au premier générateur actif de ce posting construit un arbre
de `d_x-1` arêtes qui a exactement les mêmes composantes que la clique à chaque
coupe fermée; choisir la racine par `(lot,identité canonique)` conserve aussi le
déterminisme. C'est une réduction exacte pour tout catalogue, pas une
heuristique.

Pour les ordres supérieurs, la réduction prouvée par `q_min` peut être appliquée
avant le join lorsque la source est certifiée complète : à l'ordre `k`, les
générateurs avec `q_min>k+1` sont déjà remplacés par leurs carriers stricts. Une
paire n'est utile qu'aux ordres dans la fenêtre
`max(1,q_M-1,q_N-1)..min(K,|M|,|N|,w)`. Cela réduit postings, émissions et
unions sans changer Gamma. Sous source partielle, cette optimisation change le
raffinement relatif et doit rester désactivée ou porter un statut séparé.

## Préflight : ce qui est exact et ce qui reste un modèle

Le passage de degrés et les valeurs `predicted_p_post` et
`max_batch_occurrences` sont exacts relativement au catalogue fourni. Le refus
du join par lots à 1 Mio rend maintenant le code 3 **avec** le manifeste
`P_post=6 889 344`, pic annoncé 4,2 Mio et plus gros lot 14 698 occurrences.

La formule du pic, fondée sur des constantes 32/8/40/64 octets, doit en revanche
être nommée `estimated_peak_bytes` tant qu'elle ne borne pas les capacités des
vecteurs, l'allocateur, les maps/sets, les copies de membres, les sorties, le
tri, `collect_pairs`, les partitions et une marge mesurée. La rendre
contractuelle demande : détail par poste, arithmétique vérifiée pour toutes les
sommes, high-water RSS comparé à l'estimation et facteur de sécurité reçu.

La famille `M_i={0,i}` montre une sous-estimation concrète de la forme globale
avec réception complète : `E=P_post`, et `edges`, `edge_order` et
`receipt.pairs` coexistants représentent déjà environ 40 octets par paire,
avant capacités et DSU, contre 32 dans le modèle. Ce n'est donc pas seulement
une marge d'allocateur à calibrer.

La forme globale reçoit maintenant son budget avant le CSR. Après passage aux
runs, l'admission doit porter sur le pic des buffers bornés et séparément sur un
budget de travail/spill dérivé de `P_post`; refuser l'un ne remplace pas
l'autre.

## Provenance et sémantique : petit type à ajouter

Le fold lit `n_support` comme `q_min` sans certificat runtime, tandis que le
pipeline déduit encore « famille complète » de `smax>=n`. Cette inégalité
écarte une censure par rang, mais ne prouve ni que la source a énuméré tous les
générateurs requis ni que chaque `n_support` est minimal.

Une solution compacte est de faire voyager avec le catalogue :

```text
SourceCertificate {
  catalogue_digest;
  q_min_certified;
  complete_for_order[1..K];
  construction_mode;
  rank_cap;
}
```

Le fold valide aussi `1<=n_support<=min(4,rank)` en dimension trois. La garde
`q_min=k+1` n'est fail-closed que si `complete_for_order[k]` est vrai. Sinon le
record reste un diagnostic de sous-famille, sans suffixe
`relative_to_certified_subfamily` tant qu'aucun certificat n'existe réellement.
Le juge doit enfin échouer explicitement si son calcul de sous-miniboule
échoue, au lieu de seulement compter `qmin_subset_failures`.

Une sonde UBSan du snapshot confirme que ce n'est pas une réserve abstraite :
un catalogue d'une sphère avec `n_support=-1` ou un support hors de `M` est
accepté par les trois folds avec `ok=1`, une naissance Gamma et zéro violation
de garde. Les conversions du nombre de générateurs de `size_t` vers `int`
doivent également être précédées d'une borne explicite.

## Petites corrections de réception

- La comparaison de la table indépendante doit aussi exiger
  `dump.size()==table.size()` après la boucle, afin qu'un suffixe parasite ne
  puisse pas passer.
- La branche permutation de `folds_agree` compare témoin fermé, témoins stricts
  et type, mais omet `marking_saturations`; l'invariance des marqueurs n'est
  donc pas encore reçue. Elle ignore en outre l'indice de niveau sans comparer
  sa valeur rationnelle exacte.
- Le digest et `--compare-joins` du pipeline ne consomment aucun
  `gamma_records`; leurs deux CTests restent aveugles aux témoins et marqueurs.
- La sortie manuelle du mutant atomique établit zéro `TRANSCRIPT REFUTE`, mais
  le helper CMake n'exige qu'une sous-chaîne positive. Publier un compteur
  `triple_mismatches==0` propre au mutant, ou un code dédié, rendrait cette
  absence mutation-résistante. L'ancien test combiné `skip_first+mark_first`,
  encore nommé « compensated », frappe deux niveaux et peut être retiré une
  fois le mutant atomique conservé.
- Le digest du pipeline reste diagnostique et dépend des indices catalogue des
  représentants; il n'est pas encore canonique sous permutation sémantique.
- Le message final du pipeline dit encore que la séparation `q_min` est « en
  cours de réception » après avoir publié son histogramme; remplacer ces deux
  phrases contradictoires par les deux statuts distincts de cet audit.
- Les commentaires et documents qui disent « chunks », « pic conservateur »,
  « types exacts par composante » ou « provenance certifiée » doivent employer
  les formulations bornées ci-dessus jusqu'aux portes correspondantes.

## Décision 50 k / GPU

Le join et le prédicat sont désormais de bons candidats scientifiques; aucune
raison mathématique ne justifie de repartir de zéro. Le GO 50 k attend encore
une source complète certifiée, les runs bornés, une borne mémoire contractuelle
et l'interning des témoins/marqueurs. Il n'existe aucun kernel GPU à qualifier
dans ce delta : lancer une G4 n'apporterait pas d'information supplémentaire avant
ces fermetures CPU.

GCP non utilisé.
