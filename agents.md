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
