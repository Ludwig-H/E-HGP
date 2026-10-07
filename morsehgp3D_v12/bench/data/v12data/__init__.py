"""Preparation des donnees d'entree de morsehgp3D_v12 (hors depot).

Bibliotheque standard + numpy seulement pour le coeur (`common`) et les lecteurs nus
(`formats.kitti_bin`, `formats.las`, `formats.ply`, `formats.text_xyz`). Le lecteur LAZ
(`formats.laz`) et le lecteur 7z (`formats.sevenzip`) delegent a des paquets optionnels
(laspy + lazrs, py7zr) ou a un binaire externe, et refusent explicitement sinon.
"""
