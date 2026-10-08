# Apparié : fermer l'identité du fichier de la sonde

8 octobre 2026, Git publié `0e62a7232`, pilote `fe22a681…`, lanceur partagé `7d7f0661…`.
Le pilote hache la sonde une fois avant les identités. Chaque prise appelle ensuite le même chemin,
mais ni `play`, ni `play_session_info`, ni `banc_full.run` ne rehachent son fichier. Le rapport et le juge
ne prouvent donc pas la fermeture du « même binaire ». Aucun incident de campagne réelle n'est allégué.

Deux témoins utilisent uniquement la sonde Python simulée du test officiel :

| Témoin | Publié | Proposition |
| --- | --- | --- |
| commentaire ajouté après le premier appel d'identité, puis 63 appels sous les nouveaux octets | jugement conservé | refus |
| fichier supprimé après le 64e appel, dernier appel de Session informative | jugement conservé | refus |

Tous les journaux restent conformes, les informations n'ont aucun refus ; l'écart concerne bien la
preuve des octets de la sonde. Le positif conserve son jugement. Retirer le hash final du rapport
positif corrigé entraîne aussi un refus au rejeu.

`proposition.patch` est un correctif ciblé, autonome sur 0e62 : enregistrer `sonde_fin_sha256`
**après le dernier appel, information optionnelle incluse**, puis juger. `judge` exige, lorsqu'il relit
les bruts, un SHA initial valide et un SHA final égal ; absence, suppression ou changement refusent.
L'auto-test purement synthétique sans dossier de bruts conserve son rôle. Le régime d'essai affiche
toujours `essai` ; les résultats ci-dessus concernent son `verdict_calcule`. Les nouveaux contrôles
ne changent ni les statistiques ni les seuils d'adoption.

Cette preuve compare deux relevés. Elle ne certifie pas l'immuabilité à chaque instant : une substitution
puis restauration entre relevés ne serait pas détectée. Elle ne prouve pas non plus, à elle seule,
le lien entre source et ELF. Le patch reste proposé ; son insertion dans `judge` est à réunir avec
les [correctifs d'identité et de cohorte](../apparie_livraison/README.md), qui portent sur le même contexte.

La Session informative n'a pas montré de faux succès sur processus tronqué : le lecteur impose toutes
ses passes/trames attendues et un échec est conservé dans `refus`, affiché dans les tableaux. Les prises
restantes peuvent encore être agrégées avec cette réserve visible. Limites de trace : le résumé ne
conserve pas l'inventaire des prises, leurs codes/hashes ni leurs effectifs par trame ; une archive
optionnelle non reconnue produit `session_v12set=null` sans détail. Ce sont des informations séparées,
pas un verdict d'adoption, et le nombre de trames provient de l'archive fournie, pas d'une garde fixée à 37.

```sh
python3 -B check.py DEPOT
python3 -B -O check.py DEPOT
```

Les sorties sont identiques à `results.json`. Le lecteur réutilise les 21 sources Git épinglées du reçu
[des fixtures](../pilotes_fixtures_recouvert/README.md), applique le patch dans un temporaire et utilise
la correction V=6 déjà proposée pour la fausse sonde. Deux versions × trois scénarios, 64 appels Python
simulés chacun ; aucune sonde native, compilation, action GCP ni donnée réelle. Le petit commentaire
injecté change les octets, sans prétendre changer la sémantique du calcul. Produit/main intacts.
