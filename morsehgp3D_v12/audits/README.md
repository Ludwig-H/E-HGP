# Canal d'audit de la v12

7 octobre 2026. Ce dossier présente le suivi courant ; les décisions applicables sont dans
[`docs/DECISIONS.md`](../docs/DECISIONS.md). Les constats et leurs états ont une seule autorité : le
[registre complet](CONSTATS.md), avec le pin, le témoin et la preuve de clôture de chaque ligne.

| Lecture | Contenu |
| --- | --- |
| [Auditeur Codex](AUDIT_CODEX_20261007.md) | état de sa dernière contre-lecture et travail restant |
| [Auditeur Claude](AUDIT_CLAUDE_20261007.md) | contre-lectures des lemmes, du contrat numérique et des ports |
| [Registre](CONSTATS.md) | tous les constats, y compris ceux reportés de la v11 |
| [Historique et réponse du développeur](../receipts/audit_canal_20261007/README.md) | notes antérieures, réponse du développeur, pins et table des déplacements |

`phase=exploration_v12_hors_registre`, `objet=full_pi0`, `public_status=not_claimed`.
Un avis sur un contrat, une porte locale ou un microbanc ne qualifie pas le moteur ni le contrat de temps.

Règles permanentes :

- Une note courte et vivante par acteur, nommée `AUDIT_<ACTEUR>_<DATE>.md` ; chaque avis indique son pin,
  les preuves consultées, les limites et la prochaine vérification. Une réponse développeur utilise le registre
  et un reçu ; une note active séparée n'est utile que pour une réponse encore à traiter.
- Garder ici uniquement les Markdown UTF-8 actifs, sans sous-dossier. Plafonds : **64 Kio** pour le canal,
  **8 Kio** par note, **4 Kio** pour ce README. Ce sont des plafonds ; le canal doit rester aussi court que possible.
- Une ligne par constat dans `CONSTATS.md`. Ne retirer ni identifiant, ni état, ni preuve de clôture lors d'une
  synthèse. Distinguer correction du contrat, porte exécutée et qualification du code.
- Placer les développements, témoins, résultats et anciens échanges dans `receipts/`, avec leurs pins et
  empreintes. Ne pas réécrire les reçus immuables ; les déplacements de notes gardent une table de correspondance.
- Mettre à jour ensemble note, registre et liens actifs. Le contrôle léger
  [`tools/check_constats.py`](../tools/check_constats.py) vérifie leur structure et l'hygiène du canal ; il ne juge
  pas une preuve. Réviser avant adoption les changements de mémoire, concurrence et format.
- Conserver les rôles et l'attribution des conclusions. Les audits motivent les corrections ; aucun banc ne
  promeut un statut public. Aucun contenu de jeu sous licence ni identité de compte dans les reçus.
