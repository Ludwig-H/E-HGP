# Audit du socle u32 et des interfaces de preuve

7 octobre 2026. Pin **`c3de9d73d8999f2f1e31a0f592b829efc6e7a4da`** : socle local
`6a38f7e4b`, précision du format T1 `f4a11f49e`, correction de publication `df9140b5d`
et bilan T0 `c3de9d73d`. Cadre : exploration v12 hors registre, objet `full_pi0`,
profil produit déclaré u21, sondes CPU Release **32 bits**, `public_status=not_claimed`.

| Preuve | Résultat et portée |
| --- | --- |
| [Numérique](numerique/README.md) | 224 présentations, 220 sphères valides, 880 puissances : centres, niveaux, domaines de certificat, signes, milieux et ordres conformes à Fraction ; le contre-exemple q3 de `CST-0201` emploie le repli exact large |
| [Index et identité](index/README.md) | 2 439 requêtes exactes : 52 distances, 128 Morton, 6 Cloud, 13 repères/réservoirs, 2 144 gardes et 96 census ; correction du défaut d’identité `CST-0202` confirmée |
| [Format](format/README.md) | `CST-0225` nouveau : 88 octets sans payload sont admis comme 2^62−1 éléments par débordement d’offset ; cinq témoins, aucune lecture hors fichier ni allocation géante |
| [Publication](publication/README.md) | `CST-0224` partiellement corrigé ; les collisions de noms égaux sont refusées, celles entre fichier et dossier laissent une publication incomplète après exception ; quinze appels synthétiques |
| [Qualification et provenance](qualification/README.md) | `CST-0115` corrigé historiquement ; 30 classes de ports maintenant dépassées (`CST-0226`) ; aucune qualification historique native/G4 transférée |

Deux clôtures précises sont reportées au registre par Codex : `CST-0202`
pour la clé complète et le regroupement des sites, `CST-0115` pour la déclaration
de `reference/tests.cmake` et le bilan d’intégration corrigés en `95247cf4b`.
Le régime D8 au point d’entrée reste distinct de la correction de Morton.

Les autres preuves sont partielles : le type de boule certifiée existe et rejette
les présentations non strictes, mais le mutant demandé par `CST-0108` n’est pas
rejoué. Census local et distances entières sont testés, sans appelants du futur
catalogue ni algorithme complet des k plus proches. Le milieu local est correct,
mais la canonicalisation sur appareil n’est pas portée. Les manifestes et nombres
de tests annoncés par le développeur ne remplacent pas leurs journaux ni une
matrice G4. `CST-0024` reste ouvert : cause du timeout/échec différentiel non établie.

## Contrats encore à préciser

- **T1 / `CST-0113`** : `f4a11f49e` ajoute une spécification, pas le lecteur
  `reference/transition_catalogue.py`. Le format possède les coordonnées/supports
  nécessaires à la géométrie exacte, mais aucun PointId, contrairement au § 8 bis.
  L’ordre absolu de Morton est conservé par le socle ; le départage S*, Kruskal,
  cover et l’empreinte FULL attendent le lecteur et leurs portes.
- **Numérique / `CST-0023`** : une candidate presque alignée a un centre dont
  le plancher dépasse i64. Le code i128/Big est juste sur le témoin ; restreindre
  aux boules certifiées l’énoncé de parties entières de centre sur 64 bits.
- **Provenance / `CST-0226`** : dater la table T0 et relier le nouveau port,
  ou actualiser les 30 classes et l’admission u32. Les anciennes empreintes v11
  restent correctes ; aucune modification clandestine n’est déduite de ce retard.
- **Bilan T0** : la phrase de l’architecture sur le repli DFS est corrigée.
  En revanche, additionner les coûts M2 et M5 (environ 16 ms) est une projection
  pour C, pas une mesure du catalogue intégré avec fin d’étage, replis et budgets.
  Les anciennes preuves C/D restent bornées à leurs microbancs ; aucun chrono
  FULL/100 ms supplémentaire n’est acquis. `CST-0018/0021/0211` gardent leurs limites.

Le contrat T2 `5f797660d`, arrivé après le pin de cette tranche, est la prochaine
contre-lecture ; il n’est ni jugé ni qualifié par ce reçu.

## Vérification et conservation

Trois petites sondes Release, deux oracles rationnels indépendants et les scripts
Python/Git sont conservés ici avec leurs résultats agrégés. Les captures normale
et `-O` sont identiques pour les cinq volets ; sources, témoins et binaires sont
hachés. Les doubles captures numériques et de provenance sont remplacées par
leur identité vérifiée ; les flux natifs reproductibles ne sont pas dupliqués.
`verification.json` consolide les pins, empreintes et contrôles d’intégration ;
`SHA256SUMS` ferme le reçu. Les README locaux donnent les commandes de rejeu.

Aucun fichier produit modifié, aucune campagne lourde, donnée réelle ou session
GCP/GPU lancée par cet audit. Pas de sanitizer, mutant ou mesure D6 rejoué ici.
Les adresses employées dans le témoin de publication sont exclusivement synthétiques.
