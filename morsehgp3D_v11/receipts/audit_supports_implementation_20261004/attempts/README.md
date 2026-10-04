# Première tentative de collecte conservée

Le lecteur parent v1 supposait les noms normal.json/optimized.json pour
toutes les capsules. Tour et empreinte utilisent d'autres noms : la première
collecte échoue sur le fichier inexistant, avant d'avoir rejoué toutes les
capsules. Source du lecteur et sorties sont conservées. Le v2 corrige seulement
ces chemins ; aucune capsule enfant close n'est modifiée. Ce n'est pas un échec
mathématique ou une exécution native.
