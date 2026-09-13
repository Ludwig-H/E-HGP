# Dialogue courant de l’auditeur indépendant v8

13 septembre 2026, reprise après **7f4ba045**. Écritures limitées à ce
dossier, sur main. `phase=exploration_v8_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
Les sources CreditBatch et AxisQ2 examinées sont encore non publiées ;
les mesures r3 ne leur sont pas transférées.

## Apport immédiat : additionner les colonnes exactes q2

La [preuve ajoutée à la note existante](P0_SOUS_RECTANGLES_ET_GROUPES.md#8-raffinement-axial-en-cours--additionner-les-colonnes-exactes)
permet de renforcer le filtre axial en cours. Pour une ancre a, deux
colonnes coordonnées exactes se rencontrent seulement en a, qui est
exclu des témoins. Leurs trois populations sont donc disjointes : leurs
crédits s’additionnent. Le constructeur a déjà corrigé le commentaire
sur leur recouvrement ; son filtre conserve pour l’instant les tests
d’axes isolés. Ce changement de commentaire postérieur à la capture a
été vérifié comme le seul delta de axis_q2.cpp.

La contre-fixture à quatre sites fournit un témoin sur y et un autre
sur z : aucun axe n’atteint seul h=2, mais leur somme permet le rejet.
Le filtre actuel reste sûr ; cette amélioration réduit son résidu.
Les minima/maxima du compte sur une boîte donnent une requête directe
dans l’index B : rejeter, émettre une plage, ou descendre. Aucun produit
cartésien de seuils ni développement des paires n’est nécessaire.

Sur la même grille entière à 32k, h10, le compte fermé passe de
6 483 670 à 3 928 390 candidates. Ce ne sont pas des mesures C++.
Les queues A/B de l’autre auditeur utilisent davantage de témoins ;
aucun classement général des méthodes n’est déduit de ces comptes.
L’addition ne se transfère pas à des tubes épais ou à des crédits d’autres
méthodes qui peuvent recompter les mêmes IDs.

Le [modèle et son reçu](P0_AXIS_UNION_CHECKS.json) passent en normal/−O :
80 petits plans, 11 060 paires au census et 148 améliorations strictes.
Les grandes requêtes comptent aussi visites et fragments ; leur coût
ne disparaît pas derrière le résidu réduit. Le modèle utilise encore
des recherches sur colonnes entières, sans qualifier le raccord O(log h).

## CreditBatch : partage réel, petite économie encore disponible

La contrelecture confirme le partage du tri, des cellules et des bornes
Tubes ; les balayages restent propres aux voies. Aucun nouveau défaut
de sûreté trouvé dans ce delta. Des allocations peuvent être évitées dans
[initialize](../src/pipeline/local_credits.cpp) : les vecteurs a_/b_ sont
initialisés à zéro, puis immédiatement remplacés pour chaque voie active.
Limiter ces deux resize au cas `threshold_ == core_` préserve les spans
nuls des voies inactives/saturées. Pour trois voies actives, cela évite
six allocations et 3n initialisations jetées, avec n=|A|+|B|.

La proposition a été essayée dans deux copies temporaires uniquement,
avant/après cette condition, avec GCC 13.3 C++20 strict et -O2. Les deux
gates passent dans chaque copie : batch, 10 872 contrôles et 585
comparaisons physiques ; affectation, 1 110 contrôles dont 5 et 15 échecs
d’allocation injectés. Les sorties rapportées concordent à la relecture ;
aucun gain chronométré ni résultat de tour n’est revendiqué.

Sources stables pendant ces essais : local_credits.cpp
`24dcbc9839d862e7b9e78864bb41baf726f89c3d48321f0ef8ec1c382aa8d151`,
local_credits.hpp
`7d96b38571bacf504d79b4139aaead8042bf1dca26396abd08bd8811799496db`.
La seule condition ajoutée dans la copie donne au cpp l’empreinte
`cd8f52e78aca61fea6144a12803f4eebe525e9e36861d1f3eaeec93dd61e80db`.
Rejeu dans une copie de ces sources, avec les gates correspondantes :

```bash
for gate in batch_gate plan_assignment_gate; do
  g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -I"$audit_snapshot/src" "$audit_snapshot/src/pipeline/local_credits.cpp" "$audit_snapshot/tests/$gate.cpp" -o "$audit_snapshot/$gate"
  "$audit_snapshot/$gate" --selftest
done
```

Le correctif d’affectation par copie puis échange est présent et sa gate
est maintenant enregistrée dans CMake. L’autre auditeur en conserve le
suivi ; aucun fichier produit n’est modifié par cette revue.

## Références utiles et entretien

Les groupes à moments fixes, leur preuve aux coins et leurs trois
contre-fixtures restent dans la [note](P0_SOUS_RECTANGLES_ET_GROUPES.md)
et le [modèle rationnel](P0_COLLECTIVE_CHECKS.json). Le
[complément sur les groupes recouvrants](../../audits/morsehgp3D_v8_complementaire/P0_GROUPES_RECOUVRANTS.md)
fournit les capacités par ID et la compression à quatre IDs. Les
[nappes tronquées](../../audits/morsehgp3D_v8_complementaire/P0_NAPPES_2D.md)
ont leur propre prototype ; nos grilles entières ne lui sont pas comparées
comme des entrées identiques. Recherche des groupes, travail cumulé,
census et tour restent ouverts.

Les deux défauts PreparedRectangle sont clos en r3 3589a2c9. Ses quatre
campagnes, 729 mesures et 513 configurations passent sur les sources
épinglées. L’[ancien reçu d’alias autonome](P0_INPUT_ALIAS_CHECKS.json)
conserve les captures historiques et les sources nécessaires ; les quatre
fichiers retirés et leurs anciens noms restent accessibles en e9e97e64.
Les explications résolues sont retirées de ce dialogue, sans modifier les
reçus du constructeur.

Questions secondaires conservées : raffinement Dual à budget facultatif,
combinaison par maximum avec Tubes, et négatifs NoCredit à revalider quand
le facteur opposé rétrécit. Aucun crédit parent ne s’ajoute sans disjonction.
La [preuve tubes](P0_TUBES_ET_RANGS.md) et les reçus de sous-rectangles
restent actifs. Contrôles : documentation globale, 531 fichiers ; registre,
20 phases ; validation explicite de nos deux Markdown modifiés.

Réservation de publication après 7f4ba045, index constaté vide, limitée
aux quatre chemins suivants dans ce dossier :

- DIALOGUE_COURANT.md
- P0_SOUS_RECTANGLES_ET_GROUPES.md
- p0_axis_union_probe.py
- P0_AXIS_UNION_CHECKS.json

Cette fenêtre expire au commit/push de la passe. Aucun fichier constructeur
ni de l’autre auditeur n’entre dans la préparation. Contrats 50k, massif
et FULL ouverts.
GCP non utilisé.
