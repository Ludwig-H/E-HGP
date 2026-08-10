# Audit du chrono source--référence `e406e1f`

Date du snapshot : 10 août 2026 UTC.

Cadre annoncé : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_oracle_under_audit`,
`profile=quantized_u16_input_only`, `mode=audit_chronometrique_borne`,
`public_status=not_claimed`.

Cet audit est strictement limité à `morsehgp3D_v3`. Il ne modifie aucun
prototype, n'ouvre aucune phase et ne promeut aucun résultat public.

| objet | empreinte |
| --- | --- |
| snapshot de code et de claims audité | commit `e406e1f646ef20eb222d50e8b2740e6d7d6f6aa3` |
| `prototype/direct_source.cpp` | SHA-256 `d933c3aeb6314f12769f594d30af6734c696b09ce2e67de39af23dbd0ed15ed9` |
| `CMakeLists.txt` | SHA-256 `739d21248a5fba575974aa3e40e8a0d7d4208b4a9c6710905ec4379cffa8fed7` |
| `README.md` avant réponse d'audit | SHA-256 `a5b558c1b7b2baaeacb7e779b83e0cc1d9f7a53a4f0961e317cd4a3f4e468ed3` |
| binaire Release GCC 13.3 testé | SHA-256 `9f1ef706ed0a9005a8a6fa20f56f3caa813d63f267aa0031211ec4c6f6157afc` |
| binaire ASan/UBSan/LSan testé | SHA-256 `33f7dc5d5b207f754fc7678363dc44e0888eff7ec5c6b0328f3bc9a2a9452436` |
| `LastTest.log` temporaire, empreinte relevée immédiatement après la suite | SHA-256 rapporté `9abfbe90c1b816b7bc9c8bde0bcc1fe9ecc57d07573609e178eb1822fa6d3eaf`; fichier ensuite écrasé par `ctest -N`, non revérifiable localement |

## Verdict

**GO pour l'isolation des deux appels de fold et pour la nouvelle campagne
différentielle; NO-GO pour les claims « chronos symétriques », « juge exclu »,
« même payload public », « mêmes nuages » sans précondition et pour toute
conclusion de croisement ou d'échelle tirée des temps publiés.**

Le résultat positif est réel. Les tranches `f0/f1/f2` mesurent séparément
`build_forest` sur le catalogue source et sur la référence. La correction
cumulative retire exactement le fold référence courant : si $R_i$ est son
cumul après le nuage $i$, le terme soustrait vaut $R_i-R_{i-1}$. Il n'y a ni
double comptage du fold, ni temps négatif observé. Un build Release strict passe
14/14 CTests directs; la suite fraîche complète passe 74/74 CTests en 319,87 s.
Les portes `generic`, `forest` et `same_payload` passent aussi sous
ASan/UBSan/LSan en 19,90 s. La nouvelle gate décide quatre nuages, compare vingt
forêts et 5 538 nœuds, et ne trouve aucune divergence du quotient sémantique.
La suite complète a été lancée par
`ctest --test-dir /tmp/mhgp3v-chrono-release.fj1zEQ --output-on-failure -j2`;
le chemin est temporaire. L'empreinte du log a été relevée à chaud, mais un
`ctest -N` de relecture a ensuite écrasé ce fichier; elle n'est donc pas un
sidecar conservé.

Ce crédit ne valide pas les libellés temporels.

## P0 de mesure — le juge reste facturé uniquement à la source

Le chrono source commence ligne 711 avant la génération et s'arrête lignes
999--1001 après tout le différentiel. La soustraction nouvelle retire seulement
`build_forest(pts, truth, k)`. Restent donc dans `source_seconds` :

- la construction de `expected` et la comparaison complète des catalogues,
  lignes 918--958;
- l'assemblage du catalogue source;
- le fold source;
- les deux `forest_digest`, leur comparaison et le bookkeeping du juge, lignes
  986--996.

Le chrono référence contient, lui, `flat_catalogue` puis son propre fold, sans
ces comparaisons ni empreintes. Les deux périmètres ne sont pas symétriques et
le juge n'est pas exclu. À `n=40`, quatre répétitions ont rendu 0,405 à 0,582 s
pour la source avec forêt, dont seulement 0,054 à 0,092 s de fold, contre 0,145
à 0,172 s sans forêt sous juge. À `n=120`, ajouter la forêt augmente le temps
source de 1,611 s alors que le fold source publié ne vaut que 0,409 s. Ce surplus
est compatible avec les empreintes et comparaisons encore imputées à la source.

Le croisement brut se reproduit sur une exécution, avec 11,773 s pour la
référence et 13,136 s pour la source à `n=120`. Il reste un diagnostic du chrono
asymétrique, pas une mesure attribuable aux deux constructions de catalogue.

## Le payload commun reste un quotient, pas le contrat public

Le différentiel compare les records par coquille puis une empreinte de forêt
invariante à la renumérotation. L'ordre canonique public du catalogue, le pool
concaténé, les offsets et les indices `ForestNode::source` restent différents;
la sonde antérieure compte 3 062 positions et 4 016 indices déplacés malgré
120/120 digests égaux. Le terme recevable est donc **même contenu sémantique
quotienté**, sous le même `build_forest`, pas « exactement le même payload ».

La nouvelle gate n'asserte par ailleurs aucun chrono, rapport, libellé,
périmètre de timer ou high-water commun. Elle resterait verte si toute la
comptabilité temporelle disparaissait ou réintégrait le juge. Elle renforce
positivement l'accord sémantique sur une campagne plus grande; elle ne constitue
pas une porte chronométrique.

### Mutation-résistance : 0/3 mutants tués, plus une sonde de payload

Un réaudit CPU local au `HEAD` documentaire `84adbcc`, sur les blobs produit
inchangés de `e406e1f`, a transformé uniquement des copies temporaires hors
dépôt. La baseline Release GCC 13.3, binaire SHA-256
`9f1ef706ed0a9005a8a6fa20f56f3caa813d63f267aa0031211ec4c6f6157afc`,
passe 14/14 CTests directs en 3,20 s. Aucun des trois mutants de contrat n'est
toutefois tué, et une sonde supplémentaire observe le payload public divergent :

| mutant ou sonde | observation locale hors dépôt |
| --- | --- |
| annuler entièrement l'accumulation du timer source | 14/14 verts; `same_payload` affiche `source=0.000 s` |
| neutraliser le refus final des statuts non `kOk` | 14/14 verts; le cas 19 décidés / 1 refus devient retour 0 |
| remplacer stdout par des claims de payload public et juge certifié hors timer | 14/14 verts |
| comparer en plus le payload public de la gate `same_payload` | gate verte malgré 3 762 positions catalogue, 14 579 positions pool et 4 435 `ForestNode::source` différents |

Les sources temporaires des trois mutants actifs avaient respectivement les
SHA-256 `082af09340fba1ff753d02f816d567cc2852f6581ceb3ba13cc0fb66cf336636`,
`eef72324eb2a08bb84db6d01e35f2eb99fe3b8a7cf49e86968921e30ed4110b3` et
`2b6e72fc8391322f1aba0f31c2e64aa269dafd34a7d2073961dd0011573c362c`.
La sonde de payload avait le SHA-256
`e80a6ba88a02672c16d7bc757a8692f50057e97918fda6e8b41644d63eb0e164`.
Les fichiers vivaient sous `/tmp` et ne constituent pas des fixtures
permanentes; source, CMake et helper suivis sont restés bit à bit identiques.

Ce 0/3, complété par la sonde, ne retire pas les garanties réelles des portes : accord sémantique par
coquille, support, rang, niveau et membres, unicité des émissions, vingt forêts
abstraites, rejet cover+juge et refus final live des statuts non `kOk`. Il montre
précisément que les labels, le périmètre temporel, la symétrie des refus et les
octets publics ne font pas encore partie de leur contrat.
Les commentaires CMake restent eux-mêmes à corriger : lignes 320--323 ils
annoncent encore un « même POOL DE MEMBRES », et lignes 359--362 des chronos
couvrant exactement le même payload avec juge exclu. Les deux formulations sont
contredites par les sondes ci-dessus.

## « Mêmes nuages » et libellés de modes

`reference_seconds` est incrémenté lignes 700--703 avant le contrôle du statut
lignes 704--708. Un nuage refusé est donc facturé à la référence puis sauté par
la source. La commande suivante reproduit 20 tentatives référence, 19 nuages
source et malgré tout le libellé
« MEMES nuages » :

```sh
mhgp3v_direct_source --clouds 20 --points 5 --coord 4 --smax 3 --seed 13 --judge 1 --forest 2 --min-clouds 19 --min-emitted 1 --min-forest-nodes 1
```

Le code retourne ensuite volontairement 3 sur la garde finale
`refused_status != 0`; le défaut reproduit est le faux libellé temporel imprimé
**avant** ce refus fail-closed, pas un succès attendu de la commande. La
comparaison doit agréger uniquement les mêmes décisions `kOk` et publier à part
le coût des refus. Deux autres libellés sont ambigus : en mode mesure,
`reference=0` signifie « non exécutée », pas « chronométrée à zéro »; en mode
cover, aucun catalogue source n'est assemblé mais stdout annonce encore
`catalogue seul`. La commande autonome `--cover-only 1 --judge 0` échoue en
outre sur `candidats==C_q` après avoir sauté l'énumération, avant d'atteindre son
disclaimer final. La porte permanente teste seulement le rejet cover+juge.

## Le tableau de cinq tailles ne reçoit pas encore un croisement

Le tableau committé n'a ni commande et graine explicites par ligne, ni hash de
binaire et toolchain, ni répétitions, ni ordre alterné, ni dispersion, ni reçu
brut. Cinq répétitions exactes de la gate `n=40` ont donné des rapports de 2,71
à 4,90, contre 5,10 publié. Un voisinage de 1 près de `n=120` exige donc au
minimum répétitions et intervalles avant de localiser un croisement.
Quatre premières répétitions `n=120` non scellées avaient rendu 0,90, 0,99, 1,01
et 0,97. Elles signalaient la variabilité sans constituer un reçu.

### Deux campagnes CPU0/CPU1 transcrites à `n=120`

Deux campagnes post-audit, exécutées le 10 août 2026 à partir du `HEAD`
documentaire `3d5a763511530b241ae95c0c73e5915748a868cc`, conservent exactement
le binaire Release du prototype `e406e1f`, de SHA-256
`9f1ef706ed0a9005a8a6fa20f56f3caa813d63f267aa0031211ec4c6f6157afc`.
La source et CMake restent épinglés à
`d933c3aeb6314f12769f594d30af6734c696b09ce2e67de39af23dbd0ed15ed9`
et `739d21248a5fba575974aa3e40e8a0d7d4208b4a9c6710905ec4379cffa8fed7`.
Chaque commande a été répétée cinq fois séquentiellement :

```sh
taskset -c 0 /tmp/mhgp3v-chrono-release.fj1zEQ/mhgp3v_direct_source --clouds 4 --points 120 --coord 49 --smax 6 --seed 20260810 --judge 1 --forest 5 --min-clouds 4 --min-emitted 2000 --min-forest-nodes 2000
taskset -c 1 /tmp/mhgp3v-chrono-release.fj1zEQ/mhgp3v_direct_source --clouds 4 --points 120 --coord 49 --smax 6 --seed 20260810 --judge 1 --forest 5 --min-clouds 4 --min-emitted 2000 --min-forest-nodes 2000
```

| CPU0 | chrono affiché référence | fold référence | chrono affiché source | fold source | rapport affiché | retour |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 4,795 s | 0,157 s | 4,624 s | 0,152 s | 1,04 | 0 |
| 2 | 4,505 s | 0,137 s | 4,658 s | 0,138 s | 0,97 | 0 |
| 3 | 4,874 s | 0,137 s | 4,641 s | 0,141 s | 1,05 | 0 |
| 4 | 4,586 s | 0,145 s | 4,658 s | 0,136 s | 0,98 | 0 |
| 5 | 4,738 s | 0,144 s | 4,502 s | 0,144 s | 1,05 | 0 |

À partir des millisecondes affichées, les moyennes CPU0 valent 4,6996 s et
4,6166 s, soit un rapport des moyennes de 1,018; les médianes valent 4,738 s et
4,641 s. Les rapports affichés ont une médiane de 1,04 et couvrent 0,97 à 1,05;
leur signe change quatre fois.

| CPU1 | chrono affiché référence | fold référence | chrono affiché source | fold source | rapport affiché | retour |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 5,182 s | 0,149 s | 4,971 s | 0,140 s | 1,04 | 0 |
| 2 | 4,880 s | 0,132 s | 4,685 s | 0,145 s | 1,04 | 0 |
| 3 | 4,791 s | 0,175 s | 5,127 s | 0,171 s | 0,93 | 0 |
| 4 | 5,049 s | 0,126 s | 4,572 s | 0,139 s | 1,10 | 0 |
| 5 | 4,719 s | 0,137 s | 4,430 s | 0,135 s | 1,07 | 0 |

Les moyennes CPU1 valent 4,924 s et 4,757 s; les médianes 4,880 s et 4,685 s.
Les rapports affichés ont une moyenne de 1,036, une médiane de 1,04 et couvrent
0,93 à 1,10. Sur les dix répétitions, sept rapports sont au-dessus de 1 et trois
au-dessous; aucun n'est une mesure symétrique des deux générateurs.

Les dix sorties reproduisent la **même fixture** et les mêmes compteurs imprimés
— quatre nuages décidés, zéro refus, 20 324 émissions, 33 743 104 candidats,
2 609 704 refus fenêtre, vingt forêts, 22 639 nœuds, trente racines — avec zéro
forêt différente et zéro désaccord. Cela crédite la répétabilité locale des
agrégats imprimés et du verdict sémantique quotienté sur ce cas; cela n'ajoute
pas dix entrées indépendantes et ne compare toujours pas le payload public.

Le conteneur expose deux threads du même cœur Intel Xeon Platinum 8370C : fixer
un thread ne l'isole pas de son frère SMT. GCC 13.3 et CMake 4.3.4 étaient
actifs; la charge relevée après CPU0 valait 0,83/0,73/1,35. Il n'y avait ni
warmup explicite, ni alternance de l'ordre interne référence--source. Les valeurs
temporelles affichées à la milliseconde sont transcrites ci-dessus; les sorties
stdout verbose complètes et le binaire temporaire ne sont pas conservés comme
sidecars.

Ces campagnes documentent la variabilité temporelle locale; elles n'établissent
ni équivalence de performance, ni supériorité, ni croisement, ni loi d'échelle.
Elles ne corrigent ni l'asymétrie des timers ni l'absence de high-water commun.
La formulation « croisement étroit et solide vers 110 » reste donc non reçue.

### La différence des modes CLI n'isole pas le juge

Une troisième campagne, lancée depuis le `HEAD` documentaire
`9b8954b6141aceafa01b56c9a0e8cecf47e6fc98`, conserve `CPU0`, le même binaire
et la même fixture, mais alterne l'ordre de cinq couples
`mesure` / `juge --forest 0`. Les deux commandes ne diffèrent que par
`--judge 0` contre
`--judge 1`; toutes deux imposent `--forest 0`. Le mode mesure ne construit
aucune vérité et affirme explicitement ne juger aucune exactitude; le mode juge
construit d'abord `flat_catalogue`, puis chronomètre la source et son
différentiel.

```sh
taskset -c 0 /tmp/mhgp3v-chrono-release.fj1zEQ/mhgp3v_direct_source --clouds 4 --points 120 --coord 49 --smax 6 --seed 20260810 --min-clouds 4 --min-emitted 2000 --judge 0 --forest 0
taskset -c 0 /tmp/mhgp3v-chrono-release.fj1zEQ/mhgp3v_direct_source --clouds 4 --points 120 --coord 49 --smax 6 --seed 20260810 --min-clouds 4 --min-emitted 2000 --judge 1 --forest 0
```

| couple | ordre | source affichée, mesure | source affichée, juge | juge moins mesure | référence affichée, juge |
| ---: | :---: | ---: | ---: | ---: | ---: |
| 1 | M puis J | 4,164 s | 4,048 s | -0,116 s | 4,437 s |
| 2 | J puis M | 3,909 s | 4,010 s | +0,101 s | 4,569 s |
| 3 | M puis J | 3,951 s | 3,949 s | -0,002 s | 4,520 s |
| 4 | J puis M | 3,889 s | 4,130 s | +0,241 s | 4,379 s |
| 5 | M puis J | 4,253 s | 4,068 s | -0,185 s | 4,569 s |

Les moyennes source affichées valent 4,0332 s en mesure et 4,0410 s sous juge;
la différence moyenne n'est que +0,0078 s. Les différences pairées couvrent
-0,185 à +0,241 s et leur médiane vaut -0,002 s. Toutes les exécutions rendent
0, les mêmes compteurs communs émissions/candidats/fenêtre
`20 324 / 33 743 104 / 2 609 704`; les cinq jugements rendent zéro désaccord.

Le signe observé suit exactement la position dans cet échantillon : la seconde
commande affiche le chrono source inférieur dans les cinq couples. Sa moyenne
vaut 3,9726 s contre 4,1016 s pour la première, soit -0,1290 s ou environ
-3,15 %. Lorsque le juge passe second, `juge-mesure` vaut en moyenne -0,101 s;
lorsqu'il passe premier, la même différence vaut +0,171 s. Dans cet échantillon,
le quasi-zéro global de +0,0078 s est donc une annulation entre deux effets de
position observés de sens opposé dans un plan encore déséquilibré trois/deux,
pas une preuve d'équivalence ni d'un biais stable.

Ce quasi-zéro ne prouve surtout pas que le différentiel est gratuit. L'inspection
statique établit qu'il reste dans le timer juge. Mais, dans ce mode,
`flat_catalogue` parcourt d'abord les mêmes points et primitives et rend l'état
cache/allocateur initial différent avant le départ du timer source; en mode
mesure, ce prélude n'existe pas. Le gain systématique de la seconde position est
compatible avec un état plus chaud, sans en identifier la cause : fréquence,
prédicteurs, pages, caches et ordonnancement restent confondus. Soustraire les
deux exécutions CLI n'a donc aucune autorité pour chiffrer le juge.

La correction mesurable reste donc structurelle : fermer le timer source dans
le **même mode juge** avant tout différentiel, placer le juge dans un timer
séparé et recevoir cette identité par une porte dédiée. Les modes CLI existants
ne permettent pas de reconstruire a posteriori le coût pur du juge. Ce finding
ne remet pas en cause la soustraction cumulative interne du fold référence,
dont l'algèbre reste correcte.

Les exposants annoncés ne sont pas non plus l'ajustement des cinq lignes. Une
régression log--log ordinaire sur toutes les valeurs publiées donne environ
2,71 pour la source et 1,98 pour la référence; 3,2 et 1,6 correspondent aux
quatre lignes `n=40..120`, sous-intervalle non annoncé. Aucun de ces ajustements
ne démontre une loi de coût.

Enfin la condition de high-water commun n'est pas satisfaite : seuls le CSR et
la banque source sont publiés, sans mémoire totale source, référence ou juge.
Le tableau observe des temps ponctuels utiles, mais ne ferme ni la comparaison
mémoire, ni le budget 100 ms, ni une décision d'architecture.

## Aide constructive à Claude

1. fermer le timer source dès que le `Catalogue` public canonique et ses forêts
   sont produits, avant tout différentiel ou digest;
2. chronométrer la référence sur les seuls nuages également décidés, puis garder
   les refus dans un compteur et un temps séparés;
3. alterner l'ordre source/référence, répéter, publier médiane et dispersion,
   et graver commandes, machine, toolchain, binaire, digests et sorties brutes;
4. recevoir des octets comparables de bout en bout et distinguer explicitement
   payload public, quotient sémantique et travail du juge;
5. ajouter une porte ou un mutant qui prouve que le juge est hors timer et que
   l'identité cumulative multi-nuages reste vraie.

GCP non utilisé : aucune VM créée, démarrée, arrêtée ou modifiée pour cet audit.
