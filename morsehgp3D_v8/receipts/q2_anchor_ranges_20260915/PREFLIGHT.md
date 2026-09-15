# Préflights de la tranche18

15 septembre 2026. Ces vérifications pendant l'implémentation ne sont
pas les qualifications closes décrites dans le [bilan](README.md).

- Configuration neuve GCC13.3 Release et Clang18.1.3 Debug ASan/UBSan,
  plus build séparé Clang ThreadSanitizer. Les anciens builds épinglés
  restent intacts. Boost1.83 est utilisé par les seuls juges bornés,
  via les en-têtes déjà disponibles en lecture seule ; aucun code ni
  résultat v7 n'est repris.
- Une première tentative du test Python du nouveau lecteur a rencontré
  `PermissionError: EACCES` au lancement de la sonde pendant le
  remplacement de son exécutable par le lien. Ce chevauchement de build
  et préflight ne constitue pas un échec géométrique. Aucun reçu de
  qualification n'avait été ouvert ni déclaré PASS ; reprise après fin
  du build. Conserver ce constat, pas un résultat favorable rétroactif.
- La contrelecture a remplacé les deux compteurs atomiques indépendants
  de durée de vie Pool par un suivi synchronisé, avec addition vérifiée
  avant mutation. Les maxima portent sur l'intervalle d'inscription des
  parents construits, pas sur le RSS ni les phases d'allocation/destruction.
- Les deux nouvelles fixtures rationnelles de tangence q3 passent
  normal et−O ; elles distinguent profondeur8/coquille4 et
  profondeur9/coquille3 à Kmax10. Ce sont des tests mathématiques,
  pas un moteur q3 produit.
- Le premier test C++ complet sous ThreadSanitizer passe :363 appels
  ranges,120 Coarse,8 904 paires de l'oracle,649 086 sites examinés,
  coquille30. Il observe2 501 dons mais aucun de bande Pool filtrée ;
  cela motive l'ajout d'une fixture à quatre nappes avant gel définitif.
  Ce premier passage ne qualifie pas rétroactivement la fixture ajoutée.

Les campagnes finales utilisent les mêmes114 sources gelées, tous les
binaires indirectement exécutés et leurs caches CMake épinglés. Les
72 CTests de chaque build complet passent avec deux processus au plus.
Les mesures appariées restent séquentielles entre elles ; la charge des
qualifications/audits concurrents est précisée dans le bilan. La gate
finale415 appels ranges/135 Coarse exerce positivement le don Pool filtré
dans les trois configurations, sans réinterpréter le premier préflight.
