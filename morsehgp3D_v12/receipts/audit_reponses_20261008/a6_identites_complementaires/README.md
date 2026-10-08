# A6 : complément d'identité sur les 21 grandes trames

Contrelecture du 8 octobre 2026, métadonnées seulement. **Les trois journaux contiennent réellement chacun 21 FULL et 21 libérations, puis une sortie `ok`** : base, A6 recouvert et A6 séquentiel. Les codes externes **0/0/0** sont consignés dans le journal clos `identite_grandes_72.log` (`e45995a1…`). Chaque passe correspond à l'une des 21 trames de plus de 60 000 sites du manifeste, dans son ordre : CPU, u21, K5, W3, passes 0 à 20. Les **21 SHA256 FUL1, non vides, sont égaux dans les trois bras**. Le lecteur strict `15437e5f…`, lu depuis Git `bdfca8fb1`, admet les trois flux, y compris les horloges du schéma recouvert. Rejeux Python normal et `-O` concordants ; aucun moteur relancé, aucune coordonnée ou sérialisation de forêt ouverte.

| Bras | Journal SHA-256 | FULL / libérations |
|---|---|---:|
| base | `2ebf8b13bb2fa5c6c0cdb5e0bf15eace66014862985dafcaa137e8a251f2640a` | 21 / 21 |
| A6 | `5837df1c3e7c82c8e11c6ad42da358076e285e91e2f8eb7e2b34fe8cc08929a0` | 21 / 21 |
| A6 séquentiel | `93603a9cb3b963387298861143a048823305c310c4999d930475a09c2d91d1b3` | 21 / 21 |

L'ancien essai de script découpait mal certains noms, obtenait des refus d'usage et comparait deux listes vides. **Son verdict reste nul**, comme le reconnaissait le rapport A6 observé `56aa8c1b…`. La nouvelle preuve ci-dessus corrige ce manque par des empreintes effectivement présentes et appariées ; elle ne transforme pas l'ancien essai en réussite.

Le nouveau script exige déjà les trois effectifs attendus et leur égalité. Garde-fou encore recommandé avant d'afficher `IDENTIQUE` : exiger simultanément `N=21`, les trois codes nuls, 21 SHA256 de 64 chiffres hexadécimaux par bras, puis leur égalité. Par exemple remplacer la sélection permissive des empreintes par `grep -Eo '"full_sha256":"[0-9a-f]{64}"'`, et ajouter `[ "$N" = 21 ] && [ "$c1" = 0 ] && [ "$c2" = 0 ] && [ "$c3" = 0 ]` à la condition existante. Les codes effectivement enregistrés étant nuls, ce durcissement ne remet pas en cause ces 63 résultats.

Les hashes des ELF **actuellement présents** et du lecteur sont enregistrés. Le rapport attribue les bras à `72f622a55 + terminaison` et à ce socle + A6, puis annonce leur raccord à `bdfca8fb1`. Ce reçu ne dispose pas d'une fermeture ELF avant/après chacune de ces exécutions : il établit l'identité des sorties épinglées, pas une certification rétroactive complète de la construction ni une qualification du futur produit intégré.

Trois observations locales complémentaires restent distinctes :

- Le journal `foret_a6_72.log` annonce les codes nuls et fichiers égaux pour 27 trames K5 et trois K10, avec un outil d'empreinte de forêt complète. Nous épinglons cette déclaration ; **les sérialisations et leur contenu ne sont pas relus ici**. FUL1 ne couvre pas tous les tableaux de R/historique.
- `cuda_a6.log` et `ctest_a6_cuda.log` consignent construction à code 0 et **six portes de terminaison/carte Passed**, zéro échec. Ce sont des portes dans une construction CUDA, pas une nouvelle mesure GPU ni une exécution qualifiée du pipeline CUDA complet.
- `r_direct.log` décrit quatre trames, avec `ecarts_lemme=0` et `sans_bloc=0` pour chaque ordre 2 à 5. L'outil `r_direct.cpp` (`e2769040…`) compare les branches R aux enfants M lorsque `d>0` et `q=d+1`. **Son retour final vaut toujours 0** : la nullité des écarts doit être lue explicitement. Ces observations soutiennent l'intérêt du raccourci déjà proposé ; aucun raccourci R n'est intégré ou chronométré par ce reçu.

Ces observations n'établissent ni toutes les interleavings de publication des hints ni la clôture de CST-0242. Elles ne prouvent aucun gain de temps G4. Les identités de forêts et le diagnostic de R restent séparés de l'audit de concurrence.

**Ajustement documentaire avant première publication.** Après la première relecture, le rapport actif est passé de `56aa8c1b…` (21 052 octets) à `14af7876…` (22 513 octets). Son ancien pin reste une observation datée, sans préimage intégrale conservée ; il est retiré des dépendances exécutables du rejeu. La v1 de ce reçu, tous les primaires requis et les ELF observés sont sauvegardés hors Git ; le rapport actuel y est copié séparément. Les 63 FULL, leurs pins et les codes restent inchangés. Le rapport actuel récapitule des identités locales, annonce G4 non joué et le raccourci R non intégré : cela ne fournit aucune qualification concurrente nouvelle.

`capture.json` porte les pins et seules métadonnées nécessaires ; `check.py` relit les journaux, le manifeste et le lecteur Git. Les bruts restent hors Git ; le rejeu a été fait sur leur copie persistante, avec `--a6` pointant vers `sources/a6`. Si un primaire est remplacé, son nouveau hash est refusé ; une modification ultérieure du rapport documentaire n’invalide plus les primaires clos.

```sh
python check.py --repo /chemin/depot --a6 /chemin/scratch/v12_t2d_A/a6
python -O check.py --repo /chemin/depot --a6 /chemin/scratch/v12_t2d_A/a6
```
