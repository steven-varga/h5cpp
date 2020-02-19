# H5CPP Type System
<img src="../icons/cpp_type_system.png" alt="some text" style="zoom:60%;" />

| sequence containers   | c++ layout | shape       |
|-----------------------|------------|-------------|
|`std::array<T>`        | standard   | vector      |
|`std::vector<T>`       | standard   | hypercube   |
|`std::deque<T>`        | no         | ragged      |
|`std::forward_list`    | no         | ragged      |
|`std::list<T>`         | no         | ragged      |


| associative containers   | c++ layout | shape       |
|-----------------------|------------|-------------|
	| `std::set<T>`         | | 

| HDF5 data type  | STL like containers | dataset shape|
|-----------------|---------------------|--------------|
|bitfield         | `std::vector<bool>`   | ragged     |
|VL string        | `sequential<std::string>` | ragged |

|arithmetic       | std::vector<T>      | regular      |

|STL like objects | HDF5 data type | dataset shape|
|-----------------|----------------|--------------|

# Live error check

<svg width="772" height="350">
	<text x="14" y="42" font-size="28"  class="heavy">H5CPP Test Header</text>
	<g id="chart" font-size="14" font-family="courier" color="blue" fill="currentColor" transform="translate(14 196)">
		<g id="top" transform="rotate(-45) translate(14 0)" text-anchor="start">
			<text x="0" y="0">T</text>
			<text x="14" y="14">T[i]</text>
			<text x="28" y="28">T[i,j]</text>
			<text x="42" y="42">T[i,j,k]</text>
			<text x="56" y="56">T[i,j,k,m]</text>
			<text x="70" y="70">T[i,j,k,m,n]</text>
			<text x="84" y="84">std::array&lt;T,N&gt;</text>
			<text x="98" y="98">std::vector&lt;T&gt;</text>
			<text x="112" y="112">std::deque&lt;T&gt;</text>
			<text x="126" y="126">std::list&lt;T&gt;</text>
			<text x="140" y="140">std::forward_list&lt;T&gt;</text>
			<text x="154" y="154">std::set&lt;T&gt;</text>
			<text x="168" y="168">std::multiset&lt;T&gt;</text>
			<text x="182" y="182">std::map&lt;K,V&gt;</text>
			<text x="196" y="196">std::multimap&lt;K,V&gt;</text>
			<text x="210" y="210">std::unordered_set&lt;T&gt;</text>
			<text x="224" y="224">std::unordered_multiset&lt;T&gt;</text>
			<text x="238" y="238">std::unordered_map&lt;K,V&gt;</text>
			<text x="252" y="252">std::unordered_multimap&lt;K,V&gt;</text>
			<text x="266" y="266">std::stack&lt;T,deque&gt;</text>
			<text x="280" y="280">std::stack&lt;T,vector&gt;</text>
			<text x="294" y="294">std::stack&lt;T,list&gt;</text>
			<text x="308" y="308">std::queue&lt;T, deque&gt;</text>
			<text x="322" y="322">std::queue&lt;T,list&gt;</text>
			<text x="336" y="336">std::priority_queue&lt;T,deque&gt;</text>
			<text x="350" y="350">arma::Col&lt;T&gt;</text>
			<text x="364" y="364">arma::Mat&lt;T&gt;</text>
			<text x="378" y="378">arma::SpMat&lt;T&gt;</text>
		</g>
		<g id="side" text-anchor="start"  transform="translate(565 11)" >
			<text x="0" y="0">unsigned char</text>
			<text x="0" y="14">unsigned short</text>
			<text x="0" y="28">unsigned int</text>
			<text x="0" y="42">unsigned long long int</text>
			<text x="0" y="56">char</text>
			<text x="0" y="70">short</text>
			<text x="0" y="84">int</text>
			<text x="0" y="98">long long int</text>
			<text x="0" y="112">float</text>
			<text x="0" y="126">double</text>
		</g>
		<g id="cells" text-anchor="start"  transform="translate(0 0)" >
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="0" y="0" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="19.799" y="0" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="39.598" y="0" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="59.397" y="0" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="79.196" y="0" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="98.9949" y="0" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="118.794" y="0" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="138.593" y="0" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="118.794" y="0" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="138.593" y="0" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="158.392" y="0" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="178.191" y="0" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="197.99" y="0" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="217.789" y="0" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="237.588" y="0" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="257.387" y="0" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="277.186" y="0" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="296.985" y="0" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="316.784" y="0" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="336.583" y="0" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="356.382" y="0" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="376.181" y="0" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="395.98" y="0" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="415.779" y="0" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="435.578" y="0" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="455.377" y="0" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="475.176" y="0" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="0" y="0" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="494.975" y="0" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="514.774" y="0" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="0" y="14" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="19.799" y="14" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="39.598" y="14" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="59.397" y="14" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="98.9949" y="14" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="118.794" y="14" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="138.593" y="14" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="118.794" y="14" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="138.593" y="14" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="138.593" y="14" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="158.392" y="14" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="178.191" y="14" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="197.99" y="14" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="217.789" y="14" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="237.588" y="14" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="257.387" y="14" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="277.186" y="14" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="296.985" y="14" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="316.784" y="14" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="336.583" y="14" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="356.382" y="14" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="376.181" y="14" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="395.98" y="14" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="415.779" y="14" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="435.578" y="14" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="475.176" y="14" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="0" y="14" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="514.774" y="14" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="534.573" y="14" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="0" y="28" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="19.799" y="28" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="39.598" y="28" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="59.397" y="28" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="98.9949" y="28" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="118.794" y="28" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="138.593" y="28" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="118.794" y="28" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="138.593" y="28" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="138.593" y="28" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="158.392" y="28" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="178.191" y="28" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="197.99" y="28" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="217.789" y="28" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="257.387" y="28" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="316.784" y="28" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="336.583" y="28" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="376.181" y="28" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="395.98" y="28" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="415.779" y="28" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="435.578" y="28" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="455.377" y="28" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="475.176" y="28" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="0" y="28" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="494.975" y="28" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="514.774" y="28" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="534.573" y="28" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="0" y="42" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="19.799" y="42" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="39.598" y="42" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="59.397" y="42" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="79.196" y="42" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="98.9949" y="42" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="118.794" y="42" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="138.593" y="42" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="118.794" y="42" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="138.593" y="42" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="138.593" y="42" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="158.392" y="42" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="178.191" y="42" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="197.99" y="42" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="217.789" y="42" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="237.588" y="42" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="257.387" y="42" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="277.186" y="42" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="296.985" y="42" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="316.784" y="42" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="336.583" y="42" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="356.382" y="42" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="376.181" y="42" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="395.98" y="42" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="415.779" y="42" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="455.377" y="42" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="475.176" y="42" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="0" y="42" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="494.975" y="42" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="514.774" y="42" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="534.573" y="42" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="0" y="56" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="19.799" y="56" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="39.598" y="56" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="79.196" y="56" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="118.794" y="56" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="138.593" y="56" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="118.794" y="56" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="138.593" y="56" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="138.593" y="56" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="158.392" y="56" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="178.191" y="56" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="197.99" y="56" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="217.789" y="56" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="237.588" y="56" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="257.387" y="56" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="316.784" y="56" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="336.583" y="56" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="356.382" y="56" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="376.181" y="56" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="395.98" y="56" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="415.779" y="56" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="435.578" y="56" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="455.377" y="56" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="475.176" y="56" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="0" y="56" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="494.975" y="56" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="514.774" y="56" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="534.573" y="56" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="0" y="70" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="19.799" y="70" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="39.598" y="70" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="59.397" y="70" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="79.196" y="70" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="98.9949" y="70" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="118.794" y="70" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="138.593" y="70" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="118.794" y="70" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="138.593" y="70" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="138.593" y="70" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="178.191" y="70" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="197.99" y="70" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="217.789" y="70" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="237.588" y="70" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="257.387" y="70" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="277.186" y="70" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="296.985" y="70" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="316.784" y="70" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="336.583" y="70" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="356.382" y="70" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="376.181" y="70" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="395.98" y="70" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="415.779" y="70" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="435.578" y="70" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="455.377" y="70" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="475.176" y="70" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="0" y="70" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="494.975" y="70" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="534.573" y="70" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="0" y="84" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="19.799" y="84" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="39.598" y="84" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="59.397" y="84" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="79.196" y="84" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="98.9949" y="84" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="138.593" y="84" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="138.593" y="84" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="138.593" y="84" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="158.392" y="84" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="178.191" y="84" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="197.99" y="84" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="217.789" y="84" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="237.588" y="84" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="257.387" y="84" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="277.186" y="84" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="296.985" y="84" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="316.784" y="84" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="336.583" y="84" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="356.382" y="84" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="376.181" y="84" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="395.98" y="84" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="415.779" y="84" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="435.578" y="84" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="475.176" y="84" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="0" y="84" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="494.975" y="84" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="514.774" y="84" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="534.573" y="84" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="0" y="98" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="19.799" y="98" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="39.598" y="98" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="59.397" y="98" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="79.196" y="98" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="98.9949" y="98" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="118.794" y="98" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="138.593" y="98" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="118.794" y="98" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="138.593" y="98" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="158.392" y="98" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="178.191" y="98" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="217.789" y="98" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="237.588" y="98" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="257.387" y="98" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="277.186" y="98" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="296.985" y="98" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="316.784" y="98" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="336.583" y="98" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="356.382" y="98" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="376.181" y="98" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="395.98" y="98" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="415.779" y="98" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="435.578" y="98" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="455.377" y="98" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="475.176" y="98" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="0" y="98" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="494.975" y="98" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="514.774" y="98" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="534.573" y="98" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="0" y="112" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="19.799" y="112" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="39.598" y="112" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="59.397" y="112" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="79.196" y="112" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="98.9949" y="112" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="118.794" y="112" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="138.593" y="112" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="118.794" y="112" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="138.593" y="112" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="138.593" y="112" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="158.392" y="112" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="178.191" y="112" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="197.99" y="112" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="217.789" y="112" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="237.588" y="112" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="257.387" y="112" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="277.186" y="112" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="296.985" y="112" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="336.583" y="112" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="356.382" y="112" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="376.181" y="112" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="395.98" y="112" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="435.578" y="112" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="455.377" y="112" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="475.176" y="112" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="0" y="112" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="494.975" y="112" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="514.774" y="112" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="534.573" y="112" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="0" y="126" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="39.598" y="126" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="59.397" y="126" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="79.196" y="126" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="98.9949" y="126" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="118.794" y="126" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="118.794" y="126" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="138.593" y="126" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="138.593" y="126" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="178.191" y="126" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="197.99" y="126" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="217.789" y="126" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="237.588" y="126" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="257.387" y="126" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="277.186" y="126" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="296.985" y="126" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="316.784" y="126" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="336.583" y="126" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="356.382" y="126" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="376.181" y="126" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="395.98" y="126" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="415.779" y="126" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="435.578" y="126" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="455.377" y="126" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="475.176" y="126" fill="#FF4500" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="0" y="126" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="514.774" y="126" fill="#BBC42A" />
			<rect rx="2" ry="2" width="17.8191" height="12.6" x="534.573" y="126" fill="#BBC42A" />
		</g>
	</g>
</svg>

```
arithmetic ::= (signed | unsigned)[char | short | int | long | long int, long long int] 
					  | [float | double | long double]
reference ::= [ pointer | R value reference | PR value reference]
```

## What you need to know of C++ data types
The way objects arranged in memory is called the layout. The C++ memory model is more relaxed than the one in C or Fortran therefore one can't assume contiguous arrangement of class members, or even being of the same order as defined. Since data transfer operation in HDF5 require contiguous memory arrangement which creates a mismatch between the two systems.  C++ objects may be categorized by memory layout such as:  

* **Trivial:** class members are in contiguous memory, but order is not guaranteed; consistent among same compiles
	* no virtual functions or virtual base classes,
	* no base classes with a corresponding non-trivial constructor/operator/destructor
	* no data members of class type with a corresponding non-trivial constructor/operator/destructor

* **Standard:** contiguous memory, class member order is guaranteed, good for interop
	* no virtual functions or virtual base classes
	* all non-static data members have the same access control
	* all non-static members of class type are standard-layout
	* any base classes are standard-layout
	* has no base classes of the same type as the first non-static data member.

	meets one of these conditions:
	
	* no non-static data member in the most-derived class and no more than one base class with non-static data members, or
	has no base classes with non-static data members

<div id="object" style="float: right">
	<img src="../icons/struct.svg" alt="some text" style="zoom:60%;" />
</div>
* **POD or plain old data:** members are contiguous memory location and member variables are in order although may be padded to alignment.
Standard layout C++ classes and POD types are suitable for direct interop with the CAPI. Depending the content these types may be categorized as homogeneous  and struct types.

Some C++ classes are treated special, as they *almost* fulfill the **standard layout** requirements. Linear algebra libraries with data structures supporting
 BLAS/LAPACK calls ie: `arma::Mat<T>`, or STL like objects with contiguous memory layout  such as `std::vector<T>`, `std::array<T>` may be 
 converted into Standard Layout Type by providing a shim code to grab a pointer to the underlying memory and size. Indeed the previous versions of H5CPP
 had been supporting only objects where the underlying data could be easily obtained.


# HDF5 dataset shapes

## Scalars, Vectors, Matrices and Hypercubes
<div id="object" style="float: left">
	<img src="../icons/colvector.svg" alt="some text" style="zoom:40%;" />
	<img src="../icons/matrix.svg" alt="some text" style="zoom:50%;" />
	<img src="../icons/hypercube.svg" alt="some text" style="zoom:40%;" />
</div>
Are the most frequently used objects, and the cells may take up any fixed size data format. STL like Sequential and Set containers as well as C++ built in arrays may be mapped 0 - 7 dimensions of HDF5   homogeneous, and regular in shape data structure. Note that `std::array<T,N>` requires the size `N` known at compile time, therefore it is only suitable for partial IO read operations.
```
T::= arithmetic | pod_struct
```
- `std::vector<T>`
- `std::array<T>`
- `T[N][M]...`


</br>

## Ragged Arrays
*VL datatypes are designed **allow** the amount of **data** stored **in each element of a dataset to vary**. This change could be over time as new values, with different lengths, were written to the element. Or, the change can be over "space" - the dataset's space, with each element in the dataset having the same fundamental type, but different lengths. **"Ragged arrays" are the classic example of elements that change over the "space" of the dataset.** If the elements of a dataset are not going to change over "space" or time, a VL datatype should probably not be used.*

- **Access Time Penalty**
Accessing VL information requires reading the element in the file, then using that element's location information to retrieve the VL information itself. In the worst case, this obviously doubles the number of disk accesses required to access the VL information.

	When VL information in the file is re-written, the old VL information must be released, space for the new VL information allocated and the new VL information must be written to the file. This may cause additional I/O accesses.


- **Storage Space Penalty**
Because VL information must be located and retrieved from another location in the file, extra information must be stored in the file to locate each item of VL information (i.e. each element in a dataset or each VL field in a compound datatype, etc.). Currently, that extra information amounts to 32 bytes per VL item.

- **Chunking and Filters**
Storing data as VL information has some effects on chunked storage and the filters that can be applied to chunked data. Because the data that is stored in each chunk is the location to access the VL information, the actual VL information is not broken up into chunks in the same way as other data stored in chunks. Additionally, because the actual VL information is not stored in the chunk, any filters which operate on a chunk will operate on the information to locate the VL information, not the VL information itself.

- **MPI-IO**
Because the parallel I/O file drivers (MPI-I/O and MPI-posix) don't allow objects with varying sizes to be created in the file, attemping to create a dataset or attribute with a VL datatype in a file managed by those drivers will cause the creation call to fail.

```
T::= arithmetic | pod_struct | pod_struct 
element_t ::= std::string | std::vector<T> | std::list<T> | std::forward_list 
```
<div id="object" style="float: right">
	<img src="../icons/ragged.svg" alt="some text" style="zoom:80%;" />
</div>
Sequences of variable lengths are mapped to HDF5 ragged arrays, a data structure with the fastest growing dimension of variable length. The C++ equivalent is a container within a sequential container -- with embedding limited to one level. 

- `std::vector<element_t>`
- `std::array<element_t,N>`
- `std::list<element_t>`
- `std::forward_list<element_t>`
- `std::stack, std::queue, std::priority_queue`


# Mapping C++ Non Standard Layout Classes
Since the class member variables are non-consecutive memory locations the data transfer needs to be broken into multiple pieces.

- `std::map<K,V>`
- `std::multimap<K,V>`
- `std::unordered_map<K,V>`
- `std::unordered_multimap<K,V>`
- `arma::SpMat<T>` sparse matrices in general
- all non-standard-layout types



## multiple homogeneous datasets
<img src="../icons/key-value.svg" alt="some text" style="zoom:100%;" />


## single dataset: compound data type
<img src="../icons/stream_of_struct.svg" alt="some text" style="zoom:100%;" />

multiple records



