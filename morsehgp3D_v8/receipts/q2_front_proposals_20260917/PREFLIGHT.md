# Préflights de la fenêtre de propositions élargie

17 septembre 2026. Exploration, pas qualification de tour FULL/G4.
GCP non utilisé. Journal des essais faits AVANT les captures closes ; le
bilan est dans le [README](README.md).

## Développement

Build de développement `build/v8_proposals_dev_20260917` (GCC 13.3 Release,
en-têtes Boost 1.83 empruntés en lecture seule par les juges). Il ne porte
aucune capture. Première version : option `WspdFrontProposals`, fenêtre
testable `wspd_proposal_window`, extension du filtre pour toutes les voies
actives, rejets d'extension par voie. Les 75 CTests existants passaient avec
le défaut ; la sonde au facteur 1 égalait la sonde parallèle sur tous les
champs discrets ; à uniforme 8k K10, la fenêtre 2K limite 16 donnait 845 160
candidats et 45 020 340 visites Z, à l'unité les chiffres de la copie patchée
d'audit du 15 septembre (mesure d'audit, non héritée).

La porte de reçus a trouvé deux faiblesses du premier lecteur de lignes, par
ses mutants de type : un facteur de fenêtre booléen accepté (`True == 1`), et
un booléen masqué par l'arithmétique de projection. Corrigées avant tout gel.

## Relecture indépendante avant gel

Quatre relecteurs (moteur, portes C++, lanceur et reçus, contrat), en lecture
seule sur le build de développement. Aucun défaut d'exactitude. Constats
intégrés :

- **Périmètre.** L'extension servait aussi q3/q4 sous masque 7, sans juge
  indépendant : trois mutants du moteur survivaient aux quatre portes
  (crédits q3/q4 remis à zéro, rejets d'extension sur-comptés, crédits
  d'extension comptés pour q2 seul). Décision : voie q2 seule, comme la
  note ; un facteur différent de 1 est refusé si une voie q3/q4 est active.
  Le compteur de rejets devient scalaire.
- **Vitesse du défaut.** La lambda `propose`, appelée depuis trois sites,
  n'était plus inlinée : +4 à +5 % sur le chemin par défaut, à compteurs
  identiques (mesure du relecteur, confirmée ici : 4 981 ms contre 5 079 ms
  en médiane à uniforme 8k). Le filtre est réécrit en une boucle de phases
  autour de la boucle historique, un seul corps ; l'écart tombe dans le bruit
  (capture différentielle : ×0,98 à ×1,03 du build épinglé).
- **Branche morte.** `wider == window` est inatteignable en voie q2 (une
  recherche exige Kmax sites extérieurs, donc n ≥ Kmax + 2) : remplacée par
  une garde, avec l'invariant « tout produit étendu propose au moins un rang ».
- **Portes.** Dix mutants causaux propres à l'extension au lieu de six (les
  fautes communes à la fenêtre historique sont jugées à part) ; compteurs de
  désaccord publiés par mutant ; deux fixtures minimales nommées à attendus
  exacts ; constantes du moteur d'avant la tranche, calculées avec la
  bibliothèque épinglée de la tranche 19, gravées pour la voie q2 (porte
  proposals) et pour les trois voies (porte du front) ; planchers de bord
  dans le rejeu (fenêtres butées à gauche et à droite, fenêtre 2K tronquée
  par un nuage de 14 points, tangence vraie sur produit singleton, K-ième
  témoin par la seule extension) ; limite 2 au lieu de 16 dans les portes
  front, jobs et dispatch, avec plancher « la limite mord » ; variantes
  facteur 1 explicites, avec et sans limite, égales au défaut ; trois
  vérifications tautologiques supprimées ou rendues inconditionnelles.
- **Reçus.** Quatre théorèmes du registre ajoutés à la sonde et au lecteur ;
  planchers de non-vacuité par ligne hors smoke (hors rangées, régime de
  surcoût pur) ; mutants de paires appliqués aux deux nombres de fils, message
  de rejet vérifié, un par monotonie ; campagne `differential` contre un build
  épinglé extérieur ; échelle étendue à K5 ; un et quatre workers dans la même
  capture à 32k ; trois captures d'échelle pour un minimum de temps.

## Avant les captures closes

Test de bout en bout sur le build de développement après ces corrections :
78 CTests sur 78, campagnes smoke et différentielle passées (hors reçus).
Sources gelées, inventaire de 125 sources exact. Trois builds neufs ;
préflight Release 78/78. Aucune compilation ni autre campagne pendant les dix
captures closes. La porte proposals prend 12 minutes sous ASan/UBSan et
autant sous TSan ; la qualification ASan/UBSan a donc duré 25 minutes et la
capture TSan 21 minutes, sans échec.
