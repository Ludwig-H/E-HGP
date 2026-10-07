# Juge G intégré : contre-rejeu et erratum de notre proposition

7 octobre 2026. Juge publié dans `28cf75cd18` (commit complet dans `verification.json`),
SHA-256 `650c63a13a9af242a4ea6f033a9d944f0416c357683f40f57e04942606ef3ce9`.
La copie a été figée alors que les fichiers étaient encore modifiés sur `60c041aef1` ;
le corps du juge publié est exactement celui rejoué. Aucun fichier de main modifié par
cet audit. **JSON uniquement : aucun moteur, build, 8k/16k/32k natif, CUDA ou GCP.**

## Intégration et contrôle causal

Les quinze témoins de la [proposition antérieure](../g_juge_proposition/README.md) sont
rejoués en Python normal et `-O`, en ajoutant la ligne finale effectivement émise par
`bench/tower_probe.cpp:223` : `exit/ok/none/order=0`. Résultats identiques : trois témoins
conformes acceptés, onze sorties/usages invalides refusés (code 2), suffixe SHA-256
différent entre fils détecté (code 1). La déduplication de deux sites pour N=10 est
conservée. Les ordres tronqués/dupliqués ne passent plus. Les valeurs JSON sont
fabriquées pour le juge et ne constituent aucune sortie géométrique ou prise native.

**Erratum explicite de notre reçu f7a.** Notre ancien double JSON s’arrêtait au digest ;
il omettait `exit`. La mention « format complet du producteur » dans ce reçu était donc
incorrecte. Le corps proposé `f7a30188…` accepte cet ancien format incomplet, puis refuse
le même JSON dès qu’on lui ajoute la vraie ligne finale. Ces deux branches sont rejouées
causalement ici. Le développeur a corrigé cette erreur d’interface : l’intégration accepte
le format avec `exit` et refuse le même format sans cette ligne. L’ancienne archive reste
inchangée ; sa proposition ne doit plus être présentée comme directement intégrable.

## Résidu de type limité et correction proposée

La comparaison du dictionnaire final à `order:0` accepte aussi `False` et `0.0` par
égalité Python. Deux témoins le reproduisent, code 0 ; **ce résidu ne rouvre pas le
problème de couverture des ordres corrigé ci-dessus**. Le patch d’une ligne
[exit_order_type.patch](exit_order_type.patch) ajoute `type(end.get('order')) is int`.
Sur une copie temporaire, ces deux témoins sont refusés (code 2) et les quinze cas
précédents gardent exactement code/stdout/stderr. Corps proposé :
`c88be31c7bdb1c24154eeb98c353163bc54c590664e71f5d9ab8475d00abb9b9`.
Patch non appliqué au produit ; aucune clôture générale de CST-0018 n’est déduite.

## Portée des références et rejeu

Le juge exige et compare les **64 caractères** du digest entre fils. Sa ligne humaine
et les références CTest d’échelle restent à **16 caractères**. Aucun passage à une
référence historique complète n’est annoncé ni inventé. `tests.cmake` a évolué pendant
la capture sur la nouvelle porte JSON conforme ; les lignes d’échelle sont inchangées.
Le témoin utilise le snapshot figé, et `verification.json` distingue les deux hashes.
Aucun CTest natif n’a été exécuté. Le schéma des compteurs est confronté au C++ épinglé.

Sources figées, pins, résultats et fermeture sont conservés sans chemins personnels.
Les 37 exécutions de juges par mode comprennent les quinze cas intégrés, les quinze
après patch de type, trois contrôles de notre erratum et deux résidus avant/après.

```sh
python3 -B -S check.py > /tmp/g-integration-normal.json
python3 -B -S -O check.py > /tmp/g-integration-optimized.json
cmp /tmp/g-integration-normal.json /tmp/g-integration-optimized.json
sha256sum -c SHA256SUMS
```

`phase=exploration_v12_hors_registre`, `backend=cpu_reference`, `objet=full_pi0`,
`quantification=quantized_u21_input_only`, `public_status=not_claimed`.
