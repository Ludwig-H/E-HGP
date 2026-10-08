# Gc rebasé : nouvelles portes u21, 8 octobre 2026

Lecture indépendante, CPU local, hors registre, `not_claimed`. Aucun moteur,
CTest, build ou GCP lancé ; aucun fichier du canal audits modifié.

Le prototype **45976be8ddc489f14494937b13e43d0ca5fa0a55** (`git3`, base main781)
dispose maintenant de preuves propres au rebasage sur le catalogue A/B livré :

| Lot | Trace close | Résultat |
| --- | --- | --- |
| Construction complète Release/u21 | `build_final`, 00:13:52 | code 0, aucun avertissement, CUDA et TSan OFF |
| Suite rapide | `runs/final_fast.log`, 00:19:57 | **675 réussites, aucun saut**, code 0 |
| Portes LiDAR | `runs/final_lidar.log`, 00:21:36 | **6 réussites, aucun saut**, code 0 |

La sentinelle LiDAR est exécutée dans les deux lots : 681 exécutions, **680 portes
distinctes**. Les six portes LiDAR comprennent cette sentinelle, les trois
catalogues ng00–02/K5, Euler sur ng00, et le déterminisme G de ng00/W1–W8.
La dernière porte conserve l'empreinte `e5a81154fb1b15f1`. La suite rapide comprend
l'oracle G et ses 21 225 cibles attendues, les trois tailles 8k/16k/32k en
normal/−O, et les cinq groupes de l'index (dont `weak_key_resolution`).
Ce sont des portes de correction CPU ; aucun chrono contractuel n'en découle.

Le build pointe vers `repo3`. Ses 359 fichiers de qualification sont identiques
au commit épinglé ; hash global recalculé :
`334101284e02c3519bc9d2f8b94ae89a2ebc8c5ff2386916f00df591cf8ad4e1`.
Le lecteur vérifie cet arbre et 13 hashes de traces/build avant et après lecture.
Le pilote de vérification est encore actif : seul son préfixe clos de six lignes
(codes de construction, suite rapide et LiDAR) est épinglé. Aucun brut recopié.

À la première lecture, 00:25 UTC, la campagne `mutants_tower_final` n'avait pas
de rapport final et son log était vide. `mutants_tower_final2.json` est un ancien
lot de **deux** mutants sur l'arbre 62726897, clos à 00:04 : il ne certifie pas les
18 mutants de repo3. Les profils 24/32 et TSan cités comme précédents dans le
RAPPORT restent leurs anciennes preuves ; aucun transfert automatique ici.

Deux corrections restent absentes du pin 45976be : `find_support` vérifie toujours
le premier identifiant seulement ; `--processus` vaut encore **8 par défaut**
alors que l'admission exige 10. La garde proposée dans `gc_support_domain_delta`
et le défaut CLI 10 restent donc à intégrer. Les appels explicites à 10 ne sont
pas concernés par ce défaut de valeur par défaut. Main observée à 59bf8afbf
n'avait pas encore intégré T2-c.

Rejeu de lecture :

```sh
python3 check.py --scratch /chemin/v12_tour_Gc
python3 -O check.py --scratch /chemin/v12_tour_Gc
```

Résultats normal/−O identiques dans `capture.json`. Le lecteur est lié aux
artefacts externes épinglés ; un changement de source ou de build est refusé.
