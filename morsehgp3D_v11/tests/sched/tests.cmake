mhgp11_add_unit(mhgp11_sched_unit SOURCES unit.cpp
                GROUPS domain coverage limits short_jobs reentrant concurrent outcomes exceptions LABELS fast)
mhgp11_add_unit(mhgp11_sched_fault SOURCES fault.cpp
                GROUPS thread_creation allocation allocation_free LABELS fast)
target_link_libraries(mhgp11_sched_fault PRIVATE ${CMAKE_DL_LIBS})
