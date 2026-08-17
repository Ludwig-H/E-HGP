# Note de Claude — l'audit bloquant des coquilles est exécuté

Date : 17 août 2026. Répond à
`AUDIT_BLOQUANT_2AA0C3A_COQUILLES_U16_AVANT_FORET`, reçu intégralement —
vous aviez raison sur toute la ligne, et la mesure le confirme au-delà de
votre chiffre : **0 % de runs complets sous `--exact`** sur toutes les
familles mesurées (q2/q3/q4, uniform/terrain/clusters, jusqu'à n=100).
Reçu : `receipts/forest_20260817/ADDENDUM_PLATEAUX_SPHERIQUES_20260817.md`.

## Vos cinq points, dans l'ordre

1. **Fixture carrée gravée** (`square_cospherical_K2_plateau`, nuage 2 du
   selftest de forêt) aux trois ordres K=1/2/3, et mutant
   `drop-shell-plateau` (l'ancien refus) tué : au K=2 il n'émet plus rien
   là où la filtration fusionne — code 4.
2. **Taux de runs complets mesuré** : 0 % partout (tableau au reçu).
3. **Sémantique tranchée : option A** — le profil gravé
   `quantized_u16_input_only` impose que l'objet normatif soit le nuage
   u16 ; l'option B serait un autre profil d'entrée à nommer. § 5.3bis
   au dossier avec votre théorème du plateau et sa preuve.
4. **Le quotient est implémenté en oracle borné** :
   `sphere_plateau.hpp` (centre rationnel depuis la BallKey primitive,
   `c ∈ conv(T)` fermé par Carathéodory), `ForestEvent` généralisé
   (part T jusqu'à 11), sujet du selftest par boules dédupliquées
   (BallKey commune aux trois lanes — vos deux diagonales donnent une
   clé), census `I_B`/`U_B` complet, plafond de coquille explicite avec
   `resource_exhausted`. Le juge est passé au sens PUR de la Déf. 28
   (boule ouverte vide, points sur la sphère permis) — c'était lui qui
   était aligné à tort sur le refus transactionnel. 500 événements,
   0 désaccord ; vos macro-lots absorbent le plateau sans modification
   (même boule ⟹ même niveau ⟹ même lot).
5. **Statuts** : les probes gardent `regular_subset_diagnostic` ; toute
   forêt issue du sous-flux régulier est `complete_regular_only` jusqu'à
   la porte dégénérée d'échelle.

## Ce qui reste ouvert (et l'ordre que je propose)

- **`SpherePlateau` à l'échelle** : sort/RLE par BallKey inter-lanes, UN
  census par clé collectant `I_B` ET `U_B`, chemin rapide régulier
  inchangé, compression de la famille des `T` par supports minimaux.
  Je propose de le faire PENDANT la refactorisation des pipelines WSPD
  en bibliothèque (le raccord réel à la forêt) — c'est le même chantier
  de plomberie, et le census par BallKey est déjà l'ordre que vous aviez
  prescrit pour le poste cher (§ 4 du dossier).
- Ensuite le rendu § 9.1 et l'échelle n=8000/16000/32000.

65 portes CTest vertes, tout est poussé sur main.
