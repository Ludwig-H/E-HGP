# Contre-lecture indépendante du contre-exemple inverseβ

30 septembre 2026. Paquet d'audit fermé ; aucune modification du paquet d'origine, du moteur ou d'un build. Aucun appel natif ni GCP.

`review.py` rejoue le script Fraction figé, puis effectue 44 contrôles analytiques élémentaires sur les quatre valeurs M=10,100,1024,2048 : rayons carrés des six puis trois diamètres, sommes des poids de C, couverture locale C/A unique, admissibilité forte et population K2, dates projetées C/A, coordonnées u18. Cette seconde logique utilise directement les formules des diamètres et Fraction ; elle n'appelle pas la primitive MEB pour ces 44 contrôles. Le rejeu Γ utilise en revanche la même référence MEB que le paquet initial, pas un second moteur géométrique indépendant.

Les exécutions normale et `−O` sont conformes (code 0, stderr vide), chacune avec 44 vérifications analytiques et 576 contrôles de non-scission dans le rejeu Fraction. Le SHA256 canonique des lignes géométriques est identique aux deux captures initiales. Les sources et les six fichiers manifestés du paquet d'origine sont hachés avant et après, sans changement. Le digest est seulement un contrôle d'identité du rejeu ; les vérifications géométriques restent réellement exécutées.

Résultat : aucune erreur trouvée. La marge majoritaire relative `(2w_CA−W_C)/W_C` est −1,7217 % avant, et +22,30 % à +21,83 % après. Ce n'est donc pas une égalité exacte au seuil. Les couvertures locales sont exactement `{C,A}` ; les atomes ont p=0, q_min=2 et au moins deux sites en coquille. La fusion FULL ABC reste β=41M²/4 ; son p+q_min passe de 2 à 3 et n'est pas supprimé du catalogue FULL K2. À M2048, la coordonnée maximale est 204800 < 262144 ; les rayons projetés sont 6556,799219 puis 4095,500061 unités de grille.

Cela confirme l'amplification sur grille u18 et le contre-exemple de continuité du modèle réel normalisé. Aucun résultat statistique, EOM, moteur FULL ou contrat G4 n'est déduit de cette expérience sur quatre sites.

Rejeu autonome : `python3 -B review.py`, puis `python3 -B -O review.py`. Fournir facultativement le répertoire du paquet d'origine en argument ajoute son contrôle vivant des six hashes ; les captures présentes l'ont fait. `normal.json` et `optimized.json` conservent les vérifications détaillées, codes et hashes du rejeu ; `execution.json` conserve les codes du processus de contre-lecture. Le paquet original n'est pas nécessaire au rejeu autonome car les sources Fraction sont copiées et le digest sémantique de la capture initiale est figé.
