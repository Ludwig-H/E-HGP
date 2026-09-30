# Preuves de l'audit indépendant v10

Base auditée : `6206d1d118794c9e1cabb6faeaec2aaa77d37e5b`. Conclusions et état courant : [SUIVI_AUDIT_INDEPENDANT.md](../../audits/SUIVI_AUDIT_INDEPENDANT.md).

Les captures JSON/XML, les sondes dans `probes/` et les deux notes intermédiaires remplacées dans `historique/` ont été déplacées hors du parcours de lecture. [RELOCALISATION.json](RELOCALISATION.json) associe chaque ancien chemin au nouveau et conserve son SHA-256 ; aucun contenu déplacé n'a changé. `SHA256SUMS_AVANT_RELOCALISATION` conserve le manifeste d'origine et ses anciens chemins. `SHA256SUMS` vérifie les fichiers depuis ce répertoire.

Ces preuves décrivent la base, pas l'état corrigé du produit. Les replays doivent utiliser des répertoires de sortie neufs : certaines sondes historiques écrivent à côté de leur fichier. Ne pas réécrire les captures pour qualifier une correction. Les sources et binaires de travail restent dans `build/v10-audit-independent-20260929/`, non versionné ; les reçus qui les utilisent ne sont pas des archives autonomes.
