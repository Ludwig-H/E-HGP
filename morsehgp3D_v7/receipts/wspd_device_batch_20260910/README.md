# Batch WSPD privé : preuves locales, aucun GPU exécuté

Cette qualification porte sur une **primitive privée**, pas sur son raccord au générateur ni sur la tour FULL. Backend exécuté : `stub-hote`; profil `quantized_u16_input_only`; `public_status=not_claimed`. GCP non utilisé par cet agent. La compilation et le lien NVCC 12.9/sm_120 passent, **sans exécution CUDA** et sans mesure de performance GPU.

Les deux reçus originaux sont conservés : `runs/run_r1/` est **failed** (le gate CUDA est rejeté par `-Wmaybe-uninitialized` dans le scalaire), `runs/run_r2/` est **completed** (seul ajout privé `CoreBall balls[3]{};`, options strictes conservées). Les sources actives du produit n'ont pas été changées par cette expérience. Le premier reçu n'est pas rebaptisé en succès.

Dans chaque run, O2 et ASan/UBSan/LeakSanitizer jugent chacun **74 313 checks, 73 935 lignes, 37 rejets, zéro échec**. Les trois comptes et les deux compteurs DFS/coins concordent exactement avec le scalaire. Les mutants no-mask et reverse-push donnent 6 456 et 3 290 divergences causales et sont rejetés. S=8,10,12 fournit trois populations de terminaux WSPD distinctement comptées ; cela ne qualifie pas les contrats d'échelle.

## Lecture et sources

Exécuter `python3 reader.py` puis `python3 -O reader.py` depuis ce dossier. Le lecteur vérifie l'inventaire, chaque hash de source et de sortie brute, les statuts d'échec/succès, les planchers de non-vacuité et les mutants. Il n'exécute aucun binaire. Les ELF originaux ont été hashés à l'assemblage mais ne sont pas embarqués : les hashes seuls ne permettent pas de les exécuter ou de les reconstituer.

Les sources identiques entre les deux captures n'existent qu'une fois dans `objects/`, adressées par SHA-256. `source_views/run_r1.json` et `source_views/run_r2.json` conservent les chemins exacts des sources/dépendances de chaque compilation. Pour extraire une vue sans écraser de dossier existant : `python3 reader.py --extract run_r2 --destination /tmp/mhgp7_witness_sources_r2`. Les commandes historiques, options, compilateurs, sources avant/après et sorties sont dans les reçus ; les chemins absolus de ces commandes décrivent leur environnement d'origine. Aucune archive toolkit, dépendance Linux/Boost ou ELF n'est distribuée.

## API et limites

La route possède son index immuable et expose prepare/run/close, index_digest et des compteurs de phases/copies. Chaque thread traite un rectangle complet ; CoreBall est préparée sur CPU. Pas de catalogue Gamma, de paires développées ni de matrice rectangle×nœud. Les générations sont monotones pendant la vie entière du contexte, les IDs croissent au sein d'un appel ; toute erreur vide la sortie complète, même après un chunk réussi. Le futur contrôleur doit **lier son index à index_digest()** et limiter chaque appel à B requêtes s'il souhaite O(B) résultats auxiliaires.

Payload de l'index : 38*m−24 octets host et device (les capacités physiques des vecteurs host ne sont pas mesurées). Buffers de lot : 136*B octets device ; copies explicites : 200 octets par requête lancée. Le travail est proportionnel aux nœuds/coins effectivement parcourus ; aucune borne universellement sous-quadratique ni accélération mesurée n'est revendiquée.

Le détail des invariants, rejets, coûts et échecs exploratoires est dans `IMPLEMENTATION_NOTE.md`. Avant promotion : gate device réel séparé, puis gate du raccord complet (association d'index, ordre, ownership et ledger de masse). La session G4 nominale déjà figée ne doit pas être étendue silencieusement pour cette primitive.
