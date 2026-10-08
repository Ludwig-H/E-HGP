# CST-0241 : porte native de terminaison livrée

La sous-obligation native est étayée au commit **bdfca8fb198e6711626c6506b816a2a656315c83** : témoin corrigé passant, ancienne règle tuée par l'assertion attendue, sources et cible instrumentée raccordées. Cela complète le [modèle et contre-exemple](../a_terminaison/README.md), le [correctif livré](../a_terminaison_raccord/README.md) et le [protocole de porte](../a_terminaison_porte/README.md). L'audit a seulement relu Git, objets, cartes et journaux locaux : **aucune compilation, exécution native ou action GCP**.

## Corps réellement jugé

Les **313 fichiers** de `src/`, `tests/`, `cmake/` et `CMakeLists.txt` du travail `w_term72` et du témoin des mutants sont identiques aux objets Git livrés. Les six fichiers de la porte sont épinglés, ainsi que le lanceur de mutants et `run_expect.cmake`.

Le test ouvre une vraie Session d'un site, K1, Pool2, puis termine cette région seul avec crochets désarmés. La coupe finale vérifie tous les états Done, G épuisé et compteur nul. Deux fils du harnais imposent ensuite : A retiré1→0, B annoncé0→1, A relâché. La version corrigée retourne ; l'ancienne règle atteint l'attente. **B est libéré puis les deux fils joints avant l'assertion**, sans attente temporisée utilisée comme verdict. Les échecs de création d'un fil sont traités après nettoyage des fils effectivement lancés.

La cible compile `pipeline_run.cpp` avec `MHGP12_REGION_HOOKS`. L'archive produit est construite sans cette macro. Le marqueur `region_hooks_build` est contrôlé par le test ; la carte existante nomme l'objet de la cible, pas `libmhgp12.a(pipeline_run.cpp.o)`. Le lecteur livré, réutilisé sur cette carte, rend conforme et constate l'absence d'octets `region_hook` dans l'archive. Une carte textuelle seule n'authentifie pas un ELF arbitraire : commande de lien, flags, sources, objets et marqueur natif complètent ici sa portée.

## Journaux primaires

| Preuve relue | Résultat |
|---|---|
| Porte Release/u21, LastTest09:49 | `attente_A=0`, A/B revenus, compteur0 ; **11 contrôles**, conforme |
| Témoin de la campagne, LastTest09:44 | Même positif, **11 contrôles**, conforme |
| Mutant natif, LastTest09:46 | `attente_A=1`, A/B revenus, compteur0 ; assertion `waits_a==0` ligne165 en échec, **code1** |
| Trois sorties supplémentaires du mutant | Trois fois la même assertion/code1, aucun timeout utilisé |
| CTest ciblé09:49 | **12 Passed /12**, y compris porte native, inventaire, carte, juge de carte et variantes Python `-O` |

Le rapport `mut_term.json` déclare deux mutants tués par code. Son empreinte de sources **a860f74f…** est recalculée exactement depuis le témoin conservé. Le mutant natif ne diffère de ce témoin que par les **deux substitutions du manifeste livré**, retirant la capture de `last` et restaurant la relecture tardive du compteur. Aucun autre changement de source ne porte son échec. Le travail courant a depuis changé `bench/full_probe.cpp` et deux documents, hors des313 fichiers de cette cible ; ce delta n'est pas assimilé à la capture de campagne.

Les deux manifestes d'objets avant/après contiennent les **mêmes51 SHA256**. Chaque objet actuel est rehaché et chaque membre de `libmhgp12.a` lui correspond. Les objets originaux « avant » ne sont plus conservés à côté de leur manifeste : la comparaison à la base72 repose donc sur ce manifeste primaire, pas sur une reconstruction indépendante. Aucun binaire ni archive complète n'est dupliqué dans le reçu.

## TSan et limites

Les cinq fichiers `tsan_region_1..5.log` portent chacun le positif11 contrôles, sans avertissement. Le build conservé RelWithDebInfo/u21/CUDA OFF instrumente **cible et bibliothèque** avec `-fsanitize=thread`, le lien également ; l'ELF contient `__tsan_init` et sa dépendance `libtsan.so`. Ces éléments corroborent l'annonce TSan, mais **les cinq logs ne portent ni code externe ni hash ELF par exécution**. Ils ne constituent pas ici cinq campagnes à provenance complète. Cette réserve ne porte pas sur le témoin Release et son mutant causal ci-dessus.

Les annonces **730 portes globales** et **20 répétitions** ne sont pas reprises comme acquises par ce reçu : ses primaires retenues attestent12 portes ciblées. La clôture proposée concerne le défaut de terminaison0241 et sa porte déterministe CPU/u21 ; elle ne prouve pas toute la concurrence C++, une qualification GPU, un budget ou un contrat temps FULL. Le test de la coupe finale isole précisément la décision de retrait ; il ne remplace pas les preuves de dépendances et de publication du travail.

## Relecture

```sh
python3 -B check.py --repo DEPOT --artifacts DOSSIER_A6
python3 -B -O check.py --repo DEPOT --artifacts DOSSIER_A6
```

Les deux sorties sont identiques à [results.json](results.json). [capture.json](capture.json) garde les tailles/hashes et chemins relatifs ; seules de petites copies de journaux/flags sont conservées hors Git. Le lecteur ne lance jamais les exécutables hachés. [SHA256SUMS](SHA256SUMS) couvre le lot. Contrelecture indépendante de la portée du juge de carte par l'auditeur feuilles, sans exécution supplémentaire.
