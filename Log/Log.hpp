#ifndef __Log_hpp_
#define __Log_hpp_
#include <map>
#include <list>
#include <mutex>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <functional>
#include <thread>
#include <condition_variable>
#include <type_traits>
#include <iterator>
#include <unordered_map>
#include <string_view>
#include <utility>
#include <cstring>
#ifdef _WIN32
#if defined(_USE_WINSOCK2API_)
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#endif
#include<Windows.h>
#undef ERROR
#endif // _WIN32


#ifdef JSON_CPP
#include <jsoncpp/json/json.h>
#endif


// 日志类型
enum class LOG_TYPE {
	INFO,
	ERROR,
	WARN,
	DEBUG,
};

enum class ErrorSource {
	Win32,
	Winsock
};

#ifdef _WIN32
#if defined(_USE_WINSOCK2API_)
#pragma comment(lib,"ws2_32.lib")
#endif
static DWORD GetUnifiedError(ErrorSource source) {
#if defined(_USE_WINSOCK2API_)
	if (source == ErrorSource::Winsock) {
		return WSAGetLastError();
	}
#endif
	return GetLastError();
}
#endif

typedef struct _LogData {
	_LogData(LOG_TYPE type, const std::string& file, const char* function, int line, bool showError, ErrorSource error_source)
	{
		_showError = showError;
		_type = type;
		_file = file;
		_line = line;
		_function = function;
		_local_time = std::make_shared<std::tm>();
#ifdef _WIN32
		_win_error = 0;
		_error_source = error_source;
#else
		(void)error_source;
		_errno = 0;
#endif
	}
	_LogData(const _LogData& other)
	{
		_showError = other._showError;
		_type = other._type;
		_file = other._file;
		_line = other._line;
		_function = other._function;
		_local_time = other._local_time;
		_content = other._content;
	}
	bool _showError;
	LOG_TYPE _type;
	std::string _file;
	int _line;
#ifdef _WIN32
	DWORD _win_error;
	ErrorSource _error_source;
#else
	int _errno;
#endif
	std::string _function;
	std::shared_ptr<std::tm> _local_time;
	std::string _content;
}LogData;

static constexpr const char* get_filename(const char* path) {
	const char* file = path;
	while (*path) {
		if (*path == '/' || *path == '\\') {
			file = path + 1;
		}
		path++;
	}
	return file;
}

#ifndef __FILENAME__
#define __FILENAME__ (get_filename(__FILE__))
#endif

#ifndef LOGI
#define LOGI() Log(LOG_TYPE::INFO, __FILENAME__, __func__, __LINE__)
#endif

#ifndef LOGE
#define LOGE() Log(LOG_TYPE::ERROR, __FILENAME__, __func__, __LINE__)
#endif

#ifndef LOGW
#define LOGW() Log(LOG_TYPE::WARN, __FILENAME__, __func__, __LINE__)
#endif

#ifndef LOGD
#define LOGD() Log(LOG_TYPE::DEBUG, __FILENAME__, __func__, __LINE__)
#endif


#ifndef LOG_PERRORI
#define LOG_PERRORI() Log(LOG_TYPE::INFO, __FILENAME__, __func__, __LINE__,true)
#endif

#ifndef LOG_PERRORE
#define LOG_PERRORE() Log(LOG_TYPE::ERROR, __FILENAME__, __func__, __LINE__,true)
#endif

#ifndef LOG_PERRORW
#define LOG_PERRORW() Log(LOG_TYPE::WARN, __FILENAME__, __func__, __LINE__,true)
#endif

#ifndef LOG_PERRORD
#define LOG_PERRORD() Log(LOG_TYPE::DEBUG, __FILENAME__, __func__, __LINE__,true)
#endif

#ifdef _WIN32
#if defined(_USE_WINSOCK2API_)
#ifndef LOG_WINSOCKPERRORI
#define LOG_WINSOCKPERRORI() Log(LOG_TYPE::INFO, __FILENAME__, __func__, __LINE__,true,ErrorSource::Winsock)
#endif

#ifndef LOG_WINSOCKPERRORE
#define LOG_WINSOCKPERRORE() Log(LOG_TYPE::ERROR, __FILENAME__, __func__, __LINE__,true,ErrorSource::Winsock)
#endif

#ifndef LOG_WINSOCKPERRORW
#define LOG_WINSOCKPERRORW() Log(LOG_TYPE::WARN, __FILENAME__, __func__, __LINE__,true,ErrorSource::Winsock)
#endif

#ifndef LOG_WINSOCKPERRORD
#define LOG_WINSOCKPERRORD() Log(LOG_TYPE::DEBUG, __FILENAME__, __func__, __LINE__,true,ErrorSource::Winsock)
#endif
#endif
#endif


// 是否可迭代（有 begin/end 的类型）
template <typename T>
class is_iterable {
private:
	template <typename U>
	static auto test(int) -> decltype(
		std::begin(std::declval<U>()),
		std::end(std::declval<U>()),
		std::true_type()
		);

	template <typename>
	static std::false_type test(...);
public:
	static constexpr bool value = decltype(test<T>(0))::value;
};

// 判断是否为 key-value 容器（value_type 有 first & second）
template<typename T>
struct is_kv_container {
private:
	template<typename U>
	static auto test(int)
		-> decltype(
			std::declval<typename U::value_type>().first,
			std::declval<typename U::value_type>().second,
			std::true_type{}
	);

	template<typename>
	static std::false_type test(...);

public:
	static constexpr bool value = decltype(test<T>(0))::value;
};

// 判断是否为 string-like（可以转换为 string_view）
// 这样可以屏蔽 std::string / std::string_view / const char* 等
template <typename T>
constexpr bool is_string_like_v = std::is_convertible<T, std::string>::value;




// ------------------ Log class ------------------
//Log Helper
static std::function<void(const LogData&)>& log_writer_func() {
	static std::function<void(const LogData&)> _log_writer_func;
	return _log_writer_func;
}

static std::mutex& localtime_mutex() {
	static std::mutex _localtime_mutex;
	return _localtime_mutex;
}

static std::mutex& set_writer_mutex() {
	static std::mutex _set_writer_mutex;
	return _set_writer_mutex;
}

class Log {
public:
	Log(LOG_TYPE type, const std::string& file, const char* function, int line, bool showError = false, ErrorSource error_source = ErrorSource::Win32)
		:_logger_data(type, file, function, line, showError,error_source) {
	}

	static void trim_newlines(std::string& s) {
		while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) {
			s.pop_back();
		}
	}
	static std::string SystemError(const LogData& logData)
	{
		std::string error;
		if (logData._showError) {
			error += "[";
#ifdef __linux__
			error += "error code: ";
			error += std::to_string(logData._errno);
			error += ", error reason: ";
			error += strerror(logData._errno);
#elif defined(_WIN32)
			DWORD dwError = logData._win_error;
			error += "error code: ";
			error += std::to_string(dwError);
			error += ", error reason: ";
			LPSTR lpMsgBuf = NULL;
			FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
				dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&lpMsgBuf, 0, NULL);
			if (lpMsgBuf != NULL) {
				error += lpMsgBuf;
				LocalFree(lpMsgBuf);
				lpMsgBuf = NULL;
			}
#endif
			trim_newlines(error);
			error += "]";
		}
		return error;
	}

	static std::string ToString(const LogData& logData) {
		std::ostringstream content;
		content << std::put_time(logData._local_time.get(), "%Y-%m-%d %H:%M:%S");
		content << " ";
		switch (logData._type) {
		case LOG_TYPE::INFO:  content << " INFO "; break;
		case LOG_TYPE::WARN:  content << " WARN "; break;
		case LOG_TYPE::ERROR: content << " ERROR "; break;
		case LOG_TYPE::DEBUG: content << " DEBUG "; break;
		}
		content << logData._file;
		content << "[" << logData._line << "][" << logData._function << "] ";
		return content.str() + logData._content + SystemError(logData);
	}

	// 普通单值输出（保留）
	template <typename T>
	std::enable_if_t<
		is_string_like_v<std::decay_t<T>>,
		Log&>
		operator<<(const T& data) {
#ifdef _WIN32
		if (_logger_data._win_error == 0)
			_logger_data._win_error = GetUnifiedError(_logger_data._error_source);
#else
		if (_logger_data._errno == 0)
			_logger_data._errno = errno;
#endif
		_logger_data._content += data;
		_logger_data._content += " ";
		return *this;
	}

	template <typename T>
	std::enable_if_t<
		std::is_arithmetic<std::decay_t<T>>::value,
		Log&>
		operator<<(const T& data) {
#ifdef _WIN32
		if (_logger_data._win_error == 0)
			_logger_data._win_error = GetUnifiedError(_logger_data._error_source);
#else
		if (_logger_data._errno == 0)
			_logger_data._errno = errno;
#endif
		_logger_data._content += std::to_string(data);
		_logger_data._content += " ";
		return *this;
	}

	template <typename T>
	std::enable_if_t<
		std::is_pointer<T>::value &&
		!is_string_like_v<std::decay_t<T>>,
		Log&>
		operator<<(const T& data) {
#ifdef _WIN32
		if (_logger_data._win_error == 0)
			_logger_data._win_error = GetUnifiedError(_logger_data._error_source);
#else
		if (_logger_data._errno == 0)
			_logger_data._errno = errno;
#endif           
		_logger_data._content += "address(";
		_logger_data._content += std::to_string((uintptr_t)data);
		_logger_data._content += ") ";
		return *this;
	}


	// ------------------
	// 通用可迭代容器重载（排除 string-like 与 KV 容器）
	// ------------------
	template <typename T>
	std::enable_if_t<
		is_iterable<T>::value &&
		!is_string_like_v<T> &&
		!is_kv_container<T>::value,
		Log&>
		operator<<(const T& container)
	{
		_logger_data._content += "{";
		bool first = true;
		for (const auto& item : container) {
			if (!first) _logger_data._content += ", ";
			*this << item;
			first = false;
		}
		_logger_data._content += "} ";
		return *this;
	}

	// ------------------
	// 通用 KV 容器重载（map / unordered_map / multimap / custom KV）
	// ------------------
	template<typename T>
	std::enable_if_t<is_kv_container<T>::value, Log&>
		operator<<(const T& container)
	{
		_logger_data._content += "MAP:{";
		bool first = true;
		for (const auto& item : container) {
			if (!first) _logger_data._content += ", ";
			_logger_data._content += "[";
			*this << item.first;
			_logger_data._content += ",";
			*this << item.second;
			_logger_data._content += "]";
			first = false;
		}
		_logger_data._content += "} ";
		return *this;
	}

#ifdef JSON_CPP
	Log& operator<<(const Json::Value& json) {
		Json::StreamWriterBuilder writer_builder;
		writer_builder["indentation"] = "";
		writer_builder["enableYAMLCompatibility"] = true;
		writer_builder["emitUTF8"] = true;
		std::ostringstream oss;
		std::unique_ptr<Json::StreamWriter> writer(writer_builder.newStreamWriter());
		writer->write(json, &oss);
		*this << oss.str();
		return *this;
	}
#endif


	~Log() {
		// 获取当前时间
		auto now = std::chrono::system_clock::now();
		// 转换为时间戳
		auto now_c = std::chrono::system_clock::to_time_t(now);
		// 转换为本地时间
		{
			std::lock_guard<std::mutex> lock(localtime_mutex());
			std::tm* local_time = std::localtime(&now_c);
			std::memcpy(_logger_data._local_time.get(), local_time, sizeof(std::tm));
		}
		std::function<void(const LogData&)> writer;
		{
			std::lock_guard<std::mutex> lock(set_writer_mutex());
			writer = log_writer_func();   // 受保护的拷贝
		}
		if (writer)
			writer(_logger_data);
		else
			std::cout << ToString(_logger_data) << std::endl;
	}


	static inline void SetLogWriterFunc(std::function<void(const LogData&)> func) {
		std::lock_guard<std::mutex> lock(set_writer_mutex());
		log_writer_func() = func;
	}

private:
	LogData _logger_data;
};

#endif