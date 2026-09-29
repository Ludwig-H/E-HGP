# Têtes multi-K : trois conceptions et un juge (29 septembre 2026)

`public_status=not_claimed`. Travail dev seulement : graines `dev`, réplique 0 pour le sous-ensemble commun (128
scènes), réplique 1 pour le contrôle hors échantillon. Aucun fichier du dépôt n'a été modifié par les concepteurs ; les
prototypes et les caches sont hors dépôt (`build/v10-persist/multik/`).

**Question.** L'audit du 29 septembre a montré que la tête v10-b ne lit qu'une tranche d'ordre K de la tour, et
qu'à même entrée et même tête, la hiérarchie d'HDBSCAN fait jeu égal. Une tête qui exploite l'axe K, seul atout propre
de la tour, fait-elle mieux ?

**Documents :**
- [tranche oblique](TRANCHE_OBLIQUE_20260929.md) (λ-link de Rolle et Scoccola, construite par les verticales) ;
- [persistance à travers K](PERSISTANCE_A_TRAVERS_K_20260929.md) (EOM bi-paramètre, prominence verticale) ;
- [antichaîne à ordres mêlés](ANTICHAINE_ORDRES_MELES_20260929.md) (chaque amas à son ordre, échelle commune par
  contenu de probabilité) ;
- [rapport du juge](RAPPORT_JUGE.md) : recalcul des chiffres, recherche des fuites de la vérité, contrôle hors
  échantillon.

## Verdict

- **Aucune tête multi-K ne bat la tête v10-b à une tranche au sens du lot C** (Δ ≥ 0,02).
  - À 8 000 points, avec b(1,5) et z = 6, les écarts vont de +0,004 à +0,008 ; hors échantillon, de +0,001 à +0,005.
  - Seule l'antichaîne à ordres mêlés (`mixq`) se reproduit : +0,0046 [+0,002 ; +0,008], 12 gains pour 0 perte.
    Son gain vient pour 57 % des coquilles, où les familles ne veulent pas le même z. L'idée s'applique aussi aux
    hiérarchies d'HDBSCAN : elle n'est pas propre à la tour.
- **La tranche oblique** a le fondement le plus solide (stabilité démontrée et vérifiée exactement), mais aucun gain
  de score. On la garde comme objet stable.
- **La prominence verticale** mesure la significativité d'une scission en unités de densité, sans z. On la garde pour
  publier un nombre de modes significatifs.
- **Même un oracle du meilleur z par scène**, qui lit la vérité, ne gagne que +0,02 : la marge restante côté tête est
  faible.

## Les modes des mélanges séparables

Réponse à l'objection de l'utilisateur, mesurée par le juge sur 512 composantes gaussiennes :

- 84 composantes n'ont pas de mode propre dans la vraie densité : aucune méthode de densité ne peut les séparer ;
- 12 ont moins de mcs = √n points ;
- 416 sont séparables. La tête v10-b en retient 378 (91 %), et 99 % de celles dont le col est sous 30 % du pic.
  Aucune n'est retenue au-delà de 70 % : c'est le seuil qu'annonce le bruit de l'estimateur K-NN à K = 10 (écart-type
  relatif ≈ 1/√10).
- Sur les scènes entièrement séparables, les 8 composantes sont identifiées dans 48 scènes sur 60. L'ARI_s vaut 0,886
  contre un plafond de Bayes de 0,948 ; le reste de l'écart est l'affectation de la masse sous le col et du bruit.

**Leviers restants** : l'affectation sous le col, un a priori de taille autre que √n, et des ordres K plus grands.
