// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef UTILILITIES_HPP
#define UTILILITIES_HPP

#include <algorithm>
#include <locale>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "cbasetypes.hpp"
#include "random.hpp"

#ifndef __has_builtin
	#define __has_builtin(x) 0
#endif

// Class used to perform time measurement
class cScopeTimer {
	struct sPimpl; //this is to avoid long compilation time
	std::unique_ptr<sPimpl> aPimpl;
    
	cScopeTimer();
};

int levenshtein( const std::string &s1, const std::string &s2 );

namespace rathena {
	namespace util {
		template <typename K, typename V> bool map_exists( std::map<K,V>& map, K key ){
			return map.find( key ) != map.end();
		}

		/**
		 * Find a key-value pair and return the key value as a reference
		 * @param map: Map to search through
		 * @param key: Key wanted
		 * @return Key value on success or nullptr on failure
		 */
		template <typename K, typename V> V* map_find( std::map<K,V>& map, K key ){
			auto it = map.find( key );

			if( it != map.end() ){
				return &it->second;
			}else{
				return nullptr;
			}
		}

		/**
		 * Find a key-value pair and return the key value as a reference
		 * @param map: Map to search through
		 * @param key: Key wanted
		 * @return Key value on success or nullptr on failure
		 */
		template <typename K, typename V> std::shared_ptr<V> map_find( std::map<K,std::shared_ptr<V>>& map, K key ){
			auto it = map.find( key );

			if( it != map.end() ){
				return it->second;
			}else{
				return nullptr;
			}
		}

		/**
		 * Get a key-value pair and return the key value
		 * @param map: Map to search through
		 * @param key: Key wanted
		 * @param defaultValue: Value returned if key doesn't exist
		 * @return Key value on success or defaultValue on failure
		 */
		template <typename K, typename V> V map_get(std::map<K, V>& map, K key, V defaultValue) {
			auto it = map.find(key);

			if (it != map.end())
				return it->second;
			else
				return defaultValue;
		}

		/**
		 * Resize a map.
		 * @param map: Map to resize
		 * @param size: Size to set map to
		 */
		template <typename K, typename V, typename S> void map_resize(std::map<K, V> &map, S size) {
			auto it = map.begin();

			std::advance(it, size);

			map.erase(it, map.end());
		}

		/**
		 * Find a key-value pair and return the key value as a reference
		 * @param map: Unordered Map to search through
		 * @param key: Key wanted
		 * @return Key value on success or nullptr on failure
		 */
		template <typename K, typename V> V* umap_find(std::unordered_map<K, V>& map, K key) {
			auto it = map.find(key);

			if (it != map.end())
				return &it->second;
			else
				return nullptr;
		}

		/**
		 * Find a key-value pair and return the key value as a reference
		 * @param map: Unordered Map to search through
		 * @param key: Key wanted
		 * @return Key value on success or nullptr on failure
		 */
		template <typename K, typename V> std::shared_ptr<V> umap_find(std::unordered_map<K, std::shared_ptr<V>>& map, K key) {
			auto it = map.find(key);

			if (it != map.end())
				return it->second;
			else
				return nullptr;
		}

		/**
		 * Get a key-value pair and return the key value
		 * @param map: Unordered Map to search through
		 * @param key: Key wanted
		 * @param defaultValue: Value returned if key doesn't exist
		 * @return Key value on success or defaultValue on failure
		 */
		template <typename K, typename V> V umap_get(std::unordered_map<K, V>& map, K key, V defaultValue) {
			auto it = map.find(key);

			if (it != map.end())
				return it->second;
			else
				return defaultValue;
		}

		/**
		 * Resize an unordered map.
		 * @param map: Unordered map to resize
		 * @param size: Size to set unordered map to
		 */
		template <typename K, typename V, typename S> void umap_resize(std::unordered_map<K, V> &map, S size) {
			map.erase(std::advance(map.begin(), map.min(size, map.size())), map.end());
		}

		/**
		 * Get a random value from the given unordered map
		 * @param map: Unordered Map to search through
		 * @return A random value by reference
		*/
		template <typename K, typename V> V& umap_random( std::unordered_map<K, V>& map ){
			auto it = map.begin();

			std::advance( it, rnd_value( 0, map.size() - 1 ) );

			return it->second;
		}

		/**
		 * Get a random value from the given vector
		 * @param vec: Vector to search through
		 * @return A random value by reference
		*/
		template <typename K> K &vector_random(std::vector<K> &vec) {
			auto it = vec.begin();

			std::advance(it, rnd_value(0, vec.size() - 1));

			return *it;
		}

		/**
		 * Get an iterator element
		 * @param vec: Vector to search through
		 * @param value: Value wanted
		 * @return Key value iterator on success or vector end iterator on failure
		 */
		template <typename K, typename V> typename std::vector<K>::iterator vector_get(std::vector<K> &vec, V key) {
			return std::find(vec.begin(), vec.end(), key);
		}

		/**
		 * Determine if a value exists in the vector
		 * @param vec: Vector to search through
		 * @param value: Value wanted
		 * @return True on success or false on failure
		 */
		template <typename K, typename V> bool vector_exists(const std::vector<K> &vec, V value) {
			auto it = std::find(vec.begin(), vec.end(), value);

			if (it != vec.end())
				return true;
			else
				return false;
		}

		/**
		 * Erase an index value from a vector
		 * @param vector: Vector to erase value from
		 * @param index: Index value to remove
		 */
		template <typename K> void erase_at(std::vector<K>& vector, size_t index) {
			if (vector.size() == 1) {
				vector.clear();
				vector.shrink_to_fit();
			} else
				vector.erase(vector.begin() + index);
		}

		/**
		 * Determine if a value exists in the vector and then erase it
		 * This will only erase the first occurrence of the value
		 * @param vector: Vector to erase value from
		 * @param value: Value to remove
		 */
		template <typename K, typename V> void vector_erase_if_exists(std::vector<K> &vector, V value) {
			auto it = std::find(vector.begin(), vector.end(), value);

			if (it != vector.end()) {
				if (vector.size() == 1) {
					vector.clear();
					vector.shrink_to_fit();
				} else
					vector.erase(it);
			}
		}

#if __has_builtin( __builtin_add_overflow ) || ( defined( __GNUC__ ) && !defined( __clang__ ) && defined( GCC_VERSION  ) && GCC_VERSION >= 50100 )
		template <typename T> bool safe_addition(T a, T b, T &result) {
			return __builtin_add_overflow(a, b, &result);
		}
#else
		template <typename T> bool safe_addition( T a, T b, T& result ){
			bool overflow = false;

			if( std::numeric_limits<T>::is_signed ){
				if( b < 0 ){
					if( a < ( (std::numeric_limits<T>::min)() - b ) ){
						overflow = true;
					}
				}else{
					if( a > ( (std::numeric_limits<T>::max)() - b ) ){
						overflow = true;
					}
				}
			}else{
				if( a > ( (std::numeric_limits<T>::max)() - b ) ){
					overflow = true;
				}
			}

			result = a + b;

			return overflow;
		}
#endif

		bool safe_substraction( int64 a, int64 b, int64& result );
		bool safe_multiplication( int64 a, int64 b, int64& result );

		/**
		 * Safely add values without overflowing.
		 * @param a: Holder of value to increment
		 * @param b: Increment by
		 * @param cap: Cap value
		 * @return Result of a + b
		 */
		template <typename T> T safe_addition_cap( T a, T b, T cap ){
			T result;

			if( rathena::util::safe_addition( a, b, result ) ){
				return cap;
			}else{
				return result;
			}
		}

		template <typename T> void tolower( T& string ){
			std::transform( string.begin(), string.end(), string.begin(), ::tolower );
		}

		/**
		* Pad string with arbitrary character in-place
		* @param str: String to pad
		* @param padding: Padding character
		* @param num: Maximum length of padding
		*/
		void string_left_pad_inplace(std::string& str, char padding, size_t num);

		/**
		* Pad string with arbitrary character
		* @param original: String to pad
		* @param padding: Padding character
		* @param num: Maximum length of padding
		*
		* @return A copy of original string with padding added
		*/
		std::string string_left_pad(const std::string& original, char padding, size_t num);

		/**
		* Encode base10 number to base62. Originally by lututui
		* @param val: Base10 Number
		* @return Base62 string
		**/
		std::string base62_encode( uint32 val );

#ifdef Pandas_Helper_Common_Function
		//************************************
		// Method:      umap_find
		// Description: 在无序映射容器中查找键值对, 并将找到的键值作为引用返回 (const 版本)
		// Parameter:   const std::unordered_map<K, V>& map
		// Parameter:   K key
		// Returns:     const V* 成功返回键值的指针 (Value*), 失败返回 nullptr
		// Author:      Sola丶小克(CairoLee)  2023/05/20 19:29
		//************************************
		template <typename K, typename V> const V* umap_find(const std::unordered_map<K, V>& map, K key) {
			auto it = map.find(key);

			if (it != map.end())
				return &it->second;
			else
				return nullptr;
		}

		//************************************
		// Method:      tolower_copy
		// Description: 模拟 boost::algorithm::to_lower_copy 函数
		//              将字符串中的大写字母转换为小写字母, 并返回转换后的字符串 (不修改 input 的值)
		// Parameter:   const T & input
		// Returns:     T
		// Author:      Sola丶小克(CairoLee)  2023/05/20 19:26
		//************************************
		template <typename T> T tolower_copy(const T& input) {
			T output = input;
			std::transform(output.begin(), output.end(), output.begin(),
				[](unsigned char c) { return std::tolower(c); });
			return output;
		}

		//************************************
		// Method:      trim_copy
		// Description: 模拟 boost::trim_copy 函数
		//              去除字符串首尾的空白字符, 并返回处理后的字符串 (不修改 s 的值)
		// Parameter:   const T & s
		// Returns:     T
		// Author:      Sola丶小克(CairoLee)  2023/05/20 20:38
		//************************************
		template <typename T> T trim_copy(const T& s) {
			auto wsfront = std::find_if_not(s.begin(), s.end(), [](auto c) {return std::isspace(c); });
			auto wsback = std::find_if_not(s.rbegin(), s.rend(), [](auto c) {return std::isspace(c); }).base();
			return (wsback <= wsfront ? T() : T(wsfront, wsback));
		}

		//************************************
		// Method:      istarts_with
		// Description: 模拟 boost::istarts_with 函数
		//              判断字符串 input 是否以字符串 test 开头
		// Parameter:   const T & input
		// Parameter:   const T & test
		// Returns:     bool
		// Author:      Sola丶小克(CairoLee)  2023/05/20 23:40
		//************************************
		template <typename T> bool istarts_with(const T& input, const T& test) {
			if (test.size() > input.size()) {
				return false;
			}

			auto it_input = input.begin();
			auto it_test = test.begin();

			std::locale loc;

			for (; it_test != test.end(); ++it_test, ++it_input) {
				if (std::tolower(*it_input, loc) != std::tolower(*it_test, loc)) {
					return false;
				}
			}

			return true;
		}
#endif // Pandas_Helper_Common_Function
	}
}

#endif /* UTILILITIES_HPP */
