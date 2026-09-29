# Captures du contre-audit des bancs corrigés

Le harnais propre `record.py` lit les six scripts figés de `source/` et
crée un **nouveau** dossier de sortie. Ne pas le rejouer dans un dossier
de capture existant. Aucun nuage ou clustering calculé : les valeurs CSV
sont des fixtures de validation, certaines volontairement impossibles.

- `normal_r2/` et `optimized_r2/` : neuf commandes chacune, codes et sorties,
  hashes de fermeture, plans/CSV et décision de fixture. Les tests du délai
  emploient de vrais processus, tous arrêtés et récoltés par la porte relue.
- `closure.json` : empreintes des deux reçus et du harnais, résultats communs.
- `first_fixture_failure/` : erreur de préparation du premier essai conservée,
  sans la transformer en défaut produit ni qualification close.
- `developer_observed/` : quatre documents/logs terminés copiés du développeur,
  notamment ses 11/11 gates. Ces longues commandes ne sont pas les nôtres.

Notre [note courante](../../../audits/audit_continu_20260929/timeout/CONTRE_AUDIT_BANCS_CORRIGES_20260929.md)
distingue les deux correctifs confirmés du nouveau trou de domaine des ARI.
Aucun moteur, autre audit, préenregistrement exécuté ou reçu historique
n'a été modifié ; GCP non utilisé.
