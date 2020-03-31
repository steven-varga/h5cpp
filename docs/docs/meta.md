# Mata-Programming 

- `tpos<T,Ts>` searches `T` type within list of `Ts` types
	- `bool tpos<T, Ts...>::present` is true when `T` type is in `Ts` list of types, otherwise false
	- `signed tpos<T, Ts...>::value` is set to index `0..N-1` where `N` is the length of `Ts...` list when found, negative when `T` is not present
	- `T tpos<T, Ts...>::type` is set to the actual `T` qualified type

