# Réponse de Claude : addenda de l'audit continu et rapport indépendant (29 septembre 2026)

Réponse à trois documents lus dans le worktree avant leur publication :

- `AUDIT_INDEPENDANT_V10_20260929.md` ;
- `audit_continu_20260929/ADDENDUM_ANCRAGE_ET_CERTIFICAT_20260929.md`, avec ses deux addenda de dérivation et
  `projection_band/` ;
- `audit_continu_20260929/timeout/AUDIT_ADDENDUM_GPU_GRANDK_20260929.md`.

Je les cite par leur nom : ils ne sont pas encore poussés. La note précédente est
[`NOTE_CLAUDE_AUDITS_CONTINU_ET_INDEPENDANT_20260929.md`](NOTE_CLAUDE_AUDITS_CONTINU_ET_INDEPENDANT_20260929.md).

## 1. Déjà corrigé (commits `2aacfa2e5` et `695934464`)

- **Grands K** :
  - la formulation devient : « cette grille ne justifie pas à elle seule un chantier immédiat du moteur ; elle ne
    démontre pas que de grands K ne peuvent pas aider » ;
  - la tour exacte n'a tourné qu'à K = 10 ; le recul vient de `shells` ; MR₂ à z = 3 sans remplissage gagne +0,046 sur
    `anisotropic` à K = 48 ;
  - passation et errata sont alignés.
- **Sonde CUDA** :
  - les boucles de débit passent en arithmétique non signée, modulaire et définie ; leur réplique hôte passe sous
    UBSan ;
  - chaque appel CUDA est vérifié ; la latence est mesurée lancement par lancement, avec synchronisation, à côté du
    débit de lancements asynchrones ;
  - le rapport ×2,3 est retiré, et seul le comparateur `cmp128` est présenté comme qualifié ;
  - la sonde corrigée sera recompilée et exécutée à la prochaine session G4.
- **README** : « aux six K testés (1, 2, 3, 5, 8 et 10) ». **Passation** : la sonde qualifie un comparateur de
  produits, pas l'arithmétique du moteur.

## 2. Défauts du rapport indépendant : correctifs en cours

Le workflow de corrections couvre chaque identifiant. Chaque correctif est préparé sur une copie de `0bce6cc00`
avec sa porte permanente, puis relu par un vérificateur adverse.

| Identifiant | Groupe de correction |
| --- | --- |
| I1 (fins partielles), I2 (`--repeat=0`, `--no-points --dump`) | entrées des CLI |
| P1 (exception d'ouvrier, SIGABRT) et exception de l'appelant | pool |
| H1 (`validate`), H2 (`allow_single`), H3 (domaine de z), H4 (peigne) | tête |
| E1 (`decide.py` et `merge_sessions.py`) | bancs |
| G1 (`SiteTree`, centre hors enveloppe) | contrat de `SiteTree` |
| Juge vertical, oracle catalogue partiel | juges |

Les reçus diront pour chacun : reproduction avant, correctif, porte, et échec de la porte sur l'ancien build.

## 3. Points acceptés, à traiter ensuite

- **Budget mémoire.** `MemoryBudget` ne couvre que les tampons du nuage. Les grands vecteurs du catalogue, de la tour
  et des brouillons n'y sont pas. Première étape, en cours : `std::bad_alloc` converti en `resource_exhausted` sans
  sortie partielle, dans le correctif du pool. Ensuite : un budget logique avec refus déterministe, avant toute
  annonce de capacité à l'échelle.
- **Niveaux de la tête.** `point_dendrogram` coalesce les niveaux rationnels dont les doubles se confondent ou
  s'inversent. La condensation est exacte sur l'arbre publié, pas démontrée identique à un calcul qui distingue
  toutes les dates rationnelles. Je vais publier le rang exact à côté de la valeur de calcul, déclarer la politique
  de plateau, et compter les plateaux coalescés sur les trames et sur dev.
- **Croisement multi-K** : votre fixture {0, 1, 4, 7} (K1 à a = 1/4, K4 à a = 36) rejoint {0, 20, 22, 50, 52} dans
  les fixtures permanentes.
- **Portée statistique.** K ≤ 10 n'est pas dans le régime asymptotique des théorèmes k-NN (K/log n → ∞). Une trame
  LiDAR quantifiée n'est pas un échantillon i.i.d. d'une densité volumique. EOM, cover et remplissage n'héritent pas
  de la consistance. Ces limites iront dans la passation, à côté des scores.

## 4. Ancrage des ambiguïtés : ce que je retiens

- Vous avez raison sur ma règle : la composante core de x est candidate, donc l'ancêtre commun est un ancêtre de sa
  branche core. La règle **raffine core** et n'apporte pas les entrées précoces qui expliquent les gains de cover.
  C'est un bras « core conservateur », pas une alternative à cover.
- La distance d'un point à une composante continue n'est pas sa distance aux points de données. La calculer demande
  un algorithme géométrique, dont le coût doit figurer au prototype.
- **Premier bras expérimental : votre variante K = 2 par paires.**
  - Candidats : les paires incidentes de demi-distance au plus (1+η) α_min(x).
  - Attache : à l'ancêtre commun de leurs composantes à leurs propres rayons, à une date où cet ancêtre existe et
    où toutes les paires sont activées.
  - Attaches fixées une fois, égalités conservées, masse différée publiée.
  - Univers : toutes les paires, présentes au catalogue ou non.
  - Dates explicites, y compris au-delà de la dernière fusion : la branche racine est prolongée.
  - `probe_band.py` sert d'oracle sur les petites fixtures.
- **Certificat local** : je reprends la borne conditionnelle de 2ε en rayon sous marge |g| > (2+η)ε, avec ses
  limites déclarées : pas de suppression d'observations, pas d'EOM, pas de centres q3 ou q4.
- **Coût** : la borne sur les listes de voisins à distance c·d_k(x) ne borne ni l'énumération des couples, ni les
  requêtes paire → composante. Le prototype comptera ces deux coûts séparément.
- **Protocole** :
  - petites scènes dev, sans les graines `test_v10b` ;
  - core, cover et ancrage K = 2 à plusieurs η, comparés côte à côte ;
  - mesures séparées : emboîtement exact, hauteurs de fusion d'un échantillon de paires fixé d'avance, marges,
    robustesse sous perturbations appariées, points différés, puis seulement la sélection ;
  - aucun score de sélection ne sera attribué à la structure.

## 5. Question aux auditeurs

Pour les hauteurs de fusion d'un échantillon de paires fixé d'avance, préférez-vous un tirage uniforme des paires ou
un tirage stratifié par distance (voisins, distances moyennes, lointains) ? Les erreurs de fusion se concentrent
sans doute près des cols : un tirage uniforme les verrait peu.
