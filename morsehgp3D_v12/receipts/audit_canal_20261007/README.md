# Nettoyage du canal d'audit — 7 octobre 2026

État source : `3e6e6a8e721c6dbe8aaa3a0c5cb20c56abfae5ff`. À la demande de l'utilisateur, le canal actif garde le
registre complet, une note par auditeur et son README. Les trois contre-lectures Claude et la réponse ancienne du
développeur sont déplacées ici. Les conclusions restent attribuées à leurs auteurs ; aucun état ni preuve de clôture
du registre n'est changé. Aucun moteur, décision ou reçu préexistant modifié. GCP non utilisé.

| Pièce archivée | Dernier commit de son contenu original | Rôle |
| --- | --- | --- |
| [Lemmes T](archives/AUDIT_CONTRE_LECTURE_LEMMES_T_20261007.md) | `fc1f913ce` | contre-lecture Claude, `CST-0101` à `0107` |
| [Contrat numérique](archives/AUDIT_CONTRAT_NUMERIQUE_20261007.md) | `2a7a5f346` | contre-lecture Claude, `CST-0108` à `0113` |
| [Addendum numérique](archives/ADDENDUM_CONTRAT_NUMERIQUE_20261007.md) | `f6f65a0d8` | budgets mixtes, comparaison des centres, `CST-0114` |
| [Réponse du développeur](archives/REPONSE_CLAUDE_LEMMES_T_ET_OUVERTURE_20261007.md) | `a0e31abfe` | réponse aux lemmes T et à l'ouverture Codex |

Les coordonnées et exemples de ces notes sont des témoins synthétiques, pas des données LiDAR réelles.

Les archives conservent tout le texte original ; seuls les **destinataires des liens Markdown** sont réancrés sur
des permaliens Git au dernier commit de la note. Le lien pointe ainsi vers le contexte historique qui existait lors
de son dépôt. [`archive_map.json`](archive_map.json) conserve chemins avant/après, pins, SHA-256 source et archive,
ainsi que chaque substitution de lien. Le vérificateur reconstitue le texte original et contrôle son égalité exacte
avec Git après inversion de ces seules substitutions.

Les liens actifs des contrats et du registre pointent vers ces archives. Les anciens reçus demeurent immuables :
pour une mention d'un ancien chemin `audits/`, utiliser la table ci-dessus ou son `source_permalink` dans le JSON.
Au pin source, le reçu mathématique contient une telle **mention textuelle** ; aucun lien Markdown de reçu vers les
quatre fichiers déplacés n'a été trouvé. Aucun fichier relais n'est ajouté au canal.

Le [README antérieur du canal](https://github.com/Ludwig-H/E-HGP/blob/3e6e6a8e721c6dbe8aaa3a0c5cb20c56abfae5ff/morsehgp3D_v12/audits/README.md)
reste consultable au pin : ses 17 lignes héritées de la v11 figuraient déjà dans `CST-0001` à `CST-0017`. Leur copie
est retirée du README actif ; les 43 lignes du registre, leurs états et leurs preuves sont conservés à l'identique.
La suite de l’audit ajoute quatre constats et actualise la note Codex : le canal intégré contient 47 lignes.
Le README actif formalise la règle durable : une note par acteur, preuves dans les reçus, contrôle léger des tailles,
des liens et de la structure.

Depuis la racine du dépôt, `python3 morsehgp3D_v12/receipts/audit_canal_20261007/verify_cleanup.py` rejoue le contrôle
léger. `verification.json` décrit les fichiers touchés, tailles avant/après, conservation des lignes, liens locaux
actifs et mentions historiques. Le vérificateur autorise les mises à jour des notes et des états ; il ne clôt rien
et contrôle que les identifiants historiques restent présents. La capture de ce reçu constate séparément la
conservation exacte des 43 lignes initiales. Le manifeste `SHA256SUMS` ferme ce reçu ; il n'inclut pas les fichiers actifs qui
continueront d'évoluer.
