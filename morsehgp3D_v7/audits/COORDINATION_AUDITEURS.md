# Coordination entre auditeurs

11 septembre 2026, après **93f4d110**. Écritures dans le seul
`morsehgp3D_v7/audits/`, sur `main`. Réservations précédentes closes. Réservation ponctuelle des 16 fichiers
ci-dessous pour cette publication, close automatiquement à son commit.
Aucun autre fichier n’est réservé.

## Marques dans le premier sweep : prototype qualifié séparément

Le [paquet](receipts_fused_marks_20260911/README.md) ferme O2 et ASan/UBSan/LSan :
30 essais synthétiques, mêmes champs physiques que le calendrier scellé,
6 828 comparaisons BFS, deux mutants et cinq gardes d’entrée. Compilateur,
ELF et sources sont liés ; les lecteurs normal/−O passent. Aucun benchmark
ou producteur géométrique n’est exécuté par cette qualification.

À chaque date : naissances admissibles, **plateau entier**, puis marques
fermées, dans le premier DSU. Le second DSU, active_segment et le rejeu final
disparaissent. Conserver la recherche dans forest.births normalisé, le
représentant brut de la première arête, toutes les marques silencieuses et
la vidange des naissances restantes. Le coût de chaque marque et ses tris
reste payé. Les sorties alimentent le réemploi contributif déjà accepté.

## Rangs : lecture favorable, aucun nouveau verrou signalé

Sources streaming **0e50808c** et helper **81a32d8c** : bornes avant slots,
admission, stricteté et leader du groupe conformes à la preuve. Les niveaux
exacts du resolver restent inchangés. K1 appelle first_consumer et garde son
contrôle d’indice de point ; la branche K1 du helper terminal relève de ses
tests directs. La gate ciblée lue ajoute comparaison Boost indépendante,
fractions équivalentes, liaisons de rang invalides et sept mutants réels.
Les captures du constructeur restent sous sa qualification séparée.

## Acquis et entretien

Le développeur a accepté les marques contributives liées, Chains19→10,
la préparation commune, les semis liés et les gardes de rang comme deltas
séparés. Le [dialogue](DIALOGUE_COURANT.md) garde leurs conditions utiles,
sans répéter les demandes de premières gates déjà closes dans
[34db4b3e](../receipts/parallel_birth_streaming_20260911/README.md).
Les preuves antérieures, échecs et fichiers du second auditeur restent intacts.
L’[entretien](ENTRETIEN.json) trace les notes condensées et les nouveaux reçus.
GCP non utilisé.

## Périmètre exact de cette publication

```text
morsehgp3D_v7/audits/COORDINATION_AUDITEURS.md
morsehgp3D_v7/audits/DIALOGUE_COURANT.md
morsehgp3D_v7/audits/ETAT_COURANT.md
morsehgp3D_v7/audits/README.md
morsehgp3D_v7/audits/ENTRETIEN.json
morsehgp3D_v7/audits/validation_current.json
morsehgp3D_v7/audits/receipts_fused_marks_20260911/README.md
morsehgp3D_v7/audits/receipts_fused_marks_20260911/SHA256SUMS
morsehgp3D_v7/audits/receipts_fused_marks_20260911/context_pins.json
morsehgp3D_v7/audits/receipts_fused_marks_20260911/fused_marks.hpp
morsehgp3D_v7/audits/receipts_fused_marks_20260911/gate.cpp
morsehgp3D_v7/audits/receipts_fused_marks_20260911/o2.json
morsehgp3D_v7/audits/receipts_fused_marks_20260911/record.py
morsehgp3D_v7/audits/receipts_fused_marks_20260911/result.json
morsehgp3D_v7/audits/receipts_fused_marks_20260911/san.json
morsehgp3D_v7/audits/receipts_fused_marks_20260911/verify.py
```
