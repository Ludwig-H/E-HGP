# Repo5 : profil 24 clos, profil 32 interrompu

Lecture des traces locales le 8 octobre 2026, sans nouvelle exécution native.
`phase=exploration_v12_hors_registre`, `backend=cpu_reference`, `objet=full_pi0`,
`quantification=quantized_u21_input_only`, `public_status=not_claimed`.
Cette capture complète [le reçu u21 de repo5](../audit_tmv_repo5_u21_20261008/README.md)
et ne réécrit aucun reçu publié.

Les sources sont celles de **repo5**, base `c903774b1`, arbre de 376 fichiers
`848c7a0b771b2e69a8fb9e954b8f1c4e756f1bae3365335a16ab8cab421332d9`, inchangé
avant/après lecture et identique au lot u21. Les caches désignent ce répertoire,
Release, tous les modules et CUDA OFF, avec `MHGP12_COORD_BITS=24` ou `32`.
Les logs, caches et binaires sont hachés, sans reconstruire leur chaîne de compilation.

| Profil | Construction et suite rapide | Chaîne CPU C/G → T/M/V/R | MES-M0 par vidages |
|---|---|---|---|
| 24 | codes 0 ; 681 sélectionnés, 680 Passed, sentinelle LiDAR sautée | 9 cas clos en 3 lots, codes 0 | ng00 K5, graines puis v12, codes 0 |
| 32 | codes 0 ; 681 sélectionnés, 680 Passed, même sentinelle sautée | interrompue, aucun lot clos | non lancé |

Le profil 24 finit à 01:14:17 UTC. La suite rapide 32 finit à 01:21:42 ; le
rapport du développeur déclare l'arrêt du groupe de processus de repo5 à
**01:22:17**, pour passer à la composition repo6. Les trois journaux de chaîne
32 sont vides, aucun code de clôture correspondant n'existe, ni journal MES-M0
32. L'arrêt est une déclaration recoupée avec ces traces ; l'auditeur n'a pas
rejoué le signal. Les chaînes 32 ne sont donc plus « en cours » et ne sont pas
qualifiées par cette capture.

Les neuf cas de chaîne 24 sont les uniformes 8k/16k/32k à K5 et ng00/01/02 à
K5 et K10. Le pilote compare les empreintes sémantiques FULL de la v11, puis
les sorties et compteurs entre 1 et 8 fils dans le profil. MES-M0 ng00 K5
compare ses deux modes de cibles séparément ; ces deux appels n'activent pas
JUG-EMST. La sentinelle CTest sautée n'annule pas les chaînes explicitement
lancées avec leur dossier de données, mais reste un test non joué.

Le conducteur conserve les mêmes chemins d'entrée pour les profils ;
`tower_chain.cpp` lit les entiers u32 puis prépare le nuage selon `CoordWidth()`.
Ces chaînes larges réutilisent les coordonnées déjà déclarées au profil u21 :
elles ne démontrent ni une nouvelle quantification ni des coordonnées LiDAR
plus grandes que u21. Aucun payload n'a été ouvert ou rehaché par l'auditeur :
l'identité d'octets des entrées n'est pas recertifiée. L'identité sémantique
interprofil se distingue de l'identité d'octets à nombre de fils variable,
vérifiée à l'intérieur du même profil. Les portes numériques synthétiques de
la suite rapide conservent leur propre portée.

`etape_final5.sh` enregistre `$?` immédiatement comme argument de `step`, avant
le calcul de la date ; pour chaque chaîne, `$?` précède également `$(date ...)`
dans l'expansion. Le défaut de code écrasé par la date observé sur un autre
conducteur ne s'applique pas ici. Limites : les configurations CMake n'ont pas
de code enregistré séparément, le script ne s'arrête pas au premier échec et
sa sortie finale n'est pas une porte globale. Le lecteur exige donc chaque
code nommé et le bilan primaire associé, ainsi que les caches et journaux figés.

Le fichier partagé `patch_tour_TMV.diff` a changé après la campagne : original
repo5 `6f0643ac…`, observation intermédiaire `ef27e03f…`, puis livraison repo6
`b3c78ae3…`. Ces anciens octets ne sont pas recopiés ici ; le lecteur ne fonde
aucune qualification repo5 sur ce fichier partagé mutable. Les pins complets
observés sont conservés dans `capture.json`. Les reçus historiques qui exigeaient
le patch original refusent normalement cette dérive, sans effacer leurs preuves.

Repo6 part de `a5e0dbc77` et compose G-c et TMVR : module `12219c37…` et manifeste
`75fa8a67…` correspondent au [raccord proposé](../composition_gc_tmvr_20261008/README.md)
(27 mutants). Il ajoute la correction du comparateur de naissances et réduit
explicitement le commentaire du validateur, sans ajouter les gardes de
[l'historique altéré](../audit_foret_validation_20261008/README.md). Les six pins
repo6 du champ `successor_observation_only` servent uniquement à identifier
cette observation ; ils ne sont pas les sources des binaires qualifiés ici,
ni une admission de la nouvelle campagne. Le lecteur reproduit les résultats
repo5, sans exiger que le futur patch ou repo6 reste immuable.

Relecture depuis ce dossier :

```sh
python check.py --prototype DOSSIER_TMVR
python -O check.py --prototype DOSSIER_TMVR
```

Les sorties doivent être identiques au champ `result` de `capture.json`.
Le lecteur réutilise seulement la fonction d'empreinte d'arbre du
[reçu de traces antérieur](../audit_tmv_traces_20261007/README.md), elle-même
épinglée. Aucun moteur, build, GCP, GPU ou nouveau chrono n'est exécuté ; aucune
qualification FULL 100 ms ni qualification de la composition G-c/TMVR n'en découle.
