# Audit courant — tour FULL, hiérarchie de points et fondations v11

2026-10-02 **14:36:40 UTC**. Une seule note courante de cet auteur, devenu
développeur sur instruction de l'utilisateur ; les constats d'audit antérieurs
restent distingués des nouvelles corrections et qualifications.
Cadre : `phase=exploration_v11_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u21_input_only`, `public_status=not_claimed`.
Sources capturées à 08:30:04 UTC sur `986f75799`, puis rapprochées du commit
publié **`2f9eb838a`** : 62 des 72 fichiers sont identiques, dont tous les
fichiers de code concernés par les constats ci-dessous. Les différences
documentaires et les documents privés absents du commit sont explicités dans
le [reçu de rapprochement](../receipts/audit_full_hierarchie_20261002/suivi_verrous/commit_reconciliation.json).

**Les corrections précédentes sont reconnues ; la question de la projection
sur les points reste ouverte.** Un témoin exact sépare désormais `cover` de
MR₂-bord malgré leurs scores moyens proches dans les expériences L03.
Les preuves, scripts courts et sources figées sont regroupés dans
[un seul dossier de reçus](../receipts/audit_full_hierarchie_20261002/suivi_verrous/README.md).
Aucun build ou GCP n'avait été lancé pour cette capture d'audit. Depuis la
reprise développeur, les fixtures cover/MR et mémo sont intégrées à la
référence et passent sur G4 à `92c5af705`. La porte d'arrêt anormal et les
précisions F6 sont corrigées dans ce même commit. La qualification finale
`a97180667` passe sur G4 : Release 205/205, ASan/UBSan et TSan 130/130,
profils 21/24 130/130, 103 mutants détectés. Les deux premières campagnes
en échec restent conservées. Les trois arrêts ciblés sont certifiés ; voir
l'[état développeur et ses reçus](../docs/DEVELOPPEMENT.md).
FULL et la comparaison HDBSCAN restent à porter et qualifier.

Le catalogue séquentiel T0 passe sa qualification G4 à `e6fe34cb0` :
Release 220/220, ASan/UBSan, TSan et B21/B24 145/145 chacun, poison 146/146.
Le juge Gram/Fraction distinct porte 378 requêtes ; neuf mutants catalogue
sont tués, dont le nouveau témoin qmin4/m5. Son chemin CPU mono ne respecte
pas les 100 ms. Leaf16 donne 20,741–26,018 s sur les trois trames sans sol
entières/K5 ; tous leurs K10 expirent au plafond processus de 30 s. Sur 8k/K5,
15,478 s en leaf32 deviennent 8,240 s (médiane de trois) en leaf16, à empreinte
de sortie identique ; le réglage par défaut n'est pas modifié.
Le premier échec du clone, les délais et les répétitions non jouées sont
conservés dans les [reçus](../receipts/catalogue_20261002/README.md).
Le nouvel audit indépendant
`74fc14a91` confirme les fondations et établit le croisement possible de groupes
core inter-K ; cette réserve est maintenant une fixture des deux étages de
référence, avec groupes statiques et parents exacts, passée sur G4 à `e6fe34cb0`.

Le suivi indépendant `e739d3c8c` est intégré : complétude/capacité relues
favorablement et coût des coquilles nombreuses explicité. Ses fixtures
30/150/270 sites et le certificat de famille qmin≤3 sont retenus pour un
diagnostic ultérieur, sans les présenter comme l'explication du chrono LiDAR.

La demande « u21 voire u24 » est intégrée et qualifiée à **`9df774947`** :
défaut u21, option u24, mêmes grilles et retours. Le census `i128` couvre
q1/q2/q4 aux trois profils ; q3 reste large en u21/u24. Release 227/227,
ASan/UBSan en 24, TSan en 21 et profils 21/24 152/152 chacun ; 116 mutants
au total détectés. Les extrêmes réels21/24 et les refus `2^B` sont exercés.
Le banc clos conserve 15 réussites K5, 18 délais et 3 omissions : cinq entrées
ont exactement les mêmes sorties et travail aux trois profils. Les cinq
sorties u18 égalent aussi les précédentes. LiDAR K5/u21 :20,55–25,85 s ;
u24 :20,51–25,91 s, un essai par trame/profil. K10 expire30 s partout où joué.
Le contrat FULL de 100 ms reste ouvert. [Reçus compacts](../receipts/catalogue_profiles_20261002/README.md),
lecteurs normal/−O conformes ; G4 arrêtée, clé retirée et verrou libéré.

Le suivi indépendant `19ec9de79` est aussi intégré. Son contre-exemple q3
en B21 confirme l'obligation du repli large ; il ne contredit pas la borne
q4 à ancrage commun de cette tranche. Les certificats de familles proposés
restent hors du produit : coquille complète et centre propriétaire avant
de déduire I vide/U=L ; ancre minimale réservée à qmin4 ; puissance positive
du quatrième point seulement nécessaire. Ils ne ferment ni wide_leaf ni FULL.

Le suivi `2cda9a807` relit favorablement le port numérique et le banc :
le choix de voie dépend bien de la présentation, même si la boule a un
qmin inférieur. Son rappel de précision physique est retenu : position
`o+hq`, niveau `h²β` ; élargir B conserve ici la grille 1 mm. La qualification
ASan/UBSan u18 complémentaire à `d77e4b77c` passe 13/13 portes num,
couvrant le chemin q3 natif absent du profil u24 : 207 contrôles de voies et
7 526 contrôles par oracle Fraction. Code produit identique à `9df774947` ;
seconde génération G4 certifiée arrêtée, clés retirées et verrou libéré.

## 1. FULL → points : conserver le témoin MR, sans conclure à l'équivalence

Les comparaisons MR₁/MR₂ avec entrées cœur/bord demandées par le développeur
sont pertinentes. En revanche, « égale cover au meilleur bloc » doit rester
une description prudente des lots mesurés, pas une identité des hiérarchies.

**Fixture permanente intégrée à la référence :** sites `A=(0,0,0)`, `B=(2,0,0)`,
`C=(5,0,0)`, K=2, auto-voisin inclus. Dans FULL₂, AB naît au rayon 1,
BC à 3/2 ; leur jonction est à 5/2. Les premières couvertures sont uniques :
A et B suivent AB, C suit BC. Le bloc AB existe donc dans `cover`.

Dans le témoin MR₂-bord du dépôt, les cœurs carrés mis à l'échelle sont
`[16,16,36]` et les entrées de bord `[16,16,16]`. C entre par B exactement
au plateau qui fusionne A et B : le bloc publié est ABC. **AB n'existe à
aucune coupe.** Pour la cible analytique AB, le meilleur IoU est **1 avec
cover, 2/3 avec MR₂-bord** ; MR₁-cœur, MR₁-bord et MR₂-cœur atteignent 1.
Ce constat précède toute condensation ou sélection EOM.

Le [calcul exact et ses limites](../receipts/audit_full_hierarchie_20261002/suivi_verrous/points_review/README.md)
utilisent des régions de paires collinéaires et le graphe MR complet défini
par le témoin v10, sans exécuter HDBSCAN. Normal/−O sont identiques ;
translation et homothétie sont contrôlées. Le différentiel natif sur G4 reste
à ajouter. Aucun départage ambigu de propriétaires cover n'explique ce cas.

Les intervalles exploratoires L03 de `cover − MR₂-bord` contiennent zéro ;
ils ne constituent pas un protocole d'équivalence. De plus, les dates et
propriétaires d'attache changent avec le graphe : ici les dates physiques
sont `[1,1,3/2]` contre `[2,2,2]`, sans rééchelonnement global possible.
Leur différence de score compare les chaînes complètes ; elle n'isole pas
le seul apport de la connexité FULL à attaches inchangées.

Pour chercher une meilleure hiérarchie puis un meilleur clustering,
conserver quatre diagnostics distincts : présence des groupes dans la tour,
pertes de projection sur les points, compatibilité des groupes souhaités,
puis pertes dues au sélecteur. Le meilleur bloc donne une borne de qualité
pour la sélection dans cette hiérarchie, pas une partition réalisable de
toutes les cibles. Les choix acceptés — cover ensembliste, projection LCA
distincte, core/cover d'abord, maturité hors de cette première tranche —
restent cohérents avec cette démarche. Ce témoin ne prouve aucune supériorité
universelle de cover.

## 2. Tour FULL : accord sur Q1, préciser la validité des mémos

La [contrelecture indépendante de L02 §4](../receipts/audit_full_hierarchie_20261002/suivi_verrous/tower_review/README.md)
est favorable aux lemmes et théorèmes B–F **avec les corrections Q1 déjà
acceptées** : quotient des composantes locales, surjection seulement vers
les composantes globales, raffinement exhaustif, représentants valides et
déduplication des racines pré-plateau. La note donne le chaînon de preuve du
quotient local, sans supposer de position générale.

Précision de port : un terminal mémoïsé pour `(b,k)` est valable à coupe
**fermée `a≥λ_b`**, mais à coupe **ouverte seulement `a>λ_b`**. Dans
`{0,2,4}`, K2, les deux naissances restent distinctes à la coupe ouverte de
4, puis fusionnent à sa coupe fermée. Le mémo ne remplace jamais tous les
représentants avant leur jonction ; pour résoudre un représentant R avant
un plateau parent, conserver `λ_b≤β(R)<λ_parent`.

Deux nuances supplémentaires : la terminaison utilise les niveaux de toutes
les k-parties, pas seulement ceux du catalogue admis ; et la condition
`p+q≤K+1` caractérise la fenêtre candidate, pas la nécessité de chaque
enregistrement pour π₀. Un carré cocyclique admis à K1 est inerte pour les
composantes, déjà reliées par ses côtés. Trois fixtures exactes de 3–4 sites
passent normal/−O ; elles ne certifient aucun générateur natif complet.
Euler reste un diagnostic, conformément à Q3 déjà accepté.

## 3. Numérique : F3 et domaine documentaire F6 corrigés

**L'ancien défaut F3 est fermé** par la propagation des exposants par
expression, réutilisations comprises. La [preuve F2–F4/F6 et ses petits
contrôles exacts](../receipts/audit_full_hierarchie_20261002/suivi_verrous/numeric_review/README.md)
sont favorables sous les domaines annoncés. Deux précisions de Q2 sont
rétablies dans F6 à `92c5af705` : une feuille exacte doit l'être dans binary64,
sinon sa conversion contribue à E ; le seuil `τ=2^(q+e−51)` doit lui-même
être représentable, même si l'expression évaluée ne déborde pas.

**Contre-exemple ayant motivé la correction :** avec x=2¹⁷, cinq carrés donnent a=2⁵⁴⁴ ; l'expression
`((a−a)+1)²` reste exactement 1, mais ses majorants avant annulation donnent
E=67, q=1091, e=7, donc τ=2¹⁰⁴⁷, non fini en binary64. Protéger l'exposant
du seuil et revenir à l'exact hors domaine. Ce polynôme artificiel réfute
une implication documentaire ; aucun prédicat géométrique exécuté n'est
déclaré faux. Les intermédiaires de l'inverse doivent aussi respecter le
domaine, comme le demande déjà F3.

## 4. Socle : fermetures au code, deux suivis ciblés

| Sujet précédent | État dans les sources publiées |
| --- | --- |
| Refus `Result<T>` allouant un T | Corrigé par stockage discriminé ; aucun T sur refus. |
| Destruction de `StageTimer` dépendant du nom/registre | Corrigé par `Stopwatch`, sans ces références et à destruction triviale. |
| Témoin mutant sauté, rapport absent ou sans verdict | Anciens témoins rejetés par le lecteur structuré ; contre-portes présentes. |
| Registre CTest / jeton de saut usurpé | Contrôle récursif et refus du jeton enfant présents. |
| Matrice style / mutants | Style normal/−O 2/2 ; 78 mutants core, 9 num et 16 cloud détectés sur G4 à `a97180667`, dont deux refus de compilation attendus. Aucun signal ou délai compté. |
| Provenance / oracle partagé | Table par fichier livrée ; les fonctions communes de numérotation/coupe ont été séparées. Aucun ancien compte de portes transféré. |

Le [suivi détaillé](../receipts/audit_full_hierarchie_20261002/suivi_verrous/foundation_followup/FOLLOWUP.md)
distingue inspection, simulation Python et portes natives à rejouer.
La [livraison du développeur](REPONSE_CLAUDE_OUVERTURE_ET_FONDATIONS_20261002.md)
annonce ses propres contrôles locaux ; ils restent distincts de la matrice
G4 désormais close sur `a97180667`.
Le script à interpréteur absent est désormais classé `lancement_impossible` ;
ses contre-portes font partie du Release G4 vert sur `92c5af705`. Il ne doit
pas compter comme une mutation tuée. Les défauts ultérieurs de collecte et
de compilation des mutants ont été corrigés puis rejoués ; leurs premiers
échecs restent conservés dans les reçus.
`MemoryBudget::admit` suppose encore un pilote unique.

**P2 corrigé à `92c5af705`, contre-portes G4 passées :** l'ancien `mhgp11_expect_abnormal_stop` acceptait
la présence d'une ligne `run_expect_verdict arret_anormal` quelconque.
Un enfant peut l'imprimer puis sortir normalement avec code 3 ; le wrapper
intérieur ajoute son vrai verdict `code` et rend 1, puis le wrapper extérieur
accepte ce 1 et la ligne imitée. Exiger l'issue réservée au wrapper ou son
dernier verdict. Une [fixture et la commande G4](../receipts/audit_full_hierarchie_20261002/suivi_verrous/foundation_followup/REPLAY_ON_G4.md)
sont conservées comme proposition initiale ; la correction intégrée juge le
statut réel du processus et possède ses propres portes. Aucun résultat
antérieur n'est rétroactivement remplacé par ce rejeu.

Les obligations v10 restantes demeurent : plateaux et cohortes de départ
atomiques pour la condensation, mcs y compris avec `allow_single`, mémoire
simultanée complète, distinction masse recouvrante/masse exclusive. La
stabilité ER0h démontrée à arbre et activations fixes n'est pas une stabilité
générale sous déplacement géométrique. Aucun contrat FULL v11, temps LiDAR
ou avantage global sur HDBSCAN n'est acquis par cette reprise.
