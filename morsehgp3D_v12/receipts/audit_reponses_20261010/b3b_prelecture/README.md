# B3b sur A6c : composition et admission du juge

Contre-lecture du 10 octobre 2026. `phase=exploration_v12_hors_registre`,
`objet=full_pi0`, `public_status=not_claimed`. Aucun moteur, compilateur, test natif,
accès GCP ou contenu des nuages exécuté ou lu. Le reçu porte sur la préparation ;
les chronos de la campagne en cours ne sont pas qualifiés ici.

La source `81b0883d1` compose exactement A6c `aa6338ee8` et l'ancien B3
`545ed987e` : les **neuf fichiers natifs** touchés par B3 sont identiques à ceux du
premier essai ; tous les autres fichiers natifs restent ceux d'A6c. Les quatre
bras reconstruits conservent leurs **25 fichiers et 46 substitutions exactes**,
préimages et postimages vérifiées. `avant_bis` emploie le binaire d'`avant`.
Les seuils, cohortes, statistiques et **32 fonctions** du pilote sont inchangés ;
la règle annonce une nouvelle mesure sur la base A6c, en conservant le rejet de
la précédente campagne. La déclaration datée du 10 octobre à 18:45 UTC figure
au commit de 18:52:15, avant le lancement à 19:13:51.

Le paquet réellement préparé pour `v12.20261010.t2db3b` épingle `81b0883d1`.
Ses 495 fichiers de construction, pilote et bras inclus, correspondent au commit. L'archive d'`avant` a
l'empreinte attendue `29a7df31…f91fd19c` ; ses 495 fichiers de `src/`, `tests/`,
`bench/`, `microbancs/`, CMake et `cmake/` relus sont ceux d'`aa6338ee8`. Le plan demande dix processus
par trame et bras, huit passes par processus. `capture.json` enregistre l'état
observé, sans identité de compte ni cible de VM. Ces observations de préparation
ne prouvent pas la fermeture d'une exécution distante.

Le **juge v2 proposé le 8 octobre n'est toujours pas embarqué**. Le juge v1 relit
les 300 journaux de temps, mais admet les 25 journaux FUL1 et les dix journaux de
résolution via leurs résumés. Les sept contre-exemples publics du reçu
[identité B3](../../audit_reponses_20261008/b3_identite_admission/README.md) sont
rejoués ici avec le pilote de B3b : retirer les 35 fichiers d'identité, falsifier
le hash et le code FUL1, remplacer la résolution brute par un échec, ou supprimer
les chemins/hashes/codes d'identité laisse les trois leviers « adoptés » dans le
modèle synthétique favorable. Le contrôle d'une empreinte FUL1 fausse dans le
résumé rejette ; celui d'un journal de temps modifié refuse. Il faut rejouer les
identités brutes à l'admission de la campagne ; cette lacune ne démontre pas une
sortie géométrique incorrecte. Le [correctif v2](../../audit_reponses_20261008/b3_identite_proposition/README.md)
reste la proposition existante, sans inventer rétroactivement les codes absents.

La réintégration **ne change pas la construction ou le transfert des clés** :
16 octets de stockage hôte par boule ; 16 octets supplémentaires D2H par boule
pour la fin d'étage GPU complète ; reconstruction hôte dans la voie découpée,
sans supplément D2H de clés. Le bras `transfert` conserve les sept mêmes corps
que `cles` et `apres`, y compris cette reconstruction découpée ; il n'isole donc
pas universellement un coût PCIe. Voir la [preuve de voies](../../audit_reponses_20261008/b3_memoire_voies/README.md).
Le nombre de sites ne remplace pas le nombre de boules pour chiffrer ce coût.

Les fichiers concernés par **CST-0244 et CST-0245 sont inchangés** depuis A6c.
Les deux constats restent ouverts. Le manifeste tour conserve les 72 entrées
A6c, réancre `table_s_etoile_queue_partielle`, puis ajoute deux entrées B3 pour
un plancher de 74 ; le catalogue en annonce 38. Applicabilité et contenu des
manifestes ne prouvent pas l'exécution des mutants.

Rejeu léger, depuis un dépôt possédant les commits épinglés :

```sh
python3 -B check.py /chemin/du/depot
```

Le lecteur vérifie les sources Git et substitutions en mémoire, puis réemploie
le témoin Python public immuable avec six dépendances nouvelles épinglées. Il
compare le résultat à `results.json`. Une première génération locale a échoué
parce que la redirection créait un fichier attendu vide avant sa lecture ; le
lecteur et les contrôles étaient inchangés. La génération suivante écrit le
résultat après calcul, puis le rejeu vérifie le fichier complet.
