# MES-M6 — audit du premier microbanc CUDA

**Deux corrections de mesure avant campagne : vraie médiane et séparation premier lancement/chaud.** Ce banc ne constitue pas une Session produit ni une qualification FULL. Aucun GPU/G4 utilisé ; aucun chrono local interprété comme performance.

Source auditée : `bench/mes_m6_session_cost.cu`, introduit par `c3e565f8598b3630c35283f2e94c067be2d4ebfc`, relu au pin **3e6e6a8e721c6dbe8aaa3a0c5cb20c56abfae5ff**, SHA256 `243a5ca23d7a1e0c8193082cfd113571ce45af567dcf0b550415ca5a0a3b3ee9`. Les lignes ci-dessous sont celles de ce fichier. Cadre v12 hors registre, `public_status=not_claimed`.

## CST-0209 — médiane paire incorrecte, P2 confirmé par exécution

Lignes **41–44**, `p50` est `sorted[floor(0.5·(n−1))]`, donc la médiane **basse**, en contradiction avec la « vraie médiane » de `docs/MESURE.md:151`. Avec les dix valeurs `1…10`, le code original renvoie **5**, attendu **5,5**. Les effectifs usuels du banc sont pairs : 2000, 1000, 200 et 20 avec les arguments par défaut ; minimum 10 pour les transferts et `touch`.

Le débit lignes **187–189**, **209** est calculé à partir de cette valeur : dans le témoin synthétique, il est surestimé de **10 %**. Cette amplitude est celle du témoin, pas une estimation du biais sur G4. Corriger `p50` par la moyenne des deux valeurs centrales lorsque n est pair ; déclarer séparément la convention des p05/p95, dont il existe plusieurs définitions légitimes.

`probe.py` extrait **sans réécriture** `Quantiles` et `quantiles` du fichier audité, les compile en C++ puis les exécute. Cas pair et impair conservés dans `RESULT.json`.

## CST-0210 — première exécution incluse dans les quantiles dits résidents, P2 de protocole

Le noyau vide est lancé une fois avant les mesures (85–89), mais le graphe nouvellement instancié (**151–162**) et le noyau `touch` (**195–210**) ne reçoivent aucune exécution préalable. Leurs premières exécutions sont directement incluses dans le vecteur servant à `max_us`, p95 et p50. Création/instanciation du graphe ne sont pas chronométrées séparément et aucun `cudaGraphUpload` explicite n'est effectué.

Le constat démontré est le **mélange des régimes**, pas une durée d'initialisation CUDA mesurée. Le même code de quantiles reçoit le modèle injecté `[100,1,…,1]` : maximum 100, contre 1 pour les dix observations chaudes. Une surdurée du premier lancement ne peut donc être distinguée d'une queue de latence chaude par le reçu actuel. Les valeurs injectées ne sont pas des chronos GPU. Au minimum n=10, la médiane peut rester stable tandis que le maximum demeure celui du premier lancement.

Émettre séparément préparation/capture/instanciation et premier lancement synchronisé ; mesurer ensuite les répétitions chaudes. Conserver le premier échantillon dans son régime, sans le supprimer silencieusement. Si le chargement préalable du graphe est la politique de Session choisie, chronométrer `cudaGraphUpload` et sa synchronisation dans la préparation. Relever la configuration de chargement des modules et ne pas transformer un maximum mélangé en maximum résident.

## Contrôles de code et limites restantes

**Synchronisation et initialisation : pas de défaut constaté.** Le timer de contexte commence avant `cudaSetDeviceFlags` (66–69). L'en-tête NVIDIA CUDA 12.9 précise que cet appel peut initialiser le périphérique avec les drapeaux demandés ; le chronométrage l'inclut. `cudaStreamSynchronize` est documenté comme tenant compte de `cudaDeviceScheduleBlockingSync`. Les deux flux sont non bloquants, mais l'allocation résidente est synchronisée avant les copies (121–122), chaque copie finit avant la suivante (182), et la dernière copie finit avant `touch` : absence de course interflux dans ce parcours séquentiel.

Les tampons hôte sont initialisés (97–98). Pour chaque taille, H2D précède D2H ; le dernier groupe porte sur **256 Mio**, donc tout le tampon appareil est initialisé avant `touch`. Son addition sur `unsigned` a un débordement modulo défini. Les 262144 blocs de 256 fils couvrent exactement les 67108864 mots ; pas de lecture hors borne ni d'entrée indéfinie démontrée.

**Erreurs de lancement à durcir avant campagne.** Une seule des cinq expressions de lancement a un `cudaGetLastError` immédiatement après (**86–87**). Les sites **129, 142, 152, 203** n'ont pas ce contrôle ; les synchronisations contrôlent l'achèvement et les erreurs asynchrones, mais ne doivent pas remplacer le contrôle des erreurs immédiates du lancement. Ajouter un contrôle après chaque lancement, y compris lors de la capture ; pour comparer les coûts, le placer selon une convention identique et publiée dans les deux bras. Lecture source et contrat d'API uniquement : aucun échec de lancement réel ou simulé de GPU n'est revendiqué ici.

**Ce que les métriques mesurent.** Horloge monotone côté hôte, autour de soumission + synchronisation : bon périmètre pour une latence aller-retour. Les champs `p50_gbps`/`p50_gbps_rw` valent des **Go/s décimaux** (`octets/(µs·1000)`), pas des gigabits/s ; rendre l'unité explicite. `touch` compte un trafic logique lecture+écriture de 512 Mio, pas une mesure des octets réellement servis par la DRAM. La somme des lignes froides n'est pas un temps complet de Session : initialisation hôte, allocation pageable, préparation du pool et du graphe ne sont pas toutes mesurées. Ce sont des coûts de primitives à étiqueter comme tels.

**Reçus et comparaison des attentes.** Les quantiles sont ceux d'un seul processus ; ils ne remplacent pas les répétitions de processus entrelacées de MESURE. Les lignes actuelles indiquent le GPU et `sync`, mais pas le pilote/runtime, les flags compilateur, le mode de chargement, l'identité du processus/prise ni CPU·s. Ces éléments peuvent être fournis par un manifeste externe épinglé ; il doit être présent avant de choisir spin/yield/blocking, dont le compromis porte aussi sur la consommation CPU. Les capacités de pool sont vérifiées indirectement par les retours CUDA ; sur GPU non compatible, le banc refuse code 3, sans produire une alternative comparable.

**JSON et sorties.** Les noms de mesures sont constants ; les champs numériques des témoins sont relus comme JSON. Le nom GPU est interpolé sans échappement (73–76) : un encodeur JSON serait plus robuste, mais aucune chaîne de périphérique réelle cassant le JSON n'a été observée. Les erreurs CUDA sont interceptées code 3 ; les exceptions hôte, par exemple `std::bad_alloc` du tampon pageable, ne le sont pas. Aucun de ces points n'est présenté ici comme un échec G4 avéré.

## Exécutions bornées

- Fonction originale de quantiles compilée/exécutée avec g++ C++20, `-O2 -Wall -Wextra -Werror` ; cas pair, impair et injection de premier échantillon.
- **Fichier CUDA original** compilé avec nvcc **12.9.86**, C++20, `-O3 -arch=sm_120 -Xcompiler=-Wall,-Wextra,-Werror` : succès. Aucun lancement GPU.
- Cinq arguments invalides exécutés sur ce binaire : nombre trop petit, trop grand, absent, entier décimal débordant `long`, mode de synchronisation inconnu. Tous quittent **avant `run`**, code 2, stdout vide.

Rejeu : `python3 morsehgp3D_v12/receipts/audit_suivi_20261007/session/probe.py`. Binaries et sources C++ d'extraction restent dans un répertoire temporaire supprimé. `RESULT.json` et `MANIFEST.json` suffisent ; aucune copie de source ou de sortie volumineuse.

Références d'API consultées localement, officielles NVIDIA : `/usr/local/cuda-12.9/include/cuda_runtime_api.h` **1345–1402** (`cudaGetLastError`), **2340–2408** (`cudaSetDeviceFlags`), **2925–2946** (`cudaStreamSynchronize`), **13533–13557** (`cudaGraphUpload`). Aucun recours au Web nécessaire.
