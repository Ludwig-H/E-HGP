# Reprise de JUG-EMST : CST-0232

7 octobre 2026 ; sources figées `f601b36ace16bcc8f7ac9bc532e45ab5079f9667`, correctif
`98ca0755691ed2930c886d0b8566e4d06c82c54f`. Hors produit, CPU de référence, petits témoins entiers u21/u32,
`public_status=not_claimed`. Aucun changement du juge, aucun nuage réel, aucune campagne lourde ni GCP.

**Clôture de CST-0232 proposée.** Les deux contrôles d'identité sont maintenant indépendants : le fichier
`--ids` doit être injectif avant tout calcul EMST ; tous les PointId du FULL sont ensuite contrôlés comme injectifs,
avec ou sans référence. Une égalité à une référence invalide ne remplace plus la validation du vidage.

## Preuve causale et nouveaux témoins

Le témoin historique est rejoué sans changer un octet : deux sites distincts `(0,0,0),(2,0,0)`, FULL de 586 octets,
PointId `[17,17]`, SHA-256 `ee717fa160d209b0182226fc8b87ac701e4c35d217862c2a33695cccb19bbb60`.

| Appel | Ancien constat | Sources corrigées |
| --- | ---: | --- |
| sans `--ids` | refus 3 | refus 3 : PointId en double dans le vidage |
| avec référence `[17,17]` | **identique 0** | **refus 3**, avant comparaison : PointId en double dans la référence |

Les 43 appels supplémentaires vérifient séparément les deux gardes et l'alignement des identités :

- Les six permutations d'entrée de trois sites dont les ordres entrée, Morton et lexicographique diffèrent ;
  identité exacte avec la référence correspondante, accord géométrique sans référence, désaccord 1 avec des
  identités uniques échangées. PointId `0` et `0xffffffff` demeurent valides.
- Aux six permutations : référence non injective avec FULL valide ; FULL non injectif avec référence valide ;
  FULL non injectif sans référence. Tous sont refusés 3. Les répétitions incluent des positions non adjacentes.
- Références trop courte, trop longue, vide ou terminée par un octet isolé : refus 3. PointId du FULL égal à
  `2^32`, avec et sans référence : refus 3. Référence dupliquée sans aucun `--vidage` : refus 3 avant EMST.

Une référence valide est lue dans l'ordre original des sites d'entrée, puis associée au site canonique par
`sites_.entree[attendu]` ; elle n'est pas interprétée dans l'ordre de Morton du FULL. Les permutations exercent ce
raccord effectivement. Sans référence, le juge peut vérifier le domaine et l'unicité des PointId, mais pas leur
égalité à une source externe inconnue : c'est la portée voulue.

## Rejeu géométrique et aide mathématique

Le harnais indépendant précédent est repris par référence, sans recopier son oracle : Kruskal sur toutes les paires,
composantes du graphe complet recalculées à chaque seuil et export FULL construit indépendamment du moteur. Ses
24 nuages de 1 à 32 sites donnent les mêmes arbres, EMST et trois empreintes attendues, avec sélection automatique
u64/u128 puis u128 forcé. Le contrôle d'unicité ajouté ne modifie donc aucune décision géométrique de ces témoins.
Les formes rationnelles équivalentes très larges restent admises ; les mutations de niveau, parent, enfant, centre
et site restent détectées. Les 24 lignes de résultats géométriques sont aussi confrontées à la capture antérieure.

La justification d'ordre un reste celle du reçu antérieur : deux boules de rayon √a sont connectées si leur
distance carrée est ≤4a ; la propriété de cycle d'un EMST préserve les composantes de toutes les coupes, et la
contraction de ses plateaux rend les multifusions. Un départage d'arêtes différent peut changer l'EMST mais pas
cette forêt canonique. Les identités externes ne changent aucune distance ; elles constituent un contrat d'entrée
indépendant. Il est donc correct de les valider séparément, sans les intégrer au départage géométrique des arêtes.

**107 appels CLI indépendants par mode Python** : 48 sur les nuages, 16 variantes historiques du FULL et 43 nouveaux
contrôles d'identités. En plus, la suite bornée officielle `temoins.py` est rejouée dans le même mode :
`temoins=11 controles=141 echecs=0`. Une seule petite unité C++ du juge est compilée par passage en Release, avec
avertissements stricts ; tous ses fichiers de dépendances locales sont hachés et comparés au pin avant/après.

## Limites et fermeture

Cette clôture concerne l'unicité et le raccord des PointId. Elle ne qualifie pas les neuf grands vidages historiques,
les limites de capacité, les performances G4, les ordres k≥2 ou les verticales. JUG-EMST lit seulement l'ordre un ;
le contrôle historique qui accepte un préfixe FULL annonçant K2 reste attendu et documente cette limite. Le lecteur
FULL strict et MES-M0 restent nécessaires pour la tour entière.

`check.py` vérifie le hash du harnais antérieur, change explicitement son pin et le seul verdict devenu refus, puis
ajoute les contrôles ci-dessus. Les messages contenant le répertoire temporaire sont normalisés en `<temporary>`.
Une tentative préparatoire a révélé que le lancement officiel créait `__pycache__`, ensuite refusé à raison par
le contrôle des sources épinglées ; le harnais final utilise `-B` et ne laisse aucun fichier dans les sources.
Ce défaut du harnais n'est pas attribué au juge.

```sh
python -B -S morsehgp3D_v12/receipts/audit_reprise_20261007/emst/check.py
python -B -S -O morsehgp3D_v12/receipts/audit_reprise_20261007/emst/check.py
```

Les deux sorties finales sont identiques à `normal.json`. `verification.json` consigne leurs hashes, la stabilité
des sources et le raccord à la capture précédente ; `SHA256SUMS` ferme le reçu. Aucun binaire ni fichier de données
de nuage n'est conservé.
