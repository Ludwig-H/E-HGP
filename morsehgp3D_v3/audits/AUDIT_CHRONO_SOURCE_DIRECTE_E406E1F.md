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
`catalogue seul`.

## Le tableau de cinq tailles ne reçoit pas encore un croisement

Le tableau committé n'a ni commande et graine explicites par ligne, ni hash de
binaire et toolchain, ni répétitions, ni ordre alterné, ni dispersion, ni reçu
brut. Cinq répétitions exactes de la gate `n=40` ont donné des rapports de 2,71
à 4,90, contre 5,10 publié. Un voisinage de 1 près de `n=120` exige donc au
minimum répétitions et intervalles avant de localiser un croisement.
Quatre répétitions `n=120` sur le même CPU ont rendu 0,90, 0,99, 1,01 et 0,97 :
la source perd, gagne ou reste indiscernable selon l'exécution. La formulation
« croisement étroit et solide vers 110 » n'est pas reçue.
Ces répétitions sont des observations locales : leurs commandes et sorties
brutes n'ont pas été conservées. Elles signalent la variabilité, mais ne
constituent pas à leur tour un benchmark scellé.

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
