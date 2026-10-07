# Suivi v12 — M6, échelle et entretien du canal

7 octobre 2026. Codex, auditeur. Pin examiné : `3e6e6a8e7`, notamment le microbanc M6 `c3e565f85`
et le contrat d'échelle `c0bb99fd8`. Cadre `exploration_v12_hors_registre`, objet `full_pi0`,
`public_status=not_claimed`. GCP non utilisé.

## Avis au développeur

Quatre constats nouveaux au [registre](../../audits/CONSTATS.md) ; les six points numériques du précédent audit
restent ouverts. Aucun changement du moteur ou du microbanc n'est réalisé par cet audit.

| Constat | Preuve et conséquence | Correction attendue |
| --- | --- | --- |
| CST-0209 | La fonction M6 originale donne p50=5 sur 1..10, au lieu de 5,5. Les effectifs usuels sont pairs ; le débit dérivé utilise ce p50 | Médiane arithmétique des deux valeurs centrales ; convention explicite pour p05 et p95 |
| CST-0210 | Les premières exécutions du graphe et de `touch` entrent dans les statistiques résidentes | Publier préparation et premier usage séparément, puis les répétitions chaudes |
| CST-0211 | Le pré-vol annoncé comme une borne repose sur des volumes empiriques par site | Séparer prévision et garantie ; comptage exact ou réservations budgétées avec refus transactionnel |
| CST-0212 | Le prototype de forêt emploie le bit haut u32 comme genre ; naissance `2^31` et événement 0 deviennent identiques | Domaine propre de chaque indice, sentinelles explicites ; ou nouveau codage requalifié |

Les observations de temps injectées sont des **modèles synthétiques**, pas des mesures GPU. Les collisions
d'identifiants sont vérifiées sans allocation géante ; elles ne prétendent pas montrer cette limite sur un LiDAR
dix millions de sites. Les objectifs d'échelle restent des objectifs expérimentaux, pas des théorèmes universels.

## Vérifications ciblées

- [M6](session/REPORT.md) : extraction exacte de la fonction originale de quantiles, compilation et exécution C++ ;
  fichier CUDA original compilé avec nvcc 12.9.86, sm_120, avertissements stricts ; cinq refus CLI vérifiés avant
  toute entrée dans CUDA. Synchronisation entre flux et initialisation du tampon relues sans défaut constaté.
- [Échelle](echelle/README.md) : modèles arithmétiques et deux nuages de huit sites contre l'oracle exact v11.
  Le cube à K2 a douze naissances ; la famille collinéaire produit des plateaux de taille arbitraire. Cela distingue
  mémoire avant contraction et taille finale. Python normal et `-O` donnent les mêmes résultats.
- [Canal](../audit_canal_20261007/README.md) : anciennes notes archivées sans perte de texte, sauf réancrage des liens
  vers leurs pins. Les 43 constats antérieurs sont conservés à l'identique ; quatre constats sont ajoutés.
- `tools/check_constats.py` contrôle désormais aussi le canal courant : Markdown UTF-8, absence de sous-dossier,
  liens locaux présents, une note par auditeur connu, 8 Kio par note, 4 Kio pour README, 64 Kio au total. Seize
  contrôles d'hygiène, dont sept cas de refus en normal et `-O`, sont conservés dans [canal/checks.json](canal/checks.json).

La règle demandée par l'utilisateur est inscrite dans `AGENTS.md` et `audits/README.md`. Les détails et reçus restent
hors du canal actif. Les états de constats ne sont jamais changés automatiquement par le vérificateur.

## Limites et prochaine relecture

Ni GPU lancé, ni campagne G4, ni performance publiée. La compilation de M6 ne valide pas ses temps ni toutes ses
erreurs de lancement. Les contrats d'échelle doivent encore être reliés au futur stockage réel et aux réservations.
Prochaines pièces à relire : correctifs M6, fabriques numériques avec leur domaine de requêtes, identifiants de la
forêt et contrôles des budgets. L'audit n'a pas de revendication FULL v12.

Les scripts et manifestes de chaque volet épinglent leurs sources. `SHA256SUMS` ferme les fichiers de ce reçu ;
les notes vivantes et le registre sont contrôlés séparément et continueront d'évoluer.
