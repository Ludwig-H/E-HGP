# Réponse du développeur sur le raccord CPU du census (B2-C), et intégration de T2-d-B2

8 octobre 2026. Réponse à la note de l'auditeur Codex [`b2_census_raccord`](../audit_reponses_20261008/b2_census_raccord/README.md)
(`4adbedb1e`, capture du prototype de 11:03 UTC). Chaque point est intégré ou laissé ouvert, et c'est dit.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference ; cuda_g4 pour le catalogue
objet=full_pi0
quantification=quantized_u21_input_only
public_status=not_claimed
```

Le lot T2-d-B2 de l'agent du chantier B (`t2d_b2_lot.patch`, base `72f622a55`) est intégré tel quel sur `main`, après
le retrait de A6 (`ab5614c2a`) ; seul le manifeste des mutants de la tour est recomposé (40 de `bdfca8fb1` + 9 de B2,
plancher 49). Il apporte B2-T (index des naissances de tous les ordres construits ensemble à l'ouverture de
`build_tower`), B2-S (seconde recherche du support évitée), B2-C (census à plat) et le pilote
[`mes_t2d_b2`](../../microbancs/mes_t2d_b2/pilote_t2d_b2.py). **Aucun de ces leviers n'est adopté** : ils seront jugés
sur G4 par `REGLE_T2D_B2`, écrite avant toute mesure.

| Point de l'auditeur | Réponse |
| --- | --- |
| Entrée par coins bruts : un couple inversé viole la précondition de `std::clamp` | Retirée. Le parcours passe la `Box` du nœud, construite par `index/build.cpp` avec lo ≤ hi et des coins du profil. Le mutant qui violait la précondition de `std::clamp` est remplacé par un mutant de même intention, valide |
| `side_site(u32, u32, u32)` public : u32 seul ne prouve pas l'appartenance au profil | Le contrôle de domaine est rétabli dans la forme à plat : `side_site` rend `coordinate_out_of_domain` au-delà de `kCoordMax`, le refus de `Point::make`. C'est un OU et une comparaison, vide au profil 32. Mutant `plat_site_hors_domaine_admis` tué. La voie publique ne promet donc rien de plus que `Point::make` |
| Façades publiques ordinaires | Conservées : `bound_signs(Box)`, `side(Point)` et `side_offset` sont des enveloppes des mêmes fonctions à plat ; une seule implantation |
| `inline` ne prouve pas que les appels disparaissent | Mesuré, pas supposé : callgrind sur 40 000 requêtes de ng00, 829 M → 513 M instructions (0,619) ; microbanc des requêtes réelles capturées pendant G, rapport 0,537 à 0,567 sur ng00–02, à empreintes égales requête par requête (`CensusLedger` entier, genre, I, U). Ce rapport ne se transfère pas au mur : seul le pilote G4 juge |
| Portes à fermer sur le corps livré | Égalité de tout `CensusLedger` et de I/U contre la base (requête par requête, 252 152 / 188 039 / 182 299 requêtes sur ng00–02) ; oracle `mes_g_appareil` sans écart à l'octet sur 2,95 M requêtes (K5 et K10) ; porte `mhgp12_num_local_guard_site_lanes` (quatre voies, refus hors profil, registres égaux à ceux de `side_offset`) aux profils 21, 24 et 32 ; empreintes FUL1 identiques sur 12 cas ; 23 mutants du lot tués par code |
| Norme i64/i128 au profil 32 | **Ouvert.** Le choix statique (s + 2 ≤ 30) n'a ni fixture ni mutant au profil 32 : il faudrait un support certifié d'étendue ≥ 30 en voie certifiée. Reporté avec la qualification u32 (D6) |
