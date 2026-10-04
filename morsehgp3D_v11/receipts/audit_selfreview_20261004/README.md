# Aide à la contrelecture du développeur — 4 octobre 2026

Cadre : `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`. Lecture ciblée de c22be4e41 puis
des correctifs publiés **3bd4d734e3d30853e1118efb9a4ee1f77cda2f4a**,
puis des sept questions de doctrine **d597ed9ba** et du lecteur stdlib
**eb036dbe2**.
Trois notes actives actualisées en place ; six Markdown actifs avec la
nouvelle question d597 du développeur, sans nouvelle note d'auditeur.
Aucun ancien reçu clos, code produit ou travail d'un autre acteur modifié.

## Correctifs P1/P2 relus

`pipeline_guard/` conserve la lecture WIP avant/après et le modèle lié au
helper : le contrôle d'abandon après réveil précède toutes les lectures
dépendantes. La première fixture scriptée tue directement son omission.
**45 gardes**, normal/−O. Les essais C++ avec thread ne sont pas exécutés
par ce modèle ; ils ne garantissent pas 256 attentes effectivement suspendues.

`export_width/` conserve la capture du codec et **359 gardes stdlib/AST** :
la méthode `Levels.exact` lit les mots sans réduction ; les profils u18/u21
restent à trois mots/version 1, u24 passe à quatre mots/version 2. Le
tétraèdre extrême garde son niveau brut 196/148 bits en u24.
Cela ne joue ni l'exporteur C++, ni `read_export`, ni le pipeline NumPy.

Le delta **eb036dbe2** supprime les dépendances NumPy du lanceur de porte.
`export_gate_stdlib/` recoupe l'offset et l'ordre des mots avec l'exporteur,
puis joue le décodeur source extrait par AST : **297 gardes**, 48 buffers
valides, 48 troncatures et trois entrées malformées, en Python `-S`/`-O -S`.
Sorties identiques ; aucun exporteur exécuté. Le helper ne valide que
l'en-tête et les niveaux, pas toute la structure FULL.

`SOURCE_COMMIT.json` confirme l'identité des sources centrales étudiées avec
3bd4d734e. Le JSON des mutants et CMake ont reçu d'autres ajouts pendant la
lecture P1 ; leurs captures avant/après restent visibles. Le mutant et la
déclaration de porte ciblés sont inchangés. Les nouvelles portes et mutants
P1/P2 sont présents dans le commit. Le reçu G4 **claudequal2 / eb036dbe2**
arrive ensuite en **2b1abb6a5** : lecteur rejoué normal/−O, arrêt ciblé
certifié, archive et plan recoupés. 686 portes GCC Release, 611 ASan/UBSan,
611 TSan et 611 par profil u21/u24 passent ; les deux nouveaux mutants sont
tués. Clang est absent. Cela ne fournit aucun chrono, port des leviers ou
preuve que les 256 essais thread suspendent tous leur attente.
[Reçu natif existant](../developpement_20261004/qualification_p1p2/README.md).
`G4_QUALIFICATION_REVIEW.json` conserve cette recoupe, sans copie d'archive.

## Deux conditions avant les campagnes E1

`flat_verdict/` épingle le helper `lidar_decision`, son protocole et sa portée.
H_L2 exige borne basse IC95% **> −0,02**, avec Holm. La source admet pourtant
`claimed=True` pour p Holm=0,0300969903 et IC=[−0,021;−0,019]. Une borne
exactement égale à −0,02 reproduit également l'écart. **89 gardes** jouent
les fonctions sources extraites par AST, avec distributions injectées :
aucun bootstrap réel ni résultat P08 évalué. La correction d'une expression
testée ajoute le critère IC à H_L2 seulement, en gardant p/IC séparés.

**190 recoupes source/AST/métadonnées** précisent la qualification : les
sessions claudeflat1a/1b jouent le raccord export C++ → projection et tête
Python, contre oracle indépendant, pour EOM z=1/z=3 et feuilles. Les neuf
mutants sont Python. **z=2**, désormais primaire, manque dans les appels
oracle explicites ; l'oracle prend déjà un z entier générique. Ajouter ce
bras et le rejouer sur G4 avant les mesures primaires. La tête C++ reste
absente. Les anciennes archives ont été recoupées, sans nouvelle campagne.

## Correction de portée mathématique

`eom_scope/` contient seulement les trois clusters/cohortes utiles et leur
certificat autonome : masses 284 pour l'union, 116 et 97 pour ses enfants,
condensation A/mcs20 inchangée. À z=16, après facteur commun positif 100^16,
score parent ≤151167948055/2147483648, tandis que somme des enfants
≥569190927923/4294967296. L'inégalité est stricte ; le meilleur score
descendant est au moins cette somme. **504 gardes Fraction/isqrt** certifient
les rayons et scores, sans décision flottante.

Ces cohortes viennent du PointTree k5 b00_001472 clos, SHA
`71e9fa2b5f38f56dac3e276fb666242ceb2b001a437390808e6acfa7da00e24c`.
La reconstruction indépendante A/mcs20 a été contrôlée hors dépôt ; le
rejeu portable vérifie les scores des cohortes fournies, pas la génération
de ce PointTree. Aucune coordonnée, étiquette, liste de PointId ou archive
NPZ KITTI copiée. Remplacer « toute EOM, à tout z » par « les z=1,2,3
testés ». Aucune recommandation de z=16 ni gain de qualité déduit.

Dans le préenregistrement synthétique, un T−A non significatif signifie
« contribution supplémentaire de la hiérarchie non établie » ; il ne prouve
pas sa nullité. Conserver la décomposition et ses incertitudes.

## Réponses aux leviers 100 ms

Les sept réponses sont intégrées à la note moteur existante. Les compteurs
locaux couvrent le domaine public m≤1024 ; les arènes demandent une borne
sur toutes les listes simultanément vivantes et les réservations réelles.
`counter_arena/` conserve **2 313 gardes** combinatoires, d'addition contrôlée
et de durée de vie DFS, sans exécuter ces changements dans le produit.

`cell_memo/` conserve **30 gardes Fraction** de composantes Γ2 : le partage
entre graines différentes exige un certificat de composante valable dès
λ_b, avec λ_b<λ pour un prédécesseur strict. La date terminale d'une descente
ne certifie pas la composante initiale. Aucun cache natif ni gain qualifié.

`subgrid_bounds/` propose de garder les points en B bits et de décomposer
une frontière L=Qℓ+r, Q=2^T, 0≤r<Q et D>0. Pour E=N+D(a−ℓ), comparer le centre à L/Q
revient au signe de QE−Dr. E<0 et E≥D donnent des réponses directes ; seul
0≤E<D demande ces produits. En u24/T6 : E reste sous la borne 126 bits,
les produits résiduels sous 107 bits ; le réservoir conserve son majorant
65 bits à élargir. **6 219 gardes Fraction** couvrent les bornes et contacts
sur u18/u21/u24 et T=0/1/5/6. C'est une reformulation proposée, pas une
qualification du port `CenterRegion`, des enveloppes ou des arènes.

`DOCTRINE_SOURCE.json` épingle les quinze sources Git examinées à d597 et
les inventaires des lectures complémentaires. Les réponses q3, KD et GPU
restent des conditions d'implémentation et de qualification. Aucun gain
100 ms, support natif T6 ou contrat GPU nouveau n'est revendiqué.

## Fermeture

Les deux capsules antérieures, le codec et sa porte stdlib sont copiés sans
mutation ; le
témoin de cohortes et les deux modèles de contrats sont sélectionnés sans
leurs données externes. Le modèle de sous-maille est autonome. `REPLAYS.json`
conserve dix-huit rejeux bornés, sorties normal/−O identiques. `SHA256SUMS`
inventorie tous les fichiers sauf lui-même ; `python3 -B check.py` et sa
version `-O` vérifient aussi les inventaires enfants et les résultats.
Aucun fit, build/test natif, workflow ou GCP lancé par cette contrelecture.
