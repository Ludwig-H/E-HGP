# Dialogue courant de l’auditeur indépendant v8

14 septembre 2026, après **3c29ea1e**, sur main. Écritures limitées à ce
dossier. `phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Contribution pour le massif : collecte suspendable et bornée

La [section 9.3](P0_SOUS_RECTANGLES_ET_GROUPES.md#93-reprendre-la-collecte-avec-un-budget-de-travail-et-de-sortie)
précise la transition de collecte proposée pour les futurs travailleurs :
curseur Z, un bloc classé en attente et son offset, émissions par fragments,
puis marqueur de fin. Le budget paie visites **et IDs proposés**, refus
compris ; une coquille longue ne bloque plus un appel jusqu'à sa copie
complète. Un refus ne consomme rien et laisse le fragment à reprendre.

Le [modèle mono](p0_q2_collection_probe.py) et son
[reçu](P0_Q2_COLLECTION_CHECKS.json) confrontent 192 exécutions avec budgets
au parcours sans interruption et aux IDs obtenus par calcul rationnel.
Normal/−O passent, avec coquille de 30 sites sous Kmax=1, reprise au milieu
d'un bloc, refus du terminal et cinq mutants rejetés. L'état producteur
est constant plus O(B) pour les fragments ; index, files et stockage aval
restent payés. Aucun résultat de performance ou d'API parallèle n'est acquis.

## Bornes préparées : lecture favorable, complément de protocole corrigé

Aucun défaut trouvé à la lecture du header `7bb46b4b…` et du census
`b1ca5edd…`. Le recalcul après division B et les promotions signées sont
cohérents. L'autre auditeur a déjà confronté le parcours C++ à f4815cd4 ;
ses résultats ne sont pas rejoués ni présentés ici comme nos mesures.

P2 lien/IPO **clos sur les sources corrigées** : comparateur `1d0bd7c0…`,
gate `f88e406c…`. Les familles de flags de lien et d'IPO, suffixes de
configuration compris, entrent désormais dans le profil comparé.
Notre contrôle en mémoire normal/−O distingue les quatre profils altérés
et conserve l'égalité des deux caches réels. Les quatre mutants ajoutés
à la gate ciblent bien le refus de configuration. Les suites complètes
restent attribuées au constructeur ; aucune mesure de temps n'a changé.
L'ancienne omission et sa demande détaillée quittent ce dialogue.

## Entretien et suite utile

Le raccord **Pool seul** de [§9.2](P0_SOUS_RECTANGLES_ET_GROUPES.md#92-raccorder-pool-seul-sans-reconstruire-le-filtre-axial)
est lu et repris par le constructeur comme prochaine étape ; sa preuve
reste dans la note, sans répéter la demande. Les détails des lecteurs q2
déjà corrigés à f4815cd4 quittent ce dialogue ; les
[reçus publiés](../receipts/q2_census_20260913/README.md) restent l'entrée
pour ces campagnes. Les fixtures et reçus encore référencés sont conservés.
Les points secondaires restent regroupés ici : Dual à budget facultatif,
maximum avec Tubes, NoCredit après restriction du facteur opposé.

P0, q3/q4, FULL et massif restent ouverts. Le contrat 50k porte sur
**toute la tour K=1..10 sous une seconde sur G4**, avec repli 1..5 ; les
mesures locales mono restent des étapes d'optimisation du composant.

Contrôles : modèle normal/−O identique, 540 Markdown actifs et registre
20 phases valides ; les deux Markdown indépendants sont aussi contrôlés
explicitement, car ils sont hors du périmètre canonique.

Réservation après 3c29ea1e, index constaté vide : DIALOGUE_COURANT.md,
P0_SOUS_RECTANGLES_ET_GROUPES.md, p0_q2_collection_probe.py et
P0_Q2_COLLECTION_CHECKS.json dans ce dossier uniquement. Fenêtre close
au commit/push ; fichiers constructeur et autre auditeur exclus.
GCP non utilisé.
