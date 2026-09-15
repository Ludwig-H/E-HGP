# Préflights et périmètre de capture

15 septembre 2026. Aucun reçu historique n'est transféré à cette tranche.

Les deux builds ont d'abord été configurés avec `BUILD_TESTING=OFF` pour
compiler les sondes pendant l'écriture de la nouvelle gate, puis reconfigurés
avec `BUILD_TESTING=ON` et construits intégralement avant tout reçu qualifiant.
Le message CMake indiquant que Boost n'était pas utilisé concernait cette
première configuration sans tests. Les headers Boost sont pris dans le
dossier extrait v7 ; aucun code ni résultat moteur v7 n'est importé.

Les préflights du moteur et de la gate C++ ont passé la syntaxe GCC/Clang
C++20 avec avertissements stricts, puis Release et ASan/UBSan. La nouvelle
porte de reçus a été exercée normal/−O avant le gel. Sa contrelecture a
fait renforcer l'inventaire de pins, les types JSON et les bilans de lignées.

Un lancement console de la gate Python a été fait pendant l'adaptation de
son modèle synthétique de manifeste, avant gel. Il a échoué avec
`KeyError: 'campaign'` dans `validate_pins`, parce que ce modèle n'avait pas
encore reçu le nouveau champ et l'inventaire complet des exécutables CTest.
Le modèle a été corrigé, puis les préflights normal/−O ont passé. Ce n'était
ni un échec de géométrie ni une capture qualifiante ; aucune qualification
échouée n'a été remplacée par un succès sous le même chemin.

Les captures commencent après ce gel. Elles épinglent108 sources et38
exécutables plus CMakeCache dans les builds complets ; la capture TSan
épingle sa seule gate construite et son cache. L'inventaire est versionné
afin de ne pas rendre les reçus historiques illisibles lors d'ajouts futurs.
La fermeture vérifie l'égalité des hashes avant/après, garde les sorties
brutes et décodées, lie chaque ligne à sa commande et refuse les records
orphelins. Les modèles de pins de la gate restent synthétiques, pas des
qualifications de ces modèles inexistants.

Les mesures sont des observations appariées uniques, à affinité0,2,4,6
(quatre cœurs physiques). Elles ont coexisté avec les suites fonctionnelles
et une campagne indépendante d'audit : les temps sont conservés mais ne
qualifient pas une accélération stable. Les comparaisons portent sur le
flux q2 complet de supports, jamais sur toute la tour FULL. GCP non utilisé.

Après ces captures, un redémarrage du conteneur a tué la qualification
ASan `qualification_ifknl_sm` avant que le collecteur puisse écrire ses
records et sa fermeture. Le MANIFEST et le texte du journal CTest sont
conservés dans ce dossier incomplet. La reprise distincte
`qualification_tkoq1k82` utilise les mêmes sources et binaires, avec
détection des fuites active hors sandbox ; elle ne requalifie pas rétroactivement
l'essai tué. Le fichier CTest temporaire de build peut être réutilisé après
cette sauvegarde, mais aucun reçu fermé ni ancien build épinglé n'est modifié.
