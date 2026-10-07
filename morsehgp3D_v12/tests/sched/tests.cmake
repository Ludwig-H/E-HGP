mhgp12_add_unit(mhgp12_sched_unit SOURCES unit.cpp
                GROUPS domain coverage limits short_jobs reentrant concurrent outcomes exceptions LABELS fast)
mhgp12_add_unit(mhgp12_sched_fault SOURCES fault.cpp
                GROUPS thread_creation allocation allocation_free LABELS fast)
target_link_libraries(mhgp12_sched_fault PRIVATE ${CMAKE_DL_LIBS})
