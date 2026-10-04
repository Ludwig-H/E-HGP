# Enveloppes et lecteurs des mesures — audit ciblé v11

4 octobre 2026. Sources relues jusqu'à **66372e621**, après la publication
3a56bc5ca ; cadre CPU/u21, exploration hors registre, `not_claimed`.
Quatre commits ciblés : ec55578d9, 0cf20f98c, e49ea4690 et 66372e621.
Les sources figées et leurs empreintes restent dans les trois capsules.
Les notes actives sont actualisées en place ; aucun nouveau dialogue.

| Objet | Conclusion | Preuve et limites |
|---|---|---|
| M3/E4, ec55578d9 | Raccord favorable : enveloppes sûres, contacts demi-ouverts, q4 derrière q3 et compteur avant E4 préservés | [7 552 gardes Gram/Fraction](envelopes/README.md), trois profils, permutations ; portes/mutants relus en source |
| Diagnostics pipeline, e49ea4690 | Le lecteur impose dernier départ ≤ première fin sans barrière ; un FULL correct peut être refusé | [97 gardes du vrai lecteur](pipeline/README.md), chronologie admissible et substitution d'une seule condition |
| Lecteur A/B, 66372e621 | Formules correctes ; contexte des refus, identité et exclusions perdu dans le dérivé | [127 gardes AST/stdlib](bench/README.md), rapports synthétiques au schéma actuel du producteur |
| Banc de tailles, 0cf20f98c | Un timeout empêche la publication des prises déjà terminées | Même capsule, processus factices ; refus ordinaire conservé |

Conseils concrets : borner séparément les deux temps agrégés des voies par
`forest_ns` ; conserver verdict et provenance du rapport A/B, avec ses
exclusions ; écrire un checkpoint par prise et persister l'échec du timeout.
CPU et attente couvrent toute la tâche alors qu'une queue nulle perd sa
durée : sa fin ou sa durée serait le complément utile. CPU+attente n'est
pas une partition exacte du mur ni une preuve d'un effet SMT.

Avec cinq paires, p bilatérale minimale=0,0625 : résultats descriptifs au
seuil 5 %, sans ajout rétrospectif de prises. Le lecteur ne décide aucun
seuil. Le défaut d'ordre à N=2 reste celui du
[reçu précédent](../audit_ports_20261004/README.md) ; le plan N=3
n'est pas invalidé par ce témoin. Les tranches de sites ne remplacent pas
les trames entières ; rattacher chaque entrée préparée à sa provenance.

**7 776 gardes par mode**, normal/−O identiques ; **aucun build/test natif,
fit ou GCP**. Qualification G4, identité native des sorties et gains des
nouveaux ports restent des preuves distinctes. Les replays Python utilisent
Fraction ou AST et des événements/processus factices, sans lancer le moteur.

Depuis ce dossier :

```sh
python3 -B -S check.py
python3 -B -O -S check.py
```

`SOURCE_SCOPE.json` fixe les commits et hashes ; `REPLAYS.json` fixe scripts,
sorties et comptes. Le lecteur vérifie tous les payloads, rejoue les trois
modèles et compare aux sorties closes. Le SHA racine inclut les manifestes
des capsules ; les originales privées restent intactes. La capsule bancs
emploie le schéma courant, avec son ajustement de modèle déclaré dans ORIGIN.
Les anciens reçus publiés et les sources produit sont inchangés.
