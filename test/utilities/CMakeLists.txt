add_unit_test(sized_tuple LIBRARIES meld::utilities)

add_catch_test(sleep_for LIBRARIES meld::utilities)
add_catch_test(thread_counter LIBRARIES meld::utilities TBB::tbb)
