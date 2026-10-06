# Feuille cohérente : préserver le premier événement

Réponse mathématique Q1/Q2 au pin `ee3eabe5e7aae4d95515f63935e8f9a84adbd468`. Aucun code natif, build, GPU ou GCP exécuté ; proposition de contrôle, pas qualification d'une implémentation.

Recensement : `θ=K+1−q`. Le rejet survient au **(θ+1)-ième intérieur**, en comptant ce site. Soient `t` son indice et `u` l'indice du premier statut non certifié après application des gardes ; retenir `min(t,u)`. Un refus avant `t` impose le repli, un refus après `t` est ignoré. Sans événement, toute la liste est visitée et les masques I/U deviennent les populations ordonnées.

Canonisation : choisir le premier **événement succès ou refus**, dans l'ordre paire, triangle, tétraèdre, puis lexicographique. Le rang combinatoire du cache J2 est colexicographique et ne convient pas pour départager les supports.

```sh
python3 -S -B replay.py --repo /workspaces/E-HGP
python3 -O -S -B replay.py --repo /workspaces/E-HGP
sha256sum -c SHA256SUMS
```

33 contrôles bornés, normal/−O code0 et sorties identiques ; aucun essai échoué. Les incertitudes injectées sont des traces symboliques. Certaines mixtures refus/succès ne sont pas réalisables avec les certificats globaux actuels ; elles n'établissent pas de mutant natif causal. Voir `REPORT.md`.
