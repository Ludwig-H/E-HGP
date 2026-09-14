# Dialogue courant de l’auditeur indépendant A v8

14 septembre 2026, sur main. Écritures limitées à ce dossier.
`phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé.

## Front : point de reprise vérifié, charge à redistribuer dynamiquement

Le [prototype de reprise du front](front_tasks_20260914/README.md), épinglé
à ba11e3ab, conserve exactement rectangles, masques, profondeur et travail
géométrique : 4 320 reprises contre 864 fronts publics, oracle scalaire
sur petits nuages, Release et Clang ASan/UBSan. La tâche non traitée
`{a,b,mask,depth}` occupe ici 32 octets. Les émissions et rejets du
préambule sont conservés ; les témoins restent cherchés dans l’index global.

Les 24 mesures LiDAR portent sur le **front seul, exécuté en série**.
À 50k, préparer 64 tâches laisse 32,97 % des descentes dans une tâche ;
256 tâches laissent encore 29,93 %. Ce gros job coûte 0,962 s, contre
1,47 ms pour le job représentant le plus de paires. La masse initiale
ne suffit donc pas à répartir la charge. Aucun gain parallèle n’est mesuré.

**Prochaine étape proposée : redistribuer les sous-arbres pendants des
DFS locaux**, sans refaire leurs ancêtres ni changer les décisions du front.
Une pile par worker, contexte et index partagés, état/buffers privés.
Si la file globale est pleine, poursuivre localement au lieu de bloquer
les producteurs. Le plafond Q+97W borne les descripteurs en attente,
pas les plans ni les sorties : les coquilles restent non bornées par K.
Le prochain essai devra payer le census, la collecte et les transferts,
et traiter les gros rectangles dont le callback reste indivisible ici.

## Entretien et acquis repris par le constructeur

Le port Pool publié et sa contrelecture B répondent aux points de raccord
précédents ; notre lecture ne révèle pas de défaut nouveau. Les détails
consommés quittent ce dialogue. Les preuves du [raccord Pool](q2_pool_bridge_20260914/README.md)
et des [racines singleton et obligations de coquille](q2_small_roots_20260914/README.md)
restent à leur chemin. Le cache tangent n’est toujours pas implémenté.
Les reçus anciens utilisés comme dépendances ne sont pas déplacés ;
fichiers B, constructeur et complémentaire préservés. P0, q3/q4 aval,
FULL, multi-CPU/GPU, tour50k/G4 et régime multi-millions restent ouverts.

Réservation courte d’index A : ce dialogue et `front_tasks_20260914/`
seulement ; close dès publication du commit correspondant sur main.
