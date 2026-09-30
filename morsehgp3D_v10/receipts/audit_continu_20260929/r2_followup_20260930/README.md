# Suivi R2 du 30 septembre 2026

Paquet d'audit, sans modification du moteur et sans GCP. Les groupes
développeur restent des copies distinctes ; leurs bons résultats ne
qualifient pas un binaire commun. Rapport :
[contre-audit R2](../../../audits/audit_continu_20260929/CONTRE_AUDIT_R2_20260930.md).

- `judges/` : notre rejeu R1/R2 normal/−O, sources figées et petits dumps ;
  ajout séparé de la campagne développeur terminale 33/33.
- `cli/` : deux appels natifs, étiquettes écrasées par l'arbre au même
  chemin malgré code 0 ; relevé historique conservé et erratum explicite
  sur son ancienne conclusion d'absence de build tête.
- `head/` : source actuelle et une porte native R2 code 0, ciblée ;
  journal de mutants observé, non relancé.
- `pool/` : sources/logs observés des lots déjà clos, pas campagne globale
  qualifiée ; toutes les copies ont des origines et hashes dans l'inventaire.

La contre-épreuve inverseβ et son contrôle de durée couverte sont publiés
séparément dans `../inverse_beta_shell/`, avec leurs manifestes propres.
Le petit contre-jugement indépendant y reste avec ses sources figées.

Les chemins absolus et dates des reçus décrivent les exécutions observées.
Ils ne sont pas réécrits à la publication. Les bins du développeur ne sont
pas inclus ; leurs empreintes ciblées ne constituent pas une qualification
complète de build. Rejouer un script écrivain seulement dans une copie
temporaire, jamais dans ces captures closes.

Le manifeste de ce paquet couvre les fichiers inclus, pas les binaires
externes ni les campagnes encore en cours. Les deux sous-paquets `cli`
et `pool` conservent aussi leurs manifestes historiques ; nos README de
clarification et reçus supplémentaires sont couverts par la clôture globale.
