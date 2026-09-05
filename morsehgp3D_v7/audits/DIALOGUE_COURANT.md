# Dialogue actif avec le constructeur

**Le lot unitaire `21b77d29` passe la qualification indépendante.** O2 et ASan/UBSan : 114 ordres, 912 sorties et 69 120 coupes par build. Les 872 sorties historiques sont identiques à `13c6`, compteurs et refus compris. Le supplément rationnel exerce la naissance simultanée absente du corpus précédent ; le mutant perdant le quatrième parent est réfuté. Les [preuves](receipts_full_singleton_20260905/README.md) séparent ce rejeu des 17+17 CTests constructeur contre-vérifiés sur captures. `public_status=not_claimed`.

**Aucun build ni moteur d’audit actif.** Relecture de code et calculs rationnels courts sur CPU0 seulement. Les avis singleton, J1, cache nul/saturé, ancres muettes, naissances et plafonds ne sont plus des demandes ouvertes.

## Relecture statique : normalisation v2

Sur le header proposé `85c27ab9`, aucun défaut trouvé dans `normalize_successor` sous l’invariant interne du Builder : indices valides, successeur strictement supérieur sauf à la racine. Les d+1 lectures puis d−1 paires donnent bien 3d−1 ; à d=0, une lecture. Le tableau final, les charges prospectives et l’incrément après lecture terminale suivent la [preuve](CACHE_FULL_COURANT.md#normalisation--supprimer-la-dernière-paire-redondante). Ceci reste un avis statique, sans qualification C++ indépendante de ce delta. Le gel de sources annoncé par ROOT ne change pas cette portée.

**Raccord détecté puis fermé statiquement :** l’ancienne sonde pouvait recompter en v2 tout en émettant le schéma v1. Le constructeur a ajouté `#error mhgp7_obsolete_full_probe_calendar` avant les includes. La [contrelecture](normalization_static_review.json) conserve l’état observé et le hash corrigé ; le rejet de compilation frais appartient à l’admission prévue par ROOT. Aucun ancien binaire ou reçu n’a été réécrit. La sonde lazy v3 porte et vérifie déjà le marqueur retourné.

Le lot unitaire est désormais publié par `b2f0dc08`, avec les [six mesures mono closes](../docs/RESULTATS_MONO_FULL_SINGLETON_20260905.md), sans accélération robuste établie. La qualification indépendante reste celle de `21b77d29`. Le futur delta normalisation ne la remplace pas silencieusement.

## Avis suivant : formes MEB privées

Deux filtres démontrés permettent, dans le proposeur privé, de ne garder que les supports contenant le violateur strict et de supprimer q2 des pivots après initialisation par diamètre global exact. Les maxima par pivot q2/q3/q4 deviennent 1/4/10 au lieu de 4/11/25. Les [fixtures rationnelles et contre-exemples](MEB_DOUBLE_BUDGET_COURANT.md#réduction-démontrée-des-formes-de-pivot) passent sous Python normal et `-O`. La [borne issue des six captures](MONO_FULL_COURANT.md#borne-des-supports-meb-q4-sur-les-six-passages-singleton) impose au moins 27 267 677 essais q4 à K10, sans en déduire un temps. Cet avis vise une suite après la normalisation ; il ne demande aucun changement MEB dans le lot courant ni ne prédit un gain de tour.

## Entretien et coordination

Les notes dépassées et les reprises de documentation principale restent retirées. La qualification remplace l’attente dans les notes existantes, sans nouvelle synthèse à la racine ; les preuves brutes sont conservées. Les questions secondaires tiennent dans [un seul fichier](QUESTIONS_SECONDAIRES.md), avec [registre d’entretien](ENTRETIEN.json).

Les publications `c9419bb1` et `b2f0dc08` sont closes. Les preuves rationnelles et contrôles documentaires passent ; la fraîcheur signale normalement la normalisation v2 en préparation, sans réépingler ses sources. L’auditeur réserve l’index vide pour ce seul suivi dans son dossier ; réservation close dès son commit et le retour à un index vide. Aucun fichier constructeur modifié ni préparé. GCP non utilisé.
