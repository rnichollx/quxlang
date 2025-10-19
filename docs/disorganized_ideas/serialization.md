# Quxlang Serialization

Quxlang supports two pairs of builtin functions, `.SERIALIZE`/`.DESERIALIZE` and `.MARSHALL`/`.UNMARSHALL`. The difference between these two interfaces is that serialization provides a *streaming* interface whereas marshalling provides a *block* interface. 


A type which is a *datatype* is required to implement SERIALIZE and DESERIALIZE.