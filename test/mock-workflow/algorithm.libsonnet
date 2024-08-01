{
  algorithm(name, plugin, duration_usec, inputs, outputs)::
  {
    [name]: {
      plugin: plugin,
      duration_usec: duration_usec,
      inputs: inputs,
      outputs: [name + "/" + out for out in outputs],
    }
  }
}
