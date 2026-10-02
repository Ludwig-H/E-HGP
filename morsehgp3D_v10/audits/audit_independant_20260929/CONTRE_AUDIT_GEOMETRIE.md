# Géométrie et raccord — état utile au nouveau développeur

2 octobre 2026. Produit src/cli inchangé depuis `4b7d70422`, lecture
v10 jusqu’à `afb081774`, ouverture v11 `52687f8e5` et final5 R2. `not_claimed`.
Aucun nouveau moteur ou GCP exécuté par cet audit.

| Point | État et obligation de port |
| --- | --- |
| Catalogue / FULL | Les petites preuves Γ, I/U, plateaux et attaches restent celles de leur capture. Conserver toutes les incidences, y compris internes, et les fusions à K+1 ; une couverture commune n'est pas une fusion. [Preuves](ANCRAGE_AMBIGUITES.md). |
| G1 / numérique | Arrondis et filtre FE_TONEAREST contre-vérifiés dans la copie R2 ; cela ne qualifie pas tous les filtres, FTZ/DAZ ou une entrée large. Pas de nouvelle erreur numérique démontrée. [Revue de cette copie](../audit_continu_20260929/catalogue/CONTRE_AUDIT_SITETREE_CORRIGE_20260929.md). |
| R2 | Final5 fermé ; ancien MR1 Pool tué, MR1b distinct équivalent sur le périmètre testé. Épingler les fichiers réellement portés depuis `865f5e6` et requalifier v11. [Provenance actuelle](../../../morsehgp3D_v11/receipts/audit_independant_20261002/provenance_review/README.md). |
| Grande sortie | Vérifier K attendu, ensemble de sites/IDs et tailles annoncées. Un lecteur structurel ne certifie pas seul Γ ; aucun dump massif réellement faux démontré. [Limites des juges](../audit_continu_20260929/CONTRE_AUDIT_R2_20260930.md). |
| u24/u32 | Distance/Morton96, filtre relatif et comparateur restent des briques isolées. Ne pas lever la garde u18 sans propriétaires, constructeurs, nearest, seuils KNN et ordre exact commun. [Précision et capacité](../AUDIT_MASSIF_LIDAR_20260930.md). |

La nouvelle maturité conserve le continuum géométrique : remplacer
C_r par ses seuls centres MEB changerait la règle. Une taille mûre
recouvrante ne suffit pas à une taille exclusive après propriétaire.
[Contrat minimal exact](../../receipts/audit_independant_20261002/maturity_review/README.md).

La bibliothèque actuelle possède des voies certifiées et des replis ;
ne pas transférer leurs tests à un futur port C++ par simple copie.
[Notre contrôle des nouvelles dates](../../receipts/audit_independant_20261002/date_order_review/README.md)
porte sur le helper scalaire capturé, pas sur FULL ou les grandes campagnes.

Anciens contre-tests : [géométrie](../../receipts/audit_independant_20260929/historique/base_6206d1d11/GEOMETRIE_CATALOGUE.md)
et [lecture des copies](../../receipts/audit_independant_20260930/notes_avant_synthese/README.md).
Ils restent consultables sans entretenir leurs statuts anciens ici.
