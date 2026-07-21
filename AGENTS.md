# Instructions pour les Agents IA / AI Agent Instructions

> [!IMPORTANT]
> **Règle absolue :** Ne jamais créer de branches Git dans ce dépôt sans l'accord explicite de l'utilisateur.
>
> **Absolute Rule:** Never create Git branches in this repository without the user's explicit consent.

### Formules Mathématiques (LaTeX/KaTeX) / Math Formulas (LaTeX/KaTeX)

- **Pas d'équations sur plusieurs lignes physiques** : Pour éviter que le parser Markdown de GitHub n'insère des balises de saut de ligne `<br>` ou ne brise l'équation, toutes les équations (blocs `$$ ... $$` et inline `$ ... $`) doivent être écrites sur une **unique ligne physique** dans le fichier Markdown.
- **Accolades explicites pour les commandes** : Toujours utiliser des accolades pour les commandes comme `\mathbb`, `\mathbf`, `\frac`, `\sqrt` (ex: `\mathbb{R}`, `\mathbf{1}`, `\frac{1}{K}`, `\sqrt{x}`).
- **Pas d'utilisation de `\operatorname`** : La macro `\operatorname` n'est pas autorisée, utiliser `\mathrm` ou `\text` à la place (ex: `\mathrm{clip}`, `\mathrm{ord}_K`).
- **Caractères accentués unicode** : Les mots contenant des caractères unicode/accentués dans les blocs de mathématiques doivent être enveloppés dans `\text{\textbf{texte}}` ou `\text{texte}` pour assurer leur compatibilité avec KaTeX.
- **Délimiteurs de norme (`\left` / `\right`)** : Ne jamais utiliser `\left\|` ou `\right\|`. Utiliser à la place `\left\Vert` / `\right\Vert` (ou `\left\lVert` / `\right\rVert`) pour éviter l'erreur de délimiteur non reconnu dans KaTeX.
- **Délimiteurs d'accolades (`\left` / `\right`)** : Ne jamais utiliser `\left\{` ou `\right\}`. Le parseur Markdown de GitHub consomme l'antislash d'échappement avant la transmission à KaTeX, ce qui produit l'erreur `Missing or unrecognized delimiter for \left`. Toujours utiliser `\left\lbrace` et `\right\rbrace` à la place (ajouter un espace après la commande si elle est suivie d'une lettre pour éviter qu'elle soit fusionnée, ex: `\left\lbrace y`).

- **No multi-line equations**: To prevent the GitHub Markdown parser from introducing `<br>` tags or breaking the equation, all equations (both blocks `$$ ... $$` and inline `$ ... $`) must be written on a **single physical line** in the Markdown file.
- **Explicit braces for commands**: Always use explicit curly braces for commands like `\mathbb`, `\mathbf`, `\frac`, `\sqrt` (e.g., `\mathbb{R}`, `\mathbf{1}`, `\frac{1}{K}`, `\sqrt{x}`).
- **No `\operatorname` macro**: The `\operatorname` macro is not allowed, use `\mathrm` or `\text` instead (e.g., `\mathrm{clip}`, `\mathrm{ord}_K`).
- **Accented/unicode characters**: Accent characters within math blocks must be wrapped in `\text{\textbf{text}}` or `\text{text}` to ensure KaTeX rendering compatibility.
- **No `\left\|` or `\right\|` norm delimiters**: Never use `\left\|` or `\right\|` for norms in KaTeX. Use `\left\Vert` / `\right\Vert` (or `\left\lVert` / `\right\rVert`) instead to avoid unrecognized delimiter errors.
- **Curly brace delimiters (`\left` / `\right`)**: Never use `\left\{` or `\right\}`. The GitHub Markdown parser intercepts the backslash escape, passing `\left{` to KaTeX and throwing a `Missing or unrecognized delimiter for \left` error. Always use `\left\lbrace` and `\right\rbrace` instead (add a trailing space if followed by a letter to prevent command name collision, e.g., `\left\lbrace y`).

### Sécurité des VM GCP / GCP VM Safety

> [!CAUTION]
> Toute VM GCP est une ressource facturable. Un agent ne doit jamais créer ou
> démarrer une VM sans coupe-circuit vérifié. S'il crée ou démarre une session,
> il doit certifier l'arrêt de **cette cible précise** avant de rendre la main.
> Les autres VM du projet peuvent appartenir à l'utilisateur ou à une session
> concurrente : elles sont signalées, jamais arrêtées automatiquement.

- **Autorisation explicite obligatoire** : la création ou le démarrage d'une VM est une mutation externe facturable et exige l'autorisation explicite de l'utilisateur. Les workflows GitHub restent strictement en lecture seule : ils ne créent, ne démarrent, n'arrêtent et ne suppriment aucune ressource.
- **Autorisation permanente limitée au préemptible et sobriété budgétaire** : l'utilisateur autorise les agents à choisir, créer, démarrer et arrêter de façon autonome les VM nécessaires au déploiement E-HGP uniquement lorsque le modèle vérifié est `SPOT`/préemptible et que les scripts gardés du dépôt sont utilisés. Employer le moins de VM possible, choisir la durée bornée la plus courte réaliste, arrêter dès que le résultat utile est acquis et ne jamais conserver une ressource par simple commodité. Toute VM non préemptible ou tout passage hors `SPOT` exige une nouvelle autorisation explicite de l'utilisateur.
- **Aucune mutation hors session** : si la tâche n'utilise pas GCP, ne lancer aucune commande GCP mutante et ne modifier l'état d'aucune VM. Au passage de relais, indiquer simplement `GCP non utilisé`; aucun inventaire global n'est alors obligatoire.
- **Durée bornée** : avant toute création ou tout démarrage, vérifier `instanceTerminationAction=STOP` et un `maxRunDuration` compris entre 30 secondes et huit heures. Une valeur hors de cet intervalle est interdite, même pour un benchmark long. Ne jamais désactiver ni contourner ce contrôle.
- **Point d'entrée unique** : ne jamais appeler directement `gcloud compute instances start`. Utiliser `./gcp-migration/start_and_verify.sh`, qui vérifie le projet, le label `project=e-hgp`, le type `g4-standard-48`, le provisioning `SPOT`, l'action `STOP`, la durée GCE, l'échéance après démarrage et un second arrêt programmé dans l'OS invité.
- **Double coupe-circuit** : conserver le coupe-circuit GCE et armer l'arrêt invité avec une durée au plus égale à la durée GCE. Si l'un des deux ne peut pas être vérifié, arrêter immédiatement la VM et considérer l'opération comme échouée.
- **Fermeture ciblée sans exception** : après chaque session créée ou démarrée par l'agent — succès, échec, exception, interruption ou benchmark abandonné — exécuter `./gcp-migration/stop_and_verify.sh` sur exactement les mêmes projet, zone et nom (avec `--yes` puisque l'autorisation de démarrer inclut l'arrêt de sécurité). Le script doit d'abord vérifier le label `project=e-hgp`, puis confirmer `TERMINATED` pour cette cible.
- **Passage de relais** : une cible que l'agent a créée ou démarrée et dont l'arrêt est illisible ou non certifié est un blocage. Les autres VM `project=e-hgp` actives sont inventoriées et signalées sans mutation ni échec automatique; l'agent ne doit ni se les attribuer ni les arrêter.
- **Échec fermé et signalement précis** : si GCP, SSH ou l'état de la cible est inaccessible, ne jamais supposer que la session ciblée est arrêtée. Signaler immédiatement le projet, la zone, le nom d'instance, le dernier état connu et la commande de contrôle à exécuter.
- **Pas de benchmark sans garde-fou** : aucun test GPU, installation, reboot ou benchmark ne commence avant la certification des deux coupe-circuits. Après un reboot, réarmer et revérifier le coupe-circuit invité.
- **No raw starts, no forgotten targeted shutdowns**: agents must use the guarded scripts, keep each run between 30 seconds and eight hours, treat an unverifiable target state as failure, and verify the exact session they started is `TERMINATED` before handoff. Other labelled VMs are report-only.

### Développement MorseHGP3D / MorseHGP3D Development

- Avant toute implémentation, lire les deux premières parties de `docs/references/MANUSCRIT_THESE_HAUSEUX.pdf` — Partie I, pages PDF 35 à 76, puis Partie II, pages PDF 77 à 134 — ainsi que `docs/SPECIFICATION_MORSEHGP3D.md`, `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`, `docs/ROADMAP_IMPLEMENTATION_MORSEHGP3D.md` et `docs/TEST_PLAN_MORSEHGP3D.md`.
- Garder comme invariant d'architecture que MorseHGP3D doit alléger fortement `HGP-old` : calculer la hiérarchie utile sans matérialiser la mosaïque de Delaunay d'ordre supérieur. Tout nouveau jalon doit expliquer quelles structures globales, cellules, cofaces ou incidences il évite de construire et pourquoi son coût intermédiaire reste compatible avec les cibles produit. Un oracle exhaustif borné peut falsifier ou recertifier le chemin produit, mais ne doit jamais devenir son architecture par défaut ni être réimplémenté sous un autre nom.
- Annoncer la phase, le `backend`, le `profile` et le `mode`; ne pas commencer si la porte d'entrée de la phase n'est pas documentée comme satisfaite.
- Mettre à jour `docs/implementation_status.toml` dans le même commit que toute ouverture ou fermeture de phase, puis exécuter `python tools/check_implementation_status.py`.
- Un benchmark, un accord moyen ou une sortie plausible ne peut jamais promouvoir `public_status=exact`; seuls les certificats et oracles prévus le peuvent.
- Toute contradiction mathématique devient une fixture minimale permanente et met à jour le registre des preuves avant la poursuite des optimisations.
- Distinguer dans le code et les rapports : proposition flottante, décision certifiée, réduction hiérarchique et statut public.
- L'autorisation explicite de démarrer une session GPU inclut nécessairement l'autorisation de son arrêt de sécurité. `--yes` peut donc être utilisé par le trap ou l'agent pour fermer cette session ciblée; il n'autorise aucune autre mutation ni aucun nouveau démarrage.
