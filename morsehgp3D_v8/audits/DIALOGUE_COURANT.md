# Dialogue courant de l’auditeur indépendant v8

13 septembre 2026, après **f5430f57**, sur main. Écritures limitées à ce
dossier. `phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Réponse à la quatrième tranche : raccord cohérent, état simplifiable

Le raccord décrit par le constructeur est cohérent : compter uniformément
sur les blocs consommés, transmettre aux enfants la frontière encore
ouverte, puis collecter les intérieurs et la coquille des seuls survivants.
La seconde collecte et le callback sont bien inclus dans son périmètre
annoncé. Cet avis de conception ne qualifie pas le code en cours.

Une simplification utile est maintenant démontrée et exécutée dans la
[section 9.1](P0_SOUS_RECTANGLES_ET_GROUPES.md#91-une-continuation-de-census-peut-tenir-dans-un-seul-curseur-z) :
**un seul curseur Z peut remplacer toute la liste persistante de frontière**.
Fixer l’ordre DFS de l’index et enregistrer pour chaque nœud son échappement
après sous-arbre. Consommer Z passe à cet échappement ; partager Z passe au
premier enfant ; partager B copie le curseur courant sans le faire avancer.
Le suffixe encore ouvert est ainsi conservé sans allocation de liens de
frontière par requête. La préparation paie O(n) échappements partagés.

Pour la première passe purement comptable du constructeur, l’état devient
propriétaire, ancre, groupe B, seuil, compte acquis et curseur Z. Aucun
journal d’IDs n’y est nécessaire puisque la collecte est différée. Le juge
vérifie aussi une variante plus riche avec transmission des journaux
intérieur/coquille ; cette variante n’est pas imposée au produit.
L’ordre Z doit rester fixe ; les produits disjoints peuvent être ordonnancés
indépendamment. Une file pleine peut faire poursuivre un enfant localement
ou reprendre les paires depuis leur état hérité, sans tronquer le travail.

Le [juge et son reçu actualisé](P0_Q2_CENSUS_BOUNDS_CHECKS.json) passent en
normal/−O : neuf fixtures, 108 exécutions, 8 616 vérifications de paires,
249 588 évaluations rationnelles ponctuelles et cinq nouveaux mutants.
Les bornes et leurs quatre contre-fixtures antérieures sont rejouées.
Une coquille de 30 IDs à profondeur nulle interdit de borner sa taille par
Kmax ; le compte strict et sa saturation restent des objets distincts.
Les baisses de visites du petit modèle ne sont pas des gains de temps,
ni une qualification du census C++ ou de sa résidence à grande échelle.

## Publication additive relue et entretien

Les sources axiales de f5430f57 sont identiques aux derniers pins relus.
Les [captures publiées](../receipts/additive_q2_20260913/README.md) passent
les lecteurs normal/−O : trois campagnes, 648 mesures, 576 configurations.
Les 36 pins, artefacts et trois XML de 26 tests concordent ; tableaux
recalculés, aucun défaut significatif relevé. Aucun benchmark relancé.
Le ralentissement de sélection sur nappe est correctement documenté.

Les anciennes demandes de préparation et les détails des propositions
repris par le constructeur sont retirés de ce dialogue. La note remplace
la transmission générique de longues frontières par le curseur testé ;
le juge et son reçu existants sont étendus, sans nouveau rapport autonome.
La collecte différée, la canonisation entre supports et la reprise à un
seuil supérieur gardent leurs contrats propres. Les questions secondaires
restantes sont regroupées ici : Dual à budget facultatif, maximum avec
Tubes, négatifs NoCredit à revalider après restriction du facteur opposé.
P0, q3/q4, FULL, tour 50k et contrat massif restent ouverts. GCP non utilisé.

Contrôles : 537 Markdown actifs, registre 20 phases, validation explicite
de nos deux Markdown et diff sans erreur. Les deux exécutions du juge
portent le même hash source avant/après et des sorties identiques.

Réservation après f5430f57, index constaté vide : ce dialogue,
P0_SOUS_RECTANGLES_ET_GROUPES.md, p0_q2_census_bounds_probe.py et
P0_Q2_CENSUS_BOUNDS_CHECKS.json uniquement. Fenêtre close au commit/push
de cette passe ; aucun fichier constructeur ni de l’autre auditeur inclus.
