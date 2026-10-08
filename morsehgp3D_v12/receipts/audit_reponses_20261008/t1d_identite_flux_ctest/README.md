# T1-d — raccorder les 46 injections à la porte CTest

8 octobre 2026, Codex. Complément proposé, sans modification du produit. Base `8b9eab40a902e0322314ac059a3d0c24d76c46a9` : le port d'admission c31 est déjà intégré et la porte CTest attend 36 injections. **Appliquer d'abord** [la proposition niveaux/table](../t1d_identite_flux_proposition/README.md), **puis** `proposition.patch` de ce reçu ; ne pas rejouer `port_c31.patch` sur cette base.

L'ancienne proposition modifie trois fichiers Python et porte l'autotest de 36 à 46 cas, sans modifier `tests/catalogue/tests.cmake`. La combinaison laisserait donc les portes `mhgp12_catalogue_g4_t1d_judge` et `_opt` attendre une ligne absente, bien que l'autotest termine avec le code 0. `cmake/run_expect.cmake` vérifie la ligne exacte de la même exécution ; `cmake/gates.cmake` crée sa jumelle optimisée. Ce défaut est un raccord de test, pas une faute géométrique.

Le complément remplace seulement l'attente par `juge_g4_t1d_ok injections=46` et actualise son commentaire. Il ne modifie aucun seuil, oracle, interpréteur ou commande. Le reçu précédent reste inchangé.

Rejeu Python depuis les blobs Git épinglés, copies temporaires et deux patches appliqués séparément :

```sh
python -B -S check.py /workspaces/E-HGP
```

Le vrai `g4_catalogue_t1d.py --selftest-judge` émet 36 avant la proposition, puis 46 après, sous Python normal et `-O`, code 0 et stderr vide. La sortie 46 ne contient pas la ligne attendue 36 ; elle correspond à la ligne corrigée. `results.json` conserve cette distinction. Aucun CMake/CTest, compilateur, moteur, GPU, contrôleur ou payload n'est exécuté ; la conséquence CTest est déduite de son lecteur exact épinglé, pas présentée comme une exécution native.
