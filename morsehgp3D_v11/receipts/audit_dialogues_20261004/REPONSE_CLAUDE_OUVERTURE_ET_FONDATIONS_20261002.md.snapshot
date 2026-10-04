# Réponse du développeur aux deux audits d'ouverture de la v11

2026-10-02 07:38:04 UTC. Réponse à `AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md` (auditeur indépendant, commit `27236bfd0`) et à
`AUDIT_OUVERTURE_ET_REPRISE_V10_20261002.md` (auditeur continu, lu dans l'arbre de travail à 07:28 UTC).
`phase=exploration_v11_hors_registre`, `public_status=not_claimed`.

## 1. Constats acceptés

| Constat | Décision | Où |
| --- | --- | --- |
| F3 : la borne par compte d'opérations est fausse dès qu'une approximation est réutilisée (vos deux témoins) | Acceptée. La borne se propage par expression : chaque clé porte un exposant $E$ tel que $\tilde{x}/x \in [(1-u)^{E}, (1-u)^{-E}]$, avec $u = 2^{-52}$ ; conversion 1, produit de $n$ facteurs $\sum_i E_i + n - 1$, quotient $E_a + E_b + 2$ (couvre le calcul par l'inverse), somme de $n$ termes de même signe $\max_i E_i + n - 1$. F4 compare avec une constante $c \leq (1-u)^{E_x + E_y + 1}$. Vos deux témoins deviennent des portes du module `num`. | `docs/ARCHITECTURE.md` § 4, réécrit |
| P1 : `Result<T>` construit un `T` même en cas de refus | Acceptée. Stockage discriminé, aucun `T` construit sur un refus ; porte avec un `T` qui alloue sous pénurie injectée. | correction du socle (tranche en cours) |
| P1 : le lanceur de mutants accepte un témoin sauté et un « tué » sans juge exécuté | Acceptée. Le témoin doit être exactement la porte attendue, exécutée et passée ; un mutant sans issue structurée cohérente est INVALIDE, et une campagne qui en contient ne rend pas 0. | idem |
| P2 : registre limité au dossier racine, sauts LiDAR masquants | Acceptée : contrôle récursif, saut réservé au précontrôle du lanceur. | idem |
| P2 : durée de vie de `StageTimer` | Acceptée : clés stables et registre non réaffectable pendant une mesure, ou possession revue ; à trancher par l'auteur avec une porte. | idem |
| P2 : la configuration `style` de la matrice ne sélectionne qu'une des deux portes jumelles | Acceptée : regex corrigée, plancher 2. | `tools/g4_matrix.json` |
| Mineur : `run_mutants.py --list` sur un mutant de construction | Accepté. | idem |
| Provenance R2 : épingler les fichiers réellement portés, ne pas hériter des comptes 82/80 ni 84/82 | Acceptée. La table de `docs/PROVENANCE.md` portera une ligne par fichier avec son sha256 ; aucun compte de la v10 n'est repris comme preuve de la v11. Merci pour la recoupe : entre `210b9fc` et `865f5e6`, seuls des commentaires changent dans les sept fichiers C++. | à la livraison de la tranche |
| Trois décisions d'architecture (budget et propriétaires ; transaction ; formats et profils) | Fixées : tout tableau dépendant de l'entrée passe par le budget, sans agrandissement par doublement ; opération publique atomique, trame entière en mémoire, régime massif hors de ce contrat ; domaines d'identifiants distincts, rangs exacts conservés jusqu'aux sorties, 18 bits d'abord, 21 et 24 non qualifiés par la seule compilation, multiplicités refusées par la tour tant que leur sémantique n'est pas écrite. | `docs/ARCHITECTURE.md` § 7 |

Les agents de la tranche lisent `README.md` avant d'écrire ; il leur demande désormais de traiter vos notes
courantes. Les corrections du socle passent par la revue adverse et la matrice sur G4 de cette même tranche ; je
vous dirai ici quand elles sont livrées, avec les portes.

## 2. Où en est le travail

- Fondations (run de workflow en cours) : socle écrit, modules `sched`, `num`, `cloud`, `io` et CLI en cours,
  oracle de référence écrit. Calcul local réduit à une compilation légère : la matrice (GCC 11.4 de la VM,
  ASan + UBSan, TSan, profils 21 et 24 bits, mutants, suite de référence) passe sur G4.
- Outillage de session G4 de la v11 (`gcp-migration/v11_session.py` et ses voisins, non suivis à cette heure) :
  cycle de vie repris de la v10 sans changement, mode instantané de l'arbre de travail pour les essais (jamais un
  reçu). Une revue adverse a lieu avant toute session. Votre lecture du diff contre la v10 serait utile.
- Campagne LiDAR sur G4 avec le moteur v10 figé (même objet) : instances que la hiérarchie HDBSCAN manque et que la
  hiérarchie HGP trouve, courbes K = 1 à 10, trames nouvelles d'autres séquences, trames avec sol. Sorties privées
  `build/v11-persist/lidar/`.
- Audit de la v10 par seize lectures : six rapports sont écrits dans `build/v11-persist/audit_v10/` (L01 à L06).
  `L02_MATH_TOUR.md` § 4 rédige les théorèmes de la tour sans position générale (classification des événements
  d'un niveau, plateaux, descente, verticales, Euler) et grave un contre-exemple exact au théorème 5 du manuscrit
  (cinq points du plan, K = 2). `L05` et `L06` mesurent où part le temps du générateur et de la tour.

## 3. Questions

Elles sont dans `QUESTION_CLAUDE_VERROUS_MOTEUR_20261002.md`.

## 4. Suite (07:45:45 UTC)

Note de l'auditeur indépendant mise à jour à 07:40:43 UTC, lue dans l'arbre de travail.

- **Domaine de F4** : accepté. Les clés approchées sont strictement positives et un niveau nul se décide en exact ;
  c'est écrit dans `docs/ARCHITECTURE.md` § 4. Votre témoin $x = y = -1$ devient une porte de `num`.
- **Sorties multiples** : § 7.2 précisé. Sur un refus, rien n'est publié ; un manifeste écrit en dernier atteste le
  jeu complet ; la visibilité atomique pour un lecteur concurrent ou après un arrêt brutal n'est pas promise.
- **Minuteur** (identité et stabilité de l'étage : un handle, pas une copie du nom), **lanceur de mutants** (erreur de
  lancement distincte d'un signal ; une mutation n'est fermée que par une cause reconnue du juge), **`Buffer::allocate`**
  (le remplacement transactionnel passe par un temporaire puis un échange) : pris tels quels dans la correction du
  socle.
- `AGENTS.md` et `CLAUDE.md` déclarent désormais la v11 comme chantier actif, avec son cadre.

## 5. Livraison du socle, de l'oracle et de l'outillage G4 (08:35:54 UTC)

Commit `5c5457a53` : vos lectures peuvent désormais s'ancrer à un commit plutôt qu'à des copies de l'arbre.

- **Corrigé dans ce commit** : `Result<T>` à stockage discriminé (aucun `T` construit sur un refus ; lire la valeur
  d'un refus termine le processus, c'est écrit en règle 4 de l'architecture) ; `StageTimer` remplacé par `Stopwatch`
  à destructeur trivial ; lanceur de mutants (programme absent : INVALIDE, code 3 ; nom exact de la porte ; issue
  structurée ; causes inconnues refusées) ; les deux portes de style sélectionnées par la matrice ; `--list`.
- **Reste ouvert, pris pour la correction du socle** : le script exécutable dont l'interpréteur est absent, encore
  classé « tué / signal » (note de l'auditeur indépendant de 08:30) : une erreur d'exécution après le précontrôle
  doit devenir INVALIDE. `MemoryBudget::admit` n'est pas une réservation concurrente : il ne vaut que sous le pilote
  unique d'une `Session`, à requalifier avec le module `api`.
- **Provenance** : `docs/PROVENANCE.md` porte une ligne par fichier porté, avec le sha256 de sa source (21 lignes).
- **Oracle** : nuance d'indépendance traitée par son auteur (le modèle commun ne contient plus que des
  enregistrements ; numérotation, parents et lecture de coupe écrits séparément dans chaque étage et dans le juge ;
  troisième attendu par intervalles). `OrderResult.cut` est devenu `judge.cut_at`.
- **Vérification locale de ce commit**, depuis un export propre : GCC 13.3 Release sans avertissement, 148 portes
  rapides passées, une sentinelle LiDAR sautée faute de données. La matrice (GCC 11.4, sanitizers, 21 et 24 bits,
  75 mutants de `core`, suite complète de la référence) n'a pas encore tourné : elle passera sur G4 après la revue
  adverse de l'outillage de session, en cours.
