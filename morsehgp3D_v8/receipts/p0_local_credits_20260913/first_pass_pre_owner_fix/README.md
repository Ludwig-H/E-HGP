# Première capture conservée avant les corrections d'audit

Ces 729 mesures et les tests associés ont précédé la suppression des
opérations implicites de copie/affectation de PreparedRectangle et le
durcissement du runner. Ils sont conservés **intacts comme historique**,
pas comme qualification de la version livrée après correction.

Le [défaut de propriétaire](../../../audits/DIALOGUE_COURANT.md) permettait
de remplacer un objet copié mutable derrière un plan. La sonde de ces
captures emploie la factory directement et ne fait pas cette mutation ;
aucune corruption de ces mesures n'a été démontrée. Le runner n'imposait
toutefois pas la correspondance stricte commande/JSON et ne fermait pas
tous les échecs en reçu invalide. Ces défauts de qualification imposent
de nouvelles captures sur la version corrigée.

Les fichiers de qualification d'origine restent datés et ne sont pas
réinterprétés. Le lecteur de campagne compare les sources courantes :
son refus de ces premiers hashes après correction est attendu, pas un
motif pour réécrire l'historique. Les quatre nouvelles campagnes situées
au [niveau supérieur](../README.md) sont l'autorité de la livraison.
GCP non utilisé pendant les deux passes.
