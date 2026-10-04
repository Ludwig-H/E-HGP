# Contacts des sites du support pendant le census local

Port préparé après `3fba92eed`, exclu de graph4. Qualification native et gain
temporel restent à établir. Ce changement conserve les boules, les populations
et tous les compteurs logiques du catalogue.

## Justification et portée

La feuille construit exactement la sphère de son préfixe de q sites, q=2..4.
Les fabriques exactes garantissent que ces q sites sont sur sa coquille.
Le census peut donc réutiliser leur relation zéro, sans recalculer leur
puissance. Cette propriété ne s'étend à aucun autre site de la feuille.

Le DFS garde le préfixe en positions locales strictement croissantes. Un
curseur avance lorsqu'une position visitée est la prochaine position du
préfixe ; tous les autres sites passent toujours par `num::side`. Les IDs
globaux et les positions locales ne sont pas confondus. Un census achevé
exige que les q positions aient été rencontrées et que la coquille contienne
au moins q sites. Le rejet saturé garde exactement le même préfixe de sites
visités et intervient avant toute émission.

Les sites cosphériques supplémentaires sont encore collectés. La recherche
de S* dans toute la coquille, l'arité minimale et les intérieurs stricts
restent inchangés. Le certificat porte sur la présentation construite,
indépendamment du support canonique finalement retenu.

## Mesure et vérification

`census_tests` reste son compteur **logique historique** de sites classés.
Il ne mesure pas les appels physiques à `num::side`. Les valeurs identiques
avant/après n'impliquent donc pas une quantité identique d'arithmétique.
Le port ne promet aucun facteur de gain : une comparaison par site remplace
le calcul de puissance seulement sur les sites de présentation déjà certifiés.

Le modèle indépendant Gram/Fraction compare le scan intégral et le scan
avec contacts certifiés, y compris les arrêts saturés et leur dernier indice :
504 présentations, 2772 contrôles, 5208 contacts réutilisables. Arite2/3/4,
saturation et coquille étendue sont chacun exercés en u18/u21/u24. Les
1875 divergences de la variante « q premiers sites de feuille » montrent
pourquoi l'appartenance au préfixe est indispensable. Normal et −O concordent.

Les portes natives Fraction existantes jugent toutes les populations exactes.
Le nouveau mutant remplace l'appartenance au préfixe par les q premiers
sites, sans dépassement de tableau. Sa mort géométrique et les portes
complètes doivent être observées sur G4 avant toute qualification.
