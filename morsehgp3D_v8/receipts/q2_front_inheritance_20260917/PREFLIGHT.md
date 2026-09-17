# Préflights des témoins hérités du front q2

17 septembre 2026. Exploration, pas qualification de tour FULL/G4.
GCP non utilisé. Journal des essais faits AVANT les captures closes ; le
bilan est dans le [README](README.md), le choix du levier dans
[cadrage](cadrage/README.md).

## Choix du levier

Après la fenêtre élargie, des compteurs de cycles posés sur une copie jetable
du moteur donnent la descente du proposeur à 15 à 17 % du temps q2, la fenêtre
et ses tests H à 25 %, le census à 53 %. Un profil d'instructions avait fait de
la descente le premier poste : il la surestimait. Quatre leviers prototypés
hors dépôt, puis deux portés ensemble dans le moteur réel et mesurés : les
témoins hérités rendent ×0,87 à ×0,95 hors rangées ; la reprise exacte de la
descente, dont le théorème tient et dont le pivot est identique, ne rend que
×0,94 à ×0,98. Elle est retirée avant le gel, son moteur archivé en patch.

## Réfutation de la conception avant le code

Quatre relecteurs en lecture seule (théorème de reprise, théorème d'héritage,
déterminisme d'ordonnancement, registres). Les deux théorèmes tiennent.
Corrections intégrées avant d'écrire le moteur :

- L'énoncé de monotonie de la note était faux sur deux points : l'ensemble
  crédité n'est pas un sur-ensemble (arrêt précoce), et le **nombre** de
  produits rejetés n'est pas monotone (un ancêtre rejeté cache ses descendants).
  Remplacé par la domination préfixe : arrêt jamais plus tardif, produits
  visités, rectangles et candidates en sous-ensembles, masse rejetée au moins
  égale, supports égaux.
- L'état transmis vit dans la tâche, par valeur, jamais dans l'objet front :
  sinon le produit gauche-droite de la racine, qui saute sa recherche, lirait
  la liste d'un cousin en mono et une liste vide dans la préparation des jobs.
- Deux chemins annoncés sont morts par structure sous la voie q2 seule (liste
  à travers une diagonale, liste reçue par une recherche sautée) : erreurs
  logiques dans le moteur au lieu de mutants impossibles à tuer.
- Compteur de rejets : le contrefactuel « la fenêtre seule n'aurait pas
  rejeté » n'est pas calculable sans payer les tests que l'héritage épargne ;
  le moteur publie un majorant causal, le rejeu indépendant calcule le vrai.
- Registres : doublons d'extension comptés à part, crédits reçus hors des
  crédits de l'exécution, et identité exacte des crédits qui ferme le registre.
- Le relecteur des registres a laissé un modèle Python complet du front ; il est
  archivé et ses compteurs sont gravés dans la porte comme troisième
  implémentation.

## Relecture indépendante de l'implémentation avant gel

Quatre relecteurs (moteur, portes, reçus, énoncés), chaque constat vérifié par
un contradicteur ; tout en lecture seule sur le build de développement
`build/v8_inherit_dev_20260917`. Aucun défaut d'exactitude du moteur : identité
du défaut confirmée sur 48 à 168 configurations contre le build épinglé de la
tranche 20, sûreté par force brute sur plusieurs milliers de nuages aléatoires,
cinq entrées q2, ASan/UBSan et TSan propres. Dix-sept mutants du moteur tués
par les portes. Constats intégrés :

- **Largeur des rangs stockés.** Aucun nuage des portes ne dépassait 128 sites :
  un moteur tronquant ses rangs à 8 ou 16 bits passait toutes les portes et
  perdait des supports dès n = 300. Ajout d'un nuage gravé de 320 sites jugé par
  le rejeu et l'oracle, avec plancher sur le plus grand rang reçu reproposé
  (le mutant 8 bits est tué, vérifié sur une copie mutée) ; campagne `frontier`
  à n = 70 000, où l'empreinte de supports doit égaler celle de la jumelle (le
  mutant 16 bits y perd 4 110 supports à K = 2 et 13 656 à K = 5, vérifié de
  même) ; garde statique liant le type stocké au refus des 2^32 sites.
- **Écriture hors tableau invisible aux sanitizers.** Un mutant écrivant la liste
  avant de tester le seuil débordait d'une case à l'intérieur de l'objet, sans
  effet ni rapport ASan/UBSan. L'écriture est maintenant vérifiée par une erreur
  logique.
- **Lecteur de reçus.** Deux branches neuves n'étaient exécutées par aucune
  porte : le plancher des tailles d'intérêt et le différentiel. La porte les
  exécute sur de vraies captures et y tue treize mutants à message vérifié.
  Une condition de domination par message et par mutant ; registres jugés
  avant la projection, un message par théorème ; deux bornes (Kmax − 1 par
  recherche, par rectangle) et les crédits d'extension accordés à des rangs
  reçus ont leur mutant compensé ; plancher par étiquette de mutant compensé.
- **Non-vacuité.** Le compteur de rejets du moteur est un majorant, positif
  même quand l'héritage ne retire rien (rangées) : les planchers reposent
  désormais sur un effet mesuré, la baisse stricte des candidates face à la
  jumelle.
- **Différentiel.** La fenêtre 4K, base d'un tiers des comparaisons, n'était pas
  confrontée à la tranche 20 : septuplets au lieu de quintuplets, plan de jobs
  comparé, empreintes des sondes épinglées recontrôlées contre les reçus de la
  tranche 20 par l'analyse.
- **Énoncés.** La phrase « aucun témoin universel sur les rangées » était
  fausse : des rangs y sont hérités par centaines de milliers, presque tous
  reproposés par la fenêtre de l'enfant ; seules les paires entre rangées n'ont
  aucun témoin. Les rapports du prototype pour la reprise de descente (×0,85 à
  ×0,92) contredisaient la raison donnée pour ne pas la porter : ils sont
  déclarés remplacés par la mesure du moteur réel, avec les chiffres qui
  l'expliquent. Coût du chemin par défaut déclaré (tâche de 72 octets au lieu
  de 32). Deux commentaires de contrat et deux en-têtes de fixtures corrigés.

## Avant les captures closes

Test de bout en bout sur le build de développement après ces corrections :
81 CTests sur 81 (un échec isolé d'une porte de reçus antérieure pendant que
les relecteurs chargeaient la machine, repassé seul puis dans la suite
complète), campagne smoke passée hors reçus, quatre portes du front passées
sous ASan/UBSan dans `build/v8_inherit_asan_dev_20260917` (porte d'héritage en
cinq minutes). Sources gelées, inventaire de 130 sources exact. Trois builds
neufs. Aucune compilation ni autre campagne pendant les captures closes.
