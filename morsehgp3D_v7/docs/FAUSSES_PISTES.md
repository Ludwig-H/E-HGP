# Fausses pistes et décisions écartées

5 septembre 2026. `public_status=not_claimed`. Cette note garde les raisons
des abandons et corrections sans encombrer les entrées actives. Une piste
non encore qualifiée n'est pas, à elle seule, une fausse piste.

| Idée écartée ou corrigée | Pourquoi ; décision retenue |
| --- | --- |
| Publier tous les niveaux Gamma pour reconstruire FULL | Sous régularité, minima Gabriel, vraies multifusions et parents suffisent. Les portails restent nécessaires au calcul, pas à la sortie. [Preuve](AUDIT_NIVEAUX_GABRIEL_20260905.md) |
| Supprimer aussi tous les rattachements silencieux | La contre-fixture à cinq points perd une fusion ultérieure. Garder l'effet de ces incidences par résolution certifiée. [Contre-exemple](../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md) |
| Identifier une composante par sa seule couverture de points | Deux identités peuvent avoir la même couverture. Conserver les feuilles et les parents, sans quotient par l'ensemble de points. [Contrat structurel](CONTRAT_CERTIFICAT_FULL.md) |
| Installer obligatoirement toutes les facettes incidentes comme alias | Le plafond historique bloque 16k/K9 et 32k/K7. Séparer minima et ancres obligatoires du cache dérivé facultatif ; le défaut eager reste un témoin, pas l'architecture massive visée. [Mesures](RESULTATS_MONO_FULL_20260905.md), [nouveau contrat](CONTRAT_CACHE_FULL_PARESSEUX.md) |
| Assimiler moins d'alias à une accélération automatique | Les trois paires 8k lazy économisent environ 28 % de pic mémoire, mais ajoutent des MEB/census J1 et n'accélèrent pas. Conserver le gain de résidence sans promettre un gain de temps. [Comparaison](RESULTATS_MONO_FULL_LAZY_20260905.md) |
| Borner le coût physique d'un proposeur MEB par le seul ordinal legacy | Un contre-exemple compilé distingue propositions réellement tentées et ordinal de référence. Employer deux charges persistantes distinctes. [Correction](PROPOSITION_MEB_ET_BUDGETS.md) |
| Activer généralement la proposition MEB parce qu'elle teste moins de candidats | Le q2 immédiat répété ralentit ; les petits lots sont sensibles à l'ordre. Pas d'activation générale ni de seuil choisi sur ces seules mesures. [Résultat négatif](RESULTATS_COUT_MEB_20260905.md) |
| Déclarer une accélération ou choisir s à partir d'une seule paire favorable | Les paires E/F mêlent gains et régressions. Conserver s=8/10/12, apparier les instruments et distinguer observation de qualification statistique. [Mesures](RESULTATS_MONO_F_20260905.md) |
| Promouvoir FULL depuis le lecteur structurel, un digest égal ou les reçus F | Aucun ne certifie la complétude géométrique du producteur FULL. Garder les autorités séparées et le succès relatif explicite. [Contrat producteur](CONTRAT_PRODUCTEUR_FULL_GABRIEL.md) |
| Traiter un refus ou une capture interrompue comme une tour rapide | Sans terminal de succès, aucun temps de complétion n'est acquis. Conserver les tentatives négatives sans réparer leurs octets. [Interruption réelle](../receipts/full_gabriel_lazy_interrupted_20260905/README.md) |

## Règle d'entretien du dossier actif

README et PASSATION décrivent le présent et renvoient aux preuves ; leurs
longues chronologies redondantes ont été retirées. Les anciennes versions
de ces textes restent récupérables dans Git. Dix fichiers de cache Python
non versionnés ont été supprimés de `bench/__pycache__/` et
`tests/__pycache__/` ; ils sont régénérables depuis les sources.

Les reçus scellés, fixtures de réfutation et sources nécessaires à leur
reproduction ne sont pas des déchets. Ils restent conservés, y compris
les échecs. `audits/` appartient à l'auditeur indépendant et n'est pas
nettoyé par le constructeur. Les builds et brouillons vont dans `build/`.
Le chemin canonique reste `morsehgp3D_v7/`, sans changement de casse.
