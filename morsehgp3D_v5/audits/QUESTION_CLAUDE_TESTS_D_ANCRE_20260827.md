# Pont historique — questions V7 à V14 sur les tests d'ancre et de seed

- **Statut :** échange détaillé condensé le 28 août 2026 ; version complète disponible dans l'historique Git.
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Ce chemin reste présent parce que `docs/MATHEMATIQUES.md` y renvoie encore. Il
ne constitue plus un état courant. Les décisions actives et leur ordre de
fermeture sont dans [`ETAT_COURANT.md`](ETAT_COURANT.md) ; la réception de la
grille et les interactions nouvelles avec les tests d'ancre sont dans
[`QUESTION_CLAUDE_GRILLE_DE_CELLULES_20260828.md`](QUESTION_CLAUDE_GRILLE_DE_CELLULES_20260828.md).

Les points V7 à V14 encore vérifiables au pin `369f3ac0` sont les suivants :

- le texte du théorème 10.4 doit dire que les centres réalisables sont contenus dans le segment contrôlé et que le **maximum** d'une forme affine est borné par ses extrémités ;
- la borne intermédiaire annoncée pour `v_j` ne découle pas des majorations affichées, même si le chemin i128 conserve une large marge ;
- F5 annonce 28 sites de frontière mais le tableau courant en construit 26 ;
- F7 reste coplanaire et ne qualifie pas une vraie complétion q4 non coplanaire ;
- le plan de tests et l'oracle annoncent `J > 0`, mais la porte et le chemin produit ne comptent que `J < 0`. Le théorème donne bien une positivité stricte pour toute seed aiguë valide : fermer le trou par un oracle borné `J <= 0`, sans imposer un scan défensif avant chaque certificat ni en faire un défaut produit observé ;
- `AnchorPretests::kCounterfactual` désactive plusieurs certificats à la fois. Une ablation causale doit isoler W, secteurs, corde et grille.

Ce pont pourra être supprimé lorsque le document canonique aura absorbé ces
corrections et ne le référencera plus.
