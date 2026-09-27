# Préflight mutable du constructeur événementiel A

27 septembre 2026. Ces essais de mise au point ne sont pas la qualification.
Build non autoritaire : `/workspaces/E-HGP/build/v9-audit-full-a-events-preflight-20260927`.
Le moteur et les captures closes des autres prototypes restent inchangés.

La première compilation GCC13.3 Release a échoué avec code2 sur
`-Werror=sign-compare`, dans la borne de tours de doublement :
`u64 continuation_rounds` était comparé à `std::bit_width(...) + 1`,
dont le résultat est signé. Correction : conversion explicite en u64.
Ce n'était pas une réponse géométrique incorrecte ; aucun test n'avait
encore pu s'exécuter. L'échec n'est pas remplacé par un succès silencieux.

Une contrelecture indépendante a également demandé de distinguer les
capacités des temporaires de celles des sorties qui coexistent. Les deux
maxima **observés** et la capacité finale de sortie sont désormais séparés.
Ils excluent l'entrée, les piles de tri, la surcharge de l'allocateur et
les coexistences internes de réallocation ; ce ne sont pas des pics RSS.
