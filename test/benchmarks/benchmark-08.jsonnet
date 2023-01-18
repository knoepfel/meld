local max_number = 100000;

{
  source: {
    plugin: 'benchmarks_source',
    n_events: max_number,
  },
  modules: {
    a_creator: {
      plugin: 'last_index',
      product_name: 'a',
    },
    even_filter: {
      plugin: 'accept_even_numbers',
      product_name: 'a',
    },
    fibonacci_filter: {
      plugin: 'accept_fibonacci_numbers',
      product_name: 'a',
      max_number: max_number,
    },
    d: {
      plugin: 'verify_difference',
      filtered_by: ['even_filter:accept_even_numbers', 'fibonacci_filter:accept'],
      i: 'a',
      j: 'a',
      expected: 0,
    },
  },
}
