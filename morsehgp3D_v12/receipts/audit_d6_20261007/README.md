# MES-D6 — contre-audit du pilote livré

7 octobre 2026, pin `9b2747eff364d56215b589c782b1a4e51d59a576`.
Exploration v12 hors registre ; CPU de référence, objet FULL pi0, u21 ; `not_claimed`.

**Progrès confirmé : les profils sont compilés et comparés séparément.** Le pilote publie des mesures, sans
adoption automatique. Il reste à rendre ses contrôles stricts et à recueillir des prises G4 avant toute décision
sur le profil produit. Le petit essai local déclaré par le développeur n'est pas invalidé par nos témoins.

- [Pilotage et chronométrage](pilotage/README.md) : référence u21 pouvant disparaître avec verdict conforme,
  doublons de prises et collisions de fichiers ; patch minimal proposé, cinq cas normal/−O. Les compilations et
  sondes sont simulées, le reste du pilote est exercé. Provenance et stderr à conserver avant campagne.
- [Lecteur et similitudes](math/README.md) : sorties incomplètes ou mal formées acceptées ; preuve de la
  similitude et proposition de comparaison de l'objet. Petits doubles de sondes, aucun calcul HGP.

`CST-0207` reste ouvert pour la comparaison D6 qualifiante ; les défauts du lecteur prolongent `CST-0018`.
La dilatation d'un nuage entier déjà quantifié est un test de grande étendue, sans restauration de précision
perdue. Égalité des comptes et identité des associations sont deux contrôles distincts. Les chronos de catalogue
et G proviennent de processus séparés : ni nouveau temps GPU ni latence FULL n'en découlent.

Les sous-dossiers contiennent leurs sources de témoins, résultats et empreintes. Aucun fichier produit modifié,
aucune donnée sous licence, aucune utilisation de GCP par cet audit.
