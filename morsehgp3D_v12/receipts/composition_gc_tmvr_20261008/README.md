# Raccord textuel G-c + TMVR — 8 octobre 2026

Proposition hors produit, base Git `8fe509df9`. Trois patchs épinglés :
G-c1 `499a2588…`, G-c2 `89d7b4f4…`, TMVR `6f0643ac…`.
`phase=exploration_v12_hors_registre`, `public_status=not_claimed`.
Aucune compilation, exécution native, campagne GCP ou modification du produit.

Après G-c1 puis G-c2, TMVR rencontre exactement deux conflits :
`src/tower/module.cmake` et `tests/mutants/tower.json`. Le raccord proposé
conserve les deux blocs de sources et fusionne les manifestes par identifiant,
en exigeant l'égalité intégrale des sept mutants communs.

Le lecteur reproduit dans un dossier temporaire :

1. extraction des seuls fichiers concernés depuis le commit, application des
   deux patchs G-c ; application indépendante de TMVR sur sa base de comparaison ;
2. vérification des deux conflits, puis application de TMVR en excluant ces
   deux fichiers et application de `raccord.patch` ;
3. contrôle des sources exclusives, conservation des lignes G-c et des ajouts
   TMVR dans `tower.hpp` et `tests.cmake`, présence des sources et des portes.

Résultat : **19 sources de module**, **27 mutants** (7 communs, 11 propres
à G-c, 9 propres à TMVR), **28 remplacements uniques** dans les sources de
la combinaison, y compris les deux actions successives d'un même mutant.
Les 14 noms de portes visés correspondent aux groupes unitaires déclarés.
Cela ne configure pas CMake et ne démontre aucune mise à mort effective.
Les anciennes attentes d'empreinte G remplacées volontairement par G-c1
ne sont pas réintroduites depuis la base TMVR.

Les résultats reproduisent indépendamment le raccord préparé par l'auditeur
principal, sans modifier son répertoire : module `12219c37…`, manifeste
`75fa8a67…`. L'empreinte agrégée des 48 fichiers patchés et celles des trois
dépendances inchangées relues sont conservées dans `capture.json`.

Ordre d'application de `raccord.patch` : **après G-c1/G-c2 et TMVR appliqué
avec exclusions** de `morsehgp3D_v12/src/tower/module.cmake` et
`morsehgp3D_v12/tests/mutants/tower.json`. Le patch porte uniquement sur ces
deux fichiers et part de leur état G-c. Il ne faut pas remplacer le manifeste
de 27 mutants par celui de 16 mutants du prototype TMVR.

```sh
python3 -B -S check.py --repo DEPOT --gc DOSSIER_GC --tmvr DOSSIER_TMVR
python3 -O -B -S check.py --repo DEPOT --gc DOSSIER_GC --tmvr DOSSIER_TMVR
```

Résultats identiques au champ `result` de `capture.json`, patchs vérifiés
avant/après. La composition reste **non qualifiée nativement** : ni les portes
de G-c seul, ni les comparaisons v11 de repo5 ne suffisent à qualifier leur
union. Construire et jouer les portes, mutants, oracles et chaînes complètes
sur la combinaison livrée demeure nécessaire. Aucun gain chronométrique n'est
déduit de cette application textuelle.
