# Comparaison CPU appariée proposée au développeur

10 octobre 2026. Proposition, aucune mesure ni exécution native. Sources relues :
v11 gelée `ac081a06f`, v12 `4d06b7584`. Le [bilan existant](v11_v12_cpu_gpu/README.md)
reste descriptif : CPU K5 +13,1–17,0 % sur ng00–02. But : vérifier cet écart dans une
même session G4, avec des processus entrelacés ; ce protocole ne qualifie aucun gain.

## Deux bras principaux, un diagnostic facultatif

| Bras | Source et voie | Réglages |
| --- | --- | --- |
| H, référence de banc historique | v11 gelée, CPU | K5, W48, feuille16, masque802811, cache4Gio, budget8Gio |
| P, produit actuel | v12, CPU recouvert | K5, W48, feuille24, cache8Gio, budget par défaut illimité |
| A, diagnostic si nécessaire | même v12 que P | feuille16, cache4Gio, budget8Gio ; autres options identiques |

H est la sonde de la référence MESURE, pas le CLI v11. `git diff 733912e65 ac081a06f`
est vide sur `morsehgp3D_v11/src/` et `bench/full_probe.cpp` : un témoin v11 suffit
pour ces sources. Cela ne dispense pas de reconstruire et hacher les binaires.
Les masques v11 n'ont pas d'équivalent brut v12. A aligne certains paramètres,
pas les algorithmes ni la comptabilité du cache. P conserve les réglages du produit.

Arguments vérifiés dans les sources, chemins symboliques à substituer par le pilote ;
une seule trame par processus, puis les mêmes commandes pour ng01 et ng02 :

```text
V11_PROBE XYZ IDS DUMP 5 16 256 0 4294967295 8589934592 48 802811 10
V12_PROBE --trame=XYZ,IDS,ng00 --k=5 --leaf=24 --threads=48 --passes=10 --cache=8589934592 --recouvert --digest
V12_PROBE --trame=XYZ,IDS,ng00 --k=5 --leaf=16 --threads=48 --passes=10 --cache=4294967296 --budget=8589934592 --recouvert --digest
```

Ce sont des arguments pour les sondes, pas un pilote apparié déjà livré. En particulier,
`pilote_full.py` mesure une version : il n'organise pas ces couples H/P à lui seul.
Épingler les données par noms, tailles et empreintes ; aucun contenu sous licence dans le reçu.
La conformité v11/v12 se juge par le différentiel **sémantique** de la chaîne
(`tests/tower/mes_m0.py --chaine`), les formes rationnelles exportées pouvant différer.
Les empreintes natives restent à contrôler dans chaque bras ; leur seule différence
entre versions ne prouve pas une erreur mathématique.

## Plan proposé avant lancement

Une session gardée, mêmes compilateur/Release/u21 et environnement d'allocation déclaré,
sans ajout de `@tas`. H/P : cinq couples par trame, dix passes par processus ; ordre des
bras alterné et trames tournantes. Soit 30 processus, 300 passes. Ajouter cinq P′ sur
ng00 pour le contrôle A/A, position de P′ préfixée et équilibrée dans les blocs :
**35 processus, 350 passes** au total. Première passe publiée séparément ; neuf chaudes
par processus. Les positions exactes, délais et choix statistiques doivent figurer
au plan haché avant exécution. Ne pas lancer une seconde VM pour comparer les versions.

Unité statistique : médiane des neuf chaudes d'un processus. Apparier H/P par trame
et bloc ; publier chaque trame et la moyenne géométrique des ratios : moyenne des
log-ratios et intervalle bootstrap 95 % stratifié par trame, puis exponentiation
des deux. Rééchantillonner les cinq paires de processus dans chaque trame,
avec poids égal des trois trames. Règle et graine fixées avant lancement. Une borne basse du ratio >1
confirme une régression moyenne de ces **sondes** sur cette cohorte ; sinon la régression
n'est pas établie par ce critère. Ne pas compter les 135 passes chaudes d'un bras comme
135 répétitions indépendantes. A/A : moyenne géométrique des cinq ratios P′/P hors
[0,985 ; 1,015] rend le bilan non concluant ; publier aussi son intervalle. Ce contrôle
de dérive ne démontre pas une équivalence. Ce n'est pas une règle d'adoption d'un levier.

Archiver codes, stdout/stderr, identités, versions/arguments et hashes ELF avant/après ;
un refus ou une prise absente reste visible et ne devient pas une cellule omise du ratio.
Les sources et le juge du plan doivent être hachés ; arrêt ciblé G4 vérifié en clôture.

A ne se justifie que si l'écart H/P persiste et qu'on veut isoler l'ensemble des différences
de réglage. Prévoir ce critère dans le plan ; mesurer A avec de nouveaux H contemporains
et entrelacés, sans prolonger après coup jusqu'au résultat souhaité. Ce bras ne sépare
pas les effets feuille/cache/budget. **Aucun changement de défaut proposé** : feuille16
avait déjà un catalogue légèrement plus lent que24 dans la campagne I.

## Frontières et état entre passes

Le mur H démarre après `prepare_cloud`, avant l'index ; le mur P inclut les deux.
Deux diagnostics peuvent donc être calculés, par passe puis médianés :

- `P.wall − P.etapes_ns.P` contre `H.wall` : borne favorable à la v12 pour examiner
  l'explication par la préparation seule, pas une mesure à frontière identique.
- `P.wall − P.etapes_ns.P` contre `H.wall − H.index_ns` : résidu après préparation/index
  des deux côtés ; catalogue et tour restent propres à chaque moteur.

Le verdict principal conserve **tout le mur P**. Les diagnostics ne remplacent pas
le contrat FULL et ne prédisent pas le temps après suppression d'une étape.

Autre différence vérifiée : v11 ne sérialise que la dernière passe ; v12 appelle
`validate_forests` après **chaque** passe et, avec `--digest`, calcule aussi l'empreinte.
Ces travaux sont hors mur mais peuvent changer l'état mémoire/cache entre passes.
Enlever `--digest` ne retire pas la validation. Une frontière complète et une cadence
de validation exactement identiques exigeraient une adaptation explicite des sondes,
avec une nouvelle qualification ; les seules options ci-dessus ne les égalisent pas.
Aucune part de l'écart CPU n'est attribuée causalement à cette différence.

Sources : v11 `bench/full_probe.cpp` 200–256,337–377 et `bench/gpu_ab.py` ; v12
`bench/full_probe.cpp` 85–92,146–165,275–325,412–464,
`microbancs/mes_full/pilote_full.py` 171–187, `tests/tower/mes_m0.py` 21–25,
et `docs/MESURE.md` §5, règles 3/4/6. Lecture de source seulement.
