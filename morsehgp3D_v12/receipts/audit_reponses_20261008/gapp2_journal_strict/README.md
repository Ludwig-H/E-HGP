# GAPP2 — port ciblé du lecteur strict, sans changement de règle

8 octobre 2026, Codex. Proposition **hors produit** au pin
`9815c19b9a69b00cc927dc61837ef7c5dac63615`, pilote SHA-256 `556e029f…c4d3a`.
Même corps constaté dans `3e587b17ddc6fcc2f258276e76de135d0764a46e`.
Suite du [garde-fou GAPP1](../gapp_journal_strict/README.md), adaptée aux deux conceptions.
Les sources, vingt fichiers de retour et le patch sont épinglés dans [capture.json](capture.json).

Le juge livré adopte encore les deux conceptions sur cinq campagnes synthétiques corrompues :
identité erronée écrasée par la suivante, phase inconnue, processus surnuméraire, code `False`,
indice de passe `False`. Ce sont des contre-exemples de validation, **pas des altérations des
prises réelles**. Les primaires GAPP2 restent jugés séparément : [admission](../gapp2_admission/README.md).

## Proposition

[proposition.patch](proposition.patch) ferme l'ordre et l'unicité des phases, les clés JSON
répétées et constantes non JSON, les types des indices/codes, la cohorte exacte commandée
et la cohérence du code avec le drapeau global effectivement émis. Le flux appareil est
`recolte, appareil, passes, transferts, identite` ; la voie hôte omet appareil/transferts.
L'ancienne porte est conservée et appelle une nouvelle porte JSON permanente.

Le port **ne** remplace pas l'identité globale par une conjonction des verdicts D1/D2.
`mes_g_app.cpp:967–989` détermine son code par les écarts communs, bits L4/L4F32, issues
différentes de p64 et refus p64. Les seuils de temps et de replis sont distincts. Un écart
propre à L4F32 donne donc légitimement code 1, D2 rejeté et D1 adopté ; le cas symétrique
reste également valide. Les deux témoins positifs sont vérifiés.

Le mutant de côté nul conserve son exigence code 1/identité fausse ; son journal, lorsqu'il
est présent, passe le même lecteur. Le mutant L4 reste jugé par **ses deux taux de repli**,
avec code 0 ou 1 cohérent, sans lui imposer code 1. Les nouvelles collectes d'identité du
mutant passent le lecteur complet. Le résumé historique du mutant de côté nul sans lignes
reste compatible ; il n'est pas promu en preuve autonome d'un journal complet.

K10 demeure informatif : une prise invalide est affichée comme refus de cette information,
sans veto ajouté aux verdicts K5. Aucun seuil, bootstrap, A/A, formule de ratio ni choix de
conception n'est modifié. La proposition ne ferme pas tout le schéma métier des compteurs,
ni la provenance extérieure ou l'isolation continue de la campagne.

## Vérification et primaires

[results.json](results.json) : portes officielles **14 avant / 15 après**, 22 cas JSON après,
et cinq mutations causales qui rétablissent chacune une acceptation invalide en supprimant
la garde correspondante. Les cinq contrefaçons initiales sont adoptées avant et refusées
après. Aucun programme natif, compilateur, appareil ou contrôleur n'est appelé.

Le rejeu des 15 processus décisifs, des deux mutants et de l'information K10 conserve le
rapport calculé **strictement identique avant/après** : D1 et D2 **rejetés**, aucun refus de
journal ajouté. Les lignes des 15 prises sont aussi raccordées aux fichiers JSONL et à
leurs hashes. Le mutant de côté nul a code 1 ; le mutant L4 et K10 ont code 0.
Cette égalité entre deux lecteurs du même environnement n'efface pas les deux écarts
flottants worker/rejeu déjà bornés dans l'admission indépendante ; aucune tolérance n'est
nécessaire pour comparer ce patch à son avant.

## Reproduction

```sh
python -B check.py /chemin/depot /chemin/gapp2_provenance_20261008/returned --check
python -O -B check.py /chemin/depot /chemin/gapp2_provenance_20261008/returned --check
sha256sum -c SHA256SUMS
```

Le script vérifie les hashes, extrait seulement trois blobs Git dans une copie temporaire,
contrôle puis applique le patch dans cette copie et joue les portes Python. Normal/−O
donnent les mêmes résultats. Aucun fichier produit ou ancien reçu n'a été modifié ; aucune
nouvelle mesure ni qualification de G intégré/FULL n'en découle.
