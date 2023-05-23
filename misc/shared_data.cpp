#include <iostream>
#include <string>
#include <mutex>
#include <utility>

template<typename T>
class locked_data;


template<typename T>
class shared_data {
public:
	template<typename... Args>
	shared_data(Args&&... args) : data(std::forward<Args>(args)...) {}

	// both const and non-const copy ctor need to be defined because of aggressive Args&&... matching ^
	shared_data(shared_data<T> &other) : data(*static_cast<const locked_data<T>&>(other.lock())) {}

	shared_data& operator=(shared_data<T> &other) {
		if(this != &other) {
			auto left = lock();
			auto right = other.lock();
			*left = *right;
		}
		return *this;
	}

	shared_data(const shared_data<T> &other) : data(*static_cast<const locked_data<const T>&>(other.lock())) {}

	shared_data& operator=(const shared_data<T> &other) {
		if(this != &other) {
			auto left = lock();
			auto right = other.lock();
			*left = *right;
		}
		return *this;
	}

	shared_data(shared_data &&other) : data(std::move(*static_cast<const locked_data<T>&>(other.lock()))) {}
	
	shared_data& operator=(shared_data &&other) {		// self-move is UB
		auto left = lock();
		auto right = other.lock();
		*left = std::move(*right);
		return *this;
	}

	locked_data<const T> lock() const {
		return locked_data<const T>(&mut, &data);
	}

    locked_data<T> lock() {
		return locked_data<T>(&mut, &data);
	}

private:
    mutable std::mutex mut;
    T data;
};

//template<typename T>
//shared_data(T&&) -> shared_data<std::decay_t<T>>;


template<typename T>
class locked_data {
public:
	T &operator*()  & { return *data; }
	T *operator->() & { return data; }
	T *get()        & { return data; }

	const T &operator*()  const& { return *data; }
	const T *operator->() const& { return data; }
	const T *get()        const& { return data; }

	T &operator*()  && = delete;
	T *operator->() && = delete;
	T *get()        && = delete;

	~locked_data() { mut->unlock(); }

private:
	friend class shared_data<T>;
	friend class shared_data<std::remove_cv_t<T>>;

	locked_data(std::mutex *mut, T *data) : mut(mut), data(data) {
		mut->lock();
	}

	locked_data(const locked_data &) = delete;
	locked_data& operator=(const locked_data &) = delete;

	std::mutex *mut;
	T *data;
};


int main() {
	shared_data<std::string> str("hello");
	shared_data<std::string> str2 = std::move(str);
	str2 = std::move(str);

	//*str2.lock() = "hi";
	//int l = str2.lock()->size();

	locked_data<std::string> s = str.lock();
	int len = s->size();
	std::string contents = *s;

	std::cout << contents << std::endl;
	//std::cout << *str2.lock() << std::endl;
}

