# Table des modules de la v11 : copie lisible par CMake de la table du paragraphe 2 de docs/ARCHITECTURE.md, qui fait
# foi. tools/check_style.py (porte mhgp11_style) refuse tout ecart entre cette table et le document.
#
# MHGP11_ALL_MODULES : les modules dans l'ordre de dependance (un module ne depend que de modules places avant lui).
# MHGP11_DEPS_<module> : ses dependances directes (colonne "Depend de").
set(MHGP11_ALL_MODULES core num sched cloud io index catalogue tower points head api)
set(MHGP11_DEPS_core)
set(MHGP11_DEPS_num core)
set(MHGP11_DEPS_sched core)
set(MHGP11_DEPS_cloud core)
set(MHGP11_DEPS_io core cloud)
set(MHGP11_DEPS_index num cloud)
set(MHGP11_DEPS_catalogue num sched cloud)
set(MHGP11_DEPS_tower catalogue index)
set(MHGP11_DEPS_points tower)
set(MHGP11_DEPS_head points)
set(MHGP11_DEPS_api core num sched cloud io index catalogue tower points head)

# mhgp11_module_closure(<sortie> <module>...) : les modules donnes et toutes leurs dependances, directes ou non, dans
# l'ordre de la table. Un nom hors table est une erreur de configuration.
function(mhgp11_module_closure out)
  set(pending ${ARGN})
  set(seen)
  while(pending)
    list(POP_FRONT pending module)
    if(NOT module IN_LIST MHGP11_ALL_MODULES)
      message(FATAL_ERROR "mhgp11 : module inconnu '${module}' (table : ${MHGP11_ALL_MODULES})")
    endif()
    if(NOT module IN_LIST seen)
      list(APPEND seen ${module})
      list(APPEND pending ${MHGP11_DEPS_${module}})
    endif()
  endwhile()
  set(ordered)
  foreach(module IN LISTS MHGP11_ALL_MODULES)
    if(module IN_LIST seen)
      list(APPEND ordered ${module})
    endif()
  endforeach()
  set(${out} ${ordered} PARENT_SCOPE)
endfunction()
