# adaptive1 : campagne échouée, clôture récupérée

Source exécutée `f425c5fe7f255150488c3d66c7a1e402ebabd59e`, session
`v11.20261002.adaptive1`. Le worker termine en échec, code1 ; `DONE=3`.
Le banc adaptatif refuse avant toute tentative. Cette capture ne donne aucun
chrono catalogue, mémo, FULL ou clustering.

| Configuration | Portes passées/sélectionnées | Issue |
|---|---:|---|
| Release u18 | 452/452 | conforme |
| ASan/UBSan u24, TSan u21, u21, u24 | 377/377 chacune | conformes |
| Poison u21 | 378/378 | conforme |
| Style | 2/2 | conforme |
| Mutants u18 | 20/21 | délai, dernière porte sans résultat clos |
| Clang facultatif | 0 | absent |
| Supplément ASan/UBSan u18, num/index/tower | 161/161 | conforme |

La matrice totalise **2 360/2 361** portes : zéro échec de test déclaré,
une porte démarrée sans résultat clos, `mhgp11_mutants_tower`. Le budget de
configuration de 550s est épuisé ; après configuration et construction,
CTest reçoit 535,3s et termine sur délai après 535,658s, code−9.
Les six autres portes de campagnes de mutations et les quatorze portes de
manifestes ont un verdict `Passed` dans `ctest.log`. Le JUnit mutants est
absent ; `LastTest.log` ne contient aucune transcription de ces campagnes.
**Aucun nombre de mutants individuels tués causalement n'est certifié ici.**
Ce délai ne démontre pas un défaut géométrique du produit.

La commande adaptative termine code2 en 0,031s avec le refus
`catalogue_adaptive_refused: ValueError`, sans rapport ni lancement natif.
La source figée appelle le contrôle de qualification avant de construire son
rapport ; la matrice non conforme explique ce refus. Les 36 unités prévues
ne sont pas des omissions mesurées dans un rapport : le banc n'a pas démarré.
Les portes fonctionnelles terminées ne rendent pas cette campagne conforme.

Le reçu initial atteste l'arrêt ciblé, la vérification des résultats, la
suppression de la clé privée et la libération du verrou, mais conserve
`oslogin_key_removed=false` et son avertissement. Le diagnostic local épinglé
porte `ABORTED`. La récupération **distincte**
`recovery_2026-10-02T234059Z.json` observe la même génération déjà arrêtée,
confirme `oslogin_key_removed=true` et ne comporte ni erreur ni avertissement.
L'historique initial reste inchangé.

```sh
python3 morsehgp3D_v11/receipts/catalogue_adaptive_20261002/adaptive1_failure/check.py
python3 -O morsehgp3D_v11/receipts/catalogue_adaptive_20261002/adaptive1_failure/check.py
python3 morsehgp3D_v11/receipts/catalogue_adaptive_20261002/adaptive1_failure/check_selftest.py
python3 -O morsehgp3D_v11/receipts/catalogue_adaptive_20261002/adaptive1_failure/check_selftest.py
```

Le code0 du lecteur signifie **preuves cohérentes d'une campagne échouée**.
Il exige les reçus bruts locaux et le diagnostic de nettoyage, leurs hashes,
le commit Git figé et l'archive originale unique ; ce n'est pas une archive
autonome. Il vérifie les copies, le manifeste complet du tar lu sans
extraction, les configurations et caches, les JUnit disponibles, le journal
CTest incomplet, les arguments exécutés et la récupération. Il n'importe
aucun pilote de banc WIP. Les aides historiques importées servent au
transport et aux configurations closes ; le cas sans JUnit est jugé ici.

Les contrôles purs normal/−O donnent **5 témoins positifs, 57 corruptions
refusées**, sans appel natif. La première tentative avec le lecteur historique
de matrice refusait l'absence du JUnit mutants ; l'adaptation présente conserve
cet essai inachevé, sans fabriquer de verdict. Aucun paquet source massif,
payload KITTI ni copie décompressée des journaux n'est ajouté.
