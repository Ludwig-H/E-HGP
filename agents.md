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
- **No multi-line equations**: To prevent the GitHub Markdown parser from introducing `<br>` tags or breaking the equation, all equations (both blocks `$$ ... $$` and inline `$ ... $`) must be written on a **single physical line** in the Markdown file.
- **Explicit braces for commands**: Always use explicit curly braces for commands like `\mathbb`, `\mathbf`, `\frac`, `\sqrt` (e.g., `\mathbb{R}`, `\mathbf{1}`, `\frac{1}{K}`, `\sqrt{x}`).
- **No `\operatorname` macro**: The `\operatorname` macro is not allowed, use `\mathrm` or `\text` instead (e.g., `\mathrm{clip}`, `\mathrm{ord}_K`).
- **Accented/unicode characters**: Accent characters within math blocks must be wrapped in `\text{\textbf{text}}` or `\text{text}` to ensure KaTeX rendering compatibility.

