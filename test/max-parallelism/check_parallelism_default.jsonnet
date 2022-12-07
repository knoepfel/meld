{
  source: {
    plugin: 'check_parallelism',
  },
  modules: {
    verify: {
      plugin: 'check_parallelism',
      expected_parallelism: 12,
    },
  },
}
