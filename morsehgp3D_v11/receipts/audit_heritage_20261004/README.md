# Deux idées anciennes retenues pour la v11 — 4 octobre 2026

Cadre : `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`. Revue des modèles et mécanismes
v1–v10, confrontés à **4fac501181bf9d9c6f0cb27736355bd413828253**.
Seules les deux propositions ci-dessous et leurs preuves sont déposées.
L'inventaire critique des pistes écartées reste hors dépôt ; aucun journal
supplémentaire dans `audits/`, aucun ancien reçu clos modifié.

## 1. Différer le Level q3, avec une primitive commune catalogue/MEB

La [contrelecture précédente](../audit_deep_20261004/geometry/README.md)
établit le travail jeté avant rejet propriétaire dans le catalogue v11,
contrairement à la v10. La [preuve complémentaire v7 → MEB](q3_deferred/README.md)
étend **le même candidat privé q3** à la recherche locale : sur le tétraèdre
régulier entier 0/2, quatre Level q3 rejetés sont évitables, mêmes six
présentations, 17 tests et support q4 gagnant. Ce report ne supprime ni centre
N/D, ni tests d'inclusion, ni recherche lexicographique.

Le contrat exige tag 3 et certificats q3, puis la **formule brute de degré 6**
inchangée à l'acceptation. La v7 réduisait son niveau : son PGCD n'est pas
porté. Les puissances de 128/146 bits des témoins valides u21/u24 empêchent de
réemployer un candidat q4 retagué qui ferait croire la voie native certifiée.
Le repli checked/Wide reste nécessaire.

**2 071 contrôles exacts / 80 parties**, normal et `-O` identiques. Le modèle
emploie Gram/Fraction indépendant et confronte coefficients bruts, supports
et sept compteurs ; il ne joue pas le futur code natif. La preuve de report
du catalogue garde son propre reçu antérieur. Aucun pourcentage de temps
ni ancienne qualification v7/v10 transféré.

## 2. Extrema q2 couplés et préparés pour le census

La [preuve v8/v9 → census v11](q2_coupled/README.md) utilise l'identité
**2P(z)=Σ(2z_j−a_j−b_j)²−|b−a|²**. Proche/loin par axe donne les extrema
exacts sur une boîte continue fermée. i64 suffit en u18/u21/u24, sans rayon,
division ou flottant. Une boîte strictement intérieure devient certifiable
là où les extrema séparés actuels restent ambigus.

Le port doit ajouter un **helper/préparateur de census distinct**, avec
constantes C/S privées et tag 2 garanti par factory. Les fonctions publiques
`power_bounds` et `power_bound_signs` restent conformes : ce dernier promet
les signes des premières bornes, lesquels diffèrent du nouveau certificat.
La garde en unités P/2P protège aussi le minimum demi-entier. Aucun q3/q4
à qmin=2 ne bénéficie d'un faux certificat q2.

**633 gardes exactes**, normal et `-O` identiques. Index médian modélisé,
17 sites, neuf intérieurs, six contacts, deux extérieurs : mêmes I/U et
complete/saturated aux seuils 2/9/10/18. Au seuil 10, 17→13 bornes mais huit
tests ponctuels dans les deux voies. Ce sont des comptes du modèle, aucun
chrono ni gain natif. Les feuilles linéaires du catalogue ne sont pas
accélérées par ce helper de census global.

## Publication et vérification

Les sous-capsules sont copiées **sans mutation**, avec pins, sources exactes,
résultats normal/−O et fermeture par empreintes. `REPLAYS.json` conserve
les rejeux depuis ces copies. `SHA256SUMS` inventorie chaque fichier sauf
sa propre racine ; `python3 -B check.py` et `python3 -B -O check.py`
vérifient aussi les inventaires des deux sous-capsules.
`SOURCE_PUBLICATION.json` confirme les 25 enregistrements de dépendances
à la tête **359d51a6f** : aucun delta de ces sources depuis le pin de lecture.

Une intégration exige ses propres portes natives G4 : tags/certificats,
contacts, cosphères, refus et mémoire, support canonique, compteurs logiques,
sorties FULL entières identiques. Les compteurs de constructions/parcours
réels doivent accompagner l'ablation et le temps total. La qualification
historique d'un code différent ne les remplace pas.

Aucun code produit, build/test natif, fit, workflow ou GCP lancé pour cette
revue. Deux notes actives mises à jour en place ; cinq Markdown actifs
conservés. Sources de la v11 inchangées depuis la contrelecture e02 pour les
modules concernés ; la tête Python nouvellement publiée n'est pas qualifiée
par ces deux propositions. Aucun gain 100/200 ms, GPU ou massif annoncé.
