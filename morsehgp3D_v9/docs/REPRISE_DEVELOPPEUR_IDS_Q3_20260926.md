# Reprise du développement — transport exact des intérieurs q3

26 septembre 2026, demande utilisateur : « Prends la place du développeur ;
tu as feu vert pour GCP G4 ». Cette demande remplace le rôle d'auditeur
seul pour cette tranche. Chantier sur `main`, pas de nouvelle branche.

## Base préservée

Le lot d'audit `fce85d823` est publié. Les quatre prototypes et leurs
captures restent inchangés ; les conclusions sont dans
[`AUDIT_B_RACCORD_PAYLOAD_CATALOGUE_20260926.md`](../audits/AUDIT_B_RACCORD_PAYLOAD_CATALOGUE_20260926.md).
Le travail v29 du précédent développeur (`3cf62b8ca`, sonde multitrames)
a été repris par cherry-pick, sans toucher à son worktree ni à ses
modifications v6 étrangères à cette tranche. Ses correctifs non publiés
du worker/autotest sont relus et repris explicitement avant v30.

Profil du contrat : grille entière 1 mm, FULL explicite K1..5 sans sol
prioritaire, G4 SPOT ; K10, trames brutes, s8/10/12 et croissance restent
des comparaisons séparées. Aucun nouveau contrat acquis par ce chantier.

Cadre : `phase=exploration_v9_hors_registre`,
`backend=cpu_reference` pour les portes locales, `cuda_g4` seulement pour
les exécutions CUDA reçues ; `profile=quantized_u18_input_only`,
`mode=implementation_q3_interior_payload`, `public_status=not_claimed`.

## Première implémentation

Levier `q3_interior_payload`, désactivé par défaut tant que son gain net
n'est pas mesuré. Trois éléments inséparables :

1. Conserver les IDs originaux pendant le census q3 existant, sans nouveau
   test géométrique. Sidecar possédé, `LaneRecord` restant à 128 octets.
2. Suivre exactement les placements, tâches, reports, staging, compactage
   et permutations de chaque record jusqu'à la présentation canonique.
3. Importer les boules régulières à partir de ces IDs complets et des
   comptes certifiés du producteur ; garder le census global pour q4,
   les coquilles étendues et les replis CPU sans paquet.

Le coût de collecte, synchronisation, mémoire, transfert, conversion et
import reste dans le temps de chaîne. La raison publiée doit distinguer
census global indépendant et validation locale du paquet ; pas de faux
sceau annonçant un second census absent. Le juge existant des voies
s'étend aux IDs et à la `BallData` comparée au census global.

La confiance reste celle du producteur interne : une liste accompagnée
d'un compte arbitrairement falsifié n'est pas un certificat externe de
complétude. Le nombre d'intérieurs et la taille de coquille sont certifiés
par le parcours q3 existant ; les IDs importés sont distincts et leurs
puissances revérifiées. Une coquille de trois supports et exactement autant
d'IDs que d'intérieurs suffit alors. Les paquets q4 ne sont pas importés.

## Vérifications avant la dépense G4

- CPU : objets/IDs/comptes exacts contre témoin, K2/3/5/10, coquilles
  supplémentaires, permutations, rejet après préfixe, limites de chunks,
  capacités/différés, voie fusionnée et non fusionnée ; mutants causaux.
- Catalogue/FULL : boules et tours explicites identiques, sceau activé
  et désactivé, q2 précoce, erreurs de plus petite clé, Euler et verticales.
- Release puis ASan/UBSan dans des builds neufs ; aucune réutilisation
  en écriture des builds de captures antérieures.
- Protocole G4 commité, préflight natif et lecteurs normal/−O ; chaque
  trame résidente publie statut et trois condensés, pas seulement celui
  de la tour.

La prochaine séance compare le levier ON/OFF sur trames entières et au
témoin moteur, en alternant les bras et en publiant aussi les régressions.
Elle utilise le contrôleur existant, les arrêts ciblés et la relecture
TERMINATED de la génération exacte. L'autorisation G4 n'implique ni
lancement avant ces contrôles, ni campagne complète sur tous les anciens
leviers. La VM fixe a été relue TERMINATED au début de cette reprise.

## Suite structurelle

Après la mesure du raccord, porter q4 par lentilles + masques T1, puis le
front GPU compact et la construction événementielle FULL de tous les K.
La voie q4 triée reste une option pour les longues listes difficiles :
son test dense montre qu'elle peut perdre contre le rejet précoce.
Les améliorations doivent être mesurées sur le travail complet et les
coupes spatiales LiDAR ; la borne d'une primitive ne qualifie pas la
croissance de toute la chaîne.
