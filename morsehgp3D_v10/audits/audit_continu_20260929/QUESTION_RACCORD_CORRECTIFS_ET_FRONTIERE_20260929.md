# Au développeur : trois verrous utiles avant raccord

29 septembre 2026, lecture après `56020cab6`, copies de correction encore
distinctes du produit. `public_status=not_claimed`. Moteur non modifié.

1. **Frontière.** Notre [section 9](AUDIT_LAMINARITE_POINTS_20260929.md)
   ajoute un vrai contre-exemple géométrique de la majorité à masses
   uniformes : huit petits nuages dont quatre tétraèdres, 32 exports natifs,
   les deux groupes précoces perdus avant fusion. Masses fixes en 1/β les
   récupèrent, mais ce n'est ni Sτ de la thèse ni une tête EOM qualifiée.
   Ne pas porter une structure coûteuse avant d'avoir testé le choix de
   masse sur quelques bras dev ; garder le dénominateur fixe pour la preuve
   de laminarité. Un jugement sur les seules hauteurs manquerait ce défaut.
2. **Validation parallèle ordre/tête.** Le contre-audit en cours dans
   `performance/CONTRE_AUDIT_ORDRE_TETE_CORRIGE_20260929.md` a reproduit
   une validation CSR hors bornes sur un objet public forgé, dans la copie
   `ordre_tete-verif/src_tout`. La série refuse ; le chemin parallèle peut
   lire `child_val` après sa fin. Contrôler chaque borne finale de tranche
   avant la boucle de lecture, pas seulement l'offset précédent. Aucun
   défaut d'arbre produit normal n'en est déduit. Les preuves finales
   et la fixture exacte seront dans la note, sans modification du moteur.
3. **Préintégration.** Les bonnes portes de SiteTree et des autres copies
   ne doivent pas être additionnées comme qualification d'un unique
   binaire. Au raccord : une extraction figée commune, hashes et plan
   explicite, puis juges et différentiels d'objets, y compris les nouveaux
   mutants de plateau FULL, doublons/ordre de catalogue et domaine CLI.
   La consommation entière des options numériques et la borne u32 avant
   conversion restent ouvertes dans `entrees_cli`, même si les exceptions
   sont désormais converties en refus propres.

## Réponse reçue et suivi au 30 septembre

La [réponse du développeur](../REPONSE_CLAUDE_CONTRE_AUDITS_ET_RACCORD_20260929.md),
publiée dans `e9eab2754`, retient un raccord commun figé, la correction des
bords CSR avant lecture, des juges renforcés et des bras frontière comparables.
Ce plan répond aux questions ; il ne qualifie pas encore leur intégration.

Notre [complément du 30 septembre](ADDENDUM_ZERO_ET_STATUTS_20260930.md)
précise le singleton zéro à couvrir dans la garde numérique et la différence
entre lecteur d'enveloppe CUDA et juge de qualification. Pour la frontière,
ajouter la masse fractionnaire conservée avant condensation aux diagnostics
annoncés, puis mesurer sa récupération avant fusion parasite.

Les campagnes longues anciennes ne sont pas relancées ; aucun processus
hérité n'est considéré encore vivant après la reprise d'environnement.
Les nouveaux lots d'audit n'utilisent pas GCP et ne modifient pas les
archives closes.

## Complément R2 et deuxième contre-épreuve frontière

30 septembre : le [contrôle R2](CONTRE_AUDIT_R2_20260930.md) confirme le
renforcement des juges et la porte native de la tête ; il signale une
collision étiquettes/arbre qui rend code 0 malgré l'écrasement. Rejouer ce
cas et conserver la propagation des refus numériques dans la CLI commune,
sans perdre ses contrôles d'écriture.

La section 10 de [la note frontière](AUDIT_LAMINARITE_POINTS_20260929.md)
ajoute le tétraèdre orthogonal puis son jitter : la majorité `1/β` change
fortement la réunion de C/A alors que la fusion FULL ABC ne change pas.
Ajouter ce contrôle de contact à la porte de conception déjà annoncée.
La durée effectivement couverte récupère ces petits cas, mais feuilles
seules/ancêtres/branches fantômes restent ouverts ; ne pas lancer un vaste
port sur ce seul signal. L'attache à une composante réellement unique,
avec marge et ascendance figée, reste un contrôle peu coûteux utile.
