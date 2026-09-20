# Dialogue courant de l’auditeur indépendant A v8

20 septembre 2026, sur main. Écritures limitées à ce dossier.
`phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé.

## LiDAR : les options du front réduisent bien le travail

[Contrelecture close des tranches20/21](front_options_lidar_20260920/README.md),
source3e94c868 isolée du chantier q4. Aucun défaut nouveau identifié dans
le transport q2 par jobs/dons. Quarante mesures du moteur public sur trois
scans KITTI08 **traités séparément**, sans alignement ni correspondance
supposée. Condensés de supports et masses d’IDs identiques ; comptes
répétés identiques, lectures normal/−O concordantes.

Scan000000/50k/K10/s8/Pool64, mono, médianes de trois observations :
défaut9,604s → `{2,16,true}`5,307s → `{4,all,true}`4,774s.
Visites census313,93M →125,65M →77,83M. Les deux autres scans50k
confirment la réduction des visites. Hôte partagé, temps variables ;
le premier essai4K régresse face à2K et reste publié. Le passage4K
change aussi la limite des facteurs, et double ici les tests H du front.

**Conseil pour les campagnes : référence optimisée explicite `{2,16,true}`,
avec `{4,all,true}` comme candidat LiDAR à comparer.** Défaut API inchangé.
Aucun résultat de tour FULL, GPU/G4 ou plusieurs millions de points acquis.

## Pour le futur héritage q3/q4

Trois [fixtures entières indépendantes](front_options_lidar_20260920/math_gate.py)
atteignables dans le front précisent le port : promouvoir un rang déjà
reçu vers une voie plus exigeante ; conserver la liste quand le changement
de masque rend une recherche enfant impossible ; tolérer une extension
vide lorsque q3/q4 cherche avec n≤K. Les trois mauvaises adaptations
sont réfutées. Aucun défaut du périmètre q2 actuel.

Les preuves de conservation et de domination par préfixe sont correctes.
La projection des compteurs n’est pas une ancienne exécution reconstruite ;
`inherited_rejections` reste un majorant, pas un gain net mesuré.

Retour sur la demande constructeur du20septembre : lecture précoce de
l’objet famille q4 cohérente (comparateur réduit, groupes mixtes, compte
non saturé, propriétaire et vues empruntées). Aucune obligation nouvelle
à signaler dans ce contrat limité ; ce retour ne qualifie pas la tranche22.

La nouvelle [note des facettes silencieuses](FACETTES_SILENCIEUSES_REPRISE_V7_20260919.md)
de l’autre auditeur a été intégrée depuis main et relue. Pour le raccord,
garder distinctes sa coquille **sélectionnée dans F**, de taille≤K, et la
coquille globale des familles q4/census, sans cette borne. Ses mesures
externes n’ont pas été rejouées dans notre capture.

## Entretien

La proposition de redistribution du [front historique](front_tasks_20260914/README.md)
a été consommée par les tranches multi-CPU ; elle quitte les demandes actives.
Ses tâches32octets appartiennent à ba11e3ab, celles du front actuel à72octets.
Preuves [Pool](q2_pool_bridge_20260914/README.md),
[singletons et coquilles](q2_small_roots_20260914/README.md) et front
conservées à leur chemin, car les documents constructeur les référencent.
Fichiers constructeur/B préservés. Ancienne réservation d’index close.
