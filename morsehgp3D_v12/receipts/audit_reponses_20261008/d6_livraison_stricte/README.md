# D6 : les deux corrections livrées — 8 octobre 2026

`e37fd8935e2dd320b4249f4347443c87d45ca34b` livre exactement, **octet pour
octet**, la composition du [lecteur G/Gc proposé](../gc_livraison/d6_g_schemas_proposed.patch)
et de l’[admission du plan](../../audit_d6_20261007/pilotage/proposition.patch)
sur le lecteur `d2f39fe82`, SHA `48f40fd6…`. SHA livré :
`bb1de862175814467664ae83b377238771511c264eca4150905fbec5ffe5db4f`.
Aucun patch, source ou témoin publié n’est recopié dans ce reçu.

Rejeu indépendant Python normal/−O identique : les deux petites captures
natives publiques G et Gc sont admises ; **27 corruptions** sont refusées
(champs absents/inconnus, booléens, longueurs, mélanges entre passes,
négatif/flottant/dépassement u64). Le témoin de forme K1 admet les deux tableaux
K−1 vides. Il s’agit d’admission structurelle, pas d’un nouvel oracle géométrique
ni d’une identité vérifiée à chaque passe.

Le vrai `main` et la préparation synthétique sont joués avec `build` et
`take` simulés par le harnais publié :

| Plan | Code | Prises simulées |
| --- | ---: | ---: |
| Référence u21/x1 présente | 0 | 6 |
| u21 absent, u24 possible | 2 | 0 |
| Toutes combinaisons absentes | 2 | 0 |
| Profil répété | 2 | 0 |
| Cas répété | 2 | 0 |

Cela ferme les deux résidus D6 ciblés de CST-0018/0207 : compatibilité Gc et
admission du plan. **CST-0207 reste ouvert** pour la campagne comparative et
le seuil D6 ; CST-0018 conserve ses autres portées, notamment défaut CLI huit
et collecte C du pilote T2-c. Aucun contrat global n’est clos par ce reçu et
aucune campagne démarrée avec l’ancien lecteur n’est requalifiée rétroactivement.

## Essai local retrouvé, sans nouveau moteur

`d6_local/sortie4/mes_d6.json`, daté du 8 octobre à 02:33:14 UTC :
**ng00, K3, W3, feuille 24, profils 21/24 × facteurs 1/8, un tour**.
Huit journaux de sondes C/G, chacun à deux passes (une seule chaude), soit
16 passes d’étage. Le lecteur livré les admet tous ; tous les champs de ses
résumés correspondent au rapport, après la conversion JSON normale des clés
d’ordre en chaînes. Le rapport porte `controles=[]`.

Les neuf hashes (rapport et huit JSONL) sont épinglés dans `capture.json` et
relus avant/après. Aucun brut LiDAR, export binaire ni `corps_sha256` de payload
n’est recalculé : les contrôles d’égalité des corps restent ceux du rapport.
Les codes de processus utilisés sont ceux archivés dans le rapport ; aucun
certificat externe des retours ni de la compilation n’est revendiqué.
Cette relecture confirme la compatibilité du lecteur avec ces sorties locales,
sans qualifier u24, un gain de temps, le seuil D6, FULL ou G4.

```sh
python -B check.py --repo DEPOT --check
python -B -O check.py --repo DEPOT --local SCRATCHPAD/d6_local/sortie4 --check
```

Sans `--local`, le lecteur rejoue seulement l’identité des patches, les formats,
les corruptions et les plans simulés. Les deux modes avec essai local ont été
joués et donnent le même `resultats.json`. Aucun moteur, build, benchmark ni
appel cloud exécuté par l’auditeur ; aucun fichier vivant modifié.
