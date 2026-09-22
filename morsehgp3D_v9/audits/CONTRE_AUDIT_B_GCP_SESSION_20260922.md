# Contre-audit B — session G4 SPOT du premier moteur v9

22 septembre 2026. Lecture seule des quatre scripts, publiés par `ba762036`
puis corrigés sur le plafond de fils par `e28296bb`,
`gcp-migration/tower_{snapshot,session,worker,selftest}_v9.py` dans le worktree
développeur. Aucun appel GCP, benchmark ni essai de prix dans cet audit. Le
protocole est une mesure **CPU** de la tour relative au catalogue recoupé,
pas une exécution GPU ni une qualification du contrat de temps.

## Garde-fous établis par le code

- Le CLI du contrôleur et celui du worker sont inertes sans `--execute`
  (`tower_session_v9.py:438–462`, `tower_worker_v9.py:557–579`). Le snapshot
  construit localement un paquet ; le selftest remplace le cloud par un faux.
- La cible est unique : projet, zone et instance fixés, état initial
  `TERMINATED`, `g4-standard-48`, label `project=e-hgp`, SPOT/STOP,
  `maxRunDuration=3600 s` (`tower_session_v9.py:59–73,295–305`). Le démarrage
  armé prévoit l'arrêt invité à 40 min ; le worker est borné à 1 500 s utiles
  et 600 s par cas (`tower_worker_v9.py:74–87,381–385`). Ce sont des plafonds
  de durée, **pas** une estimation de facture ni une garantie de huit résultats.
- Un arrêt ciblé avec `--expected-last-start-timestamp` est tenté dans le
  `finally`, aussi après échec de compilation, de transfert ou de capture
  (`tower_session_v9.py:358–418`). Si un démarrage a pu avoir lieu mais que sa
  génération est inconnue ou contradictoire, aucun arrêt non versionné n'est
  tenté et la fermeture reste non certifiée. Le garde invité et la limite GCE
  restent distincts de ce stop.
- Le paquet lit sources et trois entrées depuis les objets Git d'un commit,
  vérifie SHA-256, taille et empreinte FNV des fichiers de trame, et interdit
  une vraie session tant que les quatre scripts exécutés ne sont pas identiques
  à ceux du commit (`tower_snapshot_v9.py:106–146`,
  `tower_session_v9.py:425–460`). À `e28296bb`, les quatre fichiers sont
  suivis et un paquet réel peut être préparé sans dérogation ; cela ne
  lance toujours pas la VM.

## Risques à traiter avant de lire un reçu comme une qualification

1. **Un reçu `partial` peut contenir zéro tour achevée.** Un refus explicite
   (code 3) ou un cas tué au plafond est conservé comme preuve d'échec ; seuls
   code 0, `run_tower=true` et les ordres `K=1..K_effective` donnent
   `complete_relative` (`tower_worker_v9.py:286–325`). Pourtant le contrôleur
   accepte `partial` et retourne code 0 même si aucun cas n'est complet ; dans
   ce cas `FULL_executed=true` signifie seulement qu'une tentative a commencé
   et la liste des comparaisons entre workers peut être vide
   (`tower_session_v9.py:171–217,390–422`). Pour le plan par défaut, un lecteur
   de contrat doit exiger `status=completed`, les huit indices complets et les
   comparaisons attendues ; ni le code de sortie ni `FULL_executed` ne suffisent.
   Même alors, le digest FNV-1a 64 et les comptes d'ordres sont des contrôles
   différentiels, pas un oracle de complétude mathématique.
2. **Plafond de fils corrigé à `e28296bb`.** Le plan personnalisé initial
   admettait 1 024 workers ou threads statiques sur 48 vCPU ; le validateur
   accepte désormais au plus 48 de chaque. Le plan par défaut reste W48 et
   `static_threads=0`. Cela ferme ce débordement précis, pas la question du
   meilleur nombre de workers ni celle du coût des cas lourds.
3. **Portée des entrées.** Le plan par défaut mesure les trois trames entières
   *sans sol* à grille 1 mm de la seule séquence 08, **s=8 seulement**,
   K5 puis K10 à W48,
   puis la première à W24/W1 (`tower_snapshot_v9.py:95–103`). Tous les cas ont
   `repeat=0`; W1 est dernier et peut être sauté avec le budget total de
   1 500 s. Les SHA du paquet attestent ces fichiers, non la qualité du masque
   sol. Cette campagne ne couvre ni la trame brute entière principale, ni
   float32, ni plusieurs séquences, ni comparaison s=8/10/12, ni GPU ;
   `public_status=not_claimed` est
   approprié.
4. **La provenance Git n'est pas revérifiée indépendamment à la réception.**
   Le constructeur lie bien les octets au commit ; le contrôleur contrôle
   l'archive, le manifeste et l'identité du protocole exécuté, mais son test
   initial de « protocole commis » lit le champ `protocol_source=commit` de
   l'archive sans recalculer les blob IDs contre `provenance.commit/tree`
   (`tower_session_v9.py:96–123,425–435`). Une archive fabriquée hors du
   constructeur peut donc s'auto-attribuer un commit. Vérifier cette liaison
   à partir de Git lors de la clôture indépendante du reçu.

Préflight indépendant après `e28296bb` : les 17 selftests passent sous
`python3 -B` et `python3 -B -O` ; le constructeur a préparé depuis ce commit
un paquet `protocol_source=commit`, `real_session_allowed=true`, avec huit
cas s8 et trois entrées aux SHA attendus. Cette sortie est
`prepared_not_executed` (archive SHA-256
`34926aa042e8fff0d1db0a1981af6dc50e2b5d1fddedae49edea9b9f7b6f9a5d`) :
aucun appel GCP. Une demande de paquet `ba762036` depuis les scripts locaux
`e28296bb` a été refusée, comme voulu par la règle d'identité au commit.
Avant GCP : contrôler le paquet
contre les objets du commit et décider quels cas prioriser dans le budget.
Après
session : publier séparément chaque issue (`complete_relative`, refus, tué,
sauté), les fichiers bruts hachés et l'arrêt ciblé certifié ; répéter les
mesures réussies et juger les contrats sans-sol, brut et GPU sur des lignes
distinctes. Un succès du protocole de capture ne transforme pas ce premier
moteur en tour qualifiée sur G4.
