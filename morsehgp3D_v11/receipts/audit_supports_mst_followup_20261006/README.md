# Réparation de la sélection MST et de sa qualification

Complément à `audit_supports_mst_20261006`, sur le WIP développeur fondé
sur `9eee2ed4b`. Les captures précédentes restent immuables. S* seul et
le format SPv2 restent les choix de sortie demandés par l'utilisateur.

Les propositions traitent trois niveaux distincts :

- [Sélection native](native/README.md) : garder les naissances et les
  boules qui réalisent une union dans le Kruskal compressé du constructeur.
- [Différentiel exact](mathematics/README.md) : sélectionner l'attendu dans
  l'ordre du catalogue, puis comparer la sortie native sans la filtrer.
  Le reçu démontre aussi pourquoi la coquille centrale25 de la fixture
  doit être absente à K1/K2.
  Le [patch actualisé](mathematics/remaining_1054/README.md) préserve la
  correction de cette coquille déjà réalisée par le développeur à 10:51.
- [Contrôle du lecteur](../audit_supports_mst_20261006/reader_spanning_proposal/README.md) :
  refuser les boules entièrement redondantes et les enfants déconnectés.
  Le [contrôle de version](../audit_supports_mst_20261006/formats_fix_proposal/README.md)
  reste une garde complémentaire, sans décider la rétrocompatibilité.

Les deux patches du lecteur s'appliquent ensemble ; leur porte autonome
à trois fichiers synthétiques passe en Python normal et `-O`. Ce contrôle
structurel ne sélectionne pas les candidats omis ; le différentiel exact
remplit ce rôle. Les patches sont proposés, sans modification des sources
du worktree développeur.

Le [contrôle du compteur API](api_route/README.md) est déjà corrigé dans
le WIP du développeur : les naissances publiées ne figurent pas dans le
journal des cellules. Le reçu vérifie cette correction sur le triangle K2,
sans proposer un patch devenu inutile.

Aucun build ni produit natif, campagne G4 ou octet LiDAR exécuté ou copié
par cet audit. La qualification C++ reste à effectuer sur G4 après
intégration ; les modèles et contrôles de source ne la remplacent pas.
