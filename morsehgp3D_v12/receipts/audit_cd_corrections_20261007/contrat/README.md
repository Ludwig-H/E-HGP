# Réponse T1 au pin 07ee13ef6

Contre-lecture documentaire de la réponse `07ee13ef6` à notre
[première lecture](../../audit_session_t1_20261007/contrat/README.md).
Le contrat et l'architecture sont épinglés dans `sources.json` ; aucune porte
produit exécutée ni clôture de conformité du catalogue dans cette section.

Les précisions demandées sont bien intégrées au `CONTRAT_CATALOGUE.md` :

- § 1 : catalogue positif explicite, hors naissances des sites au niveau zéro.
- § 2 : J3 commun CPU/GPU accepté sous mesure du catalogue CPU entier et de
  `MES-P` (100 à 10 000 sites, froid/chaud), avec seuil CPU/GPU mesuré et
  révision ouverte de la conception si les objectifs échouent. DFS gelée et
  oracle indépendant restent des témoins ; le repli J3 utilise effectivement
  un domaine arithmétique plus large.
- § 3 : séparation entre la voie historique v11 et les paliers proposés
  s≤16/s17 ; le domaine des seuls supports ne certifie pas les requêtes
  extérieures. Les proportions citées sont celles du développeur, pas une
  nouvelle campagne de cet audit.
- § 4 : comptage exact incluant le repli avant admission, budget du front et
  de la fin d'étage, publication transactionnelle, compteurs logiques une
  seule fois ; les deux passes et tentatives restent des coûts physiques.
- § 6 : bijection par centre exact/rayon carré, populations et niveaux par
  valeur, certification de chaque convention de support, reconstruction des
  ordres et des choix de Kruskal/cover. Le triangle à trois diamètres égaux
  et la condition d'ordre parent identique pour le différentiel des feuilles
  sont repris. Une égalité géométrique seule n'est plus censée suffire.

**CST-0113 et CST-0211 restent en cours à ce pin** : contrat recevable,
lecteur gravé et implémentation transactionnelle encore à qualifier. La
réserve éditoriale `ARCHITECTURE.md` § 4.1 subsiste : « DFS exact pour les
feuilles larges et les tests » contredit sa règle 2 et le nouveau § 2 du
contrat. Aligner cette phrase sur le repli J3 exact plus large. La mention
« M5 en cours » et les questions finales sont aussi antérieures à la réponse
et à la session C ; elles ne sont pas des résultats d'exécution.

Les commits numériques et de format `6a38f7e4b`/`f4a11f49e`, arrivés pendant
cette tranche, dépassent ce pin : aucune qualification de leur code n'est
transférée depuis cette contre-lecture documentaire.
