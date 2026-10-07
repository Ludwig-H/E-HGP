# Leçons, pistes fermées et pièges

7 octobre 2026. Ce que la lignée v2 → v11 a payé, pour ne pas le repayer. Sources : audit géant de la v11 (§ 6, § 7,
§ 9.7), passation de la v11 (§ 4, § 5), mémoire du projet.

## 1. Leçons de méthode

1. **Seuls les changements d'algorithme font gagner du temps.** Les deux sauts de la lignée sont la chaîne v9 et les
   boîtes de centres de la v10 ; les reprises « plus propres » à algorithme constant (v4 → v5, v5 → v6, v10 → v11) n'ont
   rien rendu. Une version neuve déclare d'avance ce qui change d'algorithme et porte tout le reste.
2. **La conception ne vaut que si l'implantation la suit.** La v11 avait conçu le 2 octobre une plus petite boule
   certifiée, une forêt sans lots et des verticales en $O(1)$ ; elle a implanté autre chose, sans le documenter, et a fini
   trois à huit fois plus lente que ces cibles. Tenir une table « décision → implantation → mesure ».
3. **Mesurer avant de construire.** Les estimations par cycles et par instructions ont toutes été démenties d'un facteur
   2 à 5 (« 100 ms en CPU seul à K5, 65 à 91 ms », 2 octobre ; mesuré au gel : 255 à 314 ms). Un microbanc sur G4, sur les
   entrées réelles, avant tout port.
4. **Le coût par unité de travail compte plus que le volume.** À K10, la v11 fait 1,18 fois les pas de la v10 mais chaque
   pas coûte 3 à 4 fois plus : la plus petite boule était énumérée exhaustivement au lieu d'être proposée puis certifiée.
5. **Le contrat et l'oracle borné passent avant le natif.** Les tranches qui les ont posés d'abord (L0, S0–S1, V3, EOM
   exacte) ont été les plus sûres ; celles qui ont écrit le natif avant la réponse sur le contrat ont été reprises.
6. **Les témoins exacts minimaux valent plus que les campagnes.** Chaque défaut grave s'est vu sur quelques sites ou à
   une borne exacte ($2^{127}-1$, $2^{20}\pm 1$, $s=2^{21}-1$).
7. **Un invariant supposé est un défaut en attente** (« feuilles ≤ sites » : 353 456 feuilles pour 39 885 sites).
8. **Fermer le différentiel contre la version précédente dès la première tranche**, sur trames entières. Celui de la
   v10 à la v11 n'a jamais été fermé sur trames.
9. **Un seul profil produit.** Trois profils ont multiplié les défauts de qualification (attendus gravés pour un seul
   profil, mutants vides par élimination de branches, export trop étroit) pour 5 % de vitesse.
10. **Le GPU se conçoit de bout en bout.** Greffé six fois sur un étage piloté par le CPU, il a toujours été borné par
    l'hôte (contexte par processus, join global, un fil par feuille).
11. **Le chemin produit doit être le chemin mesuré.** Les temps publiés de la v11 ne sont pas ceux de son exécutable.
12. **Les seuils d'adoption se calculent** à partir du bruit et de l'effet attendu ; la v11 en a placé 2 à 6 fois
    au-delà de l'effet réel et a perdu au moins un levier positif.
13. **La qualification se dimensionne d'avance** : lots de 30 minutes, profil produit, dette tenue commit par commit ;
    ne jamais retirer une porte pour tenir une échéance ; « sans résultat » n'est jamais vert ; une porte ne se saute pas
    en silence (les portes LiDAR de la v11 le faisaient sans la variable d'environnement).
14. **Les sorties sont des vues d'un même registre**, conçues ensemble ; la v11 a refait sa sortie `supports` deux fois
    et n'a jamais livré d'export de masse.
15. **Le canal d'audit reste lisible** : un registre de constats à identifiants, des notes vivantes d'une page, des rôles
    stables, une relecture avant tout changement de budget, de concurrence ou de format.

## 2. Pistes fermées

**Définitives (preuve, fixture, invariant)** : toute réduction sans attaches silencieuses (graphe de Gabriel, fold v4,
K-MST élagué : témoins E5, L02) ; mosaïque de Delaunay d'ordre supérieur, $\Gamma$ global ou catalogue en $\binom{n}{k}$
dans le chemin produit ; fenêtre de Morton ou préfixe $k$-NN comme autorité exhaustive ; MST d'atteignabilité mutuelle sur
les points pour $K\geq 2$ ; substitut point-MST ; rang héréditaire ; sous-quadratique pour toute entrée ; Euler comme
certificat ; marge en niveau carré pour les points ; supports v1 et `kparties_reliees` comme réalisation stable ; seuil de
condensation relatif comme règle par défaut.

**Par mesure, pour leur régime** : générateur WSPD et ses variantes ; Borůvka à étiquette minimale de la v9 ; Kruskal par
lots ; lots ordre par ordre ; mémos de lane ; feuille GPU un fil par feuille ; feuille coopérative par paires ; sous-lots
GPU recouverts (L4) ; arène N1 ; pages de 2 Mio ; partition T > 0 sur LiDAR ; enveloppes M3/E4 sur CPU ; filtre flottant
F6 des puissances ; feuilles de 24 sur CPU à K5.

**Retirés sans réfutation** (à rejuger sous le protocole de [`MESURE.md`](MESURE.md) si l'étage existe encore) : filtre G1
en AVX2 (gain réel, retiré à tort) ; frontière par tranches (gain réel d'environ 3 ms) ; préchargement des graines (effet
de phase sans effet d'étage).

**Par consigne** : float32 natif ; micro-variantes q2 ; séparation WSPD sous 8.

## 3. Pièges d'exploitation

**G4 et sessions.**
- Une seule VM, sessions gardées seulement (`start_and_verify.sh` / `stop_and_verify.sh`, SPOT, label `project=e-hgp`,
  `maxRunDuration` entre 30 s et 8 h) ; certifier `TERMINATED` sur la cible exacte après chaque session.
- Arrêter un workflow ou redémarrer le codespace tue le contrôleur sans arrêter la VM : `--recover` aussitôt. La reprise
  ne rapatrie rien : prévoir un rapatriement depuis le disque de la VM.
- Le codespace redémarre environ toutes les 6 heures, et parfois hors calendrier : `/tmp` est vidé. Données et sorties à
  garder sous `build/` ; vérifier `uptime -s` avant une session.
- La VM : GCC 11.4, CMake 3.22.1 (ne connaît pas le dialecte CUDA20 : le déclarer), Python 3.10 **sans pip** (toute porte
  rapide en Python nu ; tester sous `python3 -S`), 48 fils, glibc 2.35. Clang absent.
- TSan exige `setarch -R`.
- Le lanceur n'accepte que `./mhgp11*`, `ctest` et `python3 {src}/morsehgp3D_v11/<script>.py` : la lignée v12 doit avoir
  son propre lanceur.
- Garde disque d'environ 1,15 Go sur `/workspaces`, plein à 97 % : plafonner les résultats.
- Capacité épuisée en `us-central1-b` le 2 octobre ; préemption SPOT au démarrage.

**Local et outils.**
- Ne jamais éditer un script bash en cours d'exécution.
- `pkill -f motif` tue le shell qui le lance ; tuer par PID. Une boucle `until ! pgrep -f motif` se reconnaît elle-même.
- `rsync -a` après une rejouée locale de mutants garde l'objet muté (dates anciennes).
- Le local ne prédit pas G4 : GCC 13 contre 11, CMake 3.28 contre 3.22, Python 3.12 avec numpy contre 3.10 nu, 4 cœurs
  physiques sous la charge des autres agents. Un vert local n'est pas un vert G4.

**Git et documents.**
- Index partagé : `git diff --cached --quiet` avant tout `git add` ; jamais `git add -A` ; pousser par cherry-pick sur
  `origin/main` quand le worktree partagé contient le travail d'autres acteurs, jamais `rebase` ni `stash`.
- `*.log` est ignoré par le `.gitignore` racine : un reçu qui en contient les verse avec `git add -f`.
- Équations Markdown sur une ligne physique, accolades explicites, ni `\operatorname`, ni `\left\|`, ni `\left\{` ; LaTeX
  jamais dans une chaîne Python non brute ; heredoc shell toujours quoté ; `python3 tools/check_docs.py` **sans pipe**.
  Ce contrôle n'examine pas les dossiers de la v11 ni de la v12 et échoue déjà sur 213 liens morts de la v10 : valider
  les nouveaux fichiers avec sa fonction `validate`.
- Lire l'heure par `date -u` avant de l'écrire dans une note ou un reçu.
- Aucun octet KITTI, aucune identité de compte dans le dépôt.
