# Cellules et localisation : garde locale q4 / globale q3

Capsule indépendante v11, lecture statique et calcul rationnel autonome ; aucune exécution native, compilation, GCP ou modification produit. Aucun défaut mathématique nouveau trouvé. Le résultat utile est une fixture à ajouter avant de futures optimisations du support global.

## Sources et évolution

Les captures initiales ont précédé leur lecture à **20:00:07 UTC**, DEV `952df06b751e041c81214e288801df32da6663bd` : `cells`, `locate`, `canonical` étaient alors WIP non suivis. Les trois dépendances du juge ont été capturées séparément avant lecture. À 20:12:02 UTC, cinq deltas de tests/documentation ont été capturés dans `sources_update/`. Les dix-huit dernières sources sélectionnées sont byte-identiques à leur publication **`9c883b93f1c99b299d2de218af83c5a3d0e8ab9f`** (`PUBLISHED_BINDINGS.json`). Le produit `cells/locate/canonical` est inchangé entre les deux captures. Ces rapprochements ne qualifient pas les portes natives. À la fermeture, `tests/tower/fraction_oracle.py` a encore évolué en LIVE : cette nouvelle version est préservée dans `sources_after/`, sans étendre la relecture ni remplacer le modèle publié9c capturé avant lecture.

## Fixture proposée, géométrie exacte

Dans l'ordre Morton des sites :

| SiteIdx | XYZ | clé Morton |
| --- | --- | --- |
| 0 | (5,5,0) | 195 |
| 1 | (2,1,5) | 270 |
| 2 | (10,5,5) | 910 |
| 3 | (2,9,5) | 1294 |
| 4 | (5,9,8) | 3139 |

Tous sont sur la sphère de centre `(5,5,5)` et niveau β=25. La partie **F=(0,1,2,4), k=K=4** a un support local strict tétraédrique, de poids `(3/16,5/16,3/16,5/16)`. Puisque la sphère englobe F et que son centre est dans l'enveloppe convexe de ses sites de frontière avec poids positifs, elle est la MEB unique de F.

Sur la coquille globale U des cinq sites, il n'existe aucun support q2 ; l'unique triangle strict est **S*=(1,2,3)**, avec poids `(5/16,3/8,5/16)`. Ainsi qmin global vaut 3 et S* **exclut le premier site de U**. Le minimum global d'arité doit précéder le choix lexicographique ; le lemme d'ancrage au premier site valable sous qmin=4 ne s'étend pas à qmin=3. Le code actuel boucle correctement sur tous les triplets.

Attendu proposé pour `locate_part` : la MEB garde son arité de présentation locale 4 ; sa clé locale manque le lookup. Le census du même nuage rend `Complete`, p=0 et m=5 ; son payload I/U vaut **20 octets** (cinq SiteIdx u32), sans prétendre mesurer une durée. Le support global est `(1,2,3,None)`, le BallIdx est présent dans Cat4 et le niveau exact reste 25. Cette garde peut refuser causalement un futur mutant qui limite les triplets globaux au premier site de U ; aucun tel mutant n'a été compilé ici.

`check.py` utilise seulement Fraction et des systèmes de Gram/Gauss autonomes. Il énumère les sous-supports locaux/globaux, vérifie l'absence d'antipodes, l'unicité du triangle strict, les poids, les clés Morton et les 120 permutations d'entrée. Ce dernier contrôle porte sur la normalisation géométrique, sans exécuter le Cloud natif.

## Raccords actuels : ce qui est pris en compte

La garde **hit catalogue avec p≥k** initialement absente a été ajoutée dans le delta publié `locate_test.cpp` : X=(0,2,4,6), K=3, k=2, F=(0,6) rend un hit complet p=2 sans census ni allocation ; MEB(I) descend strictement. La documentation avertit maintenant qu'un hit `Complete` ne signifie pas p<k. Voir la [preuve et les obligations FULL de la capsule16](../full_locator_contract_review_16/README.md) ; elle n'est pas recopiée ici.

Le delta ajoute aussi la ligne de treize sites : p=11, qmin=m=2, ordre12, deux traces strictes de cardinal12 sans padding. Les cellules n'interprètent pas leur nombre de traces comme un nombre de morceaux. La garde octaédrique « douze traces / un morceau » et le coût combinatoire sont déjà dans la [capsule16](../full_locator_contract_review_16/README.md).

`cells.cpp` applique T2 avec **MEB(A)<λ**, uniquement pour classer la séparabilité ; chaque trace exportée est bien **I∪A**. Le contrôle autonome rappelle ce risque aval sur la fixture régulière déjà préparée X=(0,4,5,11) : MEB(A) a β=0 pour chacun des deux singletons de coquille, mais les MEB des traces complètes ont β=25/4 et 49/4, toutes deux <121/4. Toute future descente recalcule donc la MEB de la trace complète.

## Indépendance et portée des tests relus

Le juge `cells_oracle.py` décide si le centre appartient à conv(A) par faisabilité **fermée** de témoins barycentriques de cardinal≤4 (Carathéodory) ; il rejette également les contacts au centre. Cette décision ne dépend pas de la MEB(A) du produit. Le modèle MEB est partagé pour les seuls compteurs de travail. Le catalogue attendu utilise Gram/Fraction plutôt que les prédicats polynomiaux natifs. Les tests `cells_model_test.py` construisent leurs payloads avec ce même oracle : ils contrôlent sa sensibilité aux corruptions, pas une troisième construction indépendante.

Les mutants préparés distinguent résultats géométriques (contact, qmin, intérieurs omis, exhaustivité, support global), comptabilité (deux passes) et capacités (élargissement avant produit binomial). Les portes nommées existent. La lecture ne revendique ni leur exécution ni leur mort native. Les refus et budgets sont testés comme transactions locales ; aucune borne de travail global ou coût LiDAR n'est déduite de la construction de cellules.

FULL n'est pas porté dans ces sources : la résolution de toutes les traces, leur déduplication par composante globale, les mémos datés et l'atomicité avant/après plateau demeurent des obligations futures, pas des défauts de cette brique. Cette capsule n'évalue ni projection frontière ni qualité statistique.

## Rejeu et fermeture

Depuis ce dossier : `python3 check.py` et `python3 -O check.py`. Les deux sorties exactes doivent être identiques. `COMMANDS.json` conserve codes de retour, sorties et version Python ; `SOURCE_AFTER_FINAL.json` constate les sources LIVE à la fermeture. `SHA256SUMS` inventorie tous les fichiers de la capsule sauf lui-même, y compris ses métadonnées. Aucun reçu clos n'est réécrit.
