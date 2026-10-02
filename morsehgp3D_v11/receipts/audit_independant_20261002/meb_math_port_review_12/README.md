# MEB locale portée — revue mathématique indépendante 12

Lecture du commit publié `ab04bc7b1ef61b9996eb7ec15db4fd4e5f2d511a`, sans
qualification native héritée. L'inspection initiale avait vu `24e4e04bc`,
puis l'amend est intervenu avant la capture : `SOURCE_BEFORE.json` fixe
explicitement ab04, et non une copie WIP ou une capture 24e4. Les 30 fichiers
figés étaient identiques au LIVE à la capture ; l'unique répertoire de reçus
développeur non suivi n'est pas du code. `SOURCE_AFTER.json` conserve les
empreintes après lecture et le contexte Git 24e4→ab04, sans remplacer les
copies initiales. Le HEAD OWN a avancé pendant la revue ; aucun de ses fichiers
produit n'a été modifié par cet audit.

**Conclusion : aucun défaut mathématique trouvé dans le port lu.**
[meb.cpp](source/morsehgp3D_v11/src/tower/meb.cpp) valide et trie les SiteIdx
(lignes 16–35), construit chaque arité indépendamment (38–54, 93–114), exige
un support strict puis la couverture de toute la partie (64–90), et conserve
le premier support de rayon minimal. La récursion n'utilise jamais le rejet
d'un triplet pour couper ses prolongements q4. La borne 12 concerne la partie
locale ; ni le seuil du census ni sa coquille globale ne sont plafonnés à 12.
La future descente utilisant tous les témoins saturés doit respecter ce
domaine local, notamment pour le K10 prévu. Les poids Cloud sont explicitement
hors de cette primitive géométrique ; ils ne qualifient pas une tour pondérée.

La simplification proposée est sûre sous ces mêmes conditions. Si les sites
de support sont à distance R de c et si `c=Σ w_i p_i`, avec `w_i>0` et
`Σ w_i=1`, alors pour tout autre centre y,
`Σ w_i ||p_i−y||²=R²+||c−y||²`. Toute boule contenant la partie F contient
ces sites : son rayon au carré est donc au moins R², avec égalité seulement
pour y=c. Un candidat strict contenant **tout F** est déjà sa MEB unique.
Les candidats acceptés ont ainsi tous le même centre et rayon ; dans l'ordre
actuel arité croissante puis tuples SiteIdx lexicographiques, le premier
accepté est le support strict local de cardinal minimal puis lexicographique
minimal. Un arrêt anticipé est donc mathématiquement permis. Il n'est pas
porté dans ce snapshot exhaustif et ne constitue aucun gain mesuré. Ses futurs
compteurs et portes devront décrire le travail réellement exécuté, sans
changer le juge géométrique. Après qualification de la baseline, ce certificat
permettrait aussi le chemin q4 `Q4Candidate::through → strictly_inside → side`
sur **tout F**, puis `materialize` uniquement au premier candidat accepté,
sans nouveau prédicat ni clé et avec le même ordre de supports. Le port lu
fabrique actuellement le Level de tout q4 non dégénéré avant la positivité ;
ce report de travail reste une proposition sans gain démontré.
La règle ne vaut pas pour un candidat non strict
quelconque, ni pour un lookup global par rayon seul ; voir la
[contre-garde au-dessus du minimum](../meb_bounded_contract_review_11/README.md).

[check.py](check.py) n'importe aucun code produit, fixture ou oracle du
développeur. Il résout un système barycentrique rationnel en coordonnées
absolues, énumère toutes les circonsphères contenantes sans préfiltre de
positivité, puis compare ce minimum au premier candidat strict contenant.
Il vérifie neuf géométries ciblées et 27 ordres de partie (2 589 présentations
pour ces ordres, 3 476 évaluations de circonsphère au total), dont les replis
affine, droit, obtus et à poids nul. Dans la
fixture q4 `(10,5,5),(9,8,5),(5,2,1),(1,5,8)`, le premier triplet donné a les
poids `(-34/121,175/242,135/242)`, mais le q4 est strict avec les poids
`(2/27,5/18,5/18,10/27)`, centre `(5,5,5)` et β=25. Cela confirme une garde
concrète contre un futur élagage par positivité du préfixe.

Une garde utile au futur FULL est aussi vérifiée : pour
`X={(0,0,0),(6,0,0),(3,4,0),(2,1,0),(4,1,0)}` et F les trois sommets du
triangle, la MEB a centre `(3,7/8,0)` et β=`625/64`. Son census complet à K=3
a p=2 et qmin=3, donc `p<K` mais `p+qmin=5>K+1`. Son absence de Cat3 est
légitime ; une réponse complète ne certifie pas l'admission au catalogue.
La partie `I∪{(0,0,0)}` descend strictement vers le centre `(2,1/2,0)` et
β=`17/4`. C'est une obligation du raccord FULL futur, pas un défaut du wrapper
MEB/census actuel.

Le juge développeur figé emploie Gram/Gauss/Fraction et prend le minimum parmi
**toutes** les boules contenantes, puis sélectionne le support strict local :
sa définition géométrique diffère bien du filtre natif préalable. Il partage
les données de fixtures avec les tests, et son `model_test.py` partage le même
oracle ; ce dernier n'est donc pas une troisième voie indépendante. La matrice
annoncée de 31 fixtures couvre les cas difficiles ci-dessus, les coquilles
globales et les domaines 18/21/24. Cet audit en vérifie les attendus ciblés,
pas les réponses du binaire, les mutants, ni les qualifications G4.

Lectures autonomes normales et `-O` : sorties identiques, code 0, zéro natif.
Les commandes et la première erreur de chemin de lecture, sans effet
géométrique, restent dans [EXECUTIONS.json](EXECUTIONS.json). Relire avec
`python3 check.py` et `python3 -O check.py`, puis vérifier `sha256sum -c SHA256SUMS`.
Le manifeste final inclut tous les fichiers du reçu, sauf lui-même à la racine.
