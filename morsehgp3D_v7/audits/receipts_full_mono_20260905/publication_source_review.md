# Raccord de H à la publication constructeur

Contrelecture close le 5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

La variante **H à 155 pins** et le commit **`98bb657835c66f729dc69c33b34d99311348c345`**, daté à 16:38:30 UTC, diffèrent sur exactement **deux documents** : 153 pins H sont inchangés. Les huit pins communs hors audits concordent également. Cette comparaison lit les blobs du commit avec Git en lecture seule ; elle n’utilise ni l’index ni le worktree comme substitut. Les mappings complets et différences littérales figurent dans le [reçu source](publication_source_review.json).

| Fichier déjà couvert par H | Pin H | Pin publié | Changement |
| --- | --- | --- | --- |
| `PASSATION.md` | `e1091ad4…` | `369f53ce…` | Complète les résultats 8k par s=10/12 et les cinq vérifications normal/`-O`, en conservant les deux refus comme échecs. |
| `README.md` | `e6a172e3…` | `a47861c7…` | Remplace la formulation générale « constructeur à qualifier » par la distinction entre premier composant régulier et qualification générale, plateaux hors domaine et masses encore ouverts. |

Le code produit, CMake, les portes, l’oracle et la sonde déjà épinglés par H sont **identiques**. En particulier, `src/forest/full_gabriel.hpp` reste `e02d163ced2074d6b91fe810c112fb946aca56a7724c8e2ae586e3baee97c170` et `bench/full_gabriel_probe.cpp` reste `f3de0d3ca850611f328cb41b251ec66c914afe473eed8e55f89eb889898f1849`. Le fait que le commit ajoute ces fichiers par rapport à son parent ne signifie pas qu’ils changent par rapport aux octets du worktree historiquement qualifiés sous H.

Trois fichiers source/documentaires publiés ne sont couverts ni par H155 ni par les huit pins communs :

| Fichier | SHA-256 publié | Portée |
| --- | --- | --- |
| `bench/full_gabriel_probe_audit.py` | `24e789459ee7adb8b48819dddc8bef8832b2b152ad9418c1a1d281038315e2c7` | Nouveau lecteur de cohérence ; il ne lance pas le moteur et ne juge pas la géométrie. |
| `docs/RESULTATS_MONO_FULL_20260905.md` | `f767a8ef170dcd5286368908375b959e11d213b2a310d51bed9f4c0d87e10f14` | Cinq tentatives, coûts et refus, sans égalité de forêts ni gain apparié. |
| `docs/RESIDENCE_MASSIVE.md` | `31fd18727255161d1621a2a093c0e352c9851084ac56b67d47b8e766eee57db3` | Document existant actualisé : producteur relatif livré, observation de résidence et piste de cache futur ; mesures C conservées comme historiques. |

L’absence de pin H pour `RESIDENCE_MASSIVE.md` ne permet pas de reconstruire son état historique H. Le reçu conserve donc explicitement sa différence **parent du commit → publication**, sans la présenter comme une différence prouvée depuis H. Son ajout au mapping proposé corrige cette lacune de couverture. Le mapping publié suggéré compte **158 pins** : les 155 pins H actualisés pour les deux documents, plus ces trois fichiers. Il est fourni au parent pour une variante distincte ; aucun manifeste existant n’a été modifié.

Les **95 fichiers** des trois paquets constructeur correspondent exactement aux blobs publiés et aux fichiers de reçus encore présents : 60 pour la qualification du producteur, 23 pour la campagne mono, 12 pour l’admission micro. Les pins des deux derniers paquets concordent avec la [contrelecture close](constructor_receipt_review.md). Les 51 sources du micro retrouvent leurs octets dans le commit : 50 au même chemin et le runner privé sous sa copie littérale publiée `receipts/full_gabriel_probe_20260905/capture.py`. Les reçus micro/mono désignent le même binaire `d6126f7778d7d7bb370cc59d356eb927bffa57f4cefeb72f8719a77ef6720204`, encore retrouvé dans le fichier privé. Ce binaire n’est pas un artefact Git.

Le raccord autorise une attribution précise aux sources publiées : qualification bornée H sur les mêmes octets produit, admission micro distincte, puis trois réussites horizontales relatives à 8k et refus d’alias à 16k/K9 et 32k/K7. Il ne transforme ni les comparaisons de volumes en égalité de forêts, ni les temps en gain causal avec F, ni ces observations en SLO 50k. La vérification des 60 fichiers de qualification porte ici sur leur publication ; les verdicts d’exécution restent ceux des revues antérieures, sans nouvelle campagne.

**Le worktree lazy mouvant est exclu.** Le nouveau header observé dans la contrelecture précédente ne remplace jamais le blob e02d du commit. Aucun pin proposé n’en provient, aucun test ou coût de cette publication ne lui est transféré. Les autres variantes et les preuves communes des audits restent sous leur autorité historique. Aucun build, moteur, nouveau rejeu Python, mutation Git ou modification d’index/manifeste. GCP non utilisé.
