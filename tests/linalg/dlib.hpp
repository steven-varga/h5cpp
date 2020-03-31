#include <dlib/matrix.h>

namespace h5::test {
	template <class T> using dlib_t = std::tuple<dlib::matrix<T>>;

	template <class T> struct name <::dlib::matrix<T>> {
		static constexpr char const * value = "dlib::matrix<T>";
	};
}
