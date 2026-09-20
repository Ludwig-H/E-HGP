# Mutations compilées — cover et génération q3/q4

Capture close le 20 septembre 2026 : **3/3 mutations compilées tuées**, après
baseline PASS, avec les **151 sources et le build inchangés avant/après**.
Le constructeur a confirmé leur gel avant le lancement. Build de référence :
`build/v8_q34_cover_20260920`, Release sans sanitizers.

Le [harnais](run_mutants.py) adapte explicitement celui de la tranche précédente,
sans le modifier. Il exécute d'abord `mhgp8_q34_cover_gate --selftest` non modifié,
puis compile et lie trois variantes hors dépôt. Chaque objet modifié précède
l'archive produit inchangée ; l'objet original correspondant n'est pas extrait.

| Mutation | Modification délibérément incorrecte | Contrat visé |
| --- | --- | --- |
| `cover_open_boundary` | Admission de bloc et appartenance singleton passent de `<=` à `<` | Toute la frontière fermée doit être conservée ; deux sites de code modifient un seul contrat |
| `seed_block_rejection_inverted` | Rejet `max_sum <= diameter` inversé en `>=` | La génération doit conserver toutes les seeds aiguës propriétaires |
| `q4_gated_by_q3_rejection` | Retour immédiat après rejet de profondeur q3 | Le rejet q3 ne doit pas supprimer une boule q4 admissible |

Le reçu compte **10 commandes** : une baseline, puis compilation, lien et porte
pour chaque mutation. Une mutation n'est tuée que par un code 1 de la porte avec
message d'échec sémantique, après compilation et lien réussis ; un signal, une
erreur de compilation ou le seul défaut de non-vacuité ne suffit pas. La capture
conserve aussi les échecs, les commandes/sorties brutes, les sources originales
et modifiées, les patches, les hashes des objets/binaires et ceux de fermeture.

Ces trois perturbations évaluent **une porte existante**, pas trois oracles
indépendants. Elles ne constituent ni une étude de performance ni une preuve
globale de complétude ou de complexité, et n'impliquent aucun résultat G4/FULL.

## Résultats et relecture

La baseline passe **11 702 contrôles**, **186 covers/arêtes** et **184 seeds**,
avec 198 candidats (152 q3, 46 q4), coquille maximale de 30 sites, dix sites de frontière
et huit cas q4 sans q3. Chaque mutation compile/se lie avec succès puis sort
avec le code 1 sur la comparaison géométrique attendue :

- Frontière ouverte : `cover membership disagrees with closed rational ball`.
- Rejet de bloc inversé : `edge generation differs from all canonical acute seeds`.
- q4 supprimé après q3 : `covered seed lost a valid ball, depth, presentation or shell`.

Capture : [compiled_8ejcazd9](compiled_8ejcazd9/). Le commit de base est
`785d0589da1eab2e559b2efce1dbd7b287ade6af` ; les sources de la tranche en cours
sont identifiées par leurs hashes, pas par le seul commit antérieur. Les copies
originales/modifiées et patches sont dans la capture. Les objets/binaires
temporaires restent disponibles dans `/tmp/mhgp8_q34_cover_mutants_948pp852`.

- SHA256 manifeste : `4157a038958cec84b4b054389251db61a9dbc3af1730fd63250d4f37dc4ac46c`.
- SHA256 clôture : `f126fbcc665905fbd2d20e44bb641913a3bc836f74ebe5e3467fe048ee008a83`.
- SHA256 harnais : `31adf4967004506ff866117fd67325b1e8aa66c9ab846f351434a2595c49b7d6`.

Les **quatre lectures normal/`-O`, historique/`--check-live` passent** ; leurs
commandes et sorties sont archivées dans [READBACK.json](READBACK.json).
La lecture historique ne dépend pas de la survie des objets/binaires `/tmp` ;
`--check-live` vérifie en plus les artefacts temporaires et les entrées actuelles.

Depuis la racine du dépôt :

```sh
python -B morsehgp3D_v8/receipts/q34_cover_20260920/mutants/run_mutants.py read morsehgp3D_v8/receipts/q34_cover_20260920/mutants/compiled_8ejcazd9
python -B -O morsehgp3D_v8/receipts/q34_cover_20260920/mutants/run_mutants.py read morsehgp3D_v8/receipts/q34_cover_20260920/mutants/compiled_8ejcazd9 --check-live
```
