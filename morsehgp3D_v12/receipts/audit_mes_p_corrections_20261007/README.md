# Contre-audit MES-P — correction CST-0238, 7 octobre 2026

**Séparation des pentes corrigée ; cohorte commune encore incomplète.** Capture non commise sur la base
`58761b36d69c83ff1893049cdfdc4b0061a34bdc`, sources stables avant/après copie dans `snapshot/` et empreintes dans
`sources.json`. Il s'agit d'une observation du correctif en cours, pas d'une qualification de version publiée.

Cadre : `phase=exploration_v12_hors_registre`, `backend=cpu_reference`, `objet=full_pi0`,
`quantification=quantized_u21_input_only`, `public_status=not_claimed`. Les temps ci-dessous sont **inventés** pour
tester le lecteur ; aucune performance HGP n'est mesurée. Aucun calcul natif, build, GCP ni jeu sous licence.

## Preuve bornée

`check.py` appelle réellement le CLI de l'analyseur sur cinq petits JSON. Deux nuages nommés `bout_n100` et
`bout_n200`, de 100/200 sites, reçoivent les temps `t_f(n)=b_f n`, avec `b_1=100`, `b_4=60`, `b_48=10` µs/site.
Résultats identiques en Python normal et `-O` ; captures détaillées dans `normal.json`.

| Cas | Résultat |
| --- | --- |
| Six succès, K5, 1/4/48 fils | Trois droites étiquetées, pentes 100/60/10 ; cohorte commune de deux nuages |
| Ajout K10 et familles synthétiques | Aucun mélange K/fils/famille ; quatre lignes synthétiques distinctes |
| Seul n200 à 1 fil expire | Cohorte commune de n100 seul ; écartés 0/1/1 ; aucune pente sur une taille unique |
| Les deux prises à 1 fil expirent | **Défaut :** table limitée à 4/48 fils, annonçant deux nuages communs |
| Les six prises expirent | **Défaut :** aucune table de cohorte malgré trois régimes joués |

Les échecs restent affichés avec K et nombre de fils. La première reproduction publiée de CST-0238 donnait une
pente artificielle de 56,67 µs/site ; cette agrégation est maintenant éliminée. Le résidu relève du même constat.

## Cause et correction minimale

Pour un K donné, soit `F` l'ensemble des nombres de fils **joués**, et `S_f` les noms des nuages réels **réussis** à
`f` fils. La cohorte demandée est `C = intersection(S_f pour f dans F)`. Si un régime échoue entièrement, son ensemble
est vide, donc `C` est vide. Supprimer ce régime change la comparaison et conditionne le résultat à sa réussite.

Dans la capture, `main` appelle `cohort_table(good, k, out)` et reconstruit aussi les K depuis `good`. Le correctif
minimal consiste à prendre les K et `F` depuis toutes les prises, puis à intersecter uniquement les succès pour
calculer `C`. Afficher les trois régimes, même lorsque `C` est vide ; conserver le tableau d'échecs.

## Pilote et périmètre

Trois doubles de processus vérifient les branches succès, code 3 et expiration, chacune avec trois lignes JSON de
passes déjà rendues : la médiane chaude vaut 1,5 ms seulement au succès ; échec/expiration donnent `chaud=null`.
`wall_ns` a priorité sur les sous-étages. Les doubles vérifient `start_new_session=True`, `killpg(pid, SIGKILL)` puis
collecte après délai. C'est une vérification du protocole d'appel, pas une expérience système de terminaison de
descendants. Une sélection entièrement exclue retourne 2 avant toute construction.

Le champ `froid` est le temps de la première passe FULL. Dans la sonde v11, `full_pass` commence son chronomètre après
la préparation du nuage et la création du pool ; le pool est réutilisé entre passes. `wall_ns` couvre index, domaine
et forêt, avant export. Ainsi ni `froid` ni la médiane chaude ne mesurent toute la latence d'un processus neuf ; le
coût fixe ajusté ne prouve pas un coût de création du pool. Les petits nuages restent le périmètre de MES-P.

## Rejeu

Depuis ce dossier :

```sh
python3 -B check.py
python3 -B -O check.py
python3 -B check.py --require-common
```

Les deux premières commandes retournent 0 en publiant explicitement le booléen de contrat faux. La troisième
retourne 1 tant que le résidu demeure. Pour contre-juger une nouvelle copie stable, ajouter `--source-dir CHEMIN`.
Le script n'emploie pas `assert`, et vérifie les empreintes des deux sources avant/après exécution. Son premier
essai a été ajusté pour accepter `-0.00` comme affichage d'une ordonnée à l'origine nulle ; aucune source produit
n'a été modifiée. CST-0238 ne peut être clos sur cette capture seule.
