{
  source: {
    plugin: 'benchmarks_source',
    n_events: 100000
  },
  modules: {
    a_creator: {
      plugin: 'last_index',
    },
    b_creator: {
      plugin: 'plus_one',
    },
    even_filter: {
      plugin: 'accept_even_numbers',
      consumes: 'a',
    },
    d: {
      plugin: 'read_index',
      filtered_by: ['even_filter:accept_even_numbers'],
      consumes: 'b',
    },
  },
}
