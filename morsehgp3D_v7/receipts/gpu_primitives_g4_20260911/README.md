# Primitives GPU sur G4 : preuve bornée et fermeture ciblée

Ce paquet conserve une session SPOT gardée du 11 septembre 2026. Il ne qualifie
ni une tour FULL sur GPU, ni le contrat 50 000 points en une seconde ou 100 ms,
ni plusieurs dizaines de millions de points. `public_status=not_claimed`.

## Lecture portable, sans cloud ni compilation

Depuis ce répertoire :

```bash
python3 -B verify.py
python3 -B -O verify.py
```

`storage_map.json` relie chaque chemin logique aux octets bruts de `objects/`.
Le lecteur vérifie l'inventaire physique, les hashes, les archives, les sources
locales et transportées, les résultats bruts, les compteurs et ABI des deux
gates, les deux injections device et la fermeture ciblée. Les chemins absolus
historiques des reçus restent des données, jamais des dépendances du lecteur.
`MANIFEST.json` ferme tout le paquet sauf lui-même. `capture_stability.json`
conserve les hashes avant/après copie des originaux ; les octets ne sont pas
réécrits pour donner l'illusion d'un nouveau run.

## Ce que mesure la session

- Gate MEB : 605 cas, répartis en 82/393/110/20 supports de tailles 1/2/3/4,
  197 extra-shells, comparaison exacte des supports, clés et niveaux avec le
  corpus épinglé, ABI 72/112 octets, rejets, flags causaux et pannes du propriétaire.
  Le GPU sélectionne les supports ; matérialisation et vérification finales
  restent CPU. Le premier support positif admissible reste une obligation du
  producteur qualifié, pas une conséquence de toute validation de support valide.
- Gate clés : corpus arithmétique fini épinglé par hash, vérification de tous les
  mots attendus côté CPU puis CUDA, ABI 112/96 octets et architecture SM120.
  `skip-write` et `corrupt-word` doivent échouer avec la cause exacte attendue.
- Les durées de compilation, de session et de gates sont des captures brutes.
  Aucun chronométrage de composant ne devient un contrat FULL de bout en bout.

Le champ `gcp_used:false` du binaire MEB signifie qu'il n'appelle aucune API
cloud. Le reçu du worker décrit correctement `GCP_used:true` pour son exécution
dans l'invité. Le lecteur local n'utilise aucun service cloud.

## Fermeture et données exclues

Le contrôleur vise exclusivement le projet `devpod-gpu-exploration`, zone
`us-central1-b`, instance `ehgp-v7-4fa0e0789a7d5bb06b787d35`. Le lecteur croise
cible et génération entre handoff, marques des deux coupe-circuits, invité,
commande d'arrêt et reçu hôte. Il exige le message brut d'état `TERMINATED`
pour cette cible, ainsi qu'une commande d'arrêt réussie sur la génération exacte.
La publication ne se fait qu'après notification ROOT et fermeture certifiée.

Aucune clé privée/publique de session ni sortie ou profil OSLogin n'est copié.
Les lignes d'argv historiques peuvent mentionner le chemin d'une clé : elles ne
contiennent pas son contenu. Les exécutables ELF locaux sont exclus, leurs hashes
restent dans les reçus et `exclusions.json`. Les ELF invités sont hors du dossier
récupéré par le contrôleur ; le lecteur refuse tout ELF dans les archives.

## Préflight et réserves de preuve

`local/input_r1/` contient le snapshot complet de sources préparé et ses pins.
`local/local_o3_r1/` est une compilation/lien locale seulement, jamais une
exécution CUDA. `checks_r1` et `checks_r2` restent des autotests historiques sur
leurs propres hashes ; leurs anciennes sources ne sont pas reconstruites.
`checks_r3` correspond aux scripts publiés, avec quatre commandes normal/`-O`.
Ces autotests Python contrôlent les parseurs et gardes, pas le matériel GPU.

La contrelecture n'a trouvé aucun blocage fonctionnel pour les pins publiés.
Trois limites restent explicites :

- `prepare.py` importe worker/contrôleur avant de relever leurs hashes. Le
  croisement avec les pins relus et les autotests fermés est nécessaire dans un
  worktree concurrent ; ce script n'est pas un mécanisme général de gel atomique.
- Les headers externes sont hashés après compilation seulement et les compilateurs
  identifiés par leurs versions. La chaîne système n'est pas intégralement figée.
- La résolution des chemins relatifs du depfile utilise le répertoire courant du
  worker plutôt que celui de compilation. Les depfiles observés utilisent des
  chemins absolus ; cette observation n'est pas une garantie de portabilité NVCC.

La validation arithmétique et celle du lot MEB ne remplacent pas le raccord
géométrie → calendrier → populations → ancres → verticales de toute la tour.
La session ne mesure pas de coût global sous-quadratique.

`publish.py` est conservé comme recette historique create-only : il dépend des
originaux locaux et n'est pas le lecteur portable. Il ne contient aucune commande
GCP ; ne pas le relancer dans ce paquet fermé.
