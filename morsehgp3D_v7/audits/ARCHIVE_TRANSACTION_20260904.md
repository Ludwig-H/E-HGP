# Archive v7 : publication et format courants

Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

L’archive publie seulement sur une destination absente, par
`renameat2(RENAME_NOREPLACE)`. La confirmation de synchronisation du parent
reste distincte de la publication : son échec après renommage ne transforme
pas une archive publiée en refus. Tous les échecs antérieurs interdisent
la publication. Le cycle de vie refuse les écritures après commit et
l’abandon d’une écriture interdit de publier ensuite un préfixe d’ordres.

Le nettoyage du correctif intégré utilise les descripteurs possédés et les
noms bornés du format. La [revue courante](REVUE_NETTOYAGE_ARCHIVE_COURANT.md)
explique ce chemin et ses limites OS ; le [retour de qualification](RETOUR_ARCHIVE_COURANT.md)
porte la probe et les portes du delta. Ces liens font autorité pour la
source, les commandes et les résultats, sans recopier un ancien diagnostic.

Le [rejeu indépendant des interfaces](AUDIT_INTERFACES_20260904.md) teste
26 scènes CLI et six corruptions rescellées sur le nouveau CLI. Le lecteur
contrôle types, champs, identités, digests, sémantique et structure des
forêts ; le juge par ensembles reste indépendant de son DSU.

Une archive intègre et structurellement valide n’est pas un certificat
mathématique de complétude, une verticale, un checkpoint de reprise ni une
garantie après coupure électrique. Le contrat 50k et l’échelle massive
conservent leurs preuves et mesures séparées.

Toutes les écritures de qualification sont dans `audits/`. GCP non utilisé.
