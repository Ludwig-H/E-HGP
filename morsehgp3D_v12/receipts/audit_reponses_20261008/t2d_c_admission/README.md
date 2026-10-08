# T2d-C : admission du juge et de son parseur, 8 octobre 2026

Reçu **reconstitué après la coupure du codespace**, depuis les sources de secours antérieures à celle-ci. Le précédent
reçu temporaire n'avait pas été poussé ; ses fichiers sont perdus. Ce dossier regroupe ses preuves, rejouées, avec
un lecteur unique. Attaché à **CST-0018** ; prototypes non livrés aux pins étudiés, aucune nouvelle campagne qualifiée.
Le [reçu statique publié](../t2d_c_prelecture/README.md) demeure inchangé. Les conclusions ci-dessous portent sur les
captures sauvegardées, pas sur une éventuelle correction postérieure.

Le juge pur est chargé depuis sa source exacte et reçoit sa propre fixture nominale, puis trois mutations isolées :

| Rapport synthétique | Juge c8dd9bc9 | Juge 14eb9c8a |
| --- | --- | --- |
| Nominal fourni par le juge | adopté | adopté |
| Étapes absentes des 2 100 passes | adopté | refusé |
| Passes déclarées CPU/u18/K10/W1, indice 0, `reason=memory_budget` | adopté | adopté |
| Mutant avec code 0, listes d'empreintes vides | adopté, mutant « tué » | adopté, mutant « tué » |

Ces fixtures sont des rapports partiels propres au juge, **pas des sorties natives complètes**. La version 14eb
ajoute présence/type entier des étapes : le premier trou est donc corrigé à ce pin. `run_complete` ne lie toujours
pas les métadonnées à la commande ; `check_mutant` traite une empreinte absente comme une différence géométrique.
Les fonctions causales exactes et les SHA complets sont conservés dans `capture.json`.

Le parseur amont ne ferme pas ces deux trous (pilote cdde6f84, base c477ffd8, juge 14eb). Seul son lanceur est remplacé :

- Une commande demandant appareil, deux passes et les deux sortes d'empreintes reçoit un flux vide avec code 0.
  `probe_run` rend `unreadable=0`, listes vides ; `check_mutant` conclut `tue` sans refus.
- Une fixture CPU native historique de deux points synthétiques, trois passes à un fil, est renvoyée à une commande
  demandant appareil/48 fils/trois passes. CPU/W1 est conservé sans anomalie, et `run_complete(..., 3, False)` réussit.
  Cela prouve l'absence de liaison à la commande à ce raccord ; cette seule fixture n'est pas présentée comme une
  campagne complète adoptée ni une identité GPU complète.

Les fonctions amont sont extraites par AST et exécutées inchangées avec leurs aides et constantes. La fixture
`3b6c4261` sert uniquement à conserver le format réel de l'émetteur ; aucune nouvelle exécution de son binaire,
attestation de compilation ou donnée de coordonnées n'est transférée. Aucun moteur, compilateur, GCP ou payload
LiDAR exécuté/lu par cette reconstruction.

`mutant_comparaison_complete.patch` est une **proposition partielle contre 14eb**, reconstruite après la coupure :
référence hachée présente et, pour code 0, listes non vides, cardinalités égales, empreintes complètes et compteur
u64 hors bool. Appliqué à une copie temporaire et exécuté en mémoire : le nominal reste adopté, le mutant vide devient
`refuse / comparaison incomplete`, sans verdict « tué ». L'absence de comparaison doit être refusée. Ce correctif
ne ferme pas la liaison aux paramètres commandés, ne certifie pas le nombre d'empreintes attendu par la commande
ni la cause d'un code 2/3 ou négatif. Aucun produit modifié.

La prochaine admission doit conserver les objets natifs et vérifier la séquence open/catalogue/options de
sortie/digest/exit, statuts/raisons, indices consécutifs, voie/profil/K/feuille/fils et nombres u64 hors bool, ainsi que
ledger et diagnostics complets. Une exception d'émetteur historique doit être explicite : un champ absent ne devient
pas silencieusement une mesure nulle. Le mutant doit fournir les empreintes réellement demandées.

Deux limites statiques du protocole capturé subsistent aussi. `sans_repli_compact` change seulement la fenêtre à
2^62 : il conserve la compaction des marqueurs et le rapatriement sélectif de l'ordre, et lit toutes les clés par
chaîne. Ce n'est pas l'ancienne réparation (verdicts/ordre/clés complets une fois). `flux_seul` conserve cette réparation
et les allocations anticipées : nommer les effets réellement isolés. A/A est annoncé comme contrôle publié, sans
condition d'adoption explicite ; `aa_window=0.015` n'est pas appliqué. La décision compare une borne haute arrondie
à quatre décimales : 0,99996 devient 1,0, donc faux rejet conservateur. Comparer la borne non arrondie. Aucun gain ou
intervalle de confiance réel acquis dans ce reçu.

```sh
python check.py --replay /workspaces/.ehgp-rescue-v12-20261008T0407Z/replay
python -O check.py --replay /workspaces/.ehgp-rescue-v12-20261008T0407Z/replay
```

Les sorties normales et −O coïncident avec `capture.json.result`. Le lecteur refuse toute dérive d'empreinte et
reconstruit exactement le patch ; ses comparaisons n'utilisent pas `assert`. **Rejeu non autonome** : les cinq
sources/fixture épinglées doivent rester disponibles hors Git, dans la sauvegarde persistante (noms dans la capture).
Seuls les extraits causaux sont copiés ici, aucune source complète ni journal contenant un chemin privé. La sauvegarde
du précédent lecteur amont a servi de référence de reconstruction ; ce nouveau reçu ne prétend pas reproduire
octet pour octet les fichiers temporaires perdus.
