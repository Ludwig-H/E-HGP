# MES-B1o : prélecture du protocole, 10 octobre 2026

**Protocole comparable à B1t ; aucun résultat nouveau qualifié ici.** Source déclarée
`ae8f8107cc5a13356da89addf90808b5aaad9d67`, commit poussé, avant lecture des résultats.
Cadre : `exploration_v12_hors_registre`, `full_pi0`, entrée `quantized_u21_input_only`,
CPU de référence et catalogue CUDA G4, `public_status=not_claimed`.

Le lecteur ouvre seulement les deux préflights, les deux plans et le manifeste JSON
des entrées. Leurs cinq empreintes, ainsi que les **45 déclarations nom/taille/SHA**, sont
figées dans [capture.json](capture.json). Ces 45 déclarations sont exactement identiques
à celles de la [session B1t](../../audit_reponses_20261008/session_b1t_admission/README.md).
Les tailles XYZ/IDs déclarées concordent avec 12/4 octets par site. Les payloads ne sont
ni ouverts ni rehachés : ceci compare les déclarations, pas les données effectives de la VM.
Le rattachement du paquet source, des 138 sources natives B3-K, des ELF, de l'archive
finale et de l'arrêt relève de l'admission de session, hors de ce reçu.

Les options complètes, l'ordre des cas et les délais des deux commandes sont identiques
à B1t. Quinze scènes entières **au sens des entrées préparées du manifeste** : masques sans
sol et regroupements Boreas préexistants, effectif `distinct` lorsqu'il est déclaré. Boreas
10/50 désigne plusieurs captures agrégées. Aucune nouvelle coupe par le pilote ; cette
lecture ne certifie pas la multiplicité des retours d'origine dans le moteur.

| Commande | Cas/processus prévus | Passes FULL prévues | Régime |
| --- | ---: | ---: | --- |
| Identité Boreas 10 sans sol | 2 | 3 | GPU K5 : deux passes ; CPU K5 : première passe seule |
| Principal, 15 scènes | 17 | 28 | 11 GPU K5 × deux passes ; 2 GPU K10 × une ; 4 GPU K5 × une |

Au total : **19 processus, 31 passes au plus**, dont 19 premières et 12 chaudes prévues.
Chaque paire de passes ne fournit qu'**une observation chaude**, sans répétition permettant
d'estimer sa dispersion. Le CPU n'a aucune passe chaude ; CPU première passe/GPU chaude
ne forme pas une comparaison de latence appariée. Identité : budget GPU 8 Gio ; principal :
88 Gio ; hôte 160 Gio, W48. Les effectifs exacts et les cas figurent dans
[results.json](results.json). Ils vont de 146 316 à 10 766 998 sites, sans Paris dans ce lot.
Le seuil FUL1 reste à 1 600 000 sites : ne pas annoncer d'identité différentielle archivée
sur les grandes scènes si le plan reste inchangé.

La fenêtre utile annoncée est de **2 200 s**, pour 3 800 s de délais maximaux cumulés
(900 + 2 900 ; budgets internes 840 + 2 800). Les derniers cas risquent d'être coupés ou
sautés ; ce n'est pas un échec inévitable, puisque les délais sont des maxima. B1t avait
le même plan et une fenêtre de 2 201 s. Comparer ensuite les cas réellement exécutés et les
préfixes FULL valides, sans masquer les refus ; aucun gain causal ne découle de ces deux
campagnes successives.

TU Wien sans sol est le septième cas principal, toujours limité à deux passes.
Une réussite ou un refus renseignera la réapparition de **CST0243**, mais ne prouvera pas
sa cause : B1t avait une FULL froide valide puis un refus mémoire sans identification du
budget ni de l'étage. Distinguer pic du budget appareil, mémoire retenue, mesure SMI et
RSS hôte ; le surcoût B3-K dépend du nombre de boules, pas simplement du nombre de sites.
Cette prélecture ne clôt ni l'incident mémoire ni un contrat de temps.

Relecture indépendante, sans moteur, compilateur, cloud ou payload :

```sh
python3 -B -S check.py --current /workspaces/.ehgp-sessions/v12.20261010.mesb1o --reference /workspaces/.ehgp-sessions/v12.20261008.mesb1t
python3 -B -S -O check.py --current /workspaces/.ehgp-sessions/v12.20261010.mesb1o --reference /workspaces/.ehgp-sessions/v12.20261008.mesb1t
```

Lectures normale et `-O` conformes à la capture ; aucune assertion Python désactivable.
Ce reçu fige la prélecture et reste distinct de l'admission et de l'analyse des résultats.
