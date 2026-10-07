# Table des modules de la v12 : copie lisible par CMake de la table du paragraphe 5 de docs/ARCHITECTURE.md, qui fait
# foi. tools/check_style.py (porte mhgp12_style) refuse tout ecart entre cette table et le document. Port de
# cmake/modules.cmake de la v11 (commit ac081a06f), reduit aux modules du socle : une tranche qui livre un module
# l'ajoute ici et au document dans le meme commit.
#
# MHGP12_ALL_MODULES : les modules dans l'ordre de dependance (un module ne depend que de modules places avant lui).
# MHGP12_DEPS_<module> : ses dependances directes (colonne "Depend de").
set(MHGP12_ALL_MODULES core num sched cloud io index catalogue)
set(MHGP12_DEPS_core)
set(MHGP12_DEPS_num core)
set(MHGP12_DEPS_sched core)
set(MHGP12_DEPS_cloud core)
set(MHGP12_DEPS_io core cloud)
set(MHGP12_DEPS_index num cloud)
set(MHGP12_DEPS_catalogue num sched cloud io)

# mhgp12_module_closure(<sortie> <module>...) : les modules donnes et toutes leurs dependances, directes ou non, dans
# l'ordre de la table. Un nom hors table est une erreur de configuration.
function(mhgp12_module_closure out)
  set(pending ${ARGN})
  set(seen)
  while(pending)
    list(POP_FRONT pending module)
    if(NOT module IN_LIST MHGP12_ALL_MODULES)
      message(FATAL_ERROR "mhgp12 : module inconnu '${module}' (table : ${MHGP12_ALL_MODULES})")
    endif()
    if(NOT module IN_LIST seen)
      list(APPEND seen ${module})
      list(APPEND pending ${MHGP12_DEPS_${module}})
    endif()
  endwhile()
  set(ordered)
  foreach(module IN LISTS MHGP12_ALL_MODULES)
    if(module IN_LIST seen)
      list(APPEND ordered ${module})
    endif()
  endforeach()
  set(${out} ${ordered} PARENT_SCOPE)
endfunction()
