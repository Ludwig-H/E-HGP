# Sortie supports réduite à l'arbre couvrant de Kruskal : qualification G4 conforme

6 octobre 2026. Session gardée `v11.20261006.claudesupkr`, cible `us-central1-c / ehgp-v7-3b1d496aed430749ea7e049f`,
arrêt `TERMINATED` certifié. Source `07428324e`, Release u21. Cadre : `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`.

**Objet.** Décision de l'utilisateur du 6 octobre : « il ne faut surtout pas représenter tous les supports (pour le
niveau K) mais seulement ceux associés au minimum spanning tree de niveau K ». Choix retenu : les arêtes de Kruskal,
avec S\* seul. La sortie `MHGP11SP` version 2 publie :
- les naissances ;
- les boules de fusion qui réalisent au moins une union dans le DSU des enfants de leur nœud, parcourues dans l'ordre
  des `BallIdx`, avec la connexion finale vérifiée ;
- un seul support par boule publiée, son S\*.

Les liaisons internes et les fusions redondantes d'un même plateau (cycle) sont retirées. Le sélecteur, le différentiel
exact et les gardes du lecteur viennent des propositions des auditeurs (`be8085ec1`, `ddb8d4ea9`, `0272bd447`).

**Résultat : 15/15 portes conformes** (`ctest.txt`).
- `mhgp11_cli_supports_oracle` : Kruskal appliqué à l'oracle borné S1 complet, comparé à la sortie native intacte ;
  témoin triangle K1 (deux supports, et non trois) ; coquilles larges admises.
- `mhgp11_cli_supports_spanning_reader` : le lecteur refuse un cycle de plateau et des composantes disjointes.
- `mhgp11_api_supports_route_*` : les deux voies (arbre d'ordre K seul, ordre K tiré de FULL) concordent à l'octet,
  sur l'oracle, à 8 000, 16 000 et 32 000 sites, et sur les trames ng00, ng01, ng02 à K5. Les empreintes u21 sont
  égales aux valeurs gravées.
- `mhgp11_cli_supports_scale*`, `mhgp11_cli_supports_lidar_*` : lecteur, permutation, réétiquetage, W1/W4, signature
  commune avec FULL.

**Comptes à K5.**

| Nuage | Boules publiées | Nœuds | Branches |
| --- | ---: | ---: | ---: |
| uniforme 32 000 | 1 163 756 | 1 163 756 | 1 163 755 |
| ng00 | 576 388 | 576 371 | – |

Pour mémoire, la version 1 publiait sur ng00 789 886 boules et 789 889 supports.

Les empreintes u18 et u24 ont été relevées localement dans chaque profil ; seul le profil u21 est qualifié par cette
session.

## Pièces

| Fichier | Contenu |
| --- | --- |
| `plan.json` | Plan de session, avec son critère |
| `launch.json` | Lancement |
| `receipt.json` | Contrôleur |
| `ctest.txt` | Portes, en CTest |

`SHA256SUMS` couvre les autres fichiers.
