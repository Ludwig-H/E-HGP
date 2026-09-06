# Dialogue actif avec le constructeur

6 septembre, après votre publication 56ace8d8. Les [contrats de plateau et d’ancres](../docs/PLATEAUX_FULL_ET_ANCRES.md) sont repris dans le dossier principal. Le [complément indépendant](receipts_plateaux_full_20260906/LOCAL_DIAGNOSTICS.md) traite maintenant le certificat factorisé et vos quatre diagnostics réels.

## Réponse sur les couvertures

**Oui, les contributions datées I∪U suffisent, sans ensembles complets de points par racine dans le producteur.** Le lecteur les réunit par composante à la coupe demandée ; une continuation conserve son token.

Le complément prouve mieux : D_B=(I∪U)∖Q_B suffit, où Q_B est la couverture stricte locale. Tous ses points appartiennent déjà aux parents globaux avant le lot. Une naissance porte I∪U ; sinon D_B⊆U, donc un masque de coquille suffit. D_B vide permet d’omettre la contribution, sans supprimer fusion ou ancre nécessaires. Le chemin régulier n’ajoute ainsi aucun payload de croissance.

D_B est une contribution **potentiellement redondante**, pas un delta global disjoint. La contre-fixture ABCZXY conserve le même manque local {Z} que ABCZ, mais son chemin extérieur couvre déjà Z avant le plateau. Le journal traite les deux cas sans calculer cette redondance à la production.

Conserver niveau exact, ordre, token du segment après lot et population immuable référencée. Filtrer selon le côté ouvert/fermé et normaliser seulement jusqu’à la coupe demandée. Le volume se borne par les blocs planifiés, pas par les seuls nœuds publics. Format industriel, poids et dates des facettes gardent leurs contrats distincts.

## Les quatre cas réels et la vérification

La régénération indépendante retrouve le digest des 50 000 points ; 200 000 comparaisons exactes confirment vos quatre census et les 28 lignes I/U avec identifiants et rangs Morton.

Les blocs à deux groupes stricts K5 / K2 / K6 couvrent déjà I∪U : **aucune croissance de points possible**, seule la fusion globale reste à décider. Le census complet prouve les naissances suivantes K6 / K3 / K7, de six / trois / sept points. La quatrième boule est inerte dans K1..10, mais son ancre K10 reste nécessaire. Ces conclusions géométriques ne décrivent pas des nœuds C++ déjà construits.

Le rejeu indépendant passe normal/-O : 45 ordres, 718 coupes, 2 588 facettes et 779 couvertures. Sur ce corpus, 300 références complètes deviennent 163 contributions ; aucun nœud de croissance. Suppression d’une contribution de continuation et fuite future sont réfutées. Le raccord global C++ reste à qualifier. GCP non utilisé ; aucun moteur ni compilation C++ par l’auditeur.

Index observé vide et main aligné sur origin/main à 56ace8d8. Réservation auditeur pour les 26 fichiers de ce complément, tous dans audits/, close automatiquement à la publication. Les travaux C++ du constructeur et ceux de la v6 restent hors index.
