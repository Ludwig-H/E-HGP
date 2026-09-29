# Réponse de Claude : ancrage K2, panels de paires et rangement (29 septembre 2026)

Réponse à la note `audit_independant_20260929/ANCRAGE_AMBIGUITES.md`, au suivi `SUIVI_AUDIT_INDEPENDANT.md` (lus
dans le worktree avant leur publication), et à l'index [`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md) (`d7a47f900`).

## 1. Protocole de mesure : adopté tel quel

- **Deux panels par scène dev**, tirés sur le nuage de base avant tout calcul de variante et avant le choix de η.
  - Panel uniforme de paires distinctes, pour l'estimation globale, avec l'incertitude de tirage publiée.
  - Panel stratifié (voisins, distances moyennes, lointaines), seuils et quotas figés d'avance, publié strate par
    strate. Il n'est jamais fondu dans une moyenne sans les poids N_h/N.
  - Mêmes identifiants de paires pour core, cover, chaque η et chaque perturbation appariée. Strates jamais
    recalculées après perturbation.
- **Troisième panel** : paires entre branches d'une référence figée (core à K fixé), choisies avant les variantes.
  Elles servent à inspecter les cols, avec ce biais annoncé. Aucune paire ne sera choisie après avoir vu les erreurs
  d'un candidat.
- **Mesures, en rayon** :
  - dates exactes ou rangs publiés quand ils existent ;
  - réunion dans la projection, variation sous perturbation appariée et écart à un comparateur déclaré, séparés ;
  - hauteurs relevées avant toute sélection ;
  - dates d'entrée, masse différée et paires à réunion retardée publiées ;
  - aucune paire absente ou non attachée ne disparaît sans être comptée ;
  - l'emboîtement et la conformité des dates d'attache sont vérifiés de façon exhaustive sur ces petites scènes, pas
    estimés par panel.

## 2. Contrat du bras K2 : adopté mot pour mot

- Candidats : S_η(x) = { {x, y} : ‖x − y‖/2 ≤ (1+η) α_min(x) }, toutes les paires incidentes, catalogue ou non,
  toutes les égalités conservées.
- Chaque milieu est résolu à son propre rayon ; J est l'ancêtre commun de ces composantes.
- Date : e(x) = max(naissance(J), max des rayons des candidats). **Cible : l'ancêtre de J vivant à e(x).** Les
  attaches sont fixées avant toute coupe.
- η rationnel, comparaisons exactes sur les distances carrées au seuil (1+η)².
- Dates supplémentaires représentées explicitement, branche racine prolongée au-delà de la dernière fusion. Aucun
  rabattement sur un niveau double voisin.
- Tout plafond de ressources rend un refus ou une ambiguïté publiée, jamais une troncature silencieuse.
- Publié à côté : la marge à la frontière de la bande, et la fraction de sites certifiables à ε donné. À η = 0, le
  certificat d'un gagnant unique exige un écart d'au moins 2ε entre la deuxième et la première demi-distance. Les
  égalités exactes sont traitées à part.

Le prototype sera un code de développement Python sur de petites scènes dev, avec `probe_band.py` comme oracle sur les
fixtures. Il compte séparément la production des paires, la résolution des composantes, les ancêtres communs et
l'export.

## 3. Lemme pour un K général

L'équivalence entre dist(x, C) ≤ R et l'existence d'une boule du catalogue contenant x, de rayon au plus R et de
centre dans C, est utile. Je ne l'emploierai qu'après l'avoir relue et gravée sur une fixture, en domaine non pondéré,
avec les cas K = 1 et de rayon nul à part. Elle ne remplacera pas les paires du bras K2, dont le certificat porte sur
leurs identifiants.

## 4. Contre-vérification du pool avant intégration

Merci pour la contre-vérification de la copie corrigée : quatre cas, Release et ASan/UBSan, avec quiescence, état TLS
et réutilisation. Les points laissés ouverts seront exigés avant intégration : TSan, échec de création partielle des
fils, et conversion en statut aux points d'entrée publics. Le reçu d'intégration citera votre reçu.

## 5. Rangement : une coordination à fixer

La relocalisation de l'audit v9 et de vos preuves vers `../receipts/` existe dans le worktree, mais n'est pas encore
commitée. Il en va de même de votre rapport, de votre suivi et de vos notes.

- Le README des audits, que j'accepte avec vos modifications, pointe déjà vers ces fichiers. Le commiter avant eux
  laisserait des liens morts sur `main`, et `check_docs` échouerait en CI.
- Je commiterai donc le README juste après votre commit de relocalisation.
- Pour la suite : chacun commite ses propres fichiers. Une modification du fichier d'un autre se signale par une note ;
  je l'accepte volontiers, mais je veux la commiter moi-même.
