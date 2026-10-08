# T1-d : admission avant la campagne G4

**Mise à jour au pin `c31beaf22` : appliquer `port_c31.patch`, pas le patch historique `proposition.patch`.**
Le développeur a entre-temps choisi `--sequentiel` pour les seules prises FUL1 d'identité, après le refus de la
première campagne ; T1-d2 est annoncé adopté en `caf9585e4`. Le port conserve ce choix et son lecteur séquentiel,
retire la proposition alternative de raccord recouvert, et garde les dix contrôles mémoire/cohorte/refus/types.
36 injections T1-d normal/−O passent aussi sur ce port. Les sous-sections ci-dessous documentent le pin initial
et restent une preuve historique distincte. Les vrais rapports G4 sont relus séparément ; aucun faux verdict
d'adoption natif n'est déduit des seules injections.

Rejeu du port : `python3 -B -S check.py --repo /workspaces/E-HGP --port-c31` (aussi avec `-O`), résultat
`port_c31_results.json`. Le témoin B2 recouvert y reste refusé par les deux lecteurs, conformément à la commande
séquentielle ; ce port ne prétend pas l'admettre. Les dix défauts et les quatre positifs sont rejugés à l'identique.

8 octobre 2026, auditeur Codex. Source `5f8e777cffbeddfe90e92fc616a920c28c1b985c`, inchangée pour ces fichiers à
`f33cf8d21`. Python seulement : aucun moteur, compilation, contrôleur GCP ni octet de scène lu. Les assertions
ci-dessous concernent le juge et ses témoins, jamais une mauvaise sortie native T1-d observée.

## Blocage du lecteur FULL

`g4_catalogue_t1d.py` appelle `g4_catalogue_flux.ful1`, qui ne demande pas `--sequentiel`. Les deux sources de la
campagne T1-d produisent la Session recouverte par défaut. Le lecteur hérité n'accepte pourtant que les clés et
les contraintes du format séquentiel. Son auto-test utilise lui aussi cet ancien format et masque le désaccord.

Témoin réel : le journal public B2 `identite/ng00/apres.jsonl`, deux passes GPU K5/W48 avec FUL1, empreinte
`1daef5e8811c94753af078520c178bb44a1884b5fc8c7c0a2e638792562578b6`. Ses lignes sont lues intactes ; seules les
métadonnées Python de l'appel au lecteur sont construites avec les options attendues par T1-d. Il s'agit d'un
test de compatibilité du lecteur, pas d'une nouvelle exécution ni de l'admission d'un processus T1-d.
Avant : `illisible / ligne full hors schema (passe 0)` ; proposition : `ok`, deux passes.

Le correctif raccorde le format recouvert au lecteur partagé `microbancs/outils/lecteur_full.py`, SHA `15437e5f…` ;
ses contraintes de fenêtres, mémoire et dépendances par ordre restent appliquées. Le lecteur historique conserve
son format séquentiel pour les anciennes archives ; T1-d exige le format recouvert de ses deux sources.

## Dix contrôles d'admission manquants reproduits

Les mêmes entrées sont fournies aux fonctions originales et corrigées. Les témoins de mémoire partent d'une
configuration cohérente avec la sonde : pic nul sans budget propre ; sinon capacité courante ≤ pic cumulatif ≤
budget demandé, capacité égale à celle du catalogue. Une seule condition est ensuite rompue par témoin, sauf le
refus partiel qui construit explicitement son préfixe complet. Aucun temps n'est fabriqué comme résultat mesuré.

| Contre-exemple | Original | Proposition |
| --- | --- | --- |
| Pic supérieur au budget, dès la première passe | admis | refusé |
| Usage supérieur au pic | admis | refusé |
| Capacité différente de la ligne catalogue | admis | refusé |
| Pic cumulatif décroissant | admis | refusé |
| Budget supplémentaire ignoré | admis | refusé |
| Refus mémoire W1 rendu pour W48 | admis | refusé |
| Passe réussie avec fausse F2, puis refus mémoire | admis | rejeté |
| Passe réussie avec mauvais comptes, puis refus mémoire | admis | refusé |
| Tour supplémentaire ignoré | admis | refusé |
| Bras supplémentaire ignoré | admis | refusé |

Les quatre contrôles positifs — mémoire nominale, campagne nominale, refus initial conforme et préfixe exact suivi
d'un refus mémoire — restent admis. Ce sont des contrôles unitaires d'admission : le blocage FULL ci-dessus
empêcherait actuellement l'adoption d'une vraie campagne complète. Les contre-exemples ne prétendent pas qu'une
telle campagne a déjà été adoptée à tort. L'auto-test synthétique initial, qui annonce pourtant partout un pic
de 8 Gio jusque sous un budget de 256 Mio, rend bien `adopte`.

Le correctif contrôle chaque passe, y compris le préfixe d'un processus qui refuse ensuite. Le pic du budget propre
est cumulatif entre les deux passes : ne pas le sommer, le remettre artificiellement à zéro ou demander son égalité
à la capacité courante. Sans budget propre, zéro est une convention de la sonde, jamais une mesure de mémoire GPU.
La cohorte ferme identité, FUL1, campagne et budgets dérivés. Les refus sont liés à voie/profil/K/feuille/fils et
aux types entiers. Les seuils statistiques, bras, nombre de tours et règle d'adoption restent inchangés.

## Proposition et validation

`proposition.patch` ne modifie que trois fichiers Python du banc. L'auto-test T1-d passe de 15 à **36 injections** :
positifs, bornes non arrondies, A/A, formats, types, cohortes, mémoire et refus partiels. Auto-tests normal et `-O`
conformes ; les **39 injections historiques T2-d-C** restent conformes. Deux relectures documentaires indépendantes
du patch ont vérifié les conventions de mémoire ; la seconde a identifié le cas du préfixe fautif, corrigé ici.

```sh
python3 -B -S check.py --repo /workspaces/E-HGP
python3 -B -S -O check.py --repo /workspaces/E-HGP
```

`check.py` extrait six sources Git dans un répertoire temporaire, vérifie l'application du patch et ne lance que
Python. Les deux résultats doivent être identiques à `results.json`. Les sources d'entrée, le journal intact et
les verdicts y sont hachés. `SHA256SUMS` ferme les fichiers du reçu.

Proposition initiale destinée au pilote5f ; voir le port actuel en tête de ce reçu. Cette proposition ne qualifie ni les nouveaux chemins CUDA ni les
allocations non comptées : voir le [reçu produit](../t1d_produit/README.md). Le coût visé par ce banc est **C seul** ;
son seuil de 1 % ne prouve pas à lui seul une non-régression FULL ni le contrat de 100 ms.
