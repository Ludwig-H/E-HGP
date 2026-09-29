# Note de Claude : accusé de réception et plan de correction (29 septembre 2026)

Réponse à `audit_continu_20260929/` (commit `57364454b`) et aux constats intermédiaires de
`audit_independant_20260929/`, lus dans le worktree avant leur publication. La réponse détaillée
(`REPONSE_CLAUDE_*`) suivra les correctifs, avec leurs reçus. Aucune nouvelle campagne coûteuse ne sera lancée
avant ces corrections.

## Acceptés, correctifs lancés

Chaque correctif est préparé sur une copie de `57364454b`, accompagné d'une porte permanente, puis vérifié par un
vérificateur adverse avant intégration. Les sondes des auditeurs deviennent des régressions.

| Constat | Correctif prévu |
| --- | --- |
| Exception dans le pool (appelant : ASan ; ouvrier : SIGABRT) | Garde RAII de l'état TLS ; capture de la première exception par travail ; plus aucune tranche lancée ensuite ; fermeture de la capture ; attente de tous les utilisateurs ; relance dans l'appelant ; conversion en statut `resource_exhausted` aux points d'entrée. Tests : appelant, ouvrier, réutilisation. |
| Entrées tronquées acceptées (trois CLI) | Lecteur commun qui refuse toute taille non multiple de 12 et toute erreur de lecture, sur les quatre CLI. Régression pour les restes de 1 à 11 octets. |
| `mhgp10_tower --no-points --dump` : SIGSEGV | Export des attaches seulement si elles existent. Porte sur les trois points. |
| Débordement u32 des indices de boules | Refus `index_overflow_u32` avant l'assemblage, testé par un cardinal artificiel. |
| Taille de feuille inférieure à K | Refus du paramètre avant calcul. |
| Tête : ancêtres remontés pour chaque objet (Θ(C²) sur un peigne) | Propagation descendante en une passe ; porte de comptage sur le peigne ; différentiel des étiquettes. |
| `validate` trop faible avant la tête | Rangs des points, équivalence parents–enfants, niveaux finis et croissants. Fixtures des deux entrées acceptées à tort. |
| `allow_single_cluster` : règle de la racine ; domaine de l'échelle | À corriger avec la tête, après relecture de TETE_BANCS_PREUVES H2 et H3. |
| Juge vertical : image fausse survivante | Contrôle des images à chaque naissance et fusion, par centres témoins rationnels, même si C∩X est vide. Le mutant de l'audit doit être tué. |
| Oracle catalogue partiel | Ajout de S* minimal, des valeurs de niveau, du rang dense depuis 0 et des entrées pondérées. |
| `scale_run.py` : l'enfant survit au délai | Groupe de processus tué et récolté ; JSON natif par appel ; égalité des compteurs entre les deux appels. |
| `decide.py` accepte un lot incomplet | Vérification du manifeste exact du plan. |
| `SiteTree`, centres non bien centrés | Contrat restreint ou repli exact, après relecture de GEOMETRIE_CATALOGUE G1. |

## Faits mathématiques : fixtures permanentes et registre

Les trois contre-exemples exacts deviennent des fixtures permanentes et entrent dans
`docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`, section v10 :

- recouvrement des couvertures (K = 2, 0, 2, 4) ;
- discontinuité de la projection `cover` (0, 999 ou 1 001, 2 000) ;
- croisement multi-K (0, 20, 22, 50, 52).

La borne de stabilité 2ε de `core` sera relue, puis inscrite comme `proved_here`.

## Affirmations corrigées (errata, passation)

J'accepte toutes les corrections de portée. Elles iront dans `receipts/ERRATA.md` et dans la passation, les reçus
restant immuables.

- **Échelle** :
  - octets par boule : 303,6 (et jusqu'à 314,9), non 280 ;
  - unités : 135,28 Gio, soit 145,26 Go ;
  - le seuil « 1,4 M sites LiDAR » est faux, et aucune capacité n'est qualifiée ;
  - « coût constant par boule » est trop fort : la tour passe de 86 à 174 ns par boule ;
  - ≈ 460 boules par site est empirique, non une borne ;
  - le rapport tour/catalogue n'est pas constant.
- **Session 4** : 0,22 à 0,26 s porte sur un ordre K = 5 plus la tête. FULL K = 5 prend 0,204 à 0,254 s (catalogue +
  tour). Une seule séquence (08) ; 24 cœurs et 48 fils. « Contrat tenu » vaut pour ce diagnostic seulement.
- **Bayes/Morse** :
  - « référence MAP à paramètres estimés sur les labels » ;
  - « montée discrète sur densité ajustée, voisinages 20 et 40 » ;
  - retrait de « hors de portée de toute méthode de densité » ;
  - aucune de ces références n'est un plafond d'ARI.
- **J2c** : la stagnation à 9 coupes binaires n'est pas exactement 3 niveaux d'octree. La terminaison se démontre par
  la somme des plafonds des logarithmes des côtés.
- **Tête et pool** : les reçus ne conservent ni les dumps complets, ni les hashes des binaires, ni les journaux TSan
  bruts après correction. Les prochains reçus les conserveront.

## Direction scientifique

J'adopte la séparation proposée entre arbre de densité, affectation des points et sélection d'une partition :

- `core` reste la référence fidèle au modèle, et `cover` le témoin empirique ;
- la prochaine expérience est l'ancrage différé des ambiguïtés, qui attache à l'ancêtre commun ;
- elle se fera sur de petits nuages dev, sans les graines `test_v10b`, et sera jugée sur l'emboîtement, les
  hauteurs de fusion d'un échantillon de paires fixé d'avance, la robustesse, l'affectation, la sélection et le
  coût, mesurés séparément.

Une question pour vous (`QUESTION_CLAUDE` à suivre si besoin) : pour l'ancrage différé, quelle définition des
branches « plausibles » d'un point préférez-vous tester en premier ? Je pense à l'ensemble des composantes vivantes
dont la distance au point est au plus son rayon d'entrée core, à sa date d'entrée.
