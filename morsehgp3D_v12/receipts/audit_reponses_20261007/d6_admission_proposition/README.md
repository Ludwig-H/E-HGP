# MES-D6 : proposition d'admission des sorties — 7 octobre 2026

Proposition **non intégrée au produit**, sur le pilote SHA-256 `da2fa5c0b5d48ce2aff357a1174b47773076335b1c4e7ff2ff850afac9f6e118` au commit `b24c256541ffdbe8327f894cd9efebdd7476b51c`. Les dix entrées sont épinglées dans `sources.json` ; elles sont encore identiques à la fermeture documentée dans `verification.json`. Aucun build, moteur natif, appel GCP ni nouveau temps HGP.

`proposition.patch` modifie seulement `microbancs/mes_d6_profils/pilote_d6.py` :

- `run` conserve le stdout brut et refuse la prise entière si UTF-8/JSON invalide, ligne non objet, clé dupliquée ou constante non finie. Il ne saute plus les lignes invalides.
- `summarize` reçoit la configuration **demandée** par `take`. Il exige exactement P passes indexées 0..P−1, profil/K/fils/feuille attendus, succès explicite, sites stables et entiers non booléens bornés. Les champs et phases suivent les producteurs épinglés ; une erreur ne reçoit aucun temps chaud.
- Catalogue : un digest de 64 chiffres hexadécimaux par passe, identique sur les P passes ; export unique à l'endroit attendu lorsqu'il est demandé ; puis `exit` sans champ `order`. G : P lignes de mesure, les ordres 1..min(K,sites) avec leurs compteurs, **un seul digest final**, puis `exit` avec `order` entier égal à zéro. Le patch ne prétend plus prouver l'identité de G entre passes : la sonde ne publie pas leurs empreintes intermédiaires.
- `body_sha` contrôle le format existant **MHGP12DP v1, catalogue** de `src/catalogue/export.cpp` et `tests/catalogue/catalogue_dump.py`. En-tête conforme à la configuration/trame, cinq sections dans l'ordre du producteur, étiquettes et tailles exactes, longueurs/comptes liés aux sites/boules/incidences, réserve et padding nuls, NLEVELS lié au reçu, EOF exact. Il rapproche aussi la taille et le SHA complet de l'export annoncé ; le SHA comparé entre profils reste celui des sections seules. Lecture par blocs d'au plus 1 Mio, sans copie du fichier entier.

Le schéma est fermé sur les producteurs épinglés. Une future modification des sondes, notamment leur raccord CUDA, exigera une adaptation déclarée du schéma et de ses témoins ; les phases ou champs nouveaux sont refusés explicitement par `ok=false` et `admission=protocole invalide`.

Le format accepté n'est pas un oracle : ni géométrie, ni identité des coordonnées, ni validité interne des populations CSR ne sont certifiées par cette lecture. La fixture binaire utilise délibérément des valeurs publiques de format, sans qualification géométrique. Les contrôles restent hors des `wall_ns` publiés par les sondes ; aucun temps intégré ne résulte de leur addition.

## Contre-rejeu

`check.py` reconstruit le pilote épinglé dans un répertoire temporaire, vérifie son hash, applique le patch et vérifie le hash proposé. La fabrique JSON tire les compteurs/diagnostics des producteurs C++ épinglés, avec leurs deux formes réelles de `exit`. Normal et `python -O` rendent des résultats identiques octet pour octet :

- **55 cas de protocole** : cinq formes complètes admises (catalogue/G P5, G u24/u32 et K supérieur au nombre de sites), cinquante anomalies refusées ;
- **6 décodages JSON** : un flux nominal admis, cinq anomalies refusées, stdout brut conservé ;
- **21 fichiers de format** : un MHGP12DP accepté aussi par le lecteur existant, vingt mutations refusées, dont le faux fichier historique de 64 octets ;
- un vrai appel `take` avec exécutables Python simulés accepte P5, l'export cohérent et la sortie G complète. Aucun de ces nombres n'est un temps de moteur.

Les **six témoins figés** de `audit_d6_20261007/math/check.py` sont rejoués avant/après. Précision sur notre ancien témoin nommé `schema_success_two_passes` : sa fabrique était partielle (diagnostics/travail absents et `order` absent des lignes G). Son refus par la proposition n'est donc pas une régression de la forme producteur ; les cinq positifs complets ci-dessus constituent le contrôle nominal. Les appels directs historiques reçoivent uniquement l'adaptateur de configuration devenu nécessaire. Le vrai `main` avec ses doubles historiques passe de code 0 à code 1.

**Résidu mathématique conservé et rejoué séparément sur la forme complète :** après dilatation, un digest G différent avec des compteurs identiques reste admis par `checks`. Ce patch ne ferme pas ce constat. Il ne remplace pas non plus la proposition de pilotage (référence u21×1 obligatoire, doublons/options/entrées) du reçu précédent ; `git apply --check` confirme leur compatibilité dans l'ordre pilotage puis admission, sans nouveau rejeu conjoint de `main`.

Rejeu autonome depuis un checkout contenant les commits épinglés :

```sh
python morsehgp3D_v12/receipts/audit_reponses_20261007/d6_admission_proposition/check.py --repo . --out /tmp/d6-normal.json
python -O morsehgp3D_v12/receipts/audit_reponses_20261007/d6_admission_proposition/check.py --repo . --out /tmp/d6-optimized.json
cmp /tmp/d6-normal.json /tmp/d6-optimized.json
```

`results.json` contient les verdicts ; `verification.json` décrit la fermeture et la compatibilité textuelle des propositions. Aucun test natif avec un export réel supplémentaire n'est revendiqué ; la requalification produit restera nécessaire après intégration.
