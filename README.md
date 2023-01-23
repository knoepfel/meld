# Meld
Exploratory work to meet DUNE's processing needs.

### Development notes

#### Usability

- [ ] Descriptions for available nodes (with possible lazy evaluation of configuration queries)

#### To develop

- [ ] Function registration without function names
  - [x] Implement basic facility
  - [ ] Figure out how to handle reductions
  - [ ] Implement tests that mandate an explicit name if the same function is registered twice
- [ ] Add `react_to(...)` and `produces(...)` blurbs for all inputs/outputs that are products?
  - [ ] What about `react_to_many`?  Is there a `react_to_many`?
- [ ] Replicated modules
  - [x] Implement basic facility
  - [ ] Incorporate as part of `framework_graph`
- [ ] Convert `serial_node` to work with `framework_graph`
- [ ] Product-lookup policies
- [ ] Error-detection for nodes with unassigned input ports (it this possible?)
- [ ] Creating paths (subgraphs)
- [ ] Backwards compatibility of existing art modules
