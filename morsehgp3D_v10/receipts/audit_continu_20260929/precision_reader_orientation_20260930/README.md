# Juge précision : orientations non exigées

Ce reçu autonome reproduit un angle mort du lecteur, pas un défaut nouveau du moteur. Aucun moteur natif ni compilation/GCP n'est lancé.

judge.py est une copie exacte SHA76a7140e… du juge de /tmp/mhgp10-audit-geant/precision/harness ; original.jsonl est la capture native originale exacte SHA8b00ab78… (14 lignes). Ces deux fichiers ont été hachés parmi28 sources/captures/binaires stables lors de la contre-relecture privée. Le provenance.json précise les limites : cette copie ne fabrique pas un reçu de compilation native historique manquant et ne relance pas le natif.

Le contrôle distinct original donne EXPECTED_FAILURES_CONFIRMED,16checks ; ses deux orientations sont effectivement correctes. Toutefois le juge calcule les verdicts d'orientation aux lignes95–108 puis ne les exige jamais aux lignes116–135. Retirer les deux lignes d'orientation, ou remplacer orientations/intérieur par zéro, donne encore code0/16checks. Le cas faux imprime pourtant deux correct=false.

check.py vérifie les QUATRE pins avant subprocess et après, valide les mutations byte-for-byte hors des seules lignes d'orientation, puis rejuge le contrôle et les deux mutations avec Python uniquement. Normal et−O produisent les mêmes conclusions ; six subprocesses mathématiques au total, zéro exécution native. Tous stdout/stderr/argv/codes sont conservés dans normal.stdout et optimized.stdout. Ils ne sont pas byte-identiques, car les argv/optimize_flag publient le mode.

Usage portable après copie du dossier :

    sha256sum -c SHA256SUMS
    python3 -B check.py
    python3 -B -O check.py

Les artefacts n'exigent aucune référence externe /tmp pour le rejeu. Les coordonnées/nombres natives >2^53 restent dans les lignes originales, sans conversion JSON JavaScript. Une première tentative privée avait arrondi ces autres champs : ses quatre refus code1 et fixtures demeurent dans /tmp/mhgp10-precision-current-review-20260930.4W4Gd11q, conservés historiquement mais non nécessaires à ce reçu autonome. La reprise correcte qui fonde ce reçu préserve tous les octets des lignes nonorientation.

Correction à faire dans un nouveau lecteur, pas dans cette copie scellée : inventaire EXACT de cas/B/coordonnées, rejet d'inconnus/doublons/manquants et verdicts d'orientation/intérieur explicitement exigés. Les constats historiques de niveaux u21/u24 restent distincts. Ce reçu ne certifie ni orientation native u24/u32, ni moteur large, ni FULL/performance/statistiques/sous-quadratique.
