# Contre-audit borné de la sélection et des changements d'échelle

Les sources privées sont copiées entièrement et épinglées. Aucun moteur, HDBSCAN, Decimal complet ou GCP exécuté.

Résultat : sur l'arbre abstrait API21, mcs5/z1, la sélection actuelle est A/B/C ; la condensation standard avec
départs par cohortes choisit R/C. Le surcompte de A est exactement 3/2 : les trois derniers points continuent
d'être intégrés après que la masse restante est tombée sous mcs5. La preuve porte sur l'API, pas sur la
réalisabilité géométrique de cet arbre par un nuage particulier.

Un défaut indépendant est confirmé : dev_scenes.selection_block ignore l'échelle d'origine des attaches hard.
z2→z3 donne 121/500 au lieu de 117/1000 ; z3→z2 donne une stabilité négative −2/675 pour une attache valide.
Quatre contrôles à exposants identiques sont positifs et exacts. Réexprimer les dates dans l'échelle EOM corrige
chaque petit contrôle ; ce paquet ne modifie aucun producteur.

La proposition11 déclare une condensation structurale par masse finale. Cela peut définir une nouvelle
sémantique ; ce n'est pas réfuté comme définition. En revanche le contrôle hard ne reproduit pas la
condensation standard Campello/HDBSCAN. Décider explicitement si mcs désigne masse instantanée ou finale,
et comparer séparément les deux modèles.

Lecture autonome, sans écrire dans le paquet :

    python3 -B verify.py --archive CHEMIN_PAQUET --manifest-sha256 SHA_EXTERNE
    python3 -B -O verify.py --archive CHEMIN_PAQUET --manifest-sha256 SHA_EXTERNE

Le SHA externe est fourni par l'auditeur hors de ce fichier. Le lecteur refuse un inventaire ouvert et les
symlinks, vérifie les hashes avant de charger les preuves/sources, puis après deux replays bornés.
Le collecteur capture.py n'est jamais appelé par le lecteur. Il est one-shot et hors produit.
Voir PROTOCOL.md et le premier échec de préflight conservé séparément.
