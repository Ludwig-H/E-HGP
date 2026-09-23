# Vérification adverse des constats de l'audit C

Workflow `wf_41a66a86-f5a`, base lue `0125dc18`, recoupée jusqu'aux têtes
`origin/main` du 23 septembre 2026 matin. Chaque constat des neuf lectures
([`../lectures/`](../README.md)) a été soumis à un ou deux vérificateurs
adverses : angle **code** (le constat est-il vrai dans le code et les reçus ?)
et angle **portée** (est-il nouveau, ou déjà signalé par A, B ou le
développeur ? sa gravité est-elle juste ?). Consigne : réfuter par défaut en
cas de doute.

- 83 constats, 129 contrôles : 29 « confirmé », 50 « plausible » (vrai en
  partie, avec correction), 46 « déjà traité » (signalé ailleurs), 4
  « réfuté ».
- `verdicts.json` : pour chaque constat, la gravité initiale, le titre, et
  pour chaque contrôle le verdict, la gravité corrigée, la reformulation, la
  raison et les preuves citées.
- Intégration : § 6 et § 3.1 de
  [l'audit principal](../../AUDIT_C_OBJET_ET_RECONSTRUCTION_TOUR_20260923.md),
  révision 2. Les réfutations de C1-02 (invariants de réception déjà imposés
  par la tour : racine unique par ordre, naissances K1) et de F9-08 (une
  palette H_a ne rouvre pas la fenêtre fermée en v8 : chaque candidat est
  retesté par `universal_witness`) retirent ces deux constats. Celle de
  L4-02 corrige l'audit : sous le contrat, la tour refuse une boule omise
  d'ordre haut +u\leq K_{\max}$ (voir
  [`../../c_omission_20260923/`](../../c_omission_20260923/README.md)).

Les verdicts sont des lectures d'agents, pas des preuves : un « confirmé »
motive une correction, il ne certifie rien.

| constat | gravité initiale | verdicts (code/portée) | gravité corrigée | titre |
| --- | --- | --- | --- | --- |
| C1-01 | moyenne | plausible/plausible | basse/basse | L'extension non régulière FULL est utilisée sur 100 % des trames du contrat mais n'a aucune entrée au registre ; le README la dit « prouvée » |
| C1-02 | moyenne | plausible/refute | basse/info | Invariants globaux d'objet absents du validateur de réception G4, alors qu'ils sont gratuits et toujours vrais |
| C1-03 | basse | confirme | basse | Séparation des statuts exact_full_regular / exact_full_quotient_certified / refus : acceptée mais non implémentée |
| C1-06 | basse | plausible | basse | Oracle différentiel indépendant à échelle intermédiaire : diagrammes H0 par K via mosaïques d'ordre k ou rhomboïdes |
| C1-07 | basse | deja_traite | info | Multiplicités : la spec compte les distances avec multiplicité, la v9 refuse les doublons ; sans effet sur les trois trames |
| C-L2-01 | moyenne | confirme/deja_traite | moyenne/basse | Aucune détection d'omission q2 à l'échelle LiDAR ; trois juges peu coûteux sont disponibles |
| C-L2-02 | moyenne | plausible/deja_traite | moyenne/basse | q2 seul prend 0,41 à 0,84 s à K10 sur G4, et doubler les fils (24→48) n'apporte que ×1,07 à ×1,28 |
| C-L2-03 | moyenne | confirme/confirme | moyenne/moyenne | Le grand-livre q2 n'est pas publié : trois compteurs sur plusieurs dizaines disponibles |
| C-L2-04 | basse | deja_traite | basse | Le levier {4,all,true}, favorable sur LiDAR en v8, n'est pas mesuré en v9 |
| C-L2-05 | basse | plausible | info | Ordonnancement q2 figé (Coarse, 16 jobs par worker) ; Donate prévu au retrait sans mesure à W48 |
| C-L2-06 | basse | plausible | basse | Charge utile q2 calculée puis jetée par la chaîne, puis recalculée par le recensus |
| C-L2-07 | basse | confirme | basse | Code q2 hérité compilé sans usage, nœuds d'index non réservés, arbre de plages construit pour rien |
| C-L2-08 | basse | deja_traite | basse | Preuve de complétude q2 dispersée dans les notes v8 : aucun énoncé consolidé en v9 |
| C-L2-10 | basse | plausible | basse | Deux index spatiaux construits en série : 20 à 27 ms avant toute géométrie |
| L3-02 | moyenne | plausible/plausible | basse/basse | La preuve de voie morte sur cover complet est au mieux neutre : elle échoue sur 73–81 % des voies qu'elle traite |
| L3-04 | moyenne | plausible/plausible | basse/basse | Graines traitées par feuille d'atlas : toute graine dont la droite coupe une feuille appartient à sa frontière |
| L3-05 | basse | confirme | info | Garde-fou : les frontières du prouveur de voies mortes ne sont pas des fragments exacts (sites min==0 écartés) |
| L3-06 | moyenne | deja_traite/deja_traite | basse/basse | Aucune porte d'omission à l'échelle d'une trame pour les certificats v9 (cœur, cover, cache, feuille, saturation) |
| L3-07 | basse | plausible | basse | Trous de couverture par mutants : propriété, canonicité q4, disque q4 du prouveur, élagage des graines, domaine Positive |
| L3-08 | basse | deja_traite | info | Bornes 18 bits q3/q4 vérifiées ; commentaires périmés toujours présents ; garde 2^117 sans marge |
| L3-09 | basse | deja_traite | basse | Aucune entrée q3/q4 v8/v9 au registre des preuves |
| L3-10 | basse | plausible | basse | Atlas et balayage q4 lisent les coordonnées par indirection order[rank]→points[id] alors que spatial_points() existe |
| L4-01 | moyenne | plausible/plausible | basse/basse | La porte T2 de chaîne ne juge que K_max=10 : élagage presque vide sur 12–14 sites, K5 jamais jugé par oracle |
| L4-02 | moyenne | refute/deja_traite | info/basse | Aucun juge d'omission à l'échelle ; la tour ne détecte qu'une partie des omissions et aucune à K=1 |
| L4-03 | moyenne | confirme/confirme | moyenne/moyenne | Les gardes de recoupement de la chaîne n'ont aucune porte causale |
| L4-04 | moyenne | plausible/plausible | moyenne/basse | Plomberie de la chaîne à 0,70–1,00 s à K10 (R7b), dont 182–430 ms non attribués et un recensement alourdi par du travail série |
| L4-05 | basse | plausible | basse | Refus typés : des incohérences internes sont publiées invalid_input |
| L4-06 | basse | confirme | basse | run_tower=false publie complete_relative sans positivité vérifiée des coquilles régulières ; l'en-tête surestime le recoupement q_min |
| L4-07 | basse | confirme | basse | Raison d'échec non déterministe quand plusieurs clés échouent dans la fusion ou le recensement |
| L4-08 | basse | confirme | basse | Temps payés perdus sur exception pour prepare, gen_index, q2, q34 et tower_index |
| F5-01 | moyenne | plausible/plausible | basse/basse | Phase A séquentielle de K10 : accès dispersés au catalogue de 224 o par bloc et par facette, dont un contrôle d'antériorité redondant |
| F5-02 | moyenne | plausible/plausible | basse/basse | Niveau exact de 48 o recopié dans chaque nœud, historique et contribution, alors qu'un rang u32 exact suffit après le tri global |
| F5-03 | moyenne | plausible/plausible | basse/basse | Brouillons en vecteurs imbriqués recopiés en CSR, banque de lignes à deux vecteurs ; sur LiDAR, contributions = naissances dans 60/60 exécutions G4 |
| F5-04 | moyenne | plausible/plausible | moyenne/basse | La phase A de K=Kmax, la plus longue section séquentielle, ne démarre qu'après la phase 0 de tous les ordres |
| F5-05 | moyenne | deja_traite/deja_traite | basse/basse | Compteurs FULL calculés mais non publiés, et aucun chrono par phase de la tour |
| F5-06 | moyenne | plausible/plausible | basse/basse | FULL ne détecte une omission de catalogue que si la boule omise est atteinte comme terminal de descente |
| F5-07 | basse | deja_traite | info | La tour revalide intégralement un catalogue que la chaîne vient de recenser exactement |
| F5-08 | moyenne | plausible/plausible | basse/basse | La couture GPU de la phase 0 (FullBallBatchResolver) est incompatible avec les ordres K parallèles ; la proposition MEB peut passer en FP32 |
| F5-09 | basse | plausible | basse | Images verticales séquentielles par ordre : la requête est un ancêtre pondéré, parallélisable |
| F5-10 | basse | plausible | info | Classification d'exceptions : des invariants internes de ShellTable sont rendus comme kInvalidInput, et std::out_of_range n'est pas capturé par l'API FULL |
| F5-11 | basse | plausible | basse | README : « définie et prouvée en v7 » surévalue le statut de preuve de FULL |
| C6-01 | haute | confirme/deja_traite | haute/moyenne | CI v9 rouge depuis 129 exécutions consécutives : le selftest de protocole appelle git rev-parse HEAD~1 sur un clone superficiel |
| C6-02 | haute | confirme/deja_traite | moyenne/moyenne | Aucune porte à la taille d'intérêt ne juge la complétude du catalogue ; les labels scale8000/16000/32000 recouvrent un diagnostic à une arête |
| C6-03 | moyenne | confirme/confirme | moyenne/basse | Le juge Γ de bout en bout ne tourne qu'à K=10 sur 12–14 sites : les leviers d'élagage y sont géométriquement inertes et le repli K5 n'est jamais jugé contre Γ |
| C6-04 | moyenne | confirme/plausible | moyenne/basse | Les 18 points d'injection MHGP9_MUTANT du chemin produit ne sont activables par aucun test ; le registre compte 132 noms pour 18 sites |
| C6-05 | moyenne | confirme/confirme | moyenne/moyenne | Aucun mutant produit sur le cœur FULL, sur la recoupe du catalogue de la chaîne ni sur la voie q2 |
| C6-06 | moyenne | deja_traite/deja_traite | basse/basse | Le refus « coquille > 12 » de la chaîne et ses contrôles internes de recoupe ne sont jamais exercés |
| C6-07 | moyenne | plausible/plausible | basse/basse | Concurrence et équivariance : pas de TSan/ASan en CI, au plus 8 fils pour la chaîne et la tour, aucune permutation de la chaîne au-delà de 30 sites |
| C6-08 | basse | confirme | basse | Mutant désactivé admitted_lane_recounted_in_children : réactivable immédiatement par un site à deux lignes unique |
| C6-09 | basse | confirme | basse | Mutant key_index tué sans cause ; run_expect ne voit que stdout ; convention de code 4 vs 1 incohérente |
| C6-10 | basse | confirme | basse | Bornes non jugées : K > n, chaîne à n ≤ K+1, u18 extrême composé, échec de lancement FULL, OOM |
| C6-11 | basse | confirme | basse | Hygiène : porte orpheline non compilable, selftests v8 non portés, labels oracle inexacts, références mortes, PASSATION périmée |
| C7-01 | haute | deja_traite/deja_traite | basse/moyenne | Même avec 48 fils parfaitement occupés, le travail CPU R7b est ×1,8–2,6 (K5) et ×5,6–7,7 (K10) trop grand pour 1 s ; ×18–77 pour 100 ms |
| C7-02 | moyenne | plausible/plausible | moyenne/moyenne | Machine à moitié inactive et 10–13 % de CPU système à K5, sans instrument publié pour l'expliquer |
| C7-03 | moyenne | deja_traite/deja_traite | basse/basse | Efficacité parallèle quasi inconnue : quatre paires W24/W48 sur une trame, aucune depuis R5, jamais de W1 sur G4 |
| C7-04 | moyenne | plausible/plausible | basse/basse | Chronomètre honnête dans son périmètre écrit, mais plus étroit que le contrat déclaré : grille et masque absents, sortie non convertie, 0,2 s de fin hors chrono |
| C7-05 | moyenne | plausible/plausible | basse/basse | Règles de mesure du plan non tenues : répétitions, échauffement, p95, hôte calme ; les temps locaux donnent des exposants incohérents |
| C7-06 | moyenne | confirme/deja_traite | basse/basse | Représentativité : trois trames d'une séquence, 35,5k–45,8k sites, rien dans le tiers haut (46k–60k) de la plage du contrat |
| C7-07 | moyenne | deja_traite/deja_traite | basse/basse | Lois d'échelle : locales seulement ; la sous-linéarité de la sortie est un artefact de densité des disques emboîtés |
| C7-08 | moyenne | deja_traite/deja_traite | moyenne/basse | q2 et le recensus n'ont pas bougé depuis R1 ; à K10 ils dépassent seuls 1 s sur deux trames |
| C7-10 | basse | confirme | basse | Les meilleurs chiffres publiés à K5 (5,31 et 6,40 s) sont des minima sur quatre essais, pris sur la configuration MEB OFF |
| C7-11 | basse | confirme | info | SUMMARY.json G4 : même schéma v1 mais champs et sens de cpu_s qui changent, sans générateur versionné |
| C8-01 | moyenne | plausible/deja_traite | basse/basse | Entrées d'auditeur commitées par le développeur et brouillons d'auditeur dans son worktree |
| C8-02 | moyenne | deja_traite/deja_traite | basse/basse | Recommandations restées sans réponse écrite, différées sans échéance, ou engagement non tenu |
| C8-03 | moyenne | deja_traite/deja_traite | basse/basse | Deux « états courants » v9, dont un obsolète hors couverture |
| C8-04 | moyenne | plausible/deja_traite | basse/basse | Attribution non traçable et libellés d'auteur erronés |
| C8-05 | basse | confirme | basse | Chronologie du canal non fiable : ordre, fuseaux, en-têtes datés dans le futur, base manquante |
| C8-06 | basse | confirme | basse | check_docs ne couvre que ETAT_COURANT.md dans le périmètre des audits v9 |
| C8-07 | basse | deja_traite | info | Politique de retrait incohérente : 9 notes supprimées, 6 notes au nom WIP périmé |
| C8-08 | basse | deja_traite | info | Note GPU et note de mesures sans lien entrant, sonde ABA orpheline |
| C8-09 | basse | confirme | basse | Phrase « float32 reste le défaut d'entrée » contraire au profil v9 accepté |
| C8-10 | basse | deja_traite | info | Plages R6 : README non corrigé et liste de l'ETAT incomplète |
| C8-11 | basse | deja_traite | info | Désaccord non arbitré sur le ticket borné de nœuds témoins |
| F9-01 | haute | deja_traite/deja_traite | haute/moyenne | La tour FULL ne passe pas à l'échelle au-delà de 24 fils : phase A limitée à kmax fils, séquentielle dans un ordre ; phase 0 séquentielle entre les ordres |
| F9-02 | moyenne | deja_traite/deja_traite | basse/basse | Parallélisme non mesuré : ni CPU par phase, ni temps ou tâches par ouvrier, ni ouvriers créés dans la sonde (v11 comme v12) |
| F9-03 | moyenne | plausible/deja_traite | basse/basse | « pool.hpp » n'est pas un pool : environ 100 équipes de 48 fils créées et jointes par chaîne K10 |
| F9-04 | moyenne | deja_traite/deja_traite | basse/moyenne | Après la sortie du digest, 0,18 à 0,43 s par chaîne K10 échappent encore à toute phase chronométrée (R7b) |
| F9-05 | moyenne | plausible/deja_traite | basse/basse | Le recensus reparcourt chaque clé sur un second index, alors que le générateur a déjà émis les coquilles (et les intérieurs q2) |
| F9-06 | moyenne | confirme/deja_traite | moyenne/moyenne | File q3/q4 v8 portée sans changement : mutex unique, arête atomique, deux commentaires faux |
| F9-07 | haute | deja_traite/plausible | basse/basse | Préparation GPU nulle sur le poste dominant : annotations device limitées à l'arithmétique de la tour, jamais compilées |
| F9-08 | moyenne | refute/refute | info/info | Porter les palettes H_a/octant rouvrirait la fenêtre de propositions q3/q4 fermée en v8, sans le juge alors exigé |
| F9-09 | basse | plausible | basse | docs/FAUSSES_PISTES.md v9 figé depuis le 22 septembre ; fermetures v9 éparses et en partie non épinglées |
| F9-10 | basse | plausible | basse | Binaire G4 compilé sans -march : AVX-512, BMI2 et ADX de l'EPYC 9B45 inutilisés |
